#include "linklist.h"

bool list_create(list_t **list, size_t single_table_length, size_t fingerprint_size, double single_capacity) {
	list_t *new_list=malloc(sizeof(list_t));

	cf_create(&(new_list->cf_pt), single_table_length, fingerprint_size, single_capacity);
	cf_create(&(new_list->tail_pt), single_table_length, fingerprint_size, single_capacity);
	new_list->num=0;

	*list=new_list;
	return true;
}

bool list_free(list_t *list) {
	free(list->cf_pt);
	list->cf_pt=NULL;
	free(list->tail_pt);
	list->tail_pt=NULL;
	free(list);
}

bool list_remove(list_t *list, cuckoo_filter_t *cf_remove) {
	cuckoo_filter_t *frontCF=cf_remove->front;
	if (frontCF==NULL) {
		list->cf_pt=cf_remove->next;
	}else {
		frontCF->next=cf_remove->next;
	}
	cf_remove=NULL;
	list->num--;
}