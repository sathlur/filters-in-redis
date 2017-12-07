/*
 * hashfunction.cpp
 *
 *  Created on: Nov 14, 2017
 *      Author: liaoliangyi
 */



#include "hashfunction.h"

char *sha1(const char* key){
	EVP_MD_CTX mdctx;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len;

	EVP_DigestInit(&mdctx, EVP_sha1());
	EVP_DigestUpdate(&mdctx, (const void*) key, sizeof(key));
	EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);

	char *result=calloc((size_t)md_len, sizeof(char));
	result=(char *)md_value;
	return result;
}

char *md5(const char* key){
	EVP_MD_CTX mdctx;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len;

	EVP_DigestInit(&mdctx, EVP_md5());
	EVP_DigestUpdate(&mdctx, (const void*) key, sizeof(key));
	EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);

	char *result=calloc((size_t)md_len, sizeof(char));
	result=(char *)md_value;
	return result;
}
