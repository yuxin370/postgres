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


int32 rle_compress_ctrl(unsigned char *sp,unsigned char *srcend,unsigned char *dp){
    int32 cur_index = 0;
    unsigned char cur_char;
    unsigned char *buf = (char *)palloc(MAX_SINGLE_STORE_SIZE);
    unsigned char *buf_base = buf;
	unsigned char *dstart = dp;                         //start of compressed data

    while (THRESHOLD <= srcend - sp){
        /**
         * if we already exceed the maximum result size, fail.
        */
        // if(dp - dstart >= result_max){
        //     /**
        //      * 
        //      * RETURN raw text
        //      * 
        //     */
        // } 
        cur_index = 0; /** consequent repeated size */
        if((*(sp + cur_index)) == (*(sp+cur_index+1))){
            if((*(sp+cur_index+1)) == (*(sp+cur_index+2))){
                cur_index ++;
                while(sp + cur_index + 2 < srcend  && (*(sp + cur_index + 1) == *(sp + cur_index + 2))){
                    cur_index ++;
                }
                if(buf!=buf_base){
                    store_single_buf(dp,buf_base,buf,false);
                }
                cur_char = *sp;
                while (cur_index > 0)
                {
                    *dp = ((cur_index + 2) > MAX_REPEATED_SIZE ? (MAX_REPEATED_SIZE-THRESHOLD) : (cur_index + 2 - THRESHOLD) ) | (1 << 7) ;
                    dp ++;
                    *dp = cur_char;
                    dp++;
                    sp += (cur_index + 2) > MAX_REPEATED_SIZE ? MAX_REPEATED_SIZE : (cur_index + 2);
                    // printf("[compressing] %d %c\n",cur_index + 2,cur_char);
                    cur_index -= MAX_REPEATED_SIZE;
                }
            }else{
                buf_copy_ctrl(buf,sp);
                buf_copy_ctrl(buf,sp);
            }


        }else{
            buf_copy_ctrl(buf,sp);
        }

        if(buf - buf_base >= MAX_SINGLE_STORE_SIZE){
            store_single_buf(dp,buf_base,buf,true);
        }        
    }

    memcpy(buf,sp,srcend-sp);
    buf += (srcend - sp);

    if(buf - buf_base >= MAX_SINGLE_STORE_SIZE){
        store_single_buf(dp,buf_base,buf,true);
    }  

    if(buf!=buf_base){
        store_single_buf(dp,buf_base,buf,false);
    }
    *dp = '\0';

    return (int32)(dp - dstart);
}


/**
 * rle_compress -
 * 
 * 
 *            Compress Source into dest. 
 *            Return the number of bytes written in buffer dest, 
 *            or -1 if compression fails.
*/
text * rle_compress(struct varlena *source,const RLE_Strategy *strategy,Oid collid){
    char * sp = VARDATA_ANY(source); //uncompressed data
    char * srcend = sp + VARSIZE_ANY_EXHDR(source);//end of uncompressed data
    text *result = (text *)palloc(VARSIZE_ANY_EXHDR(source) + VARHDRSZ); 
	char *dp = VARDATA_ANY(result);            //compressed data
    int32 rawsize = VARSIZE_ANY_EXHDR(source);
    buf_put_int(dp, rawsize | 0x40000000);     // record rawsize

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
    // int32 need_rate = 99;
    // int32 result_max;
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

    // }
    // if(result_size >= result_max){
    //         /**
    //          * 
    //          * RETURN raw text
    //          * 
    //         */
    // }
    // printf("in rle_compress function! source = %s size = %d \n",sp,rawsize);
    // unsigned char * start = source;
    // int size =VARSIZE_ANY_EXHDR(source);
    // unsigned char * dd = start + size + VARHDRSZ;
    // while(start <= dd){
    //     printChar(*start);
    //     start++;
    // }
    // printf("\n");

    int32 result_size = rle_compress_ctrl(sp,srcend,dp);



    SET_VARSIZE(result,4 + result_size + VARHDRSZ);

    // printf("in rle_compress function! source = %s size = %d  result = %s ,size = %d   read size = %d\n",sp,VARSIZE_ANY_EXHDR(source),dp,result_size,VARSIZE_ANY_EXHDR(result));
    //  start = result;
    //  size = 4 + result_size;
    //  dd = start + size + VARHDRSZ;
    // while(start <= dd){
    //     printChar(*start);
    //     start++;
    // }
    // printf("\n");

    return result;
}

void printChar(unsigned char a){
    for(int i = 7 ; i >= 0 ; i --){
        printf("%d",(a>>i)&0x1);
    }
    printf("\n");
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
    // instr_time	starttime;
    // INSTR_TIME_SET_CURRENT(starttime);

    unsigned char * sp = VARDATA_ANY(source);
    unsigned char * srcend = sp + VARSIZE_ANY_EXHDR(source);

    // printf("in rle_decompress function![source = %x sp = %x compressed start = %x]  compressed value = %s size = %d rawsize = %d\n",source,sp,sp + 4,sp + 4,VARSIZE_ANY_EXHDR(source));
    // unsigned char * dstart = source;
    // int size = VARSIZE_ANY_EXHDR(source);
    // unsigned char * dd = dstart + size + VARHDRSZ;
    // while(dstart <= dd){
    //     printf("%x :",dstart);
    //     printChar(*dstart);
    //     dstart++;
    // }

    int32 rawsize = buf_get_int(sp) & 0x3fffffff;
    // printf("in rle_decompress function![source = %x compressed start = %x]  compressed value = %s size = %d rawsize = %d,compression ratio = %f\n",source,sp,sp,VARSIZE_ANY_EXHDR(source),rawsize,((rawsize - VARSIZE_ANY_EXHDR(source) )/rawsize) );

    text *result = (text *)palloc(rawsize + VARHDRSZ);
    memset(result,0,sizeof(result));
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

    int count = 0;
	while (sp < srcend)
	{
        if(((*sp)&(1<<7)) == (1<<7)){ 
            repeat_count = (int32)((*sp) & 0x7F)+THRESHOLD;
            sp++;
            cur_data = *sp;
            count += repeat_count;
            for(int i = 0 ; i < repeat_count;i++){
                *dp = cur_data;
                dp++;
            }
            sp ++;
        }else{
            single_count = (int32)((*sp) & 0x7F);
            sp++;
            count += single_count;
            memcpy(dp,sp,single_count);
            dp += single_count;
            sp += single_count;

        }
    }
    *dp = '\0';


	/*
	 * check we decompressed the right amount.
	 */
	// if (dp != destend || sp != srcend)
    // {
    //     ereport(ERROR,
	// 			(errmsg("wrong decompression result. dp = %d destend = %d sp = %d srcend = %d",dp,destend,sp,srcend)));
    // }

    SET_VARSIZE(result,(char *) dp - (char *) result);
	return result;
}