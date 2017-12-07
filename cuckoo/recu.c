#include <stdlib.h>
#include "redismodule.h"
#include "cuckoo_filter.h"

static RedisModuleType *CuckooType;


int cfEcho_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	if (argc!=2) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}
	RedisModuleCallReply *rep = RedisModule_Call(ctx,"ECHO","s",argv[1]);
	// RedisModuleString *mystr = RedisModule_CreateStringFromCallReply(rep);
	size_t len = RedisModule_CallReplyLength(rep);
	// size_t len = RedisModule_ValueLength(key);
	const char *ptr = RedisModule_CallReplyStringPtr(rep,&len);
	RedisModule_ReplyWithSimpleString(ctx,ptr);
}


int cfCreate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc != 4) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	long long max_key_count;
    if (RedisModule_StringToLongLong(argv[2], &max_key_count) != REDISMODULE_OK ||
        max_key_count >= UINT32_MAX) {
        return RedisModule_ReplyWithError(ctx, "ERR bad capacity");
    }

	long long max_kick_attempts;
    if (RedisModule_StringToLongLong(argv[3], &max_kick_attempts) != REDISMODULE_OK ||
        max_kick_attempts >= UINT32_MAX) {
        return RedisModule_ReplyWithError(ctx, "ERR bad capacity");
    }

    long long seed=782386;

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    cuckoo_filter_t *cuckoo;
    if (cuckoo_filter_new(&cuckoo, max_key_count, max_kick_attempts, seed)!=0) {
    	RedisModule_ReplyWithSimpleString(ctx, "ERR could not create filter");
    	return REDISMODULE_ERR;
    }

    RedisModule_ModuleTypeSetValue(key, CuckooType, cuckoo);
    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int cfAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	cuckoo_filter_t *cuckoo=RedisModule_ModuleTypeGetValue(key);

	if (RedisModule_ModuleTypeGetType(key) != CuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);

	int result=cuckoo_filter_add(cuckoo, element, *len);
	RedisModule_CloseKey(key);

	if (result==2) {
		RedisModule_ReplyWithSimpleString(ctx, "ERR The filter is full");
		return REDISMODULE_ERR;
	}
	RedisModule_ReplyWithSimpleString(ctx,"OK");
	return REDISMODULE_OK;

}

int cfCheck_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	if (RedisModule_ModuleTypeGetType(key) != CuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}

	cuckoo_filter_t *cuckoo=RedisModule_ModuleTypeGetValue(key);
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	RedisModule_CloseKey(key);
	if (cuckoo_filter_contains(cuckoo, element, *len)==0) {
		RedisModule_ReplyWithSimpleString(ctx, "May be present");
	}
	else {
		RedisModule_ReplyWithSimpleString(ctx, "Element not present");
	}

	return REDISMODULE_OK;
}

int cfDelete_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	cuckoo_filter_t *cuckoo=RedisModule_ModuleTypeGetValue(key);

	if (RedisModule_ModuleTypeGetType(key) != CuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);

	uint8_t result=cuckoo_filter_remove(cuckoo, element, *len);
	RedisModule_CloseKey(key);

	if (result==1) {
		RedisModule_ReplyWithSimpleString(ctx, "ERR Cuckoo filter not found");
		return REDISMODULE_ERR;
	}
	RedisModule_ReplyWithSimpleString(ctx, "OK");
	return REDISMODULE_OK;
}

int cfInfo_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	cuckoo_filter_t *cuckoo=RedisModule_ModuleTypeGetValue(key);

	if (RedisModule_ModuleTypeGetType(key) != CuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}

	print_cuckoo(cuckoo);
	return REDISMODULE_OK;
}

// static int BFLoadChunk_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
// 	RedisModule_AutoMemory(ctx);
// 	RedisModule_ReplicateVerbatim(ctx);

// 	if (argc!=2) {
// 		return RedisModule_WrongArity(ctx);
// 	}

// 	RedisModuleKey *key=RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
// 	bloomFilter *bloom;
// 	int status=bfGetBloom(key, &bloom);

// 	if (status!=0) {
// 		RedisModule_ModuleTypeSetValue(key, BloomType, bloom);
// 		return RedisModule_ReplyWithSimpleString(ctx, "OK");
// 	}
// }

// DataType functions
#define CF_ENCODING_VERSION 1

static void CFRdbSave(RedisModuleIO *io, void *obj) {
	cuckoo_filter_t *cuckoo=obj;

	RedisModule_SaveUnsigned(io, cuckoo->bucket_count);
	RedisModule_SaveUnsigned(io, cuckoo->nests_per_bucket);
	RedisModule_SaveUnsigned(io, cuckoo->mask);
	RedisModule_SaveDouble(io, cuckoo->seed);
	RedisModule_SaveDouble(io, cuckoo->padding);
	RedisModule_SaveDouble(io, cuckoo->key_count);

	int i;
	for (i=0; i<(cuckoo->bucket_count*cuckoo->nests_per_bucket); i++) {
		RedisModule_SaveUnsigned(io, cuckoo->bucket[i].fingerprint);
	}

	cuckoo_item_t *ci = &(cuckoo->victim);
	RedisModule_SaveUnsigned(io, ci->fingerprint);
	RedisModule_SaveUnsigned(io, ci->h1);
	RedisModule_SaveUnsigned(io, ci->h2);
	RedisModule_SaveUnsigned(io, ci->padding);

	cuckoo_item_t *clv = (cuckoo->last_victim);
	RedisModule_SaveUnsigned(io, clv->fingerprint);
	RedisModule_SaveUnsigned(io, clv->h1);
	RedisModule_SaveUnsigned(io, clv->h2);
	RedisModule_SaveUnsigned(io, clv->padding);
	// RedisModule_SaveStringBuffer(io, (const char *)bloom->bf, bloom->sizeOfBF);
}

static void *CFRdbLoad(RedisModuleIO *io, int encver) {
	if (encver > CF_ENCODING_VERSION) {
        return NULL;
    }

    cuckoo_filter_t *cuckoo=RedisModule_Calloc(1, sizeof(cuckoo_filter_t *));
    cuckoo->bucket_count=RedisModule_LoadUnsigned(io);
    cuckoo->nests_per_bucket=RedisModule_LoadUnsigned(io);
    cuckoo->mask=RedisModule_LoadUnsigned(io);
    cuckoo->max_kick_attempts=RedisModule_LoadDouble(io);
    cuckoo->seed=RedisModule_LoadDouble(io);
    cuckoo->padding=RedisModule_LoadDouble(io);
    cuckoo->key_count=RedisModule_LoadDouble(io);

    int i;
	for (i=0; i<(cuckoo->bucket_count*cuckoo->nests_per_bucket); i++) {
		cuckoo->bucket[i].fingerprint=RedisModule_LoadUnsigned(io);
	}

	cuckoo_item_t *ci = &(cuckoo->victim);
	RedisModule_LoadUnsigned(io);
	RedisModule_LoadUnsigned(io);
	RedisModule_LoadUnsigned(io);
	RedisModule_LoadUnsigned(io);

	cuckoo_item_t *clv = cuckoo->last_victim;
	RedisModule_LoadUnsigned(io);
	RedisModule_LoadUnsigned(io);
	RedisModule_LoadUnsigned(io);
	RedisModule_LoadUnsigned(io);

    return cuckoo;
}

// static void BFAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value) {
// 	bloomFilter *bloom=value;
// 	RedisModule_EmitAOF(aof, "BF.LOADCHUNK", "s", key);
// }

static void CFFree(void *value) {
	cuckoo_filter_t *cuckoo=value;
	//free(cuckoo->fingerprints);
	free(cuckoo->last_victim);
	free(cuckoo);
}


int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	if (RedisModule_Init(ctx,"remo",1,REDISMODULE_APIVER_1)== REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"CF.Create",cfCreate_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"CF.Rcho",cfEcho_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	// if (RedisModule_CreateCommand(ctx, "BF.LOADCHUNK", BFLoadChunk_RedisCommand, "write deny-oom",1,1,1)!=REDISMODULE_ERR)
	// 	return REDISMODULE_ERR;

	// if (RedisModule_CreateCommand(ctx,"CF.read",bfRead_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
	// 	return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"CF.Add",cfAdd_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"CF.Check",cfCheck_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"CF.Delete",cfCheck_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"CF.Info",cfInfo_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	RedisModuleTypeMethods tm = {
		.version=REDISMODULE_TYPE_METHOD_VERSION,
		.rdb_load=CFRdbLoad,
		.rdb_save=CFRdbSave,
		// .aof_rewrite=BFAofRewrite,
		.free=CFFree
	};

	CuckooType = RedisModule_CreateDataType(ctx,"oneCuckoo", 1, &tm);
	return CuckooType == NULL ? REDISMODULE_ERR : REDISMODULE_OK;
	// return REDISMODULE_OK;
}