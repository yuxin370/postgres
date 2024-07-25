#include <string.h>  /* strcpy */
#include <stdlib.h>  /* malloc */
#include <stdio.h>   /* printf */
#include "uthash.h"

struct my_struct {
    const char *name;          /* key */
    int id;
    UT_hash_handle hh;         /* makes this structure hashable */
};

    struct my_struct *s, *tmp, *users = NULL;

/**
 * @brief print the hash_table
*/
void hash_print(){
    // free the hash table contents 
    struct my_struct *s, *tmp;
    printf("====================\n");
    printf("    key  |     id\n");
    printf("--------------------\n");
    HASH_ITER(hh, users, s, tmp) {
      printf("%8s |%7d\n",s->name,s->id);
    }
    printf("====================\n");

}

int main(int argc, char *argv[]) {
    const char *names[] = { "joe", "bob", "betty", NULL };

    for (int i = 0; names[i]; ++i) {
        s = (struct my_struct *)malloc(sizeof *s);
        s->name = names[i];
        s->id = i;
        HASH_ADD_KEYPTR(hh, users, s->name, strlen(s->name), s);
    }

    HASH_FIND_STR(users, "betty", s);
    if (s) printf("betty's id is %d\n", s->id);
    hash_print();
    /* free the hash table contents */
    HASH_ITER(hh, users, s, tmp) {
      HASH_DEL(users, s);
      free(s);
    }
    return 0;
}