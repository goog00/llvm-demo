#include <stdio.h>

void matrix_mul(int n, int m, int p, float A[n][m], float B[m][p], float C[n][p]) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            C[i][j] = 0;
            for (int k = 0; k < m; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

int main() {
    int n = 3, m = 3, p = 3;
    float A[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    float B[3][3] = {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}};
    float C[3][3];

    matrix_mul(n, m, p, A, B, C);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            printf("%f ", C[i][j]);
        }
        printf("\n");
    }

    return 0;
}
