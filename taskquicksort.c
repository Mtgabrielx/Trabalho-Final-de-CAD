#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void parallel_quicksort(int* data, int low, int high);
int* read_vector_from_file(const char* filename, int* size);
void start_parallel_quicksort(int *data, int size);
void swap(int *a, int *b);
void validate_sort(int *data, int size);

int main(int argc, char* argv[]) {
    int *data, size;
    double start, end;
    int threads = atoi(argv[1]);
    omp_set_num_threads(threads);

    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo\n");
        return 1;
    }

    fscanf(file, "%d", &size);

    data = (int *)malloc(size * sizeof(int));
    if (data == NULL) {
        fprintf(stderr, "Erro ao alocar memória\n");
        fclose(file);
        return 1;
    }

    for (int i = 0; i < size; i++) {
        fscanf(file, "%d", &data[i]);
    }

    fclose(file);

    start = omp_get_wtime();
    start_parallel_quicksort(data,size);
    end = omp_get_wtime();
    
    validate_sort(data,size);
    printf("%d %d\n",threads,size);
    printf("Tempo: %.4f\n", end-start);
    
    free(data);

    return 0;
}

void parallel_quicksort(int *data,int low,int high){
    if(low < high){
        int pivot = data[(high+low)/2];
        int i = low;
        int j = high;

        while(i <= j){
            while(data[i] < pivot) 
                i++;
            while(data[j] > pivot)
                j--;
            if(i<=j){
                swap(&data[i], &data[j]);
                i++;
                j--;
            }
        }
        #pragma omp task firstprivate(data,low,j)
            parallel_quicksort(data,low,j);
        #pragma omp task firstprivate(data,i,high)
            parallel_quicksort(data,i,high);
    }
}

int* read_vector_from_file(const char* filename, int* size) {
    FILE *file;
    int *vector;
    
    file = fopen(filename, "r");
    fscanf(file, "%d", size);
    vector = (int *)malloc(*size * sizeof(int));

    for (int i = 0; i < *size; i++) {
        fscanf(file, "%d", &vector[i]);
    }

    fclose(file);

    return vector;
}

void start_parallel_quicksort(int *data, int size){
    #pragma omp parallel
    {
        #pragma omp single
            parallel_quicksort(data,0,size-1);
    }
}

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

void validate_sort(int *data,int size){
    int valido = 1;
    for(int i=0; i < size-1; i++){
        printf("%d\n",i);
        if(data[i] > data[i+1]){
            valido = 0;
            break;
        }
    }
    if(valido == 0){
        printf("Erro ao ordenar\n");
    }
    else{
        printf("Ordenado\n");
    }
}