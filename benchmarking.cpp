#include<math.h>
#include "mpi.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <time.h>

int* generate_shuffled_vector(int n) {
    int *vector = (int *)malloc(n * sizeof(int));
    if (vector == NULL) {
        fprintf(stderr, "Falha na alocaÃ§Ã£o de memÃ³ria\n");
        return NULL;
    }

    for (int i = 0; i < n; i++) {
        vector[i] = i;
    }

    srand(time(NULL));

    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = vector[i];
        vector[i] = vector[j];
        vector[j] = temp;
    }

    return vector;
}


void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

// function to find the partition position
int partition(int array[], int low, int high) {
  
  // select the rightmost element as pivot
  int pivot = array[high];
  
  // pointer for greater element
  int i = (low - 1);

  // traverse each element of the array
  // compare them with the pivot
  for (int j = low; j < high; j++) {
    if (array[j] <= pivot) {
        
      // if element smaller than pivot is found
      // swap it with the greater element pointed by i
      i++;
      
      // swap element at i with element at j
      swap(&array[i], &array[j]);
    }
  }

  // swap the pivot element with the greater element at i
  swap(&array[i + 1], &array[high]);
  
  // return the partition point
  return (i + 1);
}

void DoquickSort(int array[], int low, int high) {
  if (low < high) {
    
    // find the pivot element such that
    // elements smaller than pivot are on left of pivot
    // elements greater than pivot are on right of pivot
    int pi = partition(array, low, high);
    
    // recursive call on the left of pivot
    DoquickSort(array, low, pi - 1);
    
    // recursive call on the right of pivot
    DoquickSort(array, pi + 1, high);
  }
}

void printArray(int array[], int size) {
  for (int i = 0; i < size; ++i) {
    printf("%d  ", array[i]);
  }
  printf("\n");
}

void main(int argc, char** argv){
    int local_n, logp, n, world_rank, median, np, pivot;
    int *data,*local_data, *recv_buf, *merge_buf;
    double start,end;
    char msg[100];
    MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_size (MPI_COMM_WORLD,&np);
    MPI_Comm_rank (MPI_COMM_WORLD,&world_rank);
    
    logp = round(log(np)/log(2));
    
    if(world_rank == 0){
        n = 1000;
        data = generate_shuffled_vector(n);
    }
    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
    local_data = (int*) malloc(sizeof(int)*n);
    recv_buf = (int*) malloc(sizeof(int)*n);
    merge_buf = (int*) malloc(sizeof(int)*n);
    local_n = n/np;
    MPI_Scatter(data,local_n,MPI_INT,local_data,local_n,MPI_INT,0,MPI_COMM_WORLD);
    
    printArray(local_data,local_n);

    if(world_rank == 0){
        start = MPI_Wtime();
        printf("start: %d\n",start);
    }

    DoquickSort(local_data,0,local_n-1);

    for(int iter = 0; iter < logp; iter++){
        //printf("iter : %d\n",iter);
        // create communicator based on no of iteration
        int color = pow(2,iter)*world_rank/np;
        MPI_Comm new_comm;
        MPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &new_comm);
        int rank,sz;
        MPI_Comm_rank(new_comm,&rank);
        MPI_Comm_size(new_comm,&sz);

        // process 0 will broadcast its median
        median = local_data[local_n/2];
        MPI_Bcast(&median, 1, MPI_INT, 0, new_comm);
        //printf("median : %d\n",median);
        // each process in upper half will swap its low list with high list of corresponding lower half
        pivot = 0;
        while(pivot < local_n && local_data[pivot] < median)
            pivot++;

        int pair_process = (rank+(sz>>1))%sz;
        
        if(rank >= (sz>>1)){ // in upper half
            MPI_Send(local_data,pivot,MPI_INT,pair_process,1,new_comm);    
            MPI_Recv(recv_buf,n,MPI_INT,pair_process,1,new_comm,&status);
        }else{
            MPI_Recv(recv_buf,n,MPI_INT,pair_process,1,new_comm,&status);
            MPI_Send(local_data+pivot,local_n - pivot,MPI_INT,pair_process,1,new_comm);
        }

        //printf("swapped iter: %d rank: %d\n",iter,world_rank);
        // merge the two sorted lists
        int recv_count;
            MPI_Get_count(&status,MPI_INT,&recv_count);
        
        int i,j = 0,k = 0, i_end;
        if(rank >= (sz>>1)){
            i = pivot, i_end = local_n;
            local_n = local_n - pivot + recv_count;
        }else{
            i = 0, i_end = pivot;
            local_n = pivot + recv_count;
        }

        while(i < i_end && j < recv_count){
            if(local_data[i] < recv_buf[j])
                merge_buf[k++] = local_data[i++];
            else
                merge_buf[k++] = recv_buf[j++];
        }
        while(i < i_end)
            merge_buf[k++] = local_data[i++];
        while(j < recv_count)
            merge_buf[k++] = recv_buf[j++];
    
        // copy merge buf into local_data
        for(int i = 0; i < local_n; i++)
            local_data[i] = merge_buf[i];
        
        //printf("merged iter: %d rank: %d\n",iter,world_rank);

        MPI_Comm_free(&new_comm);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    if(world_rank == 0){
        end = MPI_Wtime();
        printf("end: %d\n",end);
    }
    // print the output
    if(world_rank != 0)
        MPI_Send(local_data,local_n,MPI_INT,0,1,MPI_COMM_WORLD);

    //printf("sent %d\n",world_rank);

    if(world_rank == 0){
        int recv_count = local_n;
        for(int i = 1; i < np; i++){
            MPI_Recv(local_data + recv_count,n,MPI_INT,i,1,MPI_COMM_WORLD,&status);
            int temp_count;
            MPI_Get_count(&status,MPI_INT,&temp_count);
            recv_count += temp_count;
            //printf("recieved from %d\n",i);
        }
        char* filename = argv[2];
        FILE *file = fopen(filename,"w");
        if (file) {        
            for(int i = 0 ; i < n; i++)
                fprintf(file,"%d\n",local_data[i]);
            for(int i = 1 ; i < n; i++)
                if(local_data[i-1] > local_data[i])
                    printf("ERROR: %d\n",i);
        }
        fclose(file);

        printf("time taken: %lf\n",end-start);
    }

    MPI_Finalize();
}
