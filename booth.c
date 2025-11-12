#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

int convert_to_int(const char *k) {
    int n = (int)strtol(k, NULL, 2);
    if (k[0] == '1')
        n -= (1 << 5);
    return n;
}

int booth_algorithm(char *m, char *q) {
    int M = convert_to_int(m);
    int Q = convert_to_int(q);
    int A = 0;
    int Q_1 = 0;

    for (int i = 0; i < 5; i++) {
        int Q0 = Q & 1;  
        
        if (Q0 == 1 && Q_1 == 0) {
            A -= M;      
        }
        else if (Q0 == 0 && Q_1 == 1) {
            A += M;      
        }

        int A_LSB = A & 1;    
        int Q_LSB = Q & 1;

        A = A >> 1;
        if (A < 0 && (A & (1 << 4)) == 0) {
            A = A | (1 << 5); 
        }
        
        Q = Q >> 1;
        Q = Q | (A_LSB << 4);
        
        Q_1 = Q_LSB;
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

    int threads;
    printf("Enter number of threads: ");
    scanf("%d", &threads);
    omp_set_num_threads(threads);

    char **m = malloc(capacity * sizeof(char*));
    char **q = malloc(capacity * sizeof(char*));
    for (int i = 0; i < capacity; i++) {
        m[i] = malloc(8);
        q[i] = malloc(8);
    }
    int *result_seq = malloc(capacity * sizeof(int));
    int *result_par = malloc(capacity * sizeof(int));

    int n = load_dataset(filename, m, q);
    if (n == 0) {
        printf("\nDataset not loaded.\n");
        return 1;
    }

    clock_t start_seq = clock();  
    for (int i = 0; i < n; i++){
        result_seq[i] = booth_algorithm(m[i], q[i]);
    }
    clock_t end_seq = clock();
    double seq_time = ((double)(end_seq - start_seq)) / CLOCKS_PER_SEC;

    clock_t start_par = clock();
    #pragma omp parallel for schedule(static, 500)
    for (int i = 0; i < n; i++){
        result_par[i] = booth_algorithm(m[i], q[i]);
    }
    clock_t end_par = clock();
    double par_time = ((double)(end_par - start_par)) / CLOCKS_PER_SEC;

    int match = 1;
    for (int i = 0; i < n; i++) {
        if (result_seq[i] != result_par[i]) {
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
    free(m); 
    free(q); 
    free(result_seq); 
    free(result_par);

    return 0;
}
