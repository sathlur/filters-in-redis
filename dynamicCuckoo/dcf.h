#ifndef DYNAMICCUCKOOFILTER_H_
#define DYNAMICCUCKOOFILTER_H_


#include"cf.h"
#include"linklist.h"
// #include<list>	// create alternative
#include<math.h>
#include <stdint.h>
#include <stdbool.h>
// #include<iostream>	// will this work?

typedef struct{

	int capacity;
	int single_capacity;

	int single_table_length;

	double false_positive;
	double single_false_positive;

	double fingerprint_size_double;
	int fingerprint_size;

	Victim victim;

	cuckoo_filter_t* curCF;
	cuckoo_filter_t* nextCF; //it's temporary. uses getNextCF() to get the next CF;


	//record the items inside DCF
	int counter;

	// the link list of building blocks CF1, CF2, ...
	list_t *cf_list;
}dcf_t;

// declare functions here
bool dcf_create(dcf_t **dynamic_filter, const size_t item_num, const double fp, const size_t exp_block_num);
bool dcf_free(dcf_t *dynamic_filter);
bool dcf_add(dcf_t *dynamic_filter, const char* item);
cuckoo_filter_t* getNextCF(dcf_t *dynamic_filter, cuckoo_filter_t *curCF);
bool failureHandle(dcf_t *dynamic_filter ,Victim *victim);
bool dcf_check(dcf_t *dynamic_filter, const char* item);
bool dcf_delete(dcf_t *dynamic_filter, const char* item);
void generateIF_dcf(const char* item, size_t *index, uint32_t *fingerprint, int fingerprint_size, int single_table_length);
void generateA_dcf(size_t index, uint32_t fingerprint, size_t *alt_index, int single_table_length);
int getFingerprintSize();
float size_in_mb();
uint64_t upperpower2(uint64_t x);
#endif //DYNAMICCUCKOOFILTER_H
