
// Number of redundant bloom-filter vectors
// Time consumption is directly linear to this!
#define NUM_VECTORS 20

// Should be at least 10x the max number of keys to be stored
#define MAX_KEYS (10000000LL)
#define VECTOR_LENGTH (MAX_KEYS*10LL)

// Each query reduces the false positive rate by ~16x,
// so 8 queries reduces our false positive rate to less than 2^(-64)
#define NUM_QUERIES 8

int insert_key(unsigned char *key,unsigned long long key_length);
int get_vector_bit(int vector_number,
		   unsigned long long bit);
int set_vector_bit(int vector_number,unsigned long long bit);
unsigned long long key_bit_for_vector(int vector_number,unsigned char *key,
		       unsigned long long key_length);
int vector_initialise(void);
  
extern unsigned char *bit_vectors[NUM_VECTORS];
extern unsigned char bits[8];

