/*-------------------------------------------------------------------------
 *
 * toast_compression.c
 *	  Functions for toast compression.
 *
 * Copyright (c) 2021-2024, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/access/common/toast_compression.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#ifdef USE_LZ4
#include <lz4.h>
#endif

#include "access/detoast.h"
#include "access/toast_compression.h"
#include "access/toast_internals.h"
#include "common/pg_lzcompress.h"
#include "common/rle_compress.h"
#include "common/lzw_compress.h"
#include "common/tadoc_compress.h"
#include "varatt.h"
#include <limits.h>

/* GUC */
int			default_toast_compression = TOAST_PGLZ_COMPRESSION;

#define NO_LZ4_SUPPORT() \
	ereport(ERROR, \
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED), \
			 errmsg("compression method lz4 not supported"), \
			 errdetail("This functionality requires the server to be built with lz4 support.")))

const PGLZ_Strategy double_compress_default_data = {
	0,							/* Data chunks less than 32 bytes are not
								 * compressed */
	INT_MAX,					/* No upper limit on what we'll try to
								 * compress */
	0,							/* Require 25% compression rate, or not worth
								 * it */
	1024,						/* Give up if no compression in the first 1KB */
	128,						/* Stop history lookup if a match of 128 bytes
								 * is found */
	10							/* Lower good match size by 10% at every loop
								 * iteration */
	};

/*
 * Compress a varlena using PGLZ.
 *
 * Returns the compressed varlena, or NULL if compression fails.
 */
struct varlena *
pglz_compress_datum(const struct varlena *value)
{
	int32		valsize,
				len;
	struct varlena *tmp = NULL;

	valsize = VARSIZE_ANY_EXHDR(value);

	/*
	 * No point in wasting a palloc cycle if value size is outside the allowed
	 * range for compression.
	 */
	if (valsize < PGLZ_strategy_default->min_input_size ||
		valsize > PGLZ_strategy_default->max_input_size)
		return NULL;

	/*
	 * Figure out the maximum possible size of the pglz output, add the bytes
	 * that will be needed for varlena overhead, and allocate that amount.
	 */
	tmp = (struct varlena *) palloc(PGLZ_MAX_OUTPUT(valsize) +
									VARHDRSZ_COMPRESSED);

	len = pglz_compress(VARDATA_ANY(value),
						valsize,
						(char *) tmp + VARHDRSZ_COMPRESSED,
						NULL);
	if (len < 0)
	{
		pfree(tmp);
		return NULL;
	}

	SET_VARSIZE_COMPRESSED(tmp, len + VARHDRSZ_COMPRESSED);

	return tmp;
}

/*
 * Decompress a varlena that was compressed using PGLZ.
 */
struct varlena *
pglz_decompress_datum(const struct varlena *value)
{
	struct varlena *result;
	int32		rawsize;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ);

	/* decompress the data */
	rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
							  VARSIZE(value) - VARHDRSZ_COMPRESSED,
							  VARDATA(result),
							  VARDATA_COMPRESSED_GET_EXTSIZE(value), true);
	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed pglz data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
}

/*
 * Decompress part of a varlena that was compressed using PGLZ.
 */
struct varlena *
pglz_decompress_datum_slice(const struct varlena *value,
							int32 slicelength)
{
	struct varlena *result;
	int32		rawsize;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(slicelength + VARHDRSZ);

	/* decompress the data */
	rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
							  VARSIZE(value) - VARHDRSZ_COMPRESSED,
							  VARDATA(result),
							  slicelength, false);
	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed pglz data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
}



/**
 * yuxin tang
 * Compress a varlena using RLE.
 *
 * Returns the compressed varlena, or NULL if compression fails.
 */
struct varlena *
rle_compress_datum(const struct varlena *value)
{
	int32		valsize,
				len;
	struct varlena *tmp = NULL;
	char * inter_res;
	valsize = VARSIZE_ANY_EXHDR(value);

	/*
	 * No point in wasting a palloc cycle if value size is outside the allowed
	 * range for compression.
	 */
	if (valsize < RLE_strategy_default->min_input_size ||
		valsize > RLE_strategy_default->max_input_size)
		return NULL;

	/*
	 * Figure out the maximum possible size of the rle output, add the bytes
	 * that will be needed for varlena overhead, and allocate that amount.
	 */
	tmp = (struct varlena *) palloc(RLE_MAX_OUTPUT(valsize) +
									VARHDRSZ_COMPRESSED);

	inter_res = (char *) palloc(RLE_MAX_OUTPUT(valsize));

	ereport(LOG,(errmsg("before compression . raw size = %d.",valsize)));
	len = rle_compress(VARDATA_ANY(value),
						valsize,
						(char *) inter_res,
						NULL);
	ereport(LOG,(errmsg("rle_compress finished. compressed size = %d.",len)));
	if (len < 0)
	{
		pfree(tmp);
		return NULL;
	}
	len = pglz_compress(inter_res,
						len,
						(char *) tmp + VARHDRSZ_COMPRESSED,
						&double_compress_default_data);
	ereport(LOG,(errmsg("pglz_compress finished. compressed size = %d.",len)));

	// len = rle_compress(VARDATA_ANY(value),
	// 					valsize,
	// 					(char *) tmp + VARHDRSZ_COMPRESSED,
	// 					NULL);
	// ereport(LOG,(errmsg("rle_compress finished. compressed size = %d.",len)));
	
	
	if (len < 0)
	{
		pfree(tmp);
		return NULL;
	}

	// free(inter_res);

	SET_VARSIZE_COMPRESSED(tmp, len + VARHDRSZ_COMPRESSED);

	return tmp;
}

/**
 * yuxin tang
 * Decompress a varlena that was compressed using RLE.
 */
struct varlena *
rle_decompress_datum(const struct varlena *value,bool partialDecomp)
{
	struct varlena *result;
	char * inter_res;
	int32 rawsize;
	int32 rawsize_1;
	/* allocate memory for the uncompressed data */
	if(!partialDecomp){
		result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ);
		inter_res = (char *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value));

		/* decompress the data */
		rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
								VARSIZE(value) - VARHDRSZ_COMPRESSED,
								inter_res,
								VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
		rawsize_1 = rawsize;

		/* decompress the data */
		rawsize = rle_decompress(inter_res,
								rawsize,
								VARDATA(result),
								VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
	}else{
		result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ_COMPRESSED);
		
		rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
								VARSIZE(value) - VARHDRSZ_COMPRESSED,
								// VARDATA(result),
								(char *) result + VARHDRSZ_COMPRESSED,
								VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
	}

	// rawsize = rle_decompress((char *) value + VARHDRSZ_COMPRESSED,
	// 						VARSIZE(value) - VARHDRSZ_COMPRESSED,
	// 						VARDATA(result),
	// 						VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
	// ereport(LOG,(errmsg("rle_decompress finished. rawsize size = %d.",rawsize)));
	
	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed rle data is corrupt, pglz decompressed rawsize = %d, finally raw size = %d",rawsize_1,rawsize)));
	
	if(!partialDecomp){
		SET_VARSIZE(result, rawsize + VARHDRSZ);
	}else{
		SET_VARSIZE_COMPRESSED(result, rawsize + VARHDRSZ_COMPRESSED);
		ToastCompressionId cmid = TOAST_RLE_COMPRESSION_ID;
		TOAST_COMPRESS_SET_SIZE_AND_COMPRESS_METHOD(result, rawsize, cmid);
	}

	return result;
}

/**
 * yuxin tang
 * Decompress part of a varlena that was compressed using RLE.
 */
struct varlena *
rle_decompress_datum_slice(const struct varlena *value,
							int32 slicelength)
{
	struct varlena *result;
	char * inter_res;
	int32		rawsize;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(slicelength + VARHDRSZ);
	inter_res = (char *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value));

	/* decompress the data */
	rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
							  VARSIZE(value) - VARHDRSZ_COMPRESSED,
							  inter_res,
							  VARDATA_COMPRESSED_GET_EXTSIZE(value), false);


	/* decompress the data */
	rawsize = rle_decompress(inter_res,
							  rawsize,
							  VARDATA(result),
							  slicelength, false);


	// /* decompress the data */
	// rawsize = rle_decompress((char *) value + VARHDRSZ_COMPRESSED,
	// 						  VARSIZE(value) - VARHDRSZ_COMPRESSED,
	// 						  VARDATA(result),
	// 						  slicelength, false);

	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed rle data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
}






/**
 * yuxin tang
 * Compress a varlena using LZW.
 *
 * Returns the compressed varlena, or NULL if compression fails.
 */
struct varlena *
lzw_compress_datum(const struct varlena *value)
{
	int32		valsize,
				len;
	struct varlena *tmp = NULL;

	valsize = VARSIZE_ANY_EXHDR(value);

	/*
	 * No point in wasting a palloc cycle if value size is outside the allowed
	 * range for compression.
	 */
	if (valsize < LZW_strategy_default->min_input_size ||
		valsize > LZW_strategy_default->max_input_size)
		return NULL;

	/*
	 * Figure out the maximum possible size of the rle output, add the bytes
	 * that will be needed for varlena overhead, and allocate that amount.
	 */
	tmp = (struct varlena *) palloc(LZW_MAX_OUTPUT(valsize) +
									VARHDRSZ_COMPRESSED);

	// inter_res = (char *) palloc(LZW_MAX_OUTPUT(valsize));

	// ereport(LOG,(errmsg("before compression . raw size = %d.",valsize)));
	// len = lzw_compress(VARDATA_ANY(value),
	// 					valsize,
	// 					(char *) inter_res,
	// 					NULL);
	// ereport(LOG,(errmsg("------ lzw_compress finished. compressed size = %d.",len)));
	// if (len < 0)
	// {
	// 	pfree(tmp);
	// 	return NULL;
	// }

	// len = pglz_compress(inter_res,
	// 					len,
	// 					(char *) tmp + VARHDRSZ_COMPRESSED,
	// 					&double_compress_default_data);
	// ereport(LOG,(errmsg("----- pglz_compress finished. compressed size = %d.",len)));

	len = lzw_compress(VARDATA_ANY(value),
						valsize,
						(char *) tmp + VARHDRSZ_COMPRESSED,
						NULL);
	// ereport(LOG,(errmsg("lzw_compress finished. compressed size = %d.",len)));
	
	if (len < 0)
	{
		pfree(tmp);
		return NULL;
	}

	// free(inter_res);

	SET_VARSIZE_COMPRESSED(tmp, len + VARHDRSZ_COMPRESSED);

	return tmp;
}

/**
 * yuxin tang
 * Decompress a varlena that was compressed using LZW.
 */
struct varlena *
lzw_decompress_datum(const struct varlena *value,bool partialDecomp)
{
	struct varlena *result;
	char * inter_res;
	int32 rawsize;
	int32 rawsize_1;
	/* allocate memory for the uncompressed data */
	if(!partialDecomp){
		result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ);
		// inter_res = (char *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value));

		// /* decompress the data */
		// rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
		// 						VARSIZE(value) - VARHDRSZ_COMPRESSED,
		// 						inter_res,
		// 						VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
		// rawsize_1 = rawsize;

		// /* decompress the data */
		// rawsize = lzw_decompress(inter_res,
		// 						rawsize,
		// 						VARDATA(result),
		// 						VARDATA_COMPRESSED_GET_EXTSIZE(value), false);

		rawsize = lzw_decompress((char *) value + VARHDRSZ_COMPRESSED,
								VARSIZE(value) - VARHDRSZ_COMPRESSED,
								VARDATA(result),
								VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
		// ereport(LOG,(errmsg("lzw_decompress finished. rawsize size = %d.",rawsize)));
	}else{
		// do nothing
		result = (struct varlena *) palloc(VARSIZE(value));
		memcpy(result,value,VARSIZE(value));

		
		// result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ_COMPRESSED);
		
		// rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
		// 						VARSIZE(value) - VARHDRSZ_COMPRESSED,
		// 						// VARDATA(result),
		// 						(char *) result + VARHDRSZ_COMPRESSED,
		// 						VARDATA_COMPRESSED_GET_EXTSIZE(value), false);
	}


	
	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed lzw data is corrupt, pglz decompressed rawsize = %d, finally raw size = %d",rawsize_1,rawsize)));
	
	if(!partialDecomp){
		SET_VARSIZE(result, rawsize + VARHDRSZ);
	}

	return result;
}

/**
 * yuxin tang
 * Decompress part of a varlena that was compressed using LZW.
 */
struct varlena *
lzw_decompress_datum_slice(const struct varlena *value,
							int32 slicelength)
{
	struct varlena *result;
	// char * inter_res;
	int32		rawsize;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(slicelength + VARHDRSZ);
	// inter_res = (char *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value));

	// /* decompress the data */
	// rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
	// 						  VARSIZE(value) - VARHDRSZ_COMPRESSED,
	// 						  inter_res,
	// 						  VARDATA_COMPRESSED_GET_EXTSIZE(value), false);


	// /* decompress the data */
	// rawsize = lzw_decompress(inter_res,
	// 						  rawsize,
	// 						  VARDATA(result),
	// 						  slicelength, false);


	/* decompress the data */
	rawsize = lzw_decompress((char *) value + VARHDRSZ_COMPRESSED,
							  VARSIZE(value) - VARHDRSZ_COMPRESSED,
							  VARDATA(result),
							  slicelength, false);

	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed rle data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
}





/**
 * yuxin tang
 * Compress a varlena using TADOC.
 *
 * Returns the compressed varlena, or NULL if compression fails.
 */
struct varlena *
tadoc_compress_datum(const struct varlena *value)
{
	int32		valsize,
				len;
	struct varlena *tmp = NULL;
	char * inter_res;
	valsize = VARSIZE_ANY_EXHDR(value);

	/*
	 * No point in wasting a palloc cycle if value size is outside the allowed
	 * range for compression.
	 */
	if (valsize < RLE_strategy_default->min_input_size ||
		valsize > RLE_strategy_default->max_input_size)
		return NULL;

	/*
	 * Figure out the maximum possible size of the rle output, add the bytes
	 * that will be needed for varlena overhead, and allocate that amount.
	 */
	tmp = (struct varlena *) palloc(RLE_MAX_OUTPUT(valsize) +
									VARHDRSZ_COMPRESSED);

	inter_res = (char *) palloc(RLE_MAX_OUTPUT(valsize));

	len = tadoc_compress(VARDATA_ANY(value),
						valsize,
						(char *) inter_res,
						NULL);

	// len = pglz_compress(inter_res,
	// 					len,
	// 					(char *) tmp + VARHDRSZ_COMPRESSED,
	// 					NULL);

	if (len < 0) {
		pfree(tmp);
		return NULL;
	}

	// free(inter_res);

	SET_VARSIZE_COMPRESSED(tmp, len + VARHDRSZ_COMPRESSED);

	return tmp;
}

/**
 * yuxin tang
 * Decompress a varlena that was compressed using TADOC.
 */
struct varlena *
tadoc_decompress_datum(const struct varlena *value,bool partialDecomp)
{
	struct varlena *result;
	char * inter_res;
	int32		rawsize;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ);
	if(!partialDecomp){
		inter_res = (char *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value));

		/* decompress the data */
		rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
								VARSIZE(value) - VARHDRSZ_COMPRESSED,
								inter_res,
								VARDATA_COMPRESSED_GET_EXTSIZE(value), true);


		/* decompress the data */
		rawsize = rle_decompress(inter_res,
								rawsize,
								VARDATA(result),
								VARDATA_COMPRESSED_GET_EXTSIZE(value), true);
	}else{
		/* decompress the data */
		rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
								VARSIZE(value) - VARHDRSZ_COMPRESSED,
								VARDATA(result),
								VARDATA_COMPRESSED_GET_EXTSIZE(value), true);

		// rawsize = rle_decompress((char *) value + VARHDRSZ_COMPRESSED,
		// 						  VARSIZE(value) - VARHDRSZ_COMPRESSED,
		// 						  VARDATA(result),
		// 						  VARDATA_COMPRESSED_GET_EXTSIZE(value), true);
	}

	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed rle data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
}

/**
 * yuxin tang
 * Decompress part of a varlena that was compressed using TADOC.
 */
struct varlena *
tadoc_decompress_datum_slice(const struct varlena *value,
							int32 slicelength)
{
	struct varlena *result;
	char * inter_res;
	int32		rawsize;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(slicelength + VARHDRSZ);
	inter_res = (char *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value));

	/* decompress the data */
	rawsize = pglz_decompress((char *) value + VARHDRSZ_COMPRESSED,
							  VARSIZE(value) - VARHDRSZ_COMPRESSED,
							  inter_res,
							  VARDATA_COMPRESSED_GET_EXTSIZE(value), true);


	/* decompress the data */
	rawsize = tadoc_decompress(inter_res,
							  rawsize,
							  VARDATA(result),
							  slicelength, false);


	// /* decompress the data */
	// rawsize = rle_decompress((char *) value + VARHDRSZ_COMPRESSED,
	// 						  VARSIZE(value) - VARHDRSZ_COMPRESSED,
	// 						  VARDATA(result),
	// 						  slicelength, false);

	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed rle data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
}

/*
 * Compress a varlena using LZ4.
 *
 * Returns the compressed varlena, or NULL if compression fails.
 */
struct varlena *
lz4_compress_datum(const struct varlena *value)
{
#ifndef USE_LZ4
	NO_LZ4_SUPPORT();
	return NULL;				/* keep compiler quiet */
#else
	int32		valsize;
	int32		len;
	int32		max_size;
	struct varlena *tmp = NULL;

	valsize = VARSIZE_ANY_EXHDR(value);

	/*
	 * Figure out the maximum possible size of the LZ4 output, add the bytes
	 * that will be needed for varlena overhead, and allocate that amount.
	 */
	max_size = LZ4_compressBound(valsize);
	tmp = (struct varlena *) palloc(max_size + VARHDRSZ_COMPRESSED);

	len = LZ4_compress_default(VARDATA_ANY(value),
							   (char *) tmp + VARHDRSZ_COMPRESSED,
							   valsize, max_size);
	if (len <= 0)
		elog(ERROR, "lz4 compression failed");

	/* data is incompressible so just free the memory and return NULL */
	if (len > valsize)
	{
		pfree(tmp);
		return NULL;
	}

	SET_VARSIZE_COMPRESSED(tmp, len + VARHDRSZ_COMPRESSED);

	return tmp;
#endif
}

/*
 * Decompress a varlena that was compressed using LZ4.
 */
struct varlena *
lz4_decompress_datum(const struct varlena *value)
{
#ifndef USE_LZ4
	NO_LZ4_SUPPORT();
	return NULL;				/* keep compiler quiet */
#else
	int32		rawsize;
	struct varlena *result;

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(VARDATA_COMPRESSED_GET_EXTSIZE(value) + VARHDRSZ);

	/* decompress the data */
	rawsize = LZ4_decompress_safe((char *) value + VARHDRSZ_COMPRESSED,
								  VARDATA(result),
								  VARSIZE(value) - VARHDRSZ_COMPRESSED,
								  VARDATA_COMPRESSED_GET_EXTSIZE(value));
	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed lz4 data is corrupt")));


	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
#endif
}

/*
 * Decompress part of a varlena that was compressed using LZ4.
 */
struct varlena *
lz4_decompress_datum_slice(const struct varlena *value, int32 slicelength)
{
#ifndef USE_LZ4
	NO_LZ4_SUPPORT();
	return NULL;				/* keep compiler quiet */
#else
	int32		rawsize;
	struct varlena *result;

	/* slice decompression not supported prior to 1.8.3 */
	if (LZ4_versionNumber() < 10803)
		return lz4_decompress_datum(value);

	/* allocate memory for the uncompressed data */
	result = (struct varlena *) palloc(slicelength + VARHDRSZ);

	/* decompress the data */
	rawsize = LZ4_decompress_safe_partial((char *) value + VARHDRSZ_COMPRESSED,
										  VARDATA(result),
										  VARSIZE(value) - VARHDRSZ_COMPRESSED,
										  slicelength,
										  slicelength);
	if (rawsize < 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg_internal("compressed lz4 data is corrupt")));

	SET_VARSIZE(result, rawsize + VARHDRSZ);

	return result;
#endif
}

/*
 * Extract compression ID from a varlena.
 *
 * Returns TOAST_INVALID_COMPRESSION_ID if the varlena is not compressed.
 */
ToastCompressionId
toast_get_compression_id(struct varlena *attr)
{
	ToastCompressionId cmid = TOAST_INVALID_COMPRESSION_ID;

	/*
	 * If it is stored externally then fetch the compression method id from
	 * the external toast pointer.  If compressed inline, fetch it from the
	 * toast compression header.
	 */
	if (VARATT_IS_EXTERNAL_ONDISK(attr))
	{
		struct varatt_external toast_pointer;

		VARATT_EXTERNAL_GET_POINTER(toast_pointer, attr);

		if (VARATT_EXTERNAL_IS_COMPRESSED(toast_pointer))
			cmid = VARATT_EXTERNAL_GET_COMPRESS_METHOD(toast_pointer);
	}
	else if (VARATT_IS_COMPRESSED(attr))
		cmid = VARDATA_COMPRESSED_GET_COMPRESS_METHOD(attr);

	return cmid;
}

/*
 * CompressionNameToMethod - Get compression method from compression name
 *
 * Search in the available built-in methods.  If the compression not found
 * in the built-in methods then return InvalidCompressionMethod.
 */
char
CompressionNameToMethod(const char *compression)
{
	/** hocotext */
	/** yuxin tang */
	if (strcmp(compression, "pglz") == 0)
		return TOAST_PGLZ_COMPRESSION;
	else if(strcmp(compression, "rle") == 0){
		return TOAST_RLE_COMPRESSION;
	}else if(strcmp(compression, "lzw") == 0){
		return TOAST_LZW_COMPRESSION;
	}else if(strcmp(compression, "tadoc") == 0){
		return TOAST_TADOC_COMPRESSION;
	}else if (strcmp(compression, "lz4") == 0)
	{
#ifndef USE_LZ4
		NO_LZ4_SUPPORT();
#endif
		return TOAST_LZ4_COMPRESSION;
	}

	return InvalidCompressionMethod;
}

/*
 * GetCompressionMethodName - Get compression method name
 */
const char *
GetCompressionMethodName(char method)
{
	switch (method)
	{
		/** hocotext */
		/** yuxin tang*/
		case TOAST_RLE_COMPRESSION:
			return "rle";
		case TOAST_TADOC_COMPRESSION:
			return "tadoc";
		case TOAST_PGLZ_COMPRESSION:
			return "pglz";
		case TOAST_LZ4_COMPRESSION:
			return "lz4";
		case TOAST_LZW_COMPRESSION:
			return "lzw";
		default:
			elog(ERROR, "invalid compression method %c", method);
			return NULL;		/* keep compiler quiet */
	}
}
