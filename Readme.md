# Filters on Redis

Install Redis 4.0 or a later version

## How to compile and execute the redis modules?

Below are the instructions to compile and run bloom filters on redis
Create object files of all the dependent source files as follows:
```
gcc -c -fPIC bloom.c
gcc -c -fPIC MurmurHash.c
gcc -c -fPIC redis_bloom.c
```
Create a shared object file which could be loaded on to Redis

`ld -o redis_bloom.so redis_bloom.o bloom.o MurmurHash.o -Bsymbolic -lc -shared`

Start the redis-server with our bloom redis module loaded on it by running the following command in the `redis-stable/src/ directory`

`redis-server --loadmodule /path/to/redis_bloom.so`

The redis-cli can now be started running the command:

`redis-cli`

The cuckoo, dynamic cuckoo and dynamic bloom filters can be compiled and executed similarly.

## Usage on redis-cli

### Bloom Filter

`BF.Create <key> capacity max_allowable_error`

Creates a bloom filter named <key> which can hold membership information of ​ capacity elements with a false positive probability error of max_allowable_error
The number of hash functions to be used is calculated as `HashCounts = − log(false PositiveError)/log(2)`

The number of bits in the bloom filter created = `2*BitLength = − capacity * log(falsePositiveError)/ln(2)`

`BF.Add <key> <value>`

Adds element ​ value ​ the bloom filter ​ key

`BF.Check <key> <value>`

Checks if ​ value ​ element is present in bloom filter ​ key


### Dynamic Bloom Filter
This holds an array of bloom filters which keeps adding elements to the topmost
bloom filter. When a bloom filter reaches its maximum capacity, a new bloom
filter is initialized over it.

`DBF.Create <key> interval max_error`

Creates a dynamic bloom filter named <key> initially holding just one bloom filter where each bloom filter can  contain a maximum of ​ interval elements and maintains a false positive error of ​max_error

`DBF.Add <key> <value>`

Adds element ​ value ​ to the topmost (active) bloom filter of the dynamic bloom filter named ​ key 
If the topmost bloom filter is full, a new bloom filter is created

`DBF.Check <key> <value>`

Checks for the element ​ value ​ in the dynamic bloom filter named ​ key
Checks for the possibility of presence of the element in each bloom filter of the dynamic bloom. Only if every bloom filter denies containing the element, it is concluded that the element has indeed not been added till now


### Cuckoo filters

`CF.Create <key> capacity max_kick_attempts`

Creates a cuckoo filter that can contain ​ capacity elements and will try to kick ​ max_kick_attempts ​ elements out of their nest before ceasing 
The elements are stored as two byte fingerprints and each bucket can contain four elements

`CF.ADD <key> <value>`

The element ​ value ​ is added to cuckoo filter named ​ key 
If the both the buckets are full, one of the elements in either of the buckets is randomly chosen to be kicked out (which will then find their alternative bucket)

`CF.Check <key> <value>`

Checks if​ value ​ is present in ​ key

`CF.Delete <key> <value>`

Deletes the ​ value ​ from ​ key