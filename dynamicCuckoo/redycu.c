#include <stdlib.h>
#include "redismodule.h"
#include "dcf.h"

static RedisModuleType *dynamicCuckooType;



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

// dynamic cuckoo command functions
int dcfCreate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc != 5) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	long long item_num;	// capacity of a single bloom filter
	if (RedisModule_StringToLongLong(argv[2], &item_num) != REDISMODULE_OK ||
        item_num >= UINT32_MAX) {
        return RedisModule_ReplyWithError(ctx, "ERR bad capacity");
    }

    double error_rate;
	if (RedisModule_StringToDouble(argv[3], &error_rate)!=REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR bad error rate");
	}

	long long exp_block_num;	// capacity of a single bloom filter
	if (RedisModule_StringToLongLong(argv[4], &exp_block_num) != REDISMODULE_OK ||
        exp_block_num >= UINT32_MAX) {
        return RedisModule_ReplyWithError(ctx, "ERR bad capacity");
    }

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    dcf_t *dcf;

    if (dcf_create(&dcf, item_num, error_rate, exp_block_num)!=1) {
    	RedisModule_ReplyWithSimpleString(ctx, "ERR could not create filter");
    }

    RedisModule_ModuleTypeSetValue(key, dynamicCuckooType, dcf);
    RedisModule_ReplyWithSimpleString(ctx,"OK");
    return REDISMODULE_OK;
}

int dcfAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=2) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ|REDISMODULE_WRITE);
	dcf_t *dcf=RedisModule_ModuleTypeGetValue(key);

	if (RedisModule_ModuleTypeGetType(key) != dynamicCuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	if (dcf_add(dcf, element)!=1) {
		return REDISMODULE_ERR;	// modify this
	}
	//RedisModule_ModuleTypeSetValue(key, dynamicBloomType, dbf);
	RedisModule_CloseKey(key);
	RedisModule_ReplyWithSimpleString(ctx,"OK");
}

int dcfCheck_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=2) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ);
	if (RedisModule_ModuleTypeGetType(key) != dynamicCuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}

	dcf_t *dcf=RedisModule_ModuleTypeGetValue(key);
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	RedisModule_CloseKey(key);
	if (dcf_check(dcf, element)==1) {
		RedisModule_ReplyWithSimpleString(ctx, "Exists");
	}
	else {
		RedisModule_ReplyWithSimpleString(ctx, "Doesn't exist");
	}
	return REDISMODULE_OK;
}

int dcfDelete_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	RedisModule_AutoMemory(ctx);
	if (argc!=2) {
		RedisModule_WrongArity(ctx);
		return REDISMODULE_ERR;
	}

	RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],REDISMODULE_READ);
	if (RedisModule_ModuleTypeGetType(key) != dynamicCuckooType) {
		RedisModule_CloseKey(key);
		return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);	// change this... not the best description of error
	}

	dcf_t *dcf=RedisModule_ModuleTypeGetValue(key);
	size_t *len;
	const char *element=RedisModule_StringPtrLen(argv[2],len);
	RedisModule_CloseKey(key);
	if (dcf_delete(dcf, element)==1) {
		RedisModule_ReplyWithSimpleString(ctx, "Element deleted");
	}
	else {
		RedisModule_ReplyWithSimpleString(ctx, "Element doesn't exist");
	}
	return REDISMODULE_OK;
}



// DataType functions
#define CF_ENCODING_VERSION 1


static void DCFRdbSave(RedisModuleIO *io, void *obj) {
	dcf_t *dcf=obj;

	RedisModule_SaveUnsigned(io, dcf->capacity);
	RedisModule_SaveUnsigned(io, dcf->single_capacity);
	RedisModule_SaveUnsigned(io, dcf->single_table_length);
	RedisModule_SaveDouble(io, dcf->false_positive);
	RedisModule_SaveDouble(io, dcf->single_false_positive);
	RedisModule_SaveDouble(io, dcf->fingerprint_size_double);
	RedisModule_SaveUnsigned(io, dcf->fingerprint_size);

	RedisModule_SaveUnsigned(io, dcf->victim.index);
	RedisModule_SaveUnsigned(io, dcf->victim.fingerprint);

}

static void *DCFRdbLoad(RedisModuleIO *io, int encver) {
	if (encver > CF_ENCODING_VERSION) {
        return NULL;
    }

    dcf_t *dcf=RedisModule_Calloc(1, sizeof(*dcf));
    dcf->capacity=RedisModule_LoadUnsigned(io);
    dcf->single_capacity=RedisModule_LoadUnsigned(io);
    dcf->single_table_length=RedisModule_LoadUnsigned(io);
    dcf->false_positive=RedisModule_LoadDouble(io);
    dcf->single_false_positive=RedisModule_LoadDouble(io);
    dcf->fingerprint_size_double=RedisModule_LoadDouble(io);
    dcf->fingerprint_size=RedisModule_LoadUnsigned(io);

    dcf->victim.index=RedisModule_LoadUnsigned(io);
    dcf->victim.fingerprint=RedisModule_LoadUnsigned(io);

    return dcf;
}

static void DCFFree(void *value) {
	dcf_t *dcf=value;
	dcf_free(dcf);
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
	if (RedisModule_Init(ctx,"remo",1,REDISMODULE_APIVER_1)== REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DCF.create",dcfCreate_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DCF.add",dcfAdd_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DCF.check",dcfCheck_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	if (RedisModule_CreateCommand(ctx,"DCF.delete",dcfDelete_RedisCommand,"write deny-oom",1,1,1)==REDISMODULE_ERR)
		return REDISMODULE_ERR;

	// RedisModuleTypeMethods tm = {
	// 	.version=REDISMODULE_TYPE_METHOD_VERSION,
	// 	.rdb_load=BFRdbLoad,
	// 	.rdb_save=BFRdbSave,
	// 	// .aof_rewrite=BFAofRewrite,
	// 	.free=BFFree
	// };

	RedisModuleTypeMethods tmDyn = {
		.version=REDISMODULE_TYPE_METHOD_VERSION,
		.rdb_load=DCFRdbLoad,
		.rdb_save=DCFRdbSave,
		// .aof_rewrite=DBFAofRewrite,
		.free=DCFFree
	};

	// BloomType = RedisModule_CreateDataType(ctx,"std_bloom", 1, &tm);
	// return BloomType == NULL ? REDISMODULE_ERR : REDISMODULE_OK;

	dynamicCuckooType = RedisModule_CreateDataType(ctx, "dynCuckoo", 1, &tmDyn);
	return dynamicCuckooType == NULL ? REDISMODULE_ERR : REDISMODULE_OK;
	// return REDISMODULE_OK;
}