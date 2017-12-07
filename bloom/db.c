#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "db.h"
#include "murmurhash2.h"

int	// return error code
bloom_create(bloomFilter **result, uint32_t capacity, double MaxError) {
	// bloomFilter *result;
	if ((*result=malloc(sizeof(bloomFilter)))==NULL) {
		return 0;	// failed
	}
	(*result)->capacity=capacity;
	(*result)->sizeOfBF=ceil(abs(capacity*log(MaxError)/0.48045301391));
	if (((*result)->bf=calloc(ceil((*result)->sizeOfBF/8), sizeof(char)))==NULL) {
		return 0;	// failed;
	}
	// (*result)->MaxError=MaxError;	// not using it for the current implementation of dynamic bloom... to be changed later
	(*result)->keyCount=0;
	(*result)->HashCount=floor(abs(log(MaxError)/log(2)));	// refer to the wikipedia page on bloom filters
	//printf("HashCount inside: %d\n", result->HashCount);
	print_bloom(*result);
	return 1;	// success
}

int
dbf_create(dynamicBloom **dbf, uint32_t interval, double MaxError) {
	if ((*dbf=malloc(sizeof(dynamicBloom)))==NULL) {
		return 0; 	// failed
	}

	(*dbf)->interval=interval;
	(*dbf)->matrix_size=1;
	(*dbf)->MaxError=MaxError;
	
	(*dbf)->matrix=malloc(sizeof(bloomFilter *));
	bloomFilter *bloom;
	if (bloom_create(&((*dbf)->matrix[0]), interval, MaxError)==0) {
		return 0;
	}
	return 1;
}

void 
print_bloom(bloomFilter *bloom) {
	printf("bloom Credentials: %d %d %d %d %s\n", bloom->HashCount, bloom->capacity, bloom->keyCount, bloom->sizeOfBF, bloom->bf);
}

void
bloom_free(bloomFilter *bloom) {
	free(bloom->bf);
	free(bloom);
}

bloom_hashval bloom_calc_hash(const char *buffer, int len) {
    bloom_hashval rv;
    rv.a = murmurhash2(buffer, len, 0x9747b28c);
    rv.b = murmurhash2(buffer, len, rv.a);
    return rv;
}

int
bloom_add(bloomFilter *bloom, const char *buffer, int len) {
	bloom_hashval rv=bloom_calc_hash(buffer, len);
	int mod=bloom->sizeOfBF;
	int i;
	printf("Inside bloom_add %d %d\n", mod, bloom->HashCount);
	for (i=0; i<bloom->HashCount; i++) {
		int x=(rv.a + i*rv.b)%mod;
		printf("hash %d: %d\n", i, x);
		bloom->bf[x>>3]|=1<<(x%8);
	}
	bloom->keyCount++;
	return 1;
}

bloomFilter *
get_cur_bloom(dynamicBloom *dbf) {
	if (dbf->cur_keys>=dbf->interval) {
		return NULL;
	}
	return dbf->matrix[dbf->matrix_size-1];
}

int 
add_row(dynamicBloom *dbf) {
	// printf("*******Adding row*******\n");
	dbf->matrix_size++;
	dbf->matrix=realloc(dbf->matrix, dbf->matrix_size*sizeof(bloomFilter*));
	// bloomFilter *bloom;
	if (bloom_create(&(dbf->matrix[dbf->matrix_size-1]), dbf->interval, dbf->MaxError)==0) {
		return 0;
	}
	dbf->cur_keys=0;
	return 1;
}

int
dbf_add(dynamicBloom *dbf, const char *buffer, int len) {
	bloomFilter *bloom;
	if ((bloom=get_cur_bloom(dbf))==NULL) {
		if (add_row(dbf)<0) {
			return -1;
		}
		bloom=dbf->matrix[dbf->matrix_size-1];
	}

	bloom_add(bloom, buffer, len);
	dbf->cur_keys++;
	return 1;
	
}

int
bloom_check(bloomFilter *bloom, const char *buffer, int len) {
	bloom_hashval rv=bloom_calc_hash(buffer, len);
	int mod=bloom->sizeOfBF;
	int i;
	// printf("Bloom_check: %d %d\n", mod, bloom->HashCount);
	for (i=0; i<bloom->HashCount; i++) {
		int x=(rv.a+i*rv.b)%mod;
		printf("hash %d: %d\n", i, x);
		if ((bloom->bf[x>>3]&(1<<(x%8)))==0)
			return 0;	// element definitely absent
	}
	printf("element found\n");
	return 1; 	// element may be present
}

int 
dbf_check(dynamicBloom *dbf, const char *buffer, int len) {
	int i;
	for (i=0; i<dbf->matrix_size; i++) {
		if (bloom_check(dbf->matrix[i], buffer, len)==1) {
			return 1;
		}
	}
	return 0;
}

/*
int main(void)
{
	
	bloomFilter *bloom;
	if ((bloom_create(&bloom, 8, 0.1))==0) {
		printf("Creation failed\n");
		return 1;
	}
	print_bloom(bloom);

	char *element="Hello World", *element2="my second object", *element3="what about now";
	int retval=bloom_check(bloom, element, strlen(element));
	printf("Should be 0: %d\n", retval);
	bloom_add(bloom, element, strlen(element));
	bloom_add(bloom, element2, strlen(element2));
	print_bloom(bloom);
	retval=bloom_check(bloom, element, strlen(element));
	printf("Should be 1: %d\n", retval);
	retval=bloom_check(bloom, element3, strlen(element3));
	printf("should be 0, but can't say%d\n", retval);
	return 0;
}
*/