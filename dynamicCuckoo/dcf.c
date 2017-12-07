/*
 * dynamiccuckoofilter.cpp
 *
 *  Created on: Nov 14, 2017
 *      Author: liaoliangyi
 */

#include "dcf.h"

bool dcf_create(dcf_t **dynamic_filter, const size_t item_num, const double fp, const size_t exp_block_num){

	dcf_t *new_dynamic_filter;
	new_dynamic_filter->capacity = item_num;

	new_dynamic_filter->single_table_length = upperpower2(new_dynamic_filter->capacity/4.0/exp_block_num);//2048 1024 512 256 128 ---!!!---must be the power of 2---!!!---
	new_dynamic_filter->single_capacity = new_dynamic_filter->single_table_length*0.9375*4;//s=6 1920 s=12 960 s=24 480 s=48 240 s=96 120

	new_dynamic_filter->false_positive = fp;
	new_dynamic_filter->single_false_positive = 1-pow(1.0-new_dynamic_filter->false_positive, ((double)new_dynamic_filter->single_capacity/new_dynamic_filter->capacity));

	new_dynamic_filter->fingerprint_size_double = ceil(log(8.0/new_dynamic_filter->single_false_positive)/log(2));
	if(new_dynamic_filter->fingerprint_size_double>0 && new_dynamic_filter->fingerprint_size_double<=4){
		new_dynamic_filter->fingerprint_size = 4;
	}else if(new_dynamic_filter->fingerprint_size_double>4 && new_dynamic_filter->fingerprint_size_double<=8){
		new_dynamic_filter->fingerprint_size = 8;
	}else if(new_dynamic_filter->fingerprint_size_double>8 && new_dynamic_filter->fingerprint_size_double<=12){
		new_dynamic_filter->fingerprint_size = 12;
	}else if(new_dynamic_filter->fingerprint_size_double>12 && new_dynamic_filter->fingerprint_size_double<=16){
		new_dynamic_filter->fingerprint_size = 16;
	}else if(new_dynamic_filter->fingerprint_size_double>16 && new_dynamic_filter->fingerprint_size_double<=24){
		new_dynamic_filter->fingerprint_size = 16;
	}else if(new_dynamic_filter->fingerprint_size_double>24 && new_dynamic_filter->fingerprint_size_double<=32){
		new_dynamic_filter->fingerprint_size = 16;
	}else{
		//cout<<"fingerprint out of range!!!"<<endl;
		//printf("fingerprint out of range\n");
		new_dynamic_filter->fingerprint_size = 16;
	}

	new_dynamic_filter->counter = 0;

	cf_create(&(new_dynamic_filter->curCF), new_dynamic_filter->single_table_length, new_dynamic_filter->fingerprint_size, new_dynamic_filter->single_capacity);
	cf_create(&(new_dynamic_filter->nextCF), new_dynamic_filter->single_table_length, new_dynamic_filter->fingerprint_size, new_dynamic_filter->single_capacity);
	
	list_create(&(new_dynamic_filter->cf_list), new_dynamic_filter->single_table_length, new_dynamic_filter->fingerprint_size, new_dynamic_filter->single_capacity);
	// new_dynamic_filter->cf_list = new LinkList(single_table_length, fingerprint_size, single_capacity);
	new_dynamic_filter->cf_list->cf_pt = new_dynamic_filter->curCF;
	new_dynamic_filter->cf_list->tail_pt = new_dynamic_filter->curCF;
	new_dynamic_filter->cf_list->num = 1;

	*dynamic_filter=new_dynamic_filter;
}

bool dcf_free(dcf_t *dynamic_filter) {
	free(dynamic_filter->curCF);
	free(dynamic_filter->nextCF);
	list_free(dynamic_filter->cf_list);
	free(dynamic_filter);
}

bool dcf_add(dcf_t *dynamic_filter, const char* item){
	if(dynamic_filter->curCF->is_full == true){
		dynamic_filter->curCF = getNextCF(dynamic_filter, dynamic_filter->curCF);
	}

	if(cf_add_1(dynamic_filter->curCF, item, &dynamic_filter->victim)){
		dynamic_filter->counter++;
	}else{
		failureHandle(dynamic_filter, &dynamic_filter->victim);
		dynamic_filter->counter++;
	}

	return true;
}

cuckoo_filter_t* getNextCF(dcf_t *dynamic_filter, cuckoo_filter_t *curCF){
	if(curCF == dynamic_filter->cf_list->tail_pt){
		cf_create(&(dynamic_filter->nextCF), dynamic_filter->single_table_length, dynamic_filter->fingerprint_size, dynamic_filter->single_capacity);
		// nextCF = new CuckooFilter(single_table_length, fingerprint_size, single_capacity);
		curCF->next = dynamic_filter->nextCF;
		dynamic_filter->nextCF->front = curCF;
		dynamic_filter->cf_list->tail_pt = dynamic_filter->nextCF;
		dynamic_filter->cf_list->num++;
	}else{
		dynamic_filter->nextCF = curCF->next;
		if(dynamic_filter->nextCF->is_full){
			dynamic_filter->nextCF = getNextCF(dynamic_filter, dynamic_filter->nextCF);
		}
	}
	return dynamic_filter->nextCF;
}

bool failureHandle(dcf_t *dynamic_filter ,Victim *victim){
	dynamic_filter->nextCF = getNextCF(dynamic_filter, dynamic_filter->curCF);
	if(cf_add_2(dynamic_filter->nextCF, victim->index, victim->fingerprint,true, victim) == false){
		dynamic_filter->nextCF = getNextCF(dynamic_filter, dynamic_filter->nextCF);
		failureHandle(dynamic_filter, victim);
	}
	return true;
}

bool dcf_check(dcf_t *dynamic_filter, const char* item){
	size_t *index, *alt_index;
	uint32_t *fingerprint;

	generateIF_dcf(item, index, fingerprint, dynamic_filter->fingerprint_size, dynamic_filter->single_table_length);
	generateA_dcf(*index, *fingerprint, alt_index, dynamic_filter->single_table_length);

	cuckoo_filter_t *query_pt = dynamic_filter->cf_list->cf_pt;
	for(int count = 0; count<dynamic_filter->cf_list->num; count++){

		if(queryImpl(query_pt, *index, *fingerprint)){
			return true;
		}else if(queryImpl(query_pt, *alt_index, *fingerprint)){
			return true;
		}else{
			query_pt = query_pt->next;
		}
		if(query_pt == 0){
			break;
		}

//		if(query_pt->queryImpl(index, fingerprint)){
//			return true;
//		}
//		generateA_dcf(index, fingerprint, alt_index, single_table_length);
//		if(query_pt->queryImpl(alt_index, fingerprint)){
//			return true;
//		}else{
//			query_pt = query_pt->next;
//		}
//		if(query_pt == 0){
//			break;
//		}
	}
	return false;
}

bool dcf_delete(dcf_t *dynamic_filter, const char* item){
	size_t *index, *alt_index;
	uint32_t *fingerprint;

	generateIF_dcf(item, index, fingerprint, dynamic_filter->fingerprint_size, dynamic_filter->single_table_length);
	generateA_dcf(*index, *fingerprint, alt_index, dynamic_filter->single_table_length);
	cuckoo_filter_t *delete_pt = dynamic_filter->cf_list->cf_pt;
	for(int count = 0; count<dynamic_filter->cf_list->num; count++){
		if(queryImpl(delete_pt, *index, *fingerprint)){
			if(deleteImpl(delete_pt, *index, *fingerprint)){
				dynamic_filter->counter--;
				return true;
			}
		}else if(queryImpl(delete_pt, *alt_index, *fingerprint)){
			if(deleteImpl(delete_pt, *alt_index , *fingerprint)){
				dynamic_filter->counter--;
				return true;
			}
		}else{
			delete_pt = delete_pt->next;
		}
	}
	return false;
}


/*
bool compact(dcf_t *dynamic_filter){
	int queue_length = 0;
	CuckooFilter* temp = dynamic_filter->cf_list->cf_pt;
	for(int count = 0; count<dynamic_filter->cf_list->num; count++){
		if(!temp->is_full){
			queue_length++;
		}
		temp = temp->next;
	}
	if(queue_length == 0){
		return true;
	}

	cuckoo_filter_t *new_cuckoo;
	cf_create(&new_cuckoo, )
	CuckooFilter** cfq  = new CuckooFilter*[queue_length];
	int pos = 0;
	temp = cf_list->cf_pt;
	for(int count = 0; count<cf_list->num; count++){
		if(!temp->is_full){
			cfq[pos] = temp;
			pos++;
		}
		temp = temp->next;
	}

	sort(cfq, queue_length);
	for(int i = 0; i<queue_length-1; i++){
		for(int j = queue_length-1; j>i; j--){
			cfq[i]->transfer(cfq[j]);
			if(cfq[i]->is_empty == true){
				cf_list->remove(cfq[i]);
				break;
			}
		}
	}
	if(cfq[queue_length-1]->is_empty == true){
		cf_list->remove(cfq[queue_length-1]);
	}

	return true;
}

void DynamicCuckooFilter::sort(CuckooFilter** cfq, int queue_length){
	CuckooFilter* temp;
	for(int i = 0; i<queue_length-1; i++){
		for(int j = 0; j<queue_length-1-i; j++){
			if(cfq[j]->counter > cfq[j+1]->counter){
				temp = cfq[j];
				cfq[j] = cfq[j+1];
				cfq[j+1] = temp;
			}
		}
	}
}
*/


void generateIF_dcf(const char* item, size_t *index, uint32_t *fingerprint, int fingerprint_size, int single_table_length){
	int len=sizeof(*item)-1;
	char *value=SHA1(item, len)	;
	// uint64_t hv = *((uint64_t*) value.c_str());
	uint64_t hv = *(uint64_t*) value;

	*index = ((uint32_t) (hv >> 32)) % single_table_length;
	*fingerprint = (uint32_t) (hv & 0xFFFFFFFF);
	*fingerprint &= ((0x1ULL<<fingerprint_size)-1);
	*fingerprint += (*fingerprint == 0);
}

void generateA_dcf(size_t index, uint32_t fingerprint, size_t *alt_index, int single_table_length){
	*alt_index = (index ^ (fingerprint * 0x5bd1e995)) % single_table_length;
}


// won't be need in C -  deprecated
int getFingerprintSize(dcf_t *dynamic_filter){
	return dynamic_filter->fingerprint_size;
}

float size_in_mb(dcf_t *dynamic_filter){
	return dynamic_filter->fingerprint_size * 4.0 * dynamic_filter->single_table_length * dynamic_filter->cf_list->num / 8 / 1024 / 1024;
}

uint64_t upperpower2(uint64_t x) {
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  x++;
  return x;
}
