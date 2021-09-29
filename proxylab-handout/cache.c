#include <stdio.h>
#include "csapp.h"
#include "cache.h"

static struct object *cache[MAX_OBJECT_NUMBER];
static int readcnt = 0;
sem_t mutex, w;


void cache_init() {
    for (int i = 0; i < MAX_OBJECT_NUMBER; ++i) {
        cache[i] = (struct object *)malloc(sizeof(struct object));
        cache[i]->eff = 0;
        cache[i]->size = 0;
    }
}

int read_cache(char *url, int connfd) {
    P(&mutex);
    readcnt++;
    if (readcnt == 1)
        P(&w);
    V(&mutex);
    for (int i = 0; i < MAX_OBJECT_NUMBER ; ++i) {
        if (cache[i]->eff && !strcmp(cache[i]->url, url)) {
            Rio_writen(connfd, cache[i]->content, cache[i]->size);
            P(&mutex);
            readcnt--;
            if (readcnt == 0)
                V(&w);
            V(&mutex);
            return 1;
        }
    }
    P(&mutex);
    readcnt--;
    if (readcnt == 0)
        V(&w);
    V(&mutex);
    return 0;
}

void write_cache(char *url, char *buf, int size) {
    int num;
    P(&w);
    for (num = 0; num < MAX_OBJECT_NUMBER; ++num) {
        if (!cache[num]->eff)
            break;
    }
    if (num != MAX_OBJECT_NUMBER) {
        struct object *tmp = cache[num];
        for (int i = num; i > 0; --i) {
            cache[i] = cache[i-1];
        }
        cache[0] = tmp;
        cache[0]->eff = 1;
        strcpy(cache[0]->url, url);
        cache[0]->size = size;
        memcpy(cache[0]->content, buf, size);
    } else {
        struct object *tmp = cache[MAX_OBJECT_NUMBER-1];
        for (int i = MAX_OBJECT_NUMBER-1; i > 0; --i) {
            cache[i] = cache[i-1];
        }
        cache[0] = tmp;
        strcpy(cache[0]->url, url);
        cache[0]->size = size;
        memcpy(cache[0]->content, buf, size);
    }
    V(&w);
}
