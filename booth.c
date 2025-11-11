#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

int from_signed_5bit(const char *b) {
    int n = (int)strtol(b, NULL, 2);
    if (b[0] == '1')
        n -= (1 << 5);
    return n;
}

int booth_multiply(char *m,char *q) {
    int M = from_signed_5bit(m);
    int Q = from_signed_5bit(q);
    int A = 0, Q_1 = 0;
    int count = 5;

    for (int i = 0; i < count; i++) {
        int Q0 = Q & 1;
        if (Q0 == 1 && Q_1 == 0){
            A -= M;
        }
        else if (Q0 == 0 && Q_1 == 1){
            A += M;
        }

        int signA = 0;
        if(A<0){
            signA = 1;
        }
        else{
            signA = 0;
        }

        int combined = (A << 6) | ((Q << 1) | Q_1);
        combined &= 0x1FFFF;
        combined = (combined >> 1) | (signA << 16);

        A = (combined >> 6) & 0x3F;//
        if (A & 0x20) A -= 0x40;//
        Q = (combined >> 1) & 0x1F;//
        Q_1 = combined & 1;//
    }
    return (A << 5) | Q;
}

int load_dataset(const char *filename, char **m, char **q) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening %s\n", filename);
        return 0;
    }

    char line[32];
    int n = 0;
    fgets(line, sizeof(line), file); 
    while (fgets(line, sizeof(line), file)) {
        char *a = strtok(line, ",");
        char *b = strtok(NULL, ",");
        if (a && b) {
            strcpy(m[n], a);
            b[strcspn(b, "\r\n")] = 0;
            strcpy(q[n], b);
            n++;
        }
    }
    fclose(file);
    return n;
}

int main() {
    const char *filename = "dataset_50k.csv";
    int capacity =50000;

    int num_threads;
    int max_threads = omp_get_num_procs();
    printf("Enter number of threads (1–%d): ", max_threads);
    scanf("%d", &num_threads);
    if (num_threads > max_threads) {
        printf("Requested %d threads, but only %d cores available. Limiting to %d.\n",
               num_threads, max_threads, max_threads);
        num_threads = max_threads;
    }
    omp_set_num_threads(num_threads);

    char **m = malloc(capacity * sizeof(char*));
    char **q = malloc(capacity * sizeof(char*));
    for (int i = 0; i < capacity; i++) {
        m[i] = malloc(8);
        q[i] = malloc(8);
    }
    int *res_seq = malloc(capacity * sizeof(int));
    int *res_par = malloc(capacity * sizeof(int));

    int n = load_dataset(filename, m, q);
    if (n == 0) {
        printf("\nDataset not loaded.\n");
        return 1;
    }

    clock_t start_seq = clock();
    for (int i = 0; i < n; i++)
    res_seq[i] = booth_multiply(m[i], q[i]);
    clock_t end_seq = clock();
    double seq_time = ((double)(end_seq - start_seq)) / CLOCKS_PER_SEC;

    clock_t start_par = clock();
    #pragma omp parallel for schedule(static, 500)
    for (int i = 0; i < n; i++)
    res_par[i] = booth_multiply(m[i], q[i]);
    clock_t end_par = clock();
    double par_time = ((double)(end_par - start_par)) / CLOCKS_PER_SEC;

    int match = 1;
    for (int i = 0; i < n; i++) {
        if (res_seq[i] != res_par[i]) {
            match = 0;
            break;
        }
    }

    printf("\nBooth's Algorithm Parallel Performance\n");
    printf("Threads used       : %d\n", omp_get_max_threads());
    printf("Dataset size       : %d pairs\n", n);
    printf("Sequential time    : %.5f seconds\n", seq_time);
    printf("Parallel time      : %.5f seconds\n", par_time);
    printf("Results identical  : %s\n", match ? "Yes" : "No");
    printf("Speedup achieved   : %.2fx faster\n", (seq_time) / (par_time));

    for (int i = 0; i < capacity; i++) {
        free(m[i]);
        free(q[i]);
    }
    free(m); free(q); free(res_seq); free(res_par);

    return 0;
}
