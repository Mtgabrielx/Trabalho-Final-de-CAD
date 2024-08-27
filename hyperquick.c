#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<mpi.h>

void hyper_partition(int array[],  int low,  int high,  int pivot, int *maiores,int *size_maiores, int *menores,int *size_menores);
int partition (int *array,  int low,  int high);
void quicksort(int *array,  int low,  int high);
void swap(int* p1, int* p2);

int main(int argc, char *argv[]){
    int pair_process, rank, size, resto, dim, count, pivot, position, k=0, j=0;
    int *data, *send;
    double inicio;
    
    MPI_Init(NULL, NULL);
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    inicio = MPI_Wtime();
    dim = round(log(size) / log(2));

    if(rank == 0){
        // FILE *file = fopen(argv[1], "r");
        FILE *file = fopen("dados.txt", "r");
        // printf("%c",argv);
        if (file == NULL) {
            fprintf(stderr, "Erro ao abrir o arquivo\n");
            MPI_Finalize();
            return 1;
        }
        fscanf(file, "%d", &count);

        data = calloc(sizeof(int),count);

        for (int i = 0; i < count; i++) {
            fscanf(file, "%d", &data[i]);
        }
        fclose(file);
    }

    MPI_Bcast(&count, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int local_n = ((count % size != 0) && (rank < count % size)) ? (count / size) + 1 : count / size;
    int displs[size];
    int sends_local_n[size];

    if (rank == 0) {
        displs[0] = 0;
        for (int i = 0; i < size; i++) {
            if ((count % size) <= i) {
                resto = 0;
            } else {
                resto = 1;
            }
            sends_local_n[i] = count / size + resto;
            if (i > 0) {
                displs[i] = displs[i - 1] + sends_local_n[i - 1];
            }
        }
    }

    int *local_data = calloc(sizeof(int),local_n);
    MPI_Scatterv(data, sends_local_n, displs, MPI_INT, local_data, local_n, MPI_INT, 0, MPI_COMM_WORLD);

    for(int l=dim-1; l >= 0; l--){
        int color = rank/(size/(k+1));
        k++;
        pair_process = rank ^ (1 << l);
        
        int sub_rank,sz;
        MPI_Comm new_comm;
        MPI_Comm_split(MPI_COMM_WORLD, color, rank, &new_comm);
        MPI_Comm_rank(new_comm,&sub_rank);
        MPI_Comm_size(new_comm,&sz);
        
        if(sub_rank == 0){
            pivot = local_data[local_n/2];
        }

        MPI_Bcast(&pivot, 1, MPI_INT, 0, new_comm);
        int keep_size = 0, send_size = 0, recv_size=0;
        // printf("%d\n",local_n);
        int *keep_array =  calloc(sizeof(int),local_n);
        int *send_array =  calloc(sizeof(int),local_n);

        if(rank >= pair_process){
            hyper_partition(local_data,0,local_n,pivot,keep_array,&keep_size,send_array,&send_size);
        }
        else{
            hyper_partition(local_data,0,local_n,pivot,send_array,&send_size,keep_array,&keep_size);
        }

        if(rank >= pair_process){ 
            MPI_Send(&send_size,1,MPI_INT,pair_process,1,MPI_COMM_WORLD);    
            MPI_Recv(&recv_size,1,MPI_INT,pair_process,1,MPI_COMM_WORLD,&status);
        }else{
            MPI_Recv(&recv_size,1,MPI_INT,pair_process,1,MPI_COMM_WORLD,&status);
            MPI_Send(&send_size,1,MPI_INT,pair_process,1,MPI_COMM_WORLD);
        }
        int *recv_array = calloc(sizeof(int),recv_size);
        
        if(pair_process < rank){
            MPI_Send(send_array, send_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD);
            MPI_Recv(recv_array, recv_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else{
            MPI_Recv(recv_array, recv_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(send_array, send_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD);
        }        

        local_n = recv_size+keep_size;
        
        if(local_n > 0){
            local_data = realloc(local_data, (keep_size+recv_size) * sizeof(int));
            memcpy(local_data, keep_array, keep_size * sizeof(int));
            memcpy(local_data + keep_size, recv_array, recv_size * sizeof(int));
        }
        else{
            local_data = realloc(local_data, sizeof(int));
            local_data[0] = -1;
        }
        free(keep_array);
        free(send_array);
    }

    quicksort(local_data,0,local_n-1);
    int* sorted_chunk_array = calloc(sizeof(int),local_n);
    int* sorted_array = calloc(sizeof(int),count);
    int send_n[size];
    int displacement[size];
    
    for(int i=0; i<local_n; i++)
        sorted_chunk_array[i] = local_data[i];         
 
    if(rank==0){
        send_n[0] = local_n;
        displacement[0] = 0;

        for(int i=1; i<size; i++)
            MPI_Recv(&send_n[i], 1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);   
        
        int temp = 0;
        for(int i=1; i<size; i++){
            temp += send_n[i-1];
            displacement[i] = temp;                 
        }
    }
    else
        MPI_Send(&local_n, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);               

    MPI_Gatherv(sorted_chunk_array, local_n, MPI_INT, sorted_array, send_n, displacement, MPI_INT, 0, MPI_COMM_WORLD);

    int test = 1;
    if(rank == 0){
        printf("Tempo: %f\n",  MPI_Wtime()-inicio);
        for(int i=0;i < count-1;i++){
            // printf("%d\n",sorted_array[i]);
            if(sorted_array[i] > sorted_array[i+1]){
                test = 0;
            }
        }
        if(test == 0){
            printf("Nao ordenado\n");
        }   
        else{
            printf("Ordenado\n");
        }
    }
    MPI_Finalize();
    return 0;
}

void swap(int* p1, int* p2)
{
    int temp;
    temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

int partition (int *array,  int low,  int high){
	
    int pivot = array[(high+low)/2];
	int i = (low-1);
	for ( int j=low; j<=high-1; j++){
		if (array[j] <= pivot){
			i++;
            swap(&array[i],&array[j]);
		}
	}
    swap(&array[i+1],&array[high]);

	return (i + 1);
}

void quicksort(int *array,  int low,  int high){
	if (low<high){
		int pi = partition(array, low, high);
		quicksort(array, low, pi-1);
		quicksort(array, pi+1, high);
	}
}

void hyper_partition(int array[],  int low,  int high,  int pivot, int *maiores,int *size_maiores, int *menores,int *size_menores){
    int i;
    int temp_size_maiores = 0;
    int temp_size_menores = 0;
    if(array[0] > 0){
        i = low - 1;	
        for( int j=low; j<high; j++){
            if(array[j]<=pivot){
                menores[temp_size_menores] = array[j];
                temp_size_menores++;
                i++;
                int temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }
            else{
                maiores[temp_size_maiores] = array[j];
                temp_size_maiores++;
            }
        }
    }
    (*size_maiores) = temp_size_maiores;
    (*size_menores) = temp_size_menores;
}