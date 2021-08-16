/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32) {
    int i, j, k, t0, t1, t2, t3, t4, t5, t6, t7;
    for (i = 0; i < 32; i += 8)
        for (j = 0; j < 32; j += 8) {
	    for (k = i; k < i+8; k++) {
	        t0 = A[k][j];
	        t1 = A[k][j+1];
	        t2 = A[k][j+2];
	        t3 = A[k][j+3];
	        t4 = A[k][j+4];
	        t5 = A[k][j+5];
	        t6 = A[k][j+6];
	        t7 = A[k][j+7];
	        B[j][k] = t0;
	        B[j+1][k] = t1;
	        B[j+2][k] = t2;
	        B[j+3][k] = t3;
	        B[j+4][k] = t4;
	        B[j+5][k] = t5;
	        B[j+6][k] = t6;
	        B[j+7][k] = t7;
	    }
	}
    } else if (M == 64) {
        int v0, v1, v2, v3, v4, v5, v6, v7;
    	for (int i = 0; i < 64; i += 8)
    	    for (int j = 0; j < 64; j += 8) {
    	        for (int k = i; k < i+4; k++) {
    	            v0 = A[k][j];
    	            v1 = A[k][j+1];
    	            v2 = A[k][j+2];
    	            v3 = A[k][j+3];
    	            v4 = A[k][j+4];
    	            v5 = A[k][j+5];
    	            v6 = A[k][j+6];
    	            v7 = A[k][j+7];
    	            B[j][k] = v0;
    	            B[j+1][k] = v1;
    	            B[j+2][k] = v2;
    	            B[j+3][k] = v3;
    	            B[j][k+4] = v4;
    	            B[j+1][k+4] = v5;
    	            B[j+2][k+4] = v6;
    	            B[j+3][k+4] = v7;
    	        }
    	        for (int k = j; k < j+4; k++) {
    	            v0 = A[i+4][k];
    	            v1 = A[i+5][k];
    	            v2 = A[i+6][k];
    	            v3 = A[i+7][k];
    	            v4 = B[k][i+4];
    	            v5 = B[k][i+5];
    	            v6 = B[k][i+6];
    	            v7 = B[k][i+7];
    	            B[k][i+4] = v0;
    	            B[k][i+5] = v1;
    	            B[k][i+6] = v2;
    	            B[k][i+7] = v3;
    	            B[k+4][i] = v4;
    	            B[k+4][i+1] = v5;
    	            B[k+4][i+2] = v6;
    	            B[k+4][i+3] = v7;
    	        }
    	        for (int k = i+4; k < i+8; k++) {
    	            v0 = A[k][j+4];
    	            v1 = A[k][j+5];
    	            v2 = A[k][j+6];
    	            v3 = A[k][j+7];
    	            B[j+4][k] = v0;
    	            B[j+5][k] = v1;
    	            B[j+6][k] = v2;
    	            B[j+7][k] = v3;
    	        }
    	    }
    } else if (M == 61) {
    	for (int ii = 0; ii < N; ii += 16)
    	    for (int jj = 0; jj < M; jj += 16)
    	    	for (int i = ii; i < ii+16 && i < N; i++)
    	    	    for (int j = jj; j < jj+16 && j < M; j++)
    	    	    	B[j][i] = A[i][j];
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

