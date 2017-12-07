
#ifndef CUCKOO_FILTER_H
#define CUCKOO_FILTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define CUCKOO_NESTS_PER_BUCKET     4

static inline uint32_t murmurhash (const void *, uint32_t, uint32_t);
static inline uint32_t hash (const void *, uint32_t, uint32_t, uint32_t,
  uint32_t);

typedef struct {
  uint16_t              fingerprint;
} __attribute__((packed)) cuckoo_nest_t;

typedef struct {
  uint32_t              fingerprint;
  uint32_t              h1;
  uint32_t              h2;
  uint32_t              padding;
} __attribute__((packed)) cuckoo_item_t;

typedef struct {
  bool                  was_found;
  cuckoo_item_t         item;
} cuckoo_result_t;

typedef struct  {
  uint32_t              bucket_count;
  uint32_t              nests_per_bucket;
  uint32_t              mask;
  uint32_t              max_kick_attempts;
  uint32_t              seed;
  uint32_t              padding;
  uint32_t              key_count;
  cuckoo_item_t         victim;
  cuckoo_item_t        *last_victim;
  cuckoo_nest_t         bucket[1];
} __attribute__((packed)) cuckoo_filter_t;

typedef enum {
  CUCKOO_FILTER_OK = 0,
  CUCKOO_FILTER_NOT_FOUND,
  CUCKOO_FILTER_FULL,
  CUCKOO_FILTER_ALLOCATION_FAILED,
} CUCKOO_FILTER_RETURN;

 
CUCKOO_FILTER_RETURN
cuckoo_filter_new (
  cuckoo_filter_t     **filter,
  size_t                max_key_count,
  size_t                max_kick_attempts,
  uint32_t              seed
);

CUCKOO_FILTER_RETURN
cuckoo_filter_free (
  cuckoo_filter_t     **filter
);

CUCKOO_FILTER_RETURN
cuckoo_filter_add (
  cuckoo_filter_t      *filter,
  const void                 *key,
  size_t                key_length_in_bytes
);

CUCKOO_FILTER_RETURN
cuckoo_filter_remove (
  cuckoo_filter_t      *filter,
  const void           *key,
  size_t                key_length_in_bytes
);

CUCKOO_FILTER_RETURN
cuckoo_filter_contains (
  cuckoo_filter_t      *filter,
  const void                 *key,
  size_t                key_length_in_bytes
);

CUCKOO_FILTER_RETURN
print_cuckoo (
  cuckoo_filter_t *cuckoo
  );

#endif /* CUCKOO_FILTER_H */

