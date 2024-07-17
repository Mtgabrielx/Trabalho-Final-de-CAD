#include <iostream>
#include <vector>
#include <math.h>
#include <time.h>
#include "mpi.h"

using namespace std;

int* generate_shuffled_vector(int n);
void swap(int* p1, int* p2);
int hyper_partition(vector <int> &arr, int low, int high, int pivot);


int partition (vector<int> &array, int low, int high){
	
    int pivot = array[high];
	int i = (low-1);
	for (int j=low; j<=high-1; j++){
		if (array[j] <= pivot){
			i++;
			int temp = array[i];
			array[i] = array[j];
			array[j] = temp;
		}
	}

    int temp = array[i+1];
    array[i+1] = array[high];
    array[high] = temp;

	return (i + 1);
}


void quicksort(vector<int> &array, int low, int high){
	if (low<high){
		int pi = partition(array, low, high);
		quicksort(array, low, pi-1);
		quicksort(array, pi+1, high);
	}
}

int main(int argc, char** argv){
    int resto = 0, sub_world_rank, sub_world_size,pivot,pair_process, receive_array_size,send_array_size, color, n;
    int world_size, world_rank, local_n, position, d;
    int *recv_data, *offsets, *sends_local_n;
    vector <int> local_data;

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo\n");
        return 1;
    }

    fscanf(file, "%d", &n);
    int *temp_data = (int *)malloc(sizeof(int)*n), *sorted_array = (int *)malloc(sizeof(int)*n);
    int *data = (int *)malloc(n * sizeof(int));
    if (data == NULL) {
        fprintf(stderr, "Erro ao alocar memória\n");
        fclose(file);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        fscanf(file, "%d", &data[i]);
    }
    MPI_Init(NULL,NULL);
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD,&world_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&world_rank);
    
    double inicio = MPI_Wtime();

    d = round(log(world_size)/log(2));

    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
    
    local_n = ((n%world_size != 0) && (world_rank<n%world_size)) ? n/world_size+1 : n/world_size;
    offsets = (int *)malloc(sizeof(int) * world_size);
    sends_local_n = (int *)malloc(sizeof(int) * world_size);

    if(world_rank == 0){
        offsets[0] = 0;
        for(int i=1;i < world_size; i++){
            if((n%world_size) < i){
                resto = 0;
            }
            else{
                resto = 1;
            }
            offsets[i] = n/world_size+offsets[i-1] + resto;
            sends_local_n[i-1] = n/world_size + resto;
        }
        sends_local_n[world_size-1] = n/world_size;
    }

    MPI_Scatterv(data,sends_local_n,offsets,MPI_INT,temp_data,local_n, MPI_INT,0,MPI_COMM_WORLD);
    
    for(int i=0;i<local_n;i ++){
        local_data.push_back(temp_data[i]);
    }
    
    for(int i=0; i<d; i++){

        color = world_rank/(world_size/(i+1));     
        
        MPI_Comm SUB_HYPERCUBE;
        
        MPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &SUB_HYPERCUBE);
        MPI_Comm_rank(SUB_HYPERCUBE, &sub_world_rank);
        MPI_Comm_size(SUB_HYPERCUBE, &sub_world_size);

        if(sub_world_rank == 0)
            pivot = local_data[local_n-1];      

        MPI_Bcast(&pivot, 1, MPI_INT, 0, SUB_HYPERCUBE);

        vector<int> send_array;                     
        vector<int> keep_array;                     

        position = hyper_partition(local_data, 0, local_n-1, pivot);    

        if(sub_world_rank/(sub_world_size/2)==1){          
            for(int i=0; i<=position; i++)                  
                send_array.push_back(local_data[i]);   
            for(int i=position+1; i<local_n; i++)        
                keep_array.push_back(local_data[i]);   
        }
        else{                                               
            for(int i=position+1; i<local_n; i++)        
                send_array.push_back(local_data[i]);   
            for(int i=0; i<=position; i++)                  
                keep_array.push_back(local_data[i]);   
        }

        if(sub_world_rank/(sub_world_size/2)==0)
            pair_process = sub_world_rank+sub_world_size/2; 
        else
            pair_process = sub_world_rank-sub_world_size/2; 

        send_array_size = send_array.size();                    
        
        MPI_Send(&send_array_size, 1, MPI_INT, pair_process, d, SUB_HYPERCUBE);                        

        MPI_Recv(&receive_array_size, 1, MPI_INT, pair_process, d, SUB_HYPERCUBE, MPI_STATUS_IGNORE);  

        int *receive_array = (int *)malloc(sizeof(int)*receive_array_size);                  
        int *new_send_array = (int *)malloc(sizeof(int)*send_array_size);                    
        for(int i=0; i<send_array_size; i++)
            new_send_array[i] = send_array[i];

        MPI_Send(&new_send_array, send_array_size, MPI_INT, pair_process, d, SUB_HYPERCUBE);                           

        MPI_Recv(receive_array, receive_array_size, MPI_INT, pair_process, d, SUB_HYPERCUBE, MPI_STATUS_IGNORE);       

        local_data.clear();                            

        for(int i=0; i<keep_array.size(); i++)              
            local_data.push_back(keep_array[i]);
        for(int i=0; i<receive_array_size; i++)             
            local_data.push_back(receive_array[i]);
        
        local_n = local_data.size();                
        MPI_Comm_free(&SUB_HYPERCUBE);
    }

    quicksort(local_data, 0, local_n-1);

    int *sorted_chunk_array = (int *)malloc(sizeof(int)*local_n);                     
    int *count = (int *)malloc(sizeof(int)*world_size);                                 
    int *displacement = (int *)malloc(sizeof(int)*world_size);                        
    
    for(int i=0; i<local_n; i++)
        sorted_chunk_array[i] = local_data[i];         

    if(world_rank==0){
        count[0] = local_n;
        displacement[0] = 0;

        for(int i=1; i<world_size; i++)
            MPI_Recv(&count[i], 1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);   
        
        int temp = 0;
        for(int i=1; i<world_size; i++){
            temp += count[i-1];
            displacement[i] = temp;                 
        }
    }
    else
        MPI_Send(&local_n, 1, MPI_INT, 0, world_rank, MPI_COMM_WORLD);               

    MPI_Gatherv(sorted_chunk_array, local_n, MPI_INT, sorted_array, count, displacement, MPI_INT, 0, MPI_COMM_WORLD);
    double fim = MPI_Wtime();
    MPI_Finalize();
    
    if(world_rank == 0){
        int test = 1;
        for(int i=0;i < n-1;i++){
            if(sorted_array[i] > sorted_array[i+1]){
                test = 0;
            }
        }
        if(test == 0){
            printf("NÃo ordenado");
        }   
        else{
            printf("Ordenado");
        }
        printf("Tempo: %f", fim-inicio);
    }
}

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

void swap(int* p1, int* p2)
{
    int temp;
    temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

int hyper_partition(vector<int> &array, int low, int high, int pivot){
    
    int i = low - 1;	
    for(int j=low; j<high; j++){
		if(array[j]<=pivot){
			i++;
			int temp = array[i];
			array[i] = array[j];
			array[j] = temp;
		}
	}

	int temp = array[i + 1];
	array[i+1] = array[high];
	array[high] = temp;
	
    return (array[i+1] > pivot) ? i : (i+1);
}