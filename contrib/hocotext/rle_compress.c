/**
 * 
 * contrib/hocotext/compress_rle.c
 * 
 *    an implementation of Run Length Encoding for PostgreSQL
 * 
 * When dealing with individual non-repeating data, we count the number of discontinuous 
 * data within this interval. Then, a marker is used to indicate the number of subsequent 
 * data pieces that are discontinuous. To differentiate between marking repeated or 
 * non-repeated data, we utilize the highest bit of a byte. A value of 0 indicates non-repeated 
 * data, while 1 indicates repeated data. The remaining 7 bits are used to represent the count 
 * of data pieces. This approach allows us to mark a maximum of 127 bytes.
 * However, when there are only two duplicate data pieces, the duplicate data storage format 
 * still occupies two bytes. In such cases, we opt for the non-duplicate data storage format. 
 * Consequently, we cannot use the values 0, 1, or 2 for marking duplicate data. Instead, we
 *  use 0 to represent the count of 3, thereby expanding the maximum number of duplicate bytes 
 * to 130.
 * 
*/

#include <limits.h>
#include <string.h>
#include "hocotext.h"


/* ----------
 * Local definitions
 * ----------
 */
#define MAX_REPEATED_SIZE 130
#define MAX_SINGLE_STORE_SIZE 127
#define THRESHOLD 3

#define store_single_buf(__bp,__buf_base,__buf,reach_max) \
do { \
    if(reach_max){                                                  \
        *__bp = MAX_SINGLE_STORE_SIZE ;                             \
        __bp++;                                                     \
        memcpy(__bp,__buf_base,MAX_SINGLE_STORE_SIZE);              \
        __bp += MAX_SINGLE_STORE_SIZE;                              \
        memcpy(__buf_base,                                          \
                __buf_base + MAX_SINGLE_STORE_SIZE,                 \
                __buf - __buf_base - MAX_SINGLE_STORE_SIZE);        \
        __buf = __buf_base;                                         \
    }else{                                                          \
        *__bp = __buf - __buf_base ;                                \
        __bp++;                                                     \
        memcpy(__bp,__buf_base,__buf-__buf_base);                   \
        __bp += __buf - __buf_base;                                 \
        __buf = __buf_base;                                         \
    }                                                               \
}while(0)

/**
 * provided standard strategies
 * basic compression restriction
*/
static const RLE_Strategy rle_default_strategy = {
	32,							/* Data chunks less than 32 bytes are not
								 * compressed */
	INT_MAX,					/* No upper limit on what we'll try to
								 * compress */
	25							/* Require 25% compression rate, or not worth
								 * it */
};
const RLE_Strategy *const RLE_strategy_default = &rle_default_strategy;


/**
 * rle_compress -
 * 
 * 
 *            Compress Source into dest. 
 *            Return the number of bytes written in buffer dest, 
 *            or -1 if compression fails.
*/
text * rle_compress(struct varlena *source,const RLE_Strategy *strategy,Oid collid){

    char * dp = VARDATA_ANY(source); //uncompressed data
    char * dend = dp + VARSIZE_ANY_EXHDR(source);//end of uncompressed data
    text *result = (text *)palloc(VARSIZE_ANY_EXHDR(source) + VARHDRSZ); 
	char *bp = VARDATA_ANY(result);            //compressed data
	char *bstart = bp;                         //start of compressed data
    // int32 need_rate = 99;
    // int32 result_max;
    int32 result_size;
    int32 cur_index = 0;
    char cur_char;
    char *buf = (char *)palloc(MAX_SINGLE_STORE_SIZE);
    char *buf_base;
    buf_base = buf;
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
    /**
     * Our fallback strategy is default.
    */
    // if (strategy == NULL){
    //     strategy = RLE_strategy_default;

    // }

    // if(slen < strategy->min_input_size || 
    //     slen > strategy->max_input_size)
    // {
    //         /**
    //          * 
    //          * RETURN raw text
    //          * 
    //         */
    // }
            

    // need_rate = strategy->min_comp_rate;
    // if(need_rate < 0 ) need_rate = 0;
    // else if(need_rate > 99) need_rate = 99;

	// /*
	//  * Compute the maximum result size allowed by the strategy, namely the
	//  * input size minus the minimum wanted compression rate.  This had better
	//  * be <= slen, else we might overrun the provided output buffer.
	//  */
	// if (slen > (INT_MAX / 100))
	// {
	// 	/* Approximate to avoid overflow */
	// 	result_max = (slen / 100) * (100 - need_rate);
	// }
	// else
	// 	result_max = (slen * (100 - need_rate)) / 100;


    // record rawsize
    int32 rawsize = VARSIZE_ANY_EXHDR(source);

    // buf_put_int(bp, rawsize );
    // printf("write rawsize = %d  put in buf = %d\n",rawsize,(rawsize | 0x40000000));
    buf_put_int(bp, rawsize | 0x40000000);
    while (THRESHOLD <= dend - dp){
        /**
         * if we already exceed the maximum result size, fail.
        */
        // if(bp - bstart >= result_max){
        //     /**
        //      * 
        //      * RETURN raw text
        //      * 
        //     */
        // } 
        cur_index = 0; /** consequent repeated size */
        if((*(dp + cur_index)) == (*(dp+cur_index+1))){
            if((*(dp+cur_index+1)) == (*(dp+cur_index+2))){
                cur_index ++;
                while(dp + cur_index + 2 < dend  && (*(dp + cur_index + 1) == *(dp + cur_index + 2))){
                    cur_index ++;
                }
                if(buf!=buf_base){
                    store_single_buf(bp,buf_base,buf,false);
                }
                cur_char = *dp;
                while (cur_index > 0)
                {
                    *bp = ((cur_index + 2) > MAX_REPEATED_SIZE ? (MAX_REPEATED_SIZE-THRESHOLD) : (cur_index + 2 - THRESHOLD) ) | (1 << 7) ;
                    bp ++;
                    *bp = cur_char;
                    bp++;
                    dp += (cur_index + 2) > MAX_REPEATED_SIZE ? MAX_REPEATED_SIZE : (cur_index + 2);
                    
                    cur_index -= MAX_REPEATED_SIZE;
                }
            }else{
                buf_copy_ctrl(buf,dp);
                buf_copy_ctrl(buf,dp);
            }


        }else{
            buf_copy_ctrl(buf,dp);
        }

        if(buf - buf_base >= MAX_SINGLE_STORE_SIZE){
            store_single_buf(bp,buf_base,buf,true);
        }        
    }

    memcpy(buf,dp,dend-dp);
    buf += (dend - dp);

    if(buf - buf_base >= MAX_SINGLE_STORE_SIZE){
        store_single_buf(bp,buf_base,buf,true);
    }  

    if(buf!=buf_base){
        store_single_buf(bp,buf_base,buf,false);
    }
    *bp = '\0';

    result_size = bp - bstart;

    // }
    // if(result_size >= result_max){
    //         /**
    //          * 
    //          * RETURN raw text
    //          * 
    //         */
    // }

    SET_VARSIZE(result,result_size+VARHDRSZ);
    return result;
}


/**
 * rle_decompress -
 * 
 * 
 *      Decompress source into dest.
 *      Return the number of bytes decompressed into the destination buffer, 
 *      or -1 if the compressed data is corrupted
 *  	
 *      If check_complete is true, the data is considered corrupted
 *		if we don't exactly fill the destination buffer.  Callers that
 *		are extracting a slice typically can't apply this check.
*/

text *
rle_decompress(struct varlena *source,Oid collid){
    unsigned char * sp = VARDATA_ANY(source);

    unsigned char * srcend = sp + VARSIZE_ANY_EXHDR(source);
    unsigned char tag = (((unsigned char)(*sp)) >> 6) & 0x03;
    int32 geta = buf_get_int(sp);
    int32 rawsize = geta & 0x3fffffff;
    text *result = (text *)palloc(rawsize + VARHDRSZ); 
    memset(result,0,sizeof(result));
    // printf("get from buf %d    raw size = %d  \n",geta,rawsize);
	unsigned char *dp = VARDATA_ANY(result);
	unsigned char *destend = VARDATA_ANY(result) + rawsize;
    
    int32 repeat_count;
    int32 single_count;
    char cur_data;
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


	while (sp < srcend && dp < destend)
	{
        if(((*sp)&(1<<7)) == (1<<7)){ 
            repeat_count = (int32)((*sp) & 0x7F)+THRESHOLD;
            sp++;
            cur_data = *sp;
            for(int i = 0 ; i < repeat_count;i++){
                *dp = cur_data;
                dp++;
            }
            sp ++;
        }else{
            single_count = (int32)((*sp) & 0x7F);
            sp++;

            memcpy(dp,sp,single_count);
            dp += single_count;
            sp += single_count;
        }
    }
    *dp = '\0';

    
	/*
	 * check we decompressed the right amount.
	 */
	if (dp != destend || sp != srcend)
    {
        ereport(ERROR,
				(errmsg("wrong decompression result. dp = %d destend = %d sp = %d srcend = %d",dp,destend,sp,srcend)));
    }


    SET_VARSIZE(result,(char *) dp - (char *) result);
	return result;
}