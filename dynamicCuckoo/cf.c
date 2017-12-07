#include "cf.h"


int cf_create(cuckoo_filter_t **filter ,const size_t table_length, const size_t fingerprint_bits, const int single_capacity){
	cuckoo_filter_t *new_filter=malloc(sizeof(cuckoo_filter_t));

	new_filter->fingerprint_size = fingerprint_bits;
	new_filter->bits_per_bucket = new_filter->fingerprint_size*4;
	new_filter->bytes_per_bucket = (new_filter->fingerprint_size*4+7)>>3;
	new_filter->single_table_length = table_length;
	new_filter->counter = 0;
	new_filter->capacity = single_capacity;
	new_filter->is_full = false;
	new_filter->is_empty = true;
	new_filter->next = NULL;
	new_filter->front = NULL;
	new_filter->mask = (1ULL << new_filter->fingerprint_size) - 1;

	new_filter->bucket = malloc(table_length*sizeof(Bucket));	
	size_t i=0;
	for(i = 0; i<new_filter->single_table_length; i++){
		new_filter->bucket[i].bit_array=calloc(new_filter->bytes_per_bucket, sizeof(char));
	}

	*filter=new_filter;
}

int cf_free(cuckoo_filter_t *filter) {
	size_t i=0;
	for(i=0; i<filter->single_table_length; i++) {
		free(filter->bucket[i].bit_array);
	}
	free(filter->bucket);
	free(filter);
	return true;
}

int cf_add_1(cuckoo_filter_t *filter, const char* item, Victim *victim){
	size_t *index, *alt_index;
	uint32_t *fingerprint;
	generateIF(item, index, fingerprint, filter->fingerprint_size, filter->single_table_length);

	//edit 3-17
	size_t count;
	for(count = 0; count<MaxNumKicks; count++){
		bool kickout = (count != 0);
		if(insertImpl(filter, *index, *fingerprint, kickout, victim)){
			return true;
		}

		if (kickout){
			*index = victim->index;
			*fingerprint = victim->fingerprint;
			generateA(*index, *fingerprint, alt_index, filter->single_table_length);
			index = alt_index;
		}else{
			generateA(*index, *fingerprint, alt_index, filter->single_table_length);
			index = alt_index;
		}
	}

	return false;
}

bool cf_add_2(cuckoo_filter_t *filter, size_t index, uint32_t fingerprint, bool kickout, Victim *victim){
	size_t *alt_index;

	size_t count;
	for(count = 0; count<MaxNumKicks; count++){
		bool kickout = (count != 0);
		if(insertImpl(filter, index, fingerprint, kickout, victim)){
			return true;
		}

		if (kickout){
			index = victim->index;
			fingerprint = victim->fingerprint;
			generateA(index, fingerprint, alt_index, filter->single_table_length);
			index = *alt_index;
		}else{
			generateA(index, fingerprint, alt_index, filter->single_table_length);
			index = *alt_index;
		}
	}
	return false;
}

bool cf_check(cuckoo_filter_t *filter ,const char* item){
	size_t *index, *alt_index;
	uint32_t *fingerprint;
	generateIF(item, index, fingerprint, filter->fingerprint_size, filter->single_table_length);

	if(queryImpl(filter, *index, *fingerprint)){
		return true;
	}
	generateA(*index, *fingerprint, alt_index, filter->single_table_length);
	if(queryImpl(filter, *alt_index, *fingerprint)){
		return true;
	}
	return false;
}

bool cf_delete(cuckoo_filter_t *filter ,const char* item){
	size_t *index, *alt_index;
	uint32_t *fingerprint;
	generateIF(item, index, fingerprint, filter->fingerprint_size, filter->single_table_length);

	if(deleteImpl(filter, *index, *fingerprint)){
		return true;
	}
	generateA(*index, *fingerprint, alt_index, filter->single_table_length);
	if(deleteImpl(filter, *alt_index, *fingerprint)){
		return true;
	}

	return false;
}

bool insertImpl(cuckoo_filter_t *filter, const size_t index, const uint32_t fingerprint, const bool kickout, Victim *victim){
	size_t pos;
	for(pos = 0; pos<4; pos++){
		if(read(filter, index,pos) == 0){
			write(filter, index,pos,fingerprint);
			filter->counter++;
			if(filter->counter == filter->capacity){
				filter->is_full = true;
			}

			if(filter->counter > 0 ){
				filter->is_empty = false;
			}
			return true;
		}
	}
	if(kickout){
		int j = rand()%4;
		victim->index = index;
		victim->fingerprint = read(filter, index,j);
		write(filter, index, j, fingerprint);
	}
	return false;
}

bool queryImpl(cuckoo_filter_t *filter, const size_t index, const uint32_t fingerprint){
	if(filter->fingerprint_size == 4){
		const char* p = filter->bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;
		return hasvalue4(bits, fingerprint);
	}else if(filter->fingerprint_size == 8){
		const char* p = filter->bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;
		return hasvalue8(bits, fingerprint);
	}else if(filter->fingerprint_size == 12){
		const char* p = filter->bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;
		return hasvalue12(bits, fingerprint);
	}else if(filter->fingerprint_size == 16){
		const char* p = filter->bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;
		return hasvalue16(bits, fingerprint);
	}else{
		return false;
	}
}

bool deleteImpl(cuckoo_filter_t *filter, const size_t index, const uint32_t fingerprint){
	size_t pos;
	for(pos = 0; pos<4; pos++){
		if(read(filter, index, pos) == fingerprint){
			write(filter, index, pos, 0);
			filter->counter--;
			if(filter->counter < filter->capacity){
				filter->is_full = false;
			}
			if(filter->counter == 0){
				filter->is_empty = true;
			}
			return true;
		}
	}
	return false;
}

void generateIF(const char* item, size_t *index, uint32_t *fingerprint, int fingerprint_size, int single_table_length){
	int len=sizeof(*item)-1;
	char *value = SHA1(item, len);
	// uint64_t hv = *((uint64_t*) value.c_str());
	uint64_t hv = *((uint64_t*) value);

	*index = ((uint32_t) (hv >> 32)) % single_table_length;
	*fingerprint = (uint32_t) (hv & 0xFFFFFFFF);
	*fingerprint &= ((0x1ULL<<fingerprint_size)-1);
	*fingerprint += (*fingerprint == 0);

}

void generateA(size_t index, uint32_t fingerprint, size_t *alt_index, int single_table_length){
	*alt_index = (index ^ (fingerprint * 0x5bd1e995)) % single_table_length;
}



uint32_t read(cuckoo_filter_t *filter, size_t index, size_t pos){
	const char* p = filter->bucket[index].bit_array;
	uint32_t fingerprint;

	if(filter->fingerprint_size == 4){
		p += (pos >> 1);
		uint8_t bits_8 = *(uint8_t*)p;
		if((pos & 1) == 0){
			fingerprint = (bits_8>>4) & 0xf;
		}else{
			fingerprint = bits_8 & 0xf;
		}
	}else if(filter->fingerprint_size == 8){
		p += pos;
		uint8_t bits_8 = *(uint8_t*)p;
		fingerprint = bits_8 & 0xff;
	}else if(filter->fingerprint_size == 12){
		p += pos+(pos>>1);
		uint16_t bits_16 = *(uint16_t*)p;
		if((pos & 1) == 0){
			fingerprint = bits_16 & 0xfff;
		}else{
			fingerprint = (bits_16 >> 4) & 0xfff;
		}
	}else if(filter->fingerprint_size == 16){
		p += (pos<<1);
		uint16_t bits_16 = *(uint16_t*)p;
		fingerprint = bits_16 & 0xffff;
	}else if(filter->fingerprint_size == 24){
		p += pos+(pos<<1);
		uint32_t bits_32 = *(uint32_t*)p;
		fingerprint = (bits_32 >> 4);
	}else if(filter->fingerprint_size == 32){
		p += (pos<<2);
		uint32_t bits_32 = *(uint32_t*)p;
		fingerprint = bits_32 & 0xffffffff;
	}else{
		fingerprint =0;
	}
	return fingerprint & filter->mask;
}

void write(cuckoo_filter_t *filter, size_t index, size_t pos, uint32_t fingerprint){
	char* p = filter->bucket[index].bit_array;

	if(filter->fingerprint_size == 4){
		p += (pos>>1);
		if((pos & 1) == 0){
			*((uint8_t*)p) &= 0x0f;
			*((uint8_t*)p) |= (fingerprint<<4);
		}else{
			*((uint8_t*)p) &= 0xf0;
			*((uint8_t*)p) |= fingerprint;
		}
	}else if(filter->fingerprint_size == 8){
		((uint8_t*)p)[pos] = fingerprint;
	}else if(filter->fingerprint_size == 12){
		p += (pos + (pos>>1));
		if((pos & 1) == 0){
			*((uint16_t*)p) &= 0xf000; //Little-Endian
			*((uint16_t*)p) |= fingerprint;
		}else{
			*((uint16_t*)p) &= 0x000f;
			*((uint16_t*)p) |= fingerprint<<4;
		}
	}else if(filter->fingerprint_size == 16){
		((uint16_t*) p)[pos] = fingerprint;
	}else if(filter->fingerprint_size == 24){
		p += (pos+ (pos<<1));
		*((uint32_t*)p) &= 0xff000000; //Little-Endian
		*((uint32_t*)p) |= fingerprint;
	}else if(filter->fingerprint_size == 32){
		((uint32_t*) p)[pos] = fingerprint;
	}
}



bool transfer(cuckoo_filter_t *filter, cuckoo_filter_t* tarCF){
	uint32_t fingerprint = 0;

	for(size_t i = 0; i<filter->single_table_length; i++){
		for(int j = 0; j<4; j++){
			fingerprint = read(filter, i, j);
			if(fingerprint != 0){
				if(tarCF->is_full == true){
					return false;
				}
				if(filter->is_empty == true){
					return false;
				}
				Victim victim = {0, 0};
				if(insertImpl(tarCF, i, fingerprint, false, &victim)){
					write(filter, i, j, 0);
					filter->counter--;

					if(filter->counter < filter->capacity){
						filter->is_full = false;
					}
					if(filter->counter == 0){
						filter->is_empty = true;
					}

					if(tarCF->counter == filter->capacity){
						tarCF->is_full = true;
					}

					if(tarCF->counter > 0 ){
						tarCF->is_empty = false;
					}
				}
			}
		}
	}
	return true;
}

