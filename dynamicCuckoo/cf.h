#ifndef CUCKOOFILTER_H_
#define CUCKOOFILTER_H_

#include<string.h>
#include<stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include"hashfunction.h"
#include"bithack.h"

#define MaxNumKicks 10

typedef struct {
	size_t index;
	uint32_t fingerprint;
}Victim;

typedef struct{
	char* bit_array;
} Bucket;

struct Cuckoo_filter_t {
	int capacity;
	size_t single_table_length;
	size_t fingerprint_size;
	size_t bits_per_bucket;
	size_t bytes_per_bucket;
	Bucket* bucket;
	uint32_t mask;
	bool is_full;
	bool is_empty;
	int counter;
	struct Cuckoo_filter_t * next;
	struct Cuckoo_filter_t * front;
};

typedef struct Cuckoo_filter_t cuckoo_filter_t;

// add function declarations here

int cf_create(cuckoo_filter_t **filter ,const size_t table_length, const size_t fingerprint_bits, const int single_capacity);
int cf_free(cuckoo_filter_t *filter);
int cf_add_1(cuckoo_filter_t *filter, const char* item, Victim *victim);
bool cf_add_2(cuckoo_filter_t *filter, size_t index, uint32_t fingerprint, bool kickout, Victim *victim);
bool cf_check(cuckoo_filter_t *filter ,const char* item);
bool cf_delete(cuckoo_filter_t *filter ,const char* item);
bool insertImpl(cuckoo_filter_t *filter, const size_t index, const uint32_t fingerprint, const bool kickout, Victim *victim);
bool queryImpl(cuckoo_filter_t *filter, const size_t index, const uint32_t fingerprint);
bool deleteImpl(cuckoo_filter_t *filter, const size_t index, const uint32_t fingerprint);
void generateIF(const char* item, size_t *index, uint32_t *fingerprint, int fingerprint_size, int single_table_length);
void generateA(size_t index, uint32_t fingerprint, size_t (*alt_index), int single_table_length);
uint32_t read(cuckoo_filter_t *filter, size_t index, size_t pos);
void write(cuckoo_filter_t *filter, size_t index, size_t pos, uint32_t fingerprint);
bool transfer(cuckoo_filter_t *filter, cuckoo_filter_t* tarCF);

#endif
