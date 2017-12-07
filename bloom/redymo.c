#include <stdlib.h>
#include "redismodule.h"
#include "db.h"

static RedisModuleType *BloomType;
static RedisModuleType *dynamicBloomType;

// bloom filter command functions
int bfCreate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc != 4) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	long long capacity;
    if (RedisModule_StringToLongLong(argv[2], &capacity) != REDISMODULE_OK ||
        capacity >= UINT32_MAX) {
        return RedisModule_ReplyWithError(ctx, "ERR bad capacity");
    }

	double error_rate;
	if (RedisModule_StringToDouble(argv[3], &error_rate)!=REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR bad error rate");
	}

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
	bloomFilter *bloom;
	if (bloom_create(&bloom, capacity, error_rate)==0) {
        RedisModule_ReplyWithSimpleString(ctx, "ERR could not create filter");
        return REDISMODULE_ERR;
	}

	RedisModule_ModuleTypeSetValue(key, BloomType, bloom);
	// const char *ptr="OK";
	RedisModule_ReplyWithSimpleString(ctx,"OK");
	// RedisModule_ReplyWithLongLong(ctx,666);
	return REDISMODULE_OK;
}

int bfRead_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	if (argc!=2) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);

	/*
	int keytype = RedisModule_KeyType(key);
	if (keytype != BloomType && keytype != REDISMODULE_KEYTYPE_EMPTY) {
	    RedisModule_CloseKey(key);
	    return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);
	}
	*/

	bloomFilter *bloom=RedisModule_ModuleTypeGetValue(key);
	print_bloom(bloom);
	RedisModule_ReplyWithSimpleString(ctx,"OK");

}

int bfAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	bloomFilter *bloom=RedisModule_ModuleTypeGetValue(key);

	if (RedisModule_ModuleTypeGetType(key) != BloomType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	if (bloom_add(bloom, element, *len)!=1) {
		return REDISMODULE_ERR;	// modify this
	}
	//RedisModule_ModuleTypeSetValue(key, BloomType, bloom);
	RedisModule_CloseKey(key);
	RedisModule_ReplyWithSimpleString(ctx,"OK");

	return REDISMODULE_OK;
}

int bfCheck_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ);
	if (RedisModule_ModuleTypeGetType(key) != BloomType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}

	bloomFilter *bloom=RedisModule_ModuleTypeGetValue(key);
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	RedisModule_CloseKey(key);
	if (bloom_check(bloom, element, *len)==1) {
		RedisModule_ReplyWithSimpleString(ctx, "Exists");
	}
	else {
		RedisModule_ReplyWithSimpleString(ctx, "Doesn't exist");
	}
	return REDISMODULE_OK;
}

int bfEcho_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
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

static int bfGetBloom(RedisModuleKey *key, bloomFilter **bloom) {
	*bloom=NULL;
	if (key==NULL) {
		return 0;
	}
	*bloom=RedisModule_ModuleTypeGetValue(key);
	return 1;
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

// dynamic bloom command functions
int dbfCreate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc != 4) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	long long capacity;	// capacity of a single bloom filter
	if (RedisModule_StringToLongLong(argv[2], &capacity) != REDISMODULE_OK ||
        capacity >= UINT32_MAX) {
        return RedisModule_ReplyWithError(ctx, "ERR bad capacity");
    }

    double error_rate;
	if (RedisModule_StringToDouble(argv[3], &error_rate)!=REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR bad error rate");
	}

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    dynamicBloom *dbf;

    if (dbf_create(&dbf, capacity, error_rate)==0) {
    	RedisModule_ReplyWithSimpleString(ctx, "ERR could not create filter");
    }

    RedisModule_ModuleTypeSetValue(key, dynamicBloomType, dbf);
    RedisModule_ReplyWithSimpleString(ctx,"OK");
    return REDISMODULE_OK;
}

int dbfAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	dynamicBloom *dbf=RedisModule_ModuleTypeGetValue(key);

	if (RedisModule_ModuleTypeGetType(key) != dynamicBloomType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	if (dbf_add(dbf, element, *len)!=1) {
		return REDISMODULE_ERR;	// modify this
	}
	//RedisModule_ModuleTypeSetValue(key, dynamicBloomType, dbf);
	RedisModule_CloseKey(key);
	RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int dbfCheck_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=3) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ);
	if (RedisModule_ModuleTypeGetType(key) != dynamicBloomType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}

	dynamicBloom *dbf=RedisModule_ModuleTypeGetValue(key);
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	RedisModule_CloseKey(key);
	if (dbf_check(dbf, element, *len)==1) {
		RedisModule_ReplyWithSimpleString(ctx, "Exists");
	}
	else {
		RedisModule_ReplyWithSimpleString(ctx, "Doesn't exist");
	}
	return REDISMODULE_OK;
}



// DataType functions
#define BF_ENCODING_VERSION 1

static void BFRdbSave(RedisModuleIO *io, void *obj) {
	bloomFilter *bloom=obj;

	RedisModule_SaveUnsigned(io, bloom->capacity);
	RedisModule_SaveUnsigned(io, bloom->HashCount);
	RedisModule_SaveUnsigned(io, bloom->keyCount);
	RedisModule_SaveUnsigned(io, bloom->sizeOfBF);
	// RedisModule_SaveDouble(io, bloom->MaxError);
	RedisModule_SaveStringBuffer(io, (const char *)bloom->bf, bloom->sizeOfBF);
}

static void *BFRdbLoad(RedisModuleIO *io, int encver) {
	if (encver > BF_ENCODING_VERSION) {
        return NULL;
    }

    bloomFilter *bloom=RedisModule_Calloc(1, sizeof(*bloom));
    bloom->HashCount=RedisModule_LoadUnsigned(io);
    bloom->capacity=RedisModule_LoadUnsigned(io);
    bloom->keyCount=RedisModule_LoadUnsigned(io);
    bloom->sizeOfBF=RedisModule_LoadUnsigned(io);
    // bloom->MaxError=RedisModule_LoadDouble(io);

    size_t size_temp;
    bloom->bf=(unsigned char *)RedisModule_LoadStringBuffer(io, &size_temp);

    return bloom;
}

// static void BFAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value) {
// 	bloomFilter *bloom=value;
// 	RedisModule_EmitAOF(aof, "BF.LOADCHUNK", "s", key);
// }

static void BFFree(void *value) {
	bloomFilter *bloom=value;
	free(bloom->bf);
	free(bloom);
}

static void DBFRdbSave(RedisModuleIO *io, void *obj) {
	dynamicBloom *dbf=obj;

	RedisModule_SaveUnsigned(io, dbf->matrix_size);
	RedisModule_SaveUnsigned(io, dbf->cur_keys);
	RedisModule_SaveUnsigned(io, dbf->interval);
	RedisModule_SaveDouble(io, dbf->MaxError);

	int i;
	for (i=0; i<dbf->matrix_size; i++) {
		bloomFilter *bloom=dbf->matrix[i];
		BFRdbSave(io, bloom);
	}
}

static void *DBFRdbLoad(RedisModuleIO *io, int encver) {
	if (encver > BF_ENCODING_VERSION) {
        return NULL;
    }

    dynamicBloom *dbf=RedisModule_Calloc(1, sizeof(*dbf));
    dbf->matrix_size=RedisModule_LoadUnsigned(io);
    dbf->cur_keys=RedisModule_LoadUnsigned(io);
    dbf->interval=RedisModule_LoadUnsigned(io);
    dbf->MaxError=RedisModule_LoadDouble(io);

    int i;
    for (i=0; i<dbf->matrix_size; i++) {
    	BFRdbLoad(io, encver);
    }

    return dbf;
}

static void DBFFree(void *value) {
	dynamicBloom *dbf=value;
	int i;
	for(i=0; i<dbf->matrix_size; i++) {
		BFFree(dbf->matrix[i]);
	}
	free(dbf);
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	if (RedisModule_Init(ctx,"remo",1,REDISMODULE_APIVER_1)== REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"BF.create",bfCreate_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"BF.echo",bfEcho_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	// if (RedisModule_CreateCommand(ctx, "BF.LOADCHUNK", BFLoadChunk_RedisCommand, "write deny-oom",1,1,1)!=REDISMODULE_ERR)
	// 	return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"BF.read",bfRead_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"BF.add",bfAdd_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"BF.check",bfCheck_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DBF.create",dbfCreate_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DBF.add",dbfAdd_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DBF.check",dbfCheck_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	RedisModuleTypeMethods tm = {
		.version=REDISMODULE_TYPE_METHOD_VERSION,
		.rdb_load=BFRdbLoad,
		.rdb_save=BFRdbSave,
		// .aof_rewrite=BFAofRewrite,
		.free=BFFree
	};

	RedisModuleTypeMethods tmDyn = {
		.version=REDISMODULE_TYPE_METHOD_VERSION,
		.rdb_load=DBFRdbLoad,
		.rdb_save=DBFRdbSave,
		// .aof_rewrite=DBFAofRewrite,
		.free=DBFFree
	};

	BloomType = RedisModule_CreateDataType(ctx,"std_bloom", 1, &tm);
	// return BloomType == NULL ? REDISMODULE_ERR : REDISMODULE_OK;

	dynamicBloomType = RedisModule_CreateDataType(ctx, "dyn_bloom", 1, &tmDyn);
	return dynamicBloomType == NULL ? REDISMODULE_ERR : REDISMODULE_OK;
	// return REDISMODULE_OK;
}