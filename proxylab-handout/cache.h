#ifndef _CACHE_H_
#define _CACHE_H_

#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_OBJECT_SIZE 102400
#define MAX_OBJECT_NUMBER 10

void cache_init();
int read_cache(char *, int);
void write_cache(char* , char *, int);

struct object{
    int eff;
    char url[MAXLINE];
    int size;
    char content[MAX_OBJECT_SIZE];
};


#endif // _CACHE_H_