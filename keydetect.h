
// Should be a power of 2 for efficiency
#define NUM_VECTORS 1024

// Should be at least 10x the max number of keys to be stored
#define MAX_KEYS 10000
#define VECTOR_LENGTH (MAX_KEYS*10)

// Each query reduces the false positive rate by ~16x,
// so 8 queries reduces our false positive rate to less than 2^(-64)
#define NUM_QUERIES 8

int insert_key(unsigned char *key,int key_length);
int get_vector_bit(int vector_number,int bit);
int key_bit_for_vector(int vector_number,unsigned char *key,int key_length);
  
