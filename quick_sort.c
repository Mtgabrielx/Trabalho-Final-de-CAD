#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

// int partition(int array[], int low, int high) {

//   int pivot = array[high];
//   int i = (low - 1);

//   for (int j = low; j < high; j++) {
//     if (array[j] <= pivot){
//       i++;
//       swap(&array[i], &array[j]);
//     }
//   }

//   swap(&array[i + 1], &array[high]);
//   return (i + 1);
// }

// void quickSort(int array[], int low, int high) {
//   if (low < high) {
    
//     int pi = partition(array, low, high);

//     quickSort(array, low, pi - 1);
//     quickSort(array, pi + 1, high);

//   }
// }
void printArray(int array[], int size) {

  for (int i = 0; i < size; ++i) {
    printf("%d  ", array[i]);
  }
  printf("\n");
}

void quickSort(int *data,int low,int high){
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
      quickSort(data,low,j);
      quickSort(data,i,high);
  }
}

void validate_sort(int *data,int size){
  int valido = 1;
  for(int i=0; i < size-1; i++){
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

void validate_sort(int *data, int size);

int main(int argc, char *argv[]) {
  int n;
  clock_t inicio, fim;
  double tempo_gasto;
  FILE *file = fopen(argv[1], "r");
  if (file == NULL) {
      fprintf(stderr, "Erro ao abrir o arquivo\n");
      return 1;
  }

  fscanf(file, "%d", &n);

  int *data = (int *)malloc(n * sizeof(int));
  if (data == NULL) {
      fprintf(stderr, "Erro ao alocar memória\n");
      fclose(file);
      return 1;
  }

  for (int i = 0; i < n; i++) {
      fscanf(file, "%d", &data[i]);
  }

  fclose(file);
  
  // printf("Unsorted Array\n");
  // printArray(data, n);
  printf("Iniciando\n");
  inicio = clock();
  quickSort(data, 0, n - 1);
  fim = clock();
  tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
  printf("Finalizando\n");
  printf("Tempo de execucao: %f segundos\n", tempo_gasto);
  void validate_sort(int *data, int size);
  return 0;
  // printf("Sorted array in ascending order: \n");
  // printArray(data, n);
}