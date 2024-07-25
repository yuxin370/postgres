#include <string.h>  /* strcpy */
#include <stdlib.h>  /* malloc */
#include <stdio.h>   /* printf */
#include "uthash.h"

struct my_struct {
    int id;                    /* first key */
    char username[10];         /* second key */
    UT_hash_handle hh1;        /* handle for first hash table */
    UT_hash_handle hh2;        /* handle for second hash table */
};

int main(int argc, char *argv[]) {
    struct my_struct *users_by_id = NULL, *users_by_name = NULL, *s;
    int i;
    char *name;

    s = malloc(sizeof *s);
    s->id = 1;
    strcpy(s->username, "thanson");

    /* add the structure to both hash tables */
    HASH_ADD(hh1, users_by_id, id, sizeof(int), s);
    HASH_ADD(hh2, users_by_name, username, strlen(s->username), s);

    /* find user by username in the "users_by_name" hash table */
    name = "thanson";
    HASH_FIND(hh2, users_by_name, name, strlen(name), tmp);
    if (s) printf("found user %s: %d\n", name, tmp->id);

    /* find user by ID in the "users_by_id" hash table */
    i = 1;
    HASH_FIND(hh1, users_by_id, &i, sizeof(int), tmp);
    if (s) printf("found id %d: %s\n", i, tmp->username);


}
