/*
 * contrib/hocotext/hocotext.c
 */
#include "hocotext.h"
// #include "commands/explain.h"
PG_MODULE_MAGIC;

/**
 * basic string operators
 * 1. extract
 * 2. insert
 * 3. delete
 * 4. symbol_compare
*/

/**
 * help hander functions
 * 
 * > plain 00
 * > rle 01
 * > tadoc 10
 * > ??? 11
*/



int32 hocotext_hoco_cmp_helper(struct varlena * left, struct varlena * right, Oid collid){
    char leftTag = (((unsigned char)(*VARDATA_ANY(left)))>>6) & 0x03;
    char rightTag = (((unsigned char)(*VARDATA_ANY(right)))>>6) & 0x03;
    Assert(leftTag == rightTag);
    switch (leftTag){
        case 0x00:
            // return hocotext_common_cmp(left,right,collid);
        case 0x01:
            return hocotext_rle_hoco_cmp(left,right,collid);
        case 0x02:
            // return hocotext_tadoc_hoco_cmp(left,right,collid);
        case 0x03:
            // return hocotext_???_hoco_cmp(left,right,collid);
        default:
            ereport(ERROR,
                    errmsg("Unrecognizable compresssion code: %d",leftTag));
    }
 
}

text * hocotext_hoco_extract_helper(struct varlena * source,int32 offset,int32 len,Oid collid){
    char tag = (((unsigned char)(*VARDATA_ANY(source))) >> 6) & 0x03;
    switch (tag){
        case 0x00:
            // return hocotext_common_extract(source,offset,len,collid);
        case 0x01:
            return hocotext_rle_hoco_extract(source,offset,len,collid);
        case 0x02:
            // return hocotext_tadoc_hoco_extract(source,offset,len,collid);
        case 0x03:
            // return hocotext_???_hoco_extract(source,offset,len,collid);
        default:
            ereport(ERROR,
                    errmsg("Unrecognizable compresssion code: %d",tag));
    }
}


text * hocotext_hoco_insert_helper(struct varlena * source,int32 offset,text * str,Oid collid){
    char tag = (((unsigned char)(*VARDATA_ANY(source))) >> 6) & 0x03;
    switch (tag){
        case 0x00:
            // return hocotext_common_insert(source,offset,len,collid);
        case 0x01:
            return hocotext_rle_hoco_insert(source,offset,str,collid);
        case 0x02:
            // return hocotext_tadoc_hoco_insert(source,offset,str,collid);
        case 0x03:
            // return hocotext_???_hoco_insert(source,offset,str,collid);
        default:
            ereport(ERROR,
                    errmsg("Unrecognizable compresssion code: %d",tag));
    }
}

text * hocotext_hoco_overlay_helper(struct varlena * source,int32 offset,int32 len,text * str,Oid collid){
    char tag = (((unsigned char)(*VARDATA_ANY(source))) >> 6) & 0x03;
    switch (tag){
        case 0x00:
            // return hocotext_common_overlay(source,offset,len,collid);
        case 0x01:
            return hocotext_rle_hoco_overlay(source,offset,len,str,collid);
        case 0x02:
            // return hocotext_tadoc_hoco_overlay(source,offset,str,collid);
        case 0x03:
            // return hocotext_???_hoco_overlay(source,offset,str,collid);
        default:
            ereport(ERROR,
                    errmsg("Unrecognizable compresssion code: %d",tag));
    }
}

text * hocotext_hoco_delete_helper(struct varlena * source,int32 offset,int32 len,Oid collid){
    char tag = (((unsigned char)(*VARDATA_ANY(source))) >> 6) & 0x03;
    switch (tag){
        case 0x00:
            // return hocotext_common_delete(source,offset,len,collid);
        case 0x01:
            return hocotext_rle_hoco_delete(source,offset,len,collid);
        case 0x02:
            // return hocotext_tadoc_hoco_delete(source,offset,len,collid);
        case 0x03:
            // return hocotext_???_hoco_delete(source,offset,len,collid);
        default:
            ereport(ERROR,
                    errmsg("Unrecognizable compresssion code: %d",tag));
    }
}

/**
 * utility functions
*/

text * 
hocotext_common_extract(struct varlena * source,
                            int32 offset,
                            int32 len,
                            Oid collid){
    text *result = (text *)palloc(len + VARHDRSZ+10); 
    char * sp = VARDATA_ANY(source);
    char * srcend = (char *)source + VARSIZE(source);
    int32 result_len;

	if (!OidIsValid(collid))
	{
		/*
		 * This typically means that the parser could not resolve a conflict
		 * of implicit collations, so report it that way.
		 */
		ereport(ERROR,
				(errcode(ERRCODE_INDETERMINATE_COLLATION),
				 errmsg("could not determine which collation to use for %s function",
						"hocotext_rle_extract()"),
				 errhint("Use the COLLATE clause to set the collation explicitly.")));
	}


    if(offset > VARSIZE_ANY_EXHDR(source)){
        return NULL;
    }
    sp += offset;
    result_len = ((srcend - sp)< len? (srcend - sp):len);
    memcpy(VARDATA_ANY(result),sp,result_len);
    *(VARDATA_ANY(result) + result_len) = '\0';
    SET_VARSIZE(result,result_len+VARHDRSZ);
    return result;
}

int32 
hocotext_common_cmp(struct varlena * left, struct varlena * right, Oid collid)
{
    char * lefp = VARDATA_ANY(left);
    char * lefend = (char *)left + VARSIZE_ANY(left);
    
    char * righp = VARDATA_ANY(right);
    char * righend = (char *)right + VARSIZE_ANY(right);


	if (!OidIsValid(collid))
	{
		/*
		 * This typically means that the parser could not resolve a conflict
		 * of implicit collations, so report it that way.
		 */
		ereport(ERROR,
				(errcode(ERRCODE_INDETERMINATE_COLLATION),
				 errmsg("could not determine which collation to use for %s function",
						"hocotext_rle_cmp()"),
				 errhint("Use the COLLATE clause to set the collation explicitly.")));
	}

    while(lefp < lefend && righp < righend){
        if((*lefp) == (*righp)){
            lefp ++;
            righp ++;
            continue;
        }
        return (*lefp) > (*righp) ? 1 :  ((*lefp) == (*righp) ? 0 : -1); 
    }

    if(lefp < lefend) return 1;
    else if(righp < righend) return -1;
    else return 0;

}

/**
* input/output routines
* using internal text input/output routines
*/

/**
 * compression functions
*/
PG_FUNCTION_INFO_V1(hocotext_compress_rle); 
PG_FUNCTION_INFO_V1(hocotext_decompress_rle); 
PG_FUNCTION_INFO_V1(hocotext_compress_tadoc); 
PG_FUNCTION_INFO_V1(hocotext_decompress_tadoc); 
PG_FUNCTION_INFO_V1(hocotext_to_tsvector);

/**
* operating functions
*/
PG_FUNCTION_INFO_V1(hocotext_eq); // == 
PG_FUNCTION_INFO_V1(hocotext_nq); // !=
PG_FUNCTION_INFO_V1(hocotext_lt); // <
PG_FUNCTION_INFO_V1(hocotext_le); // <=
PG_FUNCTION_INFO_V1(hocotext_gt); // >
PG_FUNCTION_INFO_V1(hocotext_ge); // >=

PG_FUNCTION_INFO_V1(hocotext_substring);
PG_FUNCTION_INFO_V1(hocotext_insert);
PG_FUNCTION_INFO_V1(hocotext_overlay);
PG_FUNCTION_INFO_V1(hocotext_char_length);
PG_FUNCTION_INFO_V1(hocotext_concat);
PG_FUNCTION_INFO_V1(hocotext_delete);


/**
 * aggregate functions
*/

PG_FUNCTION_INFO_V1(hocotext_smaller); // return the smaller text
PG_FUNCTION_INFO_V1(hocotext_larger);  // return the larger text


/**
 * indexing support functions 
*/

PG_FUNCTION_INFO_V1(hocotext_cmp);
PG_FUNCTION_INFO_V1(hocotext_hash);



/**
 * *************************************
 * *      COMPRESSION FUNCTIONS        *
 * *************************************
*/
Datum
hocotext_compress_rle(PG_FUNCTION_ARGS){
    text *source = PG_GETARG_TEXT_PP(0);
    text *result = NULL;

    // instr_time	starttime;
    // INSTR_TIME_SET_CURRENT(starttime);
    result = rle_compress(source,NULL,PG_GET_COLLATION());
    // double totaltime =0;
    // totaltime += elapsed_time(&starttime);
    // printf("hocotext_compress_rle compress cost %f ms\n",1000.0 * totaltime);
   PG_FREE_IF_COPY(source,0);

   PG_RETURN_TEXT_P(result);
}

Datum
hocotext_decompress_rle(PG_FUNCTION_ARGS){
    struct varlena *source = PG_GETARG_TEXT_PP(0);
    text *result = NULL;
    // instr_time	starttime;

    // unsigned char * dstart = source;
    // int size = VARSIZE_ANY_EXHDR(source);
    // printf("in outter hocotext_decompress_rle function! compressed size = %d\n",size);
    // unsigned char * dp = dstart + size + VARHDRSZ;
    // while(dstart <= dp){
    //     printChar(*dstart);
    //     dstart++;
    // }

    // INSTR_TIME_SET_CURRENT(starttime);
    result = rle_decompress(source,PG_GET_COLLATION());
    // double totaltime =0;
    // totaltime += elapsed_time(&starttime);
    // printf("hocotext_decompress_rle decompress cost %f ms\n",1000.0 * totaltime);
   
   PG_FREE_IF_COPY(source,0);

   PG_RETURN_TEXT_P(result);
}

Datum
hocotext_compress_tadoc(PG_FUNCTION_ARGS) {
	struct varlena *source = PG_GETARG_TEXT_PP(0);
	instr_time starttime;
	INSTR_TIME_SET_CURRENT(starttime);
	text* result = tadoc_compress(source, PG_GET_COLLATION());
	double totaltime = elapsed_time(&starttime);
	printf("hocotext_compress_tadoc compress cost %f ms\n",1000.0 * totaltime);
	PG_FREE_IF_COPY(source,0);
	PG_RETURN_TEXT_P(result);
}

Datum
hocotext_decompress_tadoc(PG_FUNCTION_ARGS) {
	struct varlena *source = PG_GETARG_TEXT_PP(0);
	// instr_time starttime;
	// INSTR_TIME_SET_CURRENT(starttime);
	text* result = tadoc_decompress(source, PG_GET_COLLATION());
	// double totaltime = elapsed_time(&starttime);
	// printf("hocotext_decompress_tadoc decompress cost %f ms\n",1000.0 * totaltime);
	// printf("origin text is %s\n", VARDATA_ANY(result));
	PG_FREE_IF_COPY(source, 0);
	printf("after free...\n");
	PG_RETURN_TEXT_P(result);
}

/**
 * *************************************
 * *       OPERATING FUNCTIONS         *
 * *************************************
*/

Datum
hocotext_eq(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    bool result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) == 0);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_BOOL(result);
}

Datum
hocotext_nq(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    bool result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) != 0);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_BOOL(result);
}

Datum
hocotext_lt(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    bool result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) < 0);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_BOOL(result);
}

Datum
hocotext_le(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    bool result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) < 0);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_BOOL(result);
}

Datum
hocotext_gt(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    bool result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) > 0);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_BOOL(result);
}

Datum
hocotext_ge(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    bool result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) >= 0);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_BOOL(result);
}

Datum
hocotext_substring(PG_FUNCTION_ARGS){
    struct varlena *source = PG_GETARG_TEXT_PP(0);
    int32 offset = PG_GETARG_INT32(1);
    int32 len = PG_GETARG_INT32(2);
    text *result = NULL;

    // instr_time	starttime;
    // INSTR_TIME_SET_CURRENT(starttime);
    result = hocotext_hoco_extract_helper(source,offset,len,PG_GET_COLLATION());

    // double totaltime =0;
    // totaltime += elapsed_time(&starttime);
    // printf("hocotext_substring substring cost %f ms\n",1000.0 * totaltime);
   PG_FREE_IF_COPY(source,0);

   PG_RETURN_TEXT_P(result);
}


Datum
hocotext_insert(PG_FUNCTION_ARGS){
    struct varlena *source = PG_GETARG_TEXT_PP(0);
    int32 offset = PG_GETARG_INT32(1);
    text *str = PG_GETARG_TEXT_P(2);
    text *result = NULL;

    result = hocotext_hoco_insert_helper(source,offset,str,PG_GET_COLLATION());

   PG_FREE_IF_COPY(source,0);
   PG_FREE_IF_COPY(str,2);

   PG_RETURN_TEXT_P(result);
}

Datum
hocotext_overlay(PG_FUNCTION_ARGS){
    struct varlena *source = PG_GETARG_TEXT_PP(0);
    int32 offset = PG_GETARG_INT32(1);
    int32 len = PG_GETARG_INT32(2);
    text *str = PG_GETARG_TEXT_P(3);
    text *result = NULL;

    result = hocotext_hoco_overlay_helper(source,offset,len,str,PG_GET_COLLATION());

   PG_FREE_IF_COPY(source,0);
   PG_FREE_IF_COPY(str,3);

   PG_RETURN_TEXT_P(result);
}

Datum
hocotext_concat(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    text * result;

    result = hocotext_rle_hoco_concat(left,right,PG_GET_COLLATION());

   PG_FREE_IF_COPY(left,0);
   PG_FREE_IF_COPY(right,1);

   PG_RETURN_TEXT_P(result);
}

Datum
hocotext_char_length(PG_FUNCTION_ARGS){
    struct varlena *source = PG_GETARG_TEXT_PP(0);
    int32 result;

    result = hocotext_rle_hoco_char_length(source,PG_GET_COLLATION());

   PG_FREE_IF_COPY(source,0);

   PG_RETURN_INT32(result);
}

Datum
hocotext_delete(PG_FUNCTION_ARGS){
    struct varlena *source = PG_GETARG_TEXT_PP(0);
    int32 offset = PG_GETARG_INT32(1);
    int32 len = PG_GETARG_INT32(2);
    text *result = NULL;

    result = hocotext_hoco_delete_helper(source,offset,len,PG_GET_COLLATION());


   PG_FREE_IF_COPY(source,0);

   PG_RETURN_TEXT_P(result);
}




/**
 * *************************************
 * *      AGGREGATE FUNCTIONS          *
 * *************************************
*/

Datum
hocotext_smaller(PG_FUNCTION_ARGS){
    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    text *result;


    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) < 0 ? left : right);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_TEXT_P((text *)result);
}

Datum
hocotext_larger(PG_FUNCTION_ARGS){

    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    text *result;

    result = (hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION()) > 0 ? left : right);

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_TEXT_P((text *)result);
}


/**
 * *************************************
 * *      INDEXING FUNCTIONS          *
 * *************************************
*/

Datum
hocotext_cmp(PG_FUNCTION_ARGS){

    struct varlena *left = PG_GETARG_TEXT_PP(0);
    struct varlena *right = PG_GETARG_TEXT_PP(1);
    int32 result;


    result = hocotext_hoco_cmp_helper(left,right,PG_GET_COLLATION());

	PG_FREE_IF_COPY(left, 0);
	PG_FREE_IF_COPY(right, 1);

    PG_RETURN_INT32(result);
}


Datum
hocotext_hash(PG_FUNCTION_ARGS){

    /**
     * do nothing
    */

    PG_RETURN_INT32(0);
}

/**
 * tadoc_to_tsvector
*/
Datum hocotext_to_tsvector(PG_FUNCTION_ARGS) {
	text* comp_var = PG_GETARG_TEXT_PP(0);
	char* comp_data = VARDATA_ANY(comp_var);
	TSVector out = tadoc_to_tsvector(comp_data);
	PG_RETURN_TSVECTOR(out);
}