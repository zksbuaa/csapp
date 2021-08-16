#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
struct Block {
    int eff;
    long long flag;
};
int main(int argc, char *argv[])
{
    int opt = -1;
    int s, E, b, S = 1;
    int hits = 0, misses = 0, evictions = 0;
    FILE *fp;
    char buf[100];
    char mode[5];
    long long add;
    while (-1 != (opt = getopt(argc, argv, "s:E:b:t:"))) {
        switch (opt)
        {
            case 's':
	        s = atoi(optarg);
	        break;
            case 'E':
	        E = atoi(optarg);
	        break;
            case 'b':
	        b = atoi(optarg);
	        break;
            case 't':
	        fp = fopen(optarg, "r");
	        break;
        }
    }
    for (int i = 0; i < s; i++)
        S *= 2;
    struct Block ***pc = (struct Block***)malloc(S*sizeof(struct Block**));
    for (int i = 0; i < S; i++) {
        pc[i] = (struct Block**)malloc(E*sizeof(struct Block*));
    }
    for (int i = 0; i < S; i++)
        for (int j = 0; j < E; j++) {
            pc[i][j] = (struct Block*)malloc(sizeof(struct Block));
            pc[i][j]->eff = 0;
            pc[i][j]->flag = 0;
        }
    while (fgets(buf, 100, fp)) {
        if (buf[0] == 'I')
            continue;
        sscanf(buf, "%s%llx", mode, &add);
        if (mode[0] == 'M')
            hits++;
        add >>=  b;
        int g = add%S;
        add >>= s;
        int match = 0, num = 0, h = 0;	
        for (int i = 0; i < E; i++) {
	        if (pc[g][i]->eff == 0)
	            break;
            num++;
	        if (pc[g][i]->flag == add) {
	            match = 1;
	            h = i;
	        }
        }
        if (match) {
            hits++;
	        struct Block* tmp = pc[g][h];
	        for (int i = h-1; i >= 0; i--)
	            pc[g][i+1] = pc[g][i];
	    pc[g][0] = tmp;
        } else {
	        if (num != E) {
	            misses++;
	            struct Block* tmp = pc[g][num];
	            for (int i = num-1; i >= 0; i--) {
	                pc[g][i+1] = pc[g][i];
	            }
	            pc[g][0] = tmp;
	            tmp->eff = 1;
	            tmp->flag = add;
	        } else {
	            misses++;
	            evictions++;
	            struct Block* tmp = pc[g][E-1];
	            for (int i = E-2; i >= 0; i--)
	                pc[g][i+1] = pc[g][i];
	            pc[g][0] = tmp;
	            tmp->flag = add;
	        }
        }
    }
    printSummary(hits, misses, evictions);
    return 0;
}
