#include <stdio.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#include "keydetect.h"
#include "arithmetic.h"

long long gettime_us(void);

long long gettime_us(void)
{
  struct timeval nowtv;
  // If gettimeofday() fails or returns an invalid value, all else is lost!
  if (gettimeofday(&nowtv, NULL) == -1)
    return -1;
  if (nowtv.tv_sec < 0 || nowtv.tv_usec < 0 || nowtv.tv_usec >= 1000000)
    return -1;
  return nowtv.tv_sec * 1000000LL + nowtv.tv_usec;
}

int list[MAX_KEYS];
long long compressed_bits=0;

int run_tests(void)
  {
  FILE *f=fopen("/dev/random","r");

  unsigned char test_keys[1000][64];
  int test_key_count=0;

  vector_initialise();
  printf("Testing insertion speed of %lld keys...\n",MAX_KEYS);
 
  
  long long start=gettime_us();
  unsigned char key[64];
  time_t last_time=time(0);
  for(int i=0;i<MAX_KEYS;i++) {
    if (fread(key,64,1,f)!=1) {
      fprintf(stderr,"Failed to read 64 bytes from /dev/random.\n");
    }    
    if (last_time!=time(0)) {
      printf("%d (%.2f%%)\r",i,i*100.0/MAX_KEYS);
      fflush(stdout);
      last_time=time(0);
    }
  }
  long long end=gettime_us();
  long long key_gen_time=end-start;
  printf("Key generation (without inserting) of %lld keys required %f sec.\n",
	 MAX_KEYS,key_gen_time/1000000.0);
  start=gettime_us();
  for(int i=0;i<MAX_KEYS;i++) {
    if (fread(key,64,1,f)!=1) {
      fprintf(stderr,"Failed to read 64 bytes from /dev/random.\n");
    }
    if (test_key_count<1000) {
      bcopy(key,test_keys[test_key_count++],64);
    }
    insert_key(key,sizeof key);
    if (last_time!=time(0)) {
      printf("%d (%.2f%%)\r",i,i*100.0/MAX_KEYS);
      fflush(stdout);
      last_time=time(0);
    }
  }
  end=gettime_us();
  long long key_insert_time=end-start;
  printf("Key generation (with inserting) of %lld keys required %f sec.\n",
	 MAX_KEYS,key_insert_time/1000000.0);
  printf("Subtracting key generation time, key insertion required %.1f usec/key.\n",
	 (key_insert_time-key_gen_time)*1.0/MAX_KEYS);

  // Measure density in vectors
  printf("Measuring vector density...\n");
  for(int v=0;v<NUM_VECTORS;v++) {
    int bitcount=0;
    for(unsigned long long i=0;i<VECTOR_LENGTH;i++)
      if (get_vector_bit(v,i)) bitcount++;
    double density=bitcount*100.0/VECTOR_LENGTH;
    if (density>=(MAX_KEYS*100.0/VECTOR_LENGTH)) {
      printf("Vector #%d has erroneously high density of %.2f%%.\n",
	     v,density);
    }
    if (last_time!=time(0)) {
      printf("%d (%.2f%%)\r",v,v*100.0/NUM_VECTORS);
      fflush(stdout);
      last_time=time(0);
    }
  }

  // Measure entropy of each vector, be compressing using interpolative coding.
  printf("Compressing each vector...\n");
  start=gettime_us();
  for(int v=0;v<NUM_VECTORS;v++) {

    // Compare zlib and interpolative coder compression performance
    int zlib_bytes=0;
    {
      z_stream strm;
      unsigned char out_buffer[(VECTOR_LENGTH>>3)+1024];
      /* allocate deflate state */
      strm.zalloc = Z_NULL;
      strm.zfree = Z_NULL;
      strm.opaque = Z_NULL;
      int ret = deflateInit(&strm, 9);
      if (ret != Z_OK)
        return ret;
      strm.avail_in=VECTOR_LENGTH>>3;
      strm.next_in=bit_vectors[v];

      strm.next_out = out_buffer;
      strm.avail_out = sizeof out_buffer;

      ret = deflate(&strm, 1);    /* no bad return value */
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      zlib_bytes = (sizeof out_buffer) - strm.avail_out;             
      (void)deflateEnd(&strm);
    }
    
    int list_length=0;
    for(unsigned long long i=0;i<VECTOR_LENGTH;i++)
      if (get_vector_bit(v,i)) {
	if (list_length<MAX_KEYS) list[list_length++]=(int)i;
	else {
	  fprintf(stderr,"Too many bits set in vector.\n");
	  exit(-1);
	}
      }

    // Allow plenty of space
    range_coder *c=range_new_coder(list_length*(int)sizeof(int)*2);
    ic_encode_recursive(list,list_length,VECTOR_LENGTH,c);
    range_emit_stable_bits(c);
    compressed_bits+=c->bits_used;

    printf("Vector #%d : zlib = %f KB, interpolative = %f KB\n",
	   v,zlib_bytes*1.0/1024,c->bits_used*1.0/8192);
    
    range_coder_free(c);

    if (last_time!=time(0)) {
      printf("%d (%.2f%%)\r",v,v*100.0/NUM_VECTORS);
      fflush(stdout);
      last_time=time(0);
    }

  }
  end=gettime_us();
  printf("Vectors compress to a total of %lld bits (%.2fMB).\n",
	 compressed_bits,compressed_bits*1.0/(8*1024*1024));
  printf("Average compressed vector is %.2fKB\n",
	 compressed_bits*1.0/(8*1024)/NUM_VECTORS);
  printf("Average compressed query data should be %.4fKB\n",
	 compressed_bits*1.0/(8*1024)*NUM_QUERIES/NUM_VECTORS);
  printf("Compressing all vectors took %fsec\n",(end-start)/1000000.0);
  

  // Now look-up each key to make sure we can find it
  printf("Verifying that all inserted keys can be found...\n");
  int vectors[NUM_QUERIES];
  for(int i=0;i<test_key_count;i++) {
    for(int vc=0;vc<NUM_QUERIES;vc++) {
      vectors[vc]=random()%NUM_VECTORS;    
      if (!get_vector_bit(vectors[vc],
			  key_bit_for_vector(vectors[vc],test_keys[i],64))) {
	printf("ERROR: Test key #%d could not be found (bit not set in vector #%d).\n",
	       i,vectors[vc]);
      }
    }
  }
  printf("  ok.\n");

  // Now test that non-existant keys cannot be found, and see how many queries are
  // required on average to exclude false positives.
  printf("Testing number of queries required to reject a non-inserted key...\n");
  int failures=0;
  int successes=0;
  int query_count_tallies[NUM_QUERIES+1];  
  for(int i=0;i<NUM_QUERIES+1;i++) query_count_tallies[i]=0;
  while(1) {
    if (fread(key,64,1,f)!=1) {
      fprintf(stderr,"Failed to read 64 bytes from /dev/random.\n");
    }
    int query_count=0;
    int vn=random()%NUM_VECTORS;
    while (get_vector_bit(vn,key_bit_for_vector(vn,key,64))) {
      query_count++;
      vn=random()%NUM_VECTORS;
      if (query_count>NUM_QUERIES) break;
    }
    if (query_count<=NUM_QUERIES) {
      successes++;
      query_count_tallies[query_count]++;
    } else failures++;
    if (time(0)!=last_time) {
      last_time=time(0);
      printf("# of queries required to resolve missing key:");
      for(int i=0;i<NUM_QUERIES+1;i++)
	if (query_count_tallies[i]>100) {
	  printf(" %.1f%%",query_count_tallies[i]*100.0/(failures+successes));
	} else printf(" %d",query_count_tallies[i]);
      if (failures) printf(" and %d failures.",failures);
      printf(" (from %d samples)\n",successes+failures);
    }
  }
  

}


int main(void)
{
  printf("Vector length = %lld ($%llx)\n",VECTOR_LENGTH,VECTOR_LENGTH);
  run_tests();
}
