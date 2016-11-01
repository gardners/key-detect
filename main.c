#include <stdio.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "keydetect.h"

long long gettime_us()
{
  struct timeval nowtv;
  // If gettimeofday() fails or returns an invalid value, all else is lost!
  if (gettimeofday(&nowtv, NULL) == -1)
    return -1;
  if (nowtv.tv_sec < 0 || nowtv.tv_usec < 0 || nowtv.tv_usec >= 1000000)
    return -1;
  return nowtv.tv_sec * 1000000LL + nowtv.tv_usec;
}

int main(int argc,char **argv)
{
  printf("Testing insertion speed of %d keys...\n",MAX_KEYS);

  FILE *f=fopen("/dev/random","r");

  unsigned char test_keys[1000][64];
  int test_key_count=0;
  
  long long start=gettime_us();
  unsigned char key[64];
  time_t last_time=time(0);
  for(int i=0;i<MAX_KEYS;i++) {
    if (fread(key,64,1,f)!=1) {
      fprintf(stderr,"Failed to read 64 bytes from /dev/random.\n");
    }    
    if (last_time!=time(0)) { printf("%d\n",i); last_time=time(0); }
  }
  long long end=gettime_us();
  long long key_gen_time=end-start;
  printf("Key generation (without inserting) of %d keys required %lldusec.\n",
	 MAX_KEYS,key_gen_time);
  start=gettime_us();
  for(int i=0;i<MAX_KEYS;i++) {
    if (fread(key,64,1,f)!=1) {
      fprintf(stderr,"Failed to read 64 bytes from /dev/random.\n");
    }
    if (test_key_count<1000) {
      bcopy(key,test_keys[test_key_count++],64);
    }
    insert_key(key,sizeof key);
    if (last_time!=time(0)) { printf("%d\n",i); last_time=time(0); }
  }
  end=gettime_us();
  long long key_insert_time=end-start;
  printf("Key generation (with inserting) of %d keys required %lldusec.\n",
	 MAX_KEYS,key_insert_time);
  printf("Subtracting key generation time, key insertion required %.1f usec/key.\n",
	 (key_insert_time-key_gen_time)*1.0/MAX_KEYS);

  // Measure density in vectors
  for(int v=0;v<NUM_VECTORS;v++) {
    int bitcount=0;
    for(int i=0;i<VECTOR_LENGTH;i++)
      if (get_vector_bit(v,i)) bitcount++;
    float density=bitcount*100.0/VECTOR_LENGTH;
    if (density>=(MAX_KEYS*100.0/VECTOR_LENGTH)) {
      printf("Vector #%d has erroneously high density of %.2f%%.\n",
	     v,density);
    }
  }

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
      if (failures) printf(" and %.1f%% failures.",
			   failures*100.0/(failures+successes));
      printf(" (from %d samples)\n",successes+failures);
    }
  }
  
  return 0;
}
