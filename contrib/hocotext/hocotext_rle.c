/**
 * contrib/hocotext/hocotext_rle.c
*/
#include "hocotext.h"


#define buf_rle_value(__sp,__dp,__count,__sp_ctrlable)  \
do{ \
    if(__sp_ctrlable){                                  \
        if(__count >= THRESHOLD){                       \
            *__dp = (__count - THRESHOLD) | (1 << 7);   \
            __dp ++;                                    \
            __sp ++;                                    \
            buf_copy_ctrl(__dp,__sp);                   \
        }else{                                          \
            *__dp = __count;                            \
            __dp ++;                                    \
            __sp ++;                                    \
            repeat_buf_copy(__dp,*__sp,__count);        \
            __sp++;                                     \
        }                                               \
    }else{                                              \
        if(__count >= THRESHOLD){                       \
            *__dp = (__count - THRESHOLD) | (1 << 7);   \
            __dp ++;                                    \
            *dp = *__sp;                                  \
            dp ++;                                      \
        }else{                                          \
            *__dp = __count;                            \
            __dp ++;                                    \
            repeat_buf_copy(__dp,*__sp,__count);        \
        }                                               \
    }                                                   \
}while(0)                  

#define buf_bit_packed_value(__dp,__buf_base,count) \
do { \
        *__dp = count ;                                \
        __dp++;                                        \
        memcpy(__dp,__buf_base,count);                 \
        __dp += count;                                 \
        __buf_base += count;                           \
}while(0)


int32 
hocotext_rle_hoco_cmp(struct varlena * left, 
                    struct varlena * right, 
                    Oid collid)
{

    unsigned char *lefp;
    unsigned char *lefend;
	unsigned char *righp;
    unsigned char *righend;
    unsigned char* cur_lef = (char *)palloc(131);
    unsigned char* cur_righ = (char *)palloc(131);
    unsigned char *cur_lef_p = cur_lef;
    unsigned char *cur_righ_p = cur_righ;
    int32 lef_count = 0;
    int32 righ_count = 0;

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

    lefp = VARDATA_ANY(left);
    righp = VARDATA_ANY(right);
    lefend = (char *) left + VARSIZE_ANY_EXHDR(left);
    righend = (char *) right + VARSIZE_ANY_EXHDR(right);
    lefp += RAWDATA_BYTE;
    righp += RAWDATA_BYTE;

    while(lefp < lefend && righp < righend){
        if(lef_count == 0){
            lef_count =  (0x1&((*lefp) >> 7)) == 1 ? (int32)((*lefp) & 0x7f) + THRESHOLD :  (int32)((*lefp) & 0x7f);
            if((0x1&((*lefp) >> 7)) == 1) {
                lefp ++;
                cur_lef_p = cur_lef;
                repeat_buf_copy(cur_lef_p,(*(lefp)),lef_count);
                *cur_lef_p = '\0';
                lefp++;
            }else{
                lefp++;
                memcpy(cur_lef,(lefp),lef_count);
                *(cur_lef + lef_count) = '\0';
                lefp += lef_count;
            }
            cur_lef_p = cur_lef;
        }

        if(righ_count == 0){
            righ_count =  (0x1&((*righp) >> 7)) == 1 ? (int32)((*righp) & 0x7f) + THRESHOLD :  (int32)((*righp) & 0x7f);
            if((0x1&((*righp) >> 7)) == 1) {
                righp ++;
                cur_righ_p = cur_righ;
                repeat_buf_copy(cur_righ_p,(*(righp)),righ_count);
                *cur_righ_p = '\0';
                righp++;
            }else{
                righp++;
                memcpy(cur_righ,(righp),righ_count);
                *(cur_righ + righ_count) = '\0';
                righp += righ_count;
            }
            cur_righ_p = cur_righ;
        }
        

        while(lef_count >0 && righ_count > 0){

            if((*cur_lef_p) == (*cur_righ_p)){
                cur_lef_p ++;
                cur_righ_p++;
                lef_count --;
                righ_count --;
            }else{
                return (*cur_lef_p) > (*cur_righ_p) ? 1 :  ((*cur_lef_p) == (*cur_righ_p) ? 0 : -1);
            }
        }
    }
    
    while(lefp < lefend && righ_count > 0){
        if(lef_count == 0){
            lef_count =  (0x1&((*lefp) >> 7)) == 1 ? (int32)((*lefp) & 0x7f) + THRESHOLD :  (int32)((*lefp) & 0x7f);
            if((0x1&((*lefp) >> 7)) == 1) {
                lefp ++;
                cur_lef_p = cur_lef;
                repeat_buf_copy(cur_lef_p,(*(lefp)),lef_count);
                *cur_lef_p = '\0';
                lefp++;
            }else{
                lefp++;
                memcpy(cur_lef,(lefp),lef_count);
                lefp += lef_count;
            }
            cur_lef_p = cur_lef;
        }
        while(lef_count >0 && righ_count > 0){
            if((*cur_lef_p) == (*cur_righ_p)){
                cur_lef_p ++;
                cur_righ_p++;
                lef_count --;
                righ_count --;
            }else{
                return (*cur_lef_p) > (*cur_righ_p) ? 1 :  ((*cur_lef_p) == (*cur_righ_p) ? 0 : -1);
            }
        }
    }

    while(righp < righend && lef_count > 0){

        if(righ_count == 0){
            righ_count =  (0x1&((*righp) >> 7)) == 1 ? (int32)((*righp) & 0x7f) + THRESHOLD :  (int32)((*righp) & 0x7f);
            if((0x1&((*righp) >> 7)) == 1) {
                righp ++;
                cur_righ_p = cur_righ;
                repeat_buf_copy(cur_righ_p,(*(righp)),righ_count);
                *cur_righ_p = '\0';
                righp++;
            }else{
                righp++;
                memcpy(cur_righ,(righp),righ_count);
                *(cur_righ + righ_count) = '\0';
                righp += righ_count;
            }
            cur_righ_p = cur_righ;
        }
        while(lef_count >0 && righ_count > 0){
            if((*cur_lef_p) == (*cur_righ_p)){
                cur_lef_p ++;
                cur_righ_p++;
                lef_count --;
                righ_count --;
            }else{
                return (*cur_lef_p) > (*cur_righ_p) ? 1 :  ((*cur_lef_p) == (*cur_righ_p) ? 0 : -1);
            }
        }
    }

    if(lefp < lefend || lef_count > 0) return 1;
    else if(righp < righend || righ_count > 0) return -1;
    else return 0;

}

text * 
hocotext_rle_hoco_extract(struct varlena * source,
                        int32 offset,
                        int32 len,
                        Oid collid){
	unsigned char *dp;
	unsigned char *destend;
    int32 cur_offset = 0; //offset in raw text
    int32 count = 0;
    int32 type = 1; // 1: repeated  0: single
    int32 reach = 0;
    unsigned char * sp = VARDATA_ANY(source);
    // unsigned char * srcend = sp + VARSIZE_ANY_EXHDR(source);
    int32 rawsize = buf_get_int(sp) & 0x3fffffff;

    offset --;
    if(offset < 0){
        len += offset;
        offset = 0;
    }
    if(len < 0){
        len = 0;
    }
    // int32 source_len = 0;
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

    if(rawsize < offset){
        offset = rawsize;
    }


    len = offset + len > rawsize ? rawsize - offset : len;
    text *result = (text *)palloc(len + VARHDRSZ + 10); 
    memset(result,0,sizeof(result));    
    dp = VARDATA_ANY(result);
    destend = VARDATA_ANY(result) + len;

    /**
     * locate the offset
     */
    while(cur_offset < offset || !reach){
        type = (int32)(((*sp) >> 7) & 1);
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        reach = offset - (count + cur_offset) <= 0 ? 1 : 0;
        if(type == 1){
            sp += reach ? 1 : 2; 
        }else{
            sp += reach ?  offset - cur_offset + 1 : count + 1; 
        }
        count -= reach ? offset - cur_offset : 0;
        cur_offset += reach ? offset - cur_offset : count ; 
    }
    while(cur_offset - offset < len && cur_offset < rawsize){
        if(sp == source->vl_dat){
            type = (int32)(((*sp) >> 7)&0x1);
            count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        }
        if(count + cur_offset - offset > len) count = len + offset - cur_offset;
        
        if(type == 1){
            //repeat
            repeat_buf_copy(dp,(*sp),count);
            sp ++ ;
        }else{
            //single
            memcpy(dp,sp,count);
            dp += count;
            sp += count;
        }
        cur_offset += count;
        type = (int32)(((*sp) >> 7)&0x1);
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        sp++;
    }
    *dp = '\0';
    if(!(dp=destend) || cur_offset > rawsize){
		ereport(ERROR,
				(errmsg("wrong extraction result.")));
    }
    SET_VARSIZE(result,len+VARHDRSZ);
    return result;
}



text * 
hocotext_rle_hoco_insert(struct varlena * source,
                        int32 offset,
                        text *str,Oid collid){

    unsigned char * sp = VARDATA_ANY(source);
    unsigned char * srcend = sp + VARSIZE_ANY_EXHDR(source);
    int32 rawsize = buf_get_int(sp) & 0x3fffffff;
    int32 cur_offset = 0; //offset in raw text;
    int32 type = 1; // 1: repeated  0: single;
    int32 reach = 0,count = 0;

    if((--offset) < 0){
        offset = 0;
    }
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

    if(rawsize < offset){
        offset = rawsize;
    }

    int32 str_size = VARSIZE_ANY_EXHDR(str);
    unsigned char *strp = VARDATA_ANY(str);;
    unsigned char *cstrp = (unsigned char *)palloc(str_size + 2); // in case the str is bitpacked and of big size. 
    int32 cstr_size = rle_compress_ctrl(strp,strp + str_size,cstrp);
    unsigned char *cstrend = cstrp + cstr_size;
    
    text *result = (text *)palloc(6 + VARSIZE_ANY_EXHDR(source) + cstr_size + VARHDRSZ); 
    memset(result,0,sizeof(result));    
    unsigned char * dstart = VARDATA_ANY(result);
    unsigned char *dp = dstart;
    buf_put_int(dp,(rawsize + str_size)|0x40000000);

    /**
     * locate the offset
     */
    while(!reach){
        type = (int32)(((*sp) >> 7) & 1);
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        reach = offset - (count + cur_offset) <= 0 ? 1 : 0;
        if(!reach){
            if(type == 1){
                buf_copy_ctrl(dp,sp);
                buf_copy_ctrl(dp,sp);
            }else{
                memcpy(dp,sp,count + 1);
                dp += count+1;
                sp += count+1;
            }
            cur_offset += count ; 
        }
    }

    /**
     * insert characters 
     */
    int32 front_gap = offset - cur_offset;
    int32 back_gap = cur_offset + count - offset;
    if(front_gap == 0){
        // insert as a new pattern at the end of an entire pattern
        memcpy(dp,cstrp,cstr_size);
        dp += cstr_size;
    }else{
        // insert at the middle of currrent pattern, fore_type = back_type = type 
        int32 str_type = (int32)(((*cstrp) >> 7) & 1);
        int32 str_count = type == 1 ? (int32)((*cstrp) & 0x7f) + THRESHOLD : (int32)((*cstrp) & 0x7f);
        sp ++;
        bool is_same_type = (type == str_type);
        if(is_same_type && type == 1 && (*cstrp) == (*sp)){
            // rle type and merge the fisrt inserted pattern
            cstrp ++;
            bool only_one_pattern = (cstrp == cstrend - 1);
            int32 total_count = (only_one_pattern) ? str_count + count : str_count + front_gap;
            while(total_count > 0){
                *dp = (total_count > MAX_REPEATED_SIZE ? (MAX_REPEATED_SIZE-THRESHOLD) : (total_count - THRESHOLD) ) | (1 << 7);
                dp ++;
                *dp = *cstrp;
                dp ++;
                total_count -= MAX_REPEATED_SIZE;
            }
            cstrp ++;
            if(!only_one_pattern){
                memcpy(dp,cstrp,cstr_size - 2);
                dp += cstr_size - 2;
                buf_rle_value(sp,dp,back_gap,false);
            }
            sp++;
        }else if (is_same_type && type == 0){
            // bit-packed type and merge the fisrt inserted pattern
            cstrp ++;
            bool only_one_pattern = (cstrp == cstrend - str_count);
            // int32 total_count =  (only_one_pattern) ? str_count + count : str_count + front_gap;
            int32 total_count =  str_count + front_gap;
            int pattern_length = total_count > MAX_REPEATED_SIZE ? MAX_REPEATED_SIZE : total_count;
            // write length
            *dp = pattern_length;
            dp ++;
            // write value in front fap
            memcpy(dp,sp,front_gap);
            sp += front_gap;
            dp += front_gap;
            pattern_length -= front_gap;
            // write value in compressed str
            memcpy(dp,cstrp,pattern_length);
            dp += pattern_length;
            cstrp += pattern_length;
            // loop to write remained value 
            total_count -= MAX_REPEATED_SIZE;
            while(total_count > 0){
                pattern_length = total_count > MAX_REPEATED_SIZE ? MAX_REPEATED_SIZE : total_count;
                buf_bit_packed_value(dp,cstrp,pattern_length);
                total_count -= MAX_REPEATED_SIZE;
            }
            // write remained patterns in inserted str
            if(!only_one_pattern){
                memcpy(dp,cstrp,cstr_size - total_count - 1);
                dp += cstr_size - total_count - 1;
            }
            // write remained patterns in the source pattern
            buf_bit_packed_value(dp,sp,back_gap);
        }else{
            // insert as an independent pattern in the middle of a pattern.
            if(type == 1){
                buf_rle_value(sp,dp,front_gap,false);
                memcpy(dp,cstrp,cstr_size);
                dp += cstr_size;
                buf_rle_value(sp,dp,back_gap,false);
                sp++;
            }else{
                buf_bit_packed_value(dp,sp,front_gap);
                memcpy(dp,cstrp,cstr_size);
                dp += cstr_size;
                buf_bit_packed_value(dp,sp,back_gap);
            }            
        }
    }
    

    
    /**
     * [todo] reorganize retained values
     */
    

    /**
     * copy rest following values
     */

    if(srcend != sp){
        memcpy(dp,sp,srcend - sp);
        dp += srcend - sp;
    }

    // copy

    *dp = '\0';
    SET_VARSIZE(result,dp -  dstart + VARHDRSZ);
    return result;
}


text * 
hocotext_rle_hoco_delete(struct varlena * source,
                        int32 offset,
                        int32 len,Oid collid){
    unsigned char * sp = VARDATA_ANY(source);
    unsigned char * srcend = sp + VARSIZE_ANY_EXHDR(source);
    int32 rawsize = buf_get_int(sp) & 0x3fffffff;
    int32 cur_offset = 0; //offset in raw text;
    int32 type = 1; // 1: repeated  0: single;
    int32 reach = 0,count = 0;

    printf("in outter hocotext_rle_hoco_delete function! source = %s size = %d\n",sp + 4,VARSIZE_ANY_EXHDR(source));
    unsigned char * start = source;
    int size = VARSIZE_ANY_EXHDR(source);
    unsigned char * dd = start + size + VARHDRSZ;
    while(start <= dd){
        printChar(*start);
        start++;
    }

    if((--offset) < 0){
        len += offset;
        offset = 0;
    }
    if(len < 0) len = 0;
    // int32 source_len = 0;
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

    if(rawsize < offset){
        offset = rawsize;
    }


    len = offset + len > rawsize ? rawsize - offset : len;

    text *result = (text *)palloc(VARSIZE_ANY_EXHDR(source) + VARHDRSZ); 
    memset(result,0,sizeof(result));    
	unsigned char *dstart = VARDATA_ANY(result);
    unsigned char *dp = dstart;
    buf_put_int(dp,(rawsize - len) | 0x40000000);
 
    /**
     * locate the offset
     */
    while(!reach){
        type = (int32)(((*sp) >> 7) & 1);
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        reach = offset - (count + cur_offset) <= 0 ? 1 : 0;
        if(!reach){
            if(type == 1){
                buf_copy_ctrl(dp,sp);
                buf_copy_ctrl(dp,sp);
            }else{
                memcpy(dp,sp,count + 1);
                dp += count+1;
                sp += count+1;
            }
            cur_offset += count ; 
            // fore_type = type;
        }
    }

    /**
     * delete characters 
     */
    while(cur_offset - offset < len && cur_offset < rawsize){
        type = (int32)(((*sp) >> 7) & 1);
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        int front_gap = offset - cur_offset;
        int back_gap = cur_offset + count - offset - len;
        if(front_gap > 0 && back_gap > 0){
            // the pattern can cover the deleted characters
            if(type == 1){
                buf_rle_value(sp,dp,back_gap + front_gap,true);
            }else{
                *dp = (back_gap + front_gap);
                dp ++;
                sp ++;
                memcpy(dp,sp,front_gap);
                sp += front_gap + len;
                dp += front_gap;
                memcpy(dp,sp,back_gap);
                sp += back_gap;
                dp += back_gap;
            }
        }else if (front_gap > 0){
            // the first pattern to be deleted
            if(type == 1){
                buf_rle_value(sp,dp,front_gap,true);
            }else{
                *dp = front_gap;
                dp ++;
                sp ++;
                memcpy(dp,sp,front_gap);
                sp += count;
                dp += front_gap;
            }
        }else if (back_gap > 0){
            // the last pattern to be deleted
            if(type == 1){
                buf_rle_value(sp,dp,back_gap,true);
            }else{
                *dp = back_gap;
                dp ++;
                sp += 1 + count - back_gap;
                memcpy(dp,sp,back_gap);
                sp += back_gap;
                dp += back_gap;
            }
        }else{
            // the entire pattern should be deleted
            if(type == 1){
                sp += 2;
            }else{
                sp += 1 + count;
            }
        }
        cur_offset += count;
    }

    /**
     * [todo] reorganize retained values
     */
    

    /**
     * copy rest values
     */
    if(srcend != sp){
        memcpy(dp,sp,srcend - sp);
        dp += srcend - sp;
    }
    *dp = '\0';
    SET_VARSIZE(result,dp -  dstart + VARHDRSZ);
    return result;
}

text * 
hocotext_rle_hoco_overlay(struct varlena * source,int32 offset,int32 len,text *str,Oid collid){
unsigned char * sp = VARDATA_ANY(source);
    unsigned char * srcend = sp + VARSIZE_ANY_EXHDR(source);
    int32 rawsize = buf_get_int(sp) & 0x3fffffff;
    int32 cur_offset = 0; //offset in raw text;
    int32 type = 1; // 1: repeated  0: single;
    int32 reach = 0,count = 0;

    if((--offset) < 0){
        len += offset;
        offset = 0;
    }
    if(len < 0) len = 0;
    // int32 source_len = 0;
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

    if(rawsize < offset){
        offset = rawsize;
    }


    len = offset + len > rawsize ? rawsize - offset : len;


    int32 str_size = VARSIZE_ANY_EXHDR(str);
    unsigned char *strp = VARDATA_ANY(str);;
    unsigned char *cstrp = (unsigned char *)palloc(str_size + 2); // in case the str is bitpacked and of big size. 
    int32 cstr_size = rle_compress_ctrl(strp,strp + str_size,cstrp);
    unsigned char *cstrend = cstrp + cstr_size;
    

    text *result = (text *)palloc(VARSIZE_ANY_EXHDR(source) + cstr_size + 6 + VARHDRSZ); 
    memset(result,0,sizeof(result));    
	unsigned char *dstart = VARDATA_ANY(result);
    unsigned char *dp = dstart;
    buf_put_int(dp,(rawsize - len + str_size) | 0x40000000);
 
    /**
     * locate the offset
     */
    while(!reach){
        type = (int32)(((*sp) >> 7) & 1);
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        reach = offset - (count + cur_offset) <= 0 ? 1 : 0;
        if(!reach){
            if(type == 1){
                buf_copy_ctrl(dp,sp);
                buf_copy_ctrl(dp,sp);
            }else{
                memcpy(dp,sp,count + 1);
                dp += count+1;
                sp += count+1;
            }
            cur_offset += count ; 
            // fore_type = type;
        }
    }

    /**
     * delete characters 
     */
    int front_gap , back_gap;
    while(cur_offset - offset < len && cur_offset < rawsize){
        count = type == 1 ? (int32)((*sp) & 0x7f) + THRESHOLD : (int32)((*sp) & 0x7f);
        reach = offset - (count + cur_offset) <= 0 ? 1 : 0;
        front_gap = offset - cur_offset;
        back_gap = cur_offset + count - offset - len;
        if(front_gap > 0 && back_gap > 0){
            // the pattern can cover the deleted characters
            if(type == 1){
                sp ++;
                buf_rle_value(sp,dp,front_gap,false);
            }else{
                *dp = front_gap;
                dp ++;
                sp ++;
                memcpy(dp,sp,front_gap);
                sp += front_gap + len;
                dp += front_gap;
            }
        }else if (front_gap > 0){
            // the first pattern to be deleted
            if(type == 1){
                buf_rle_value(sp,dp,front_gap,true);
            }else{
                *dp = front_gap;
                dp ++;
                sp ++;
                memcpy(dp,sp,front_gap);
                sp += count;
                dp += front_gap;
            }
        }else if (back_gap > 0){
            // the last pattern to be deleted
            if(type == 1){
                sp++;
                // buf_rle_value(sp,dp,back_gap,true);
            }else{
                // *dp = back_gap;
                // dp ++;
                sp += 1 + count - back_gap;
                // memcpy(dp,sp,back_gap);
                // sp += back_gap;
                // dp += back_gap;
            }
        }else{
            // the entire pattern should be deleted
            if(type == 1){
                sp += 2;
            }else{
                sp += 1 + count;
            }
        }
        cur_offset += count;
    }


    /**
     * insert characters 
     */
    if(back_gap == 0){
        // insert as a new pattern at the end of an entire pattern
        memcpy(dp,cstrp,cstr_size);
        dp += cstr_size;
    }else{
        // insert as an independent pattern in the middle of a pattern.
        if(type == 1){
            // buf_rle_value(sp,dp,front_gap,false);
            memcpy(dp,cstrp,cstr_size);
            dp += cstr_size;
            buf_rle_value(sp,dp,back_gap,false);
            sp++;
        }else{
            // buf_bit_packed_value(dp,sp,front_gap);
            memcpy(dp,cstrp,cstr_size);
            dp += cstr_size;
            buf_bit_packed_value(dp,sp,back_gap);
        }            
    }
    
    /**
     * [todo] reorganize retained values
     */
    

    /**
     * copy rest values
     */
    if(srcend != sp){
        memcpy(dp,sp,srcend - sp);
        dp += srcend - sp;
    }
    *dp = '\0';
    SET_VARSIZE(result,dp -  dstart + VARHDRSZ);
    return result;
}

int32
hocotext_rle_hoco_char_length(struct varlena *source,Oid collid){
    unsigned char * sp = VARDATA_ANY(source);
    int32 rawsize = buf_get_int(sp) & 0x3fffffff;

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

	return rawsize;
}