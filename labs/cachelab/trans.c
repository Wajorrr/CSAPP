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
    int i, j, k, l;
    int temp[8];
    if (M == 32 && N == 32)
    {
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {

                if (i != j) // 非对角线子矩阵
                {
                    for (k = i; k < i + 8; k++)     // 子矩阵的8行
                        for (l = j; l < j + 8; l++) // 子矩阵的8列
                            B[l][k] = A[k][l];
                }
                else // i==j，对角线子矩阵
                {
                    for (k = i; k < i + 8; k++) // 子矩阵的8行
                    {
                        for (l = j; l < j + 8; l++) // 子矩阵的8列
                            temp[l - j] = A[k][l];  // 取数组A的第k行暂存
                        for (l = j; l < j + 8; l++)
                        {
                            if (l < j + (k - i)) // 对数组B的第k行前k-i个元素以及它们的转置位置的元素进行赋值
                            {
                                B[k][l] = B[l][k];
                                B[l][k] = temp[l - j];
                            }
                            else
                                B[k][l] = temp[l - j];
                        }
                    }
                }
            }
        }
    }
    else if (M == 64 && N == 64)
    {

        for (j = 0; j < M; j += 8)
        {
            i = j; // i==j，对角线子矩阵

            for (k = i + 4; k < i + 8; k++) // 先把矩阵的后4行转移位置
            {
                // 按行暂存到数组B相邻子矩阵的对应行
                for (l = j; l < j + 8; l++)
                    if (j == 0) // 第一行第一个对角线子矩阵
                        B[k - 4][l + 8] = A[k][l];
                    else // 其他对角线子矩阵
                        B[k - 4][l - 8] = A[k][l];
            }
            for (k = i; k < i + 4; k++) // 矩阵的前4行按行暂存到数组B的对应行
            {
                for (l = j; l < j + 8; l++)
                    temp[l - j] = A[k][l];
                for (l = j; l < j + 8; l++)
                    B[k][l] = temp[l - j];
            }
            for (k = i; k < i + 4; k++) // 对B数组上方的两个子矩阵进行转置，注意这里循环只能循环一半元素，否则每个位置会交换两次
            {
                for (l = j; l < j + k - i; l++) // 对左上角子矩阵进行转置处理
                {
                    temp[0] = B[k][l];
                    B[k][l] = B[l][k];
                    B[l][k] = temp[0];
                }
                for (l = j + 4; l < j + 4 + k - i; l++) // 对右上角子矩阵进行转置处理
                {
                    // k=i，l=j+4 => l-j-4+i，k-i+j+4
                    temp[0] = B[k][l];
                    B[k][l] = B[l - j - 4 + i][k - i + j + 4];
                    B[l - j - 4 + i][k - i + j + 4] = temp[0];
                }
            }
            // 然后套用非对角线子矩阵的逻辑就可以了
            for (l = j; l < j + 4; l++) // 后4行，按列遍历，先遍历前4列
            {
                for (k = i + 4; k < i + 8; k++) // 先把右上角暂存的一行4个值取出来
                    temp[k - i - 4] = B[l][k];
                for (k = i + 4; k < i + 8; k++) // 后4行前4列，给右上角转置位置的对应行赋值
                    if (j == 0)                 // 第一行第一个对角线子矩阵
                        B[l][k] = B[k - 4][l + 8];
                    else // 其他对角线子矩阵
                        B[l][k] = B[k - 4][l - 8];
                for (k = i + 4; k < i + 8; k++) // 再给左下角当前行前4个赋值
                    B[l + 4][k - 4] = temp[k - i - 4];
            }
            for (l = j + 4; l < j + 8; l++)
                for (k = i + 4; k < i + 8; k++) // 后4行后4列
                    if (j == 0)
                        B[l][k] = B[k - 4][l + 8];
                    else
                        B[l][k] = B[k - 4][l - 8];

            for (i = 0; i < N; i += 8)
            {
                if (i != j) // 非对角线子矩阵
                {
                    for (k = i; k < i + 4; k++) // 前4行
                    {
                        for (l = j; l < j + 4; l++) // 前4行前4列，直接转置位置赋值
                            B[l][k] = A[k][l];
                        for (l = j + 4; l < j + 8; l++) // 前4行后4列，暂存在子矩阵右上方
                            B[l - 4][k + 4] = A[k][l];
                    }
                    for (l = j; l < j + 4; l++) // 后4行，按列遍历，先遍历前4列
                    {
                        for (k = i + 4; k < i + 8; k++) // 先把右上角暂存的一行4个值取出来
                            temp[k - i - 4] = B[l][k];
                        for (k = i + 4; k < i + 8; k++) // 后4行前4列，给右上角转置位置的对应行赋值
                            B[l][k] = A[k][l];
                        for (k = i + 4; k < i + 8; k++) // 再给左下角当前行前4个赋值
                            B[l + 4][k - 4] = temp[k - i - 4];
                    }
                    for (l = j + 4; l < j + 8; l++)
                        for (k = i + 4; k < i + 8; k++) // 后4行后4列
                            B[l][k] = A[k][l];
                }
            }
        }
    }
    else
    {
        for (i = 0; i < N; i += 16)
        {
            for (j = 0; j < M; j += 16)
            {
                for (k = i; k < (i + 16 >= N ? N : i + 16); k++)     // 子矩阵的8行
                    for (l = j; l < (j + 16 >= M ? M : j + 16); l++) // 子矩阵的8列
                        B[l][k] = A[k][l];
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */
char transpose_32_plus_32_desc[] = "32 plus 32 matrix";
void transpose_32_plus_32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    int temp[8];
    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {

            if (i != j) // 非对角线子矩阵
            {
                for (k = i; k < i + 8; k++)     // 子矩阵的8行
                    for (l = j; l < j + 8; l++) // 子矩阵的8列
                        B[l][k] = A[k][l];
            }
            else // i==j，对角线子矩阵
            {
                for (k = i; k < i + 8; k++) // 子矩阵的8行
                {
                    for (l = j; l < j + 8; l++) // 子矩阵的8列
                        temp[l - j] = A[k][l];  // 取数组A的第k行暂存
                    for (l = j; l < j + 8; l++)
                    {
                        if (l < j + (k - i)) // 对数组B的第k行前k-i个元素以及它们的转置位置的元素进行赋值
                        {
                            B[k][l] = B[l][k];
                            B[l][k] = temp[l - j];
                        }
                        else
                            B[k][l] = temp[l - j];
                    }
                }
            }
        }
    }
}

char transpose_64_plus_64_desc[] = "64 plus 64 matrix";
void transpose_64_plus_64(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    int temp[8];
    for (j = 0; j < M; j += 8)
    {
        i = j; // i==j，对角线子矩阵

        for (k = i + 4; k < i + 8; k++) // 先把矩阵的后4行转移位置
        {
            // 按行暂存到数组B相邻子矩阵的对应行
            for (l = j; l < j + 8; l++)
                if (j == 0) // 第一行第一个对角线子矩阵
                    B[k - 4][l + 8] = A[k][l];
                else // 其他对角线子矩阵
                    B[k - 4][l - 8] = A[k][l];
        }
        for (k = i; k < i + 4; k++) // 矩阵的前4行按行暂存到数组B的对应行
        {
            for (l = j; l < j + 8; l++)
                temp[l - j] = A[k][l];
            for (l = j; l < j + 8; l++)
                B[k][l] = temp[l - j];
        }
        for (k = i; k < i + 4; k++) // 对B数组上方的两个子矩阵进行转置，注意这里循环只能循环一半元素，否则每个位置会交换两次
        {
            for (l = j; l < j + k - i; l++) // 对左上角子矩阵进行转置处理
            {
                temp[0] = B[k][l];
                B[k][l] = B[l][k];
                B[l][k] = temp[0];
            }
            for (l = j + 4; l < j + 4 + k - i; l++) // 对右上角子矩阵进行转置处理
            {
                // k=i，l=j+4 => l-j-4+i，k-i+j+4
                temp[0] = B[k][l];
                B[k][l] = B[l - j - 4 + i][k - i + j + 4];
                B[l - j - 4 + i][k - i + j + 4] = temp[0];
            }
        }
        // 然后套用非对角线子矩阵的逻辑就可以了
        for (l = j; l < j + 4; l++) // 后4行，按列遍历，先遍历前4列
        {
            for (k = i + 4; k < i + 8; k++) // 先把右上角暂存的一行4个值取出来
                temp[k - i - 4] = B[l][k];
            for (k = i + 4; k < i + 8; k++) // 后4行前4列，给右上角转置位置的对应行赋值
                if (j == 0)                 // 第一行第一个对角线子矩阵
                    B[l][k] = B[k - 4][l + 8];
                else // 其他对角线子矩阵
                    B[l][k] = B[k - 4][l - 8];
            for (k = i + 4; k < i + 8; k++) // 再给左下角当前行前4个赋值
                B[l + 4][k - 4] = temp[k - i - 4];
        }
        for (l = j + 4; l < j + 8; l++)
            for (k = i + 4; k < i + 8; k++) // 后4行后4列
                if (j == 0)
                    B[l][k] = B[k - 4][l + 8];
                else
                    B[l][k] = B[k - 4][l - 8];

        for (i = 0; i < N; i += 8)
        {
            if (i != j) // 非对角线子矩阵
            {
                for (k = i; k < i + 4; k++) // 前4行
                {
                    for (l = j; l < j + 4; l++) // 前4行前4列，直接转置位置赋值
                        B[l][k] = A[k][l];
                    for (l = j + 4; l < j + 8; l++) // 前4行后4列，暂存在子矩阵右上方
                        B[l - 4][k + 4] = A[k][l];
                }
                for (l = j; l < j + 4; l++) // 后4行，按列遍历，先遍历前4列
                {
                    for (k = i + 4; k < i + 8; k++) // 先把右上角暂存的一行4个值取出来
                        temp[k - i - 4] = B[l][k];
                    for (k = i + 4; k < i + 8; k++) // 后4行前4列，给右上角转置位置的对应行赋值
                        B[l][k] = A[k][l];
                    for (k = i + 4; k < i + 8; k++) // 再给左下角当前行前4个赋值
                        B[l + 4][k - 4] = temp[k - i - 4];
                }
                for (l = j + 4; l < j + 8; l++)
                    for (k = i + 4; k < i + 8; k++) // 后4行后4列
                        B[l][k] = A[k][l];
            }
        }
    }
}

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
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
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

    // registerTransFunction(transpose_32_plus_32, transpose_32_plus_32_desc);
    // registerTransFunction(transpose_64_plus_64, transpose_64_plus_64_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
