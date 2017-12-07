// bloom.h
#ifndef _DYNAMICBLOOM_H
#define _DYNAMICBLOOM_H
#include <stdlib.h>

// the bloom filter data type (structure)
typedef struct {
	uint32_t capacity;
	unsigned char *bf;
	uint32_t HashCount;
	uint32_t keyCount;	// number of keys added
	uint32_t sizeOfBF;
	// double MaxError;
} bloomFilter;

typedef struct {
	bloomFilter **matrix;
	uint32_t matrix_size;
	uint32_t cur_keys;
	uint32_t interval;
	double MaxError;
}dynamicBloom;

typedef struct {
    unsigned int a;
    unsigned int b;
} bloom_hashval;

int	// return error code
bloom_create(bloomFilter **bloom, uint32_t sizeOfBF, double MaxError);

void	// free a created bloom - reference sent as an argument
bloom_free(bloomFilter *bloom);

bloom_hashval
bloom_calc_hash(const char *buffer, int len);

int
bloom_add(bloomFilter *bloom, const char *buffer, int len);

int
bloom_check(bloomFilter *bloom, const char *buffer, int len);

void
print_bloom(bloomFilter *bloom);

int
dbf_create(dynamicBloom **dbf,uint32_t interval, double MaxError);

bloomFilter *
get_cur_bloom(dynamicBloom *dbf);

int 
add_row(dynamicBloom *dbf);

int
dbf_add(dynamicBloom *dbf, const char *buffer, int len);

int 
dbf_check(dynamicBloom *dbf, const char *buffer, int len);



#endif