#ifndef HASHFUNCTION_H
#define HASHFUNCTION_H

// #include<string>
#include<openssl/evp.h>


struct HashFunc{
	
};

char *sha1(const char *key);
char *md5(const char *key);

#endif //HASHFUNCTION_H
