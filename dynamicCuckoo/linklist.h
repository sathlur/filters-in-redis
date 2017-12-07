/*bool remove(list_t *list, cuckoo_filter_t *cf_remove) {

 * linklist.h
 *
 *  Created on: Jan 4, 2017
 *      Author: liaoliangyi
 */

#ifndef LINKLIST_H_
#define LINKLIST_H_
#include <stdlib.h>
#include <stdbool.h>
#include "cf.h"

typedef struct {
	cuckoo_filter_t *cf_pt;
	cuckoo_filter_t *tail_pt;
	int num;
}list_t;

bool list_create(list_t **list, size_t single_table_length, size_t fingerprint_size, double single_capacity);
bool list_free(list_t *list);
bool list_remove(list_t *list, cuckoo_filter_t *cf_remove);

#endif /* LINKLIST_H_ */
