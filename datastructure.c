/*
  This program maintains a list of 2^k vectors of n bits length.
  Each key inserted will set exactly one bit in each vector.
  Thus a client can ask for a random selection of vectors, and check that
  all have the required bits set, thus confirming that (provided the vectors
  are maintained to a certain degree of sparseness) the key is almost certainly
  present, or almost certainly not present in the key store.

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <assert.h>

#include "keydetect.h"
#include "sha2.h"

unsigned char bit_vectors[NUM_VECTORS][VECTOR_LENGTH>>3];

unsigned char bits[8]={1,2,4,8,16,32,64,128};

int get_vector_bit(int vector_number,int bit)
{
  assert(vector_number>=0);
  assert(vector_number<NUM_VECTORS);
  assert(bit>=0);
  assert(bit<VECTOR_LENGTH);
  if (bit_vectors[vector_number][bit>>3]&bits[bit&7]) return 1;
  return 0;
}


int set_vector_bit(int vector_number,int bit)
{
  assert(vector_number>=0);
  assert(vector_number<NUM_VECTORS);
  assert(bit>=0);
  assert(bit<VECTOR_LENGTH);
  bit_vectors[vector_number][bit>>3]|=bits[bit&7];
  return 0;
}

int key_bit_for_vector(int vector_number,unsigned char *key,int key_length)
{
  unsigned char hash[crypto_hash_sha512_BYTES];
  crypto_hash_sha512_state s;
  bzero(&s,sizeof s);

  // Calculate pre and post key salts
  unsigned char presalt[16] ="DEEPFRYINOIL";
  unsigned char postsalt[16]="EATWHILEHOT!";
  for(int i=0;i<4;i++) {
    unsigned char v=(vector_number>>0)&0xff;
    presalt[12+i]=v;
    presalt[12+i]=v;
    vector_number=vector_number>>8;
  }
  
  crypto_hash_sha512_init(&s);
  crypto_hash_sha512_update(&s,presalt,sizeof presalt);
  crypto_hash_sha512_update(&s,key,key_length);
  crypto_hash_sha512_update(&s,postsalt,sizeof postsalt);
  crypto_hash_sha512_final(&s,hash);

#if 0
  // Now hash the hash a few times
  for(int i=0;i<10;i++) {
    crypto_hash_sha512_init(&s);
    crypto_hash_sha512_update(&s,presalt,sizeof presalt);
    crypto_hash_sha512_update(&s,hash,sizeof hash);
    crypto_hash_sha512_update(&s,postsalt,sizeof postsalt);
    crypto_hash_sha512_final(&s,hash);
  }
#endif

  unsigned int bit=hash[0]+(hash[1]<<8)+(hash[2]<<16)+(hash[3]<<24);
  bit=bit%VECTOR_LENGTH;
  return bit;
}

int insert_key(unsigned char *key,int key_length)
{
  int i;
  for(i=0;i<NUM_VECTORS;i++) {
    set_vector_bit(i,key_bit_for_vector(i,key,key_length));
  }
  return 0;
}
