#include <iostream>
#include <vector>
#include <math.h>
#include <time.h>
#include "mpi.h"

using namespace std;

int hyper_partition(vector <int> &arr,  int low,  int high,  int pivot);
int partition (vector<int> &array,  int low,  int high);
void quicksort(vector<int> &array,  int low,  int high);
void swap(int* p1, int* p2);

int* binary(int x, int dim){
    
    int* bin = (int*)malloc(dim*sizeof(int));
    for(int i=0; i<dim; i++)
        bin[i]=0;
    
    int i = dim-1;                                  // start with least signficant position
    while (x > 0){
        bin[i] = x % 2;
        x /= 2;
        i--;                                        // go to next most significant position
    }
    
    return bin;
}

// check which processes generate pivot
bool check_id(int* bin_process_id, int step, int dim){
    for (int i=dim-1; i>=step; i--)
        if (bin_process_id[i] == 1)
            return false;
    return true;
}


int main(int argc, char** argv) {
    int resto = 0, pair_process, color, num_process, process_id, sub_process_id, sub_num_process, d;
    int pivot, receive_array_size, send_array_size, n, local_n, position;
    double inicio;

    vector<int> local_data;
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo\n");
        MPI_Finalize();
        return 1;
    }
    fscanf(file, "%d", &n);
    int *data = new int[n];
    for (int i = 0; i < n; i++) {
        fscanf(file, "%d", &data[i]);
    }
    fclose(file);
    
    MPI_Init(NULL, NULL);
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    
    
    inicio = MPI_Wtime();

    // Assume d is initialized after knowing num_process
    d = round(log(num_process) / log(2));
    int* bin_process_id = binary(process_id, d);

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    local_n = ((n % num_process != 0) && (process_id < n % num_process)) ? (n / num_process) + 1 : n / num_process;
    int displs[num_process];
    int sends_local_n[num_process];

    if (process_id == 0) {
        displs[0] = 0;
        for (int i = 0; i < num_process; i++) {
            if ((n % num_process) <= i) {
                resto = 0;
            } else {
                resto = 1;
            }
            sends_local_n[i] = n / num_process + resto;
            if (i > 0) {
                displs[i] = displs[i - 1] + sends_local_n[i - 1];
            }
        }
    }

    int *temp_data = new int[local_n];
    MPI_Scatterv(data, sends_local_n, displs, MPI_INT, temp_data, local_n, MPI_INT, 0, MPI_COMM_WORLD);
    
    for (int i = 0; i < local_n; i++) {
        local_data.push_back(temp_data[i]);
    }
    int k = d-1;
    for (int j = 0; j < d; j++) {

        color = process_id/(num_process/(j+1));     
        pair_process = process_id ^ (1 << k);
        k--;
        MPI_Comm SUB_HYPERCUBE;
        
        MPI_Comm_split(MPI_COMM_WORLD, color, process_id, &SUB_HYPERCUBE);
        MPI_Comm_rank(SUB_HYPERCUBE, &sub_process_id);
        MPI_Comm_size(SUB_HYPERCUBE, &sub_num_process);
        
        if(check_id(bin_process_id, j, d))
            pivot = local_data[local_n-1];

        MPI_Bcast(&pivot, 1, MPI_INT, 0, SUB_HYPERCUBE);
        MPI_Comm_free(&SUB_HYPERCUBE);

        vector<int> send_array;                     
        vector<int> keep_array;                      

        position = hyper_partition(local_data, 0, local_n-1, pivot);    
        // printf("%d %d %d\n", process_id, position, pivot);
        if(pair_process < process_id){          
            for(int i=0; i<=position; i++)
                if(local_data.size() > 0){
                    send_array.push_back(local_data[i]);   
                }                  
            for(int i=position+1; i<local_n; i++)        
                keep_array.push_back(local_data[i]);   
        }
        else{                                               
            for(int i=position+1; i<local_n; i++)        
                if(local_data.size() > 0){
                    send_array.push_back(local_data[i]);   
                }   
            for(int i=0; i<=position; i++)                  
                keep_array.push_back(local_data[i]);   
        }
        
        // for( int i=0; i < send_array.size(); i++){
        //     printf("{%d , %d, %ld}\n", send_array[i],process_id,send_array.size());
        // }

        // for( int i=0; i < keep_array.size(); i++){
        //     printf("[%d , %d, %ld]\n", keep_array[i],process_id,keep_array.size());
        // }

        send_array_size = send_array.size();                    
        // std::cout << pair_process << " " << process_id << std::endl;
        
        if(pair_process < process_id){
            MPI_Send(&send_array_size, 1, MPI_INT, pair_process, 0, MPI_COMM_WORLD);                        
            MPI_Recv(&receive_array_size, 1, MPI_INT, pair_process, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
        }
        else{
            MPI_Recv(&receive_array_size, 1, MPI_INT, pair_process, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  
            MPI_Send(&send_array_size, 1, MPI_INT, pair_process, 0, MPI_COMM_WORLD);                        
        }
        // printf("{%d , %d, %d}\n", receive_array_size,send_array_size,process_id);
        
        int *receive_array = new int[receive_array_size];                  
        int *new_send_array = new int[send_array_size];                    
        
        for(int i=0; i<send_array_size; i++)
            new_send_array[i] = send_array[i];
        
        if(pair_process < process_id){
            MPI_Send(new_send_array, send_array_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD);
            MPI_Recv(receive_array, receive_array_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else{
            MPI_Recv(receive_array, receive_array_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(new_send_array, send_array_size, MPI_INT, pair_process, 0, MPI_COMM_WORLD);
        }

        // for(int i=0; i<keep_array.size(); i++)    
        //     printf("%d\n",keep_array[i]);
        // for(int i=0; i<receive_array_size; i++)
        //     printf("%d\n",receive_array[i]);

        // if(process_id == 3)
        //     printf("aqui\n");
        // printf("%ld %d\n",local_data.size(),process_id);

        if(local_data.size() > 0)
            local_data.clear();
        int x = 0;
        
        for(int i=0; i<keep_array.size(); i++)  {            
            local_data.push_back(keep_array[i]);
            x++;
        }
        
        for(int i=0; i<receive_array_size; i++){
            local_data.push_back(receive_array[i]);
            // printf("{%d, %d %d}\n",local_data[x],receive_array[i],process_id);
            x++;
        }

        local_n = local_data.size();                
        
    }

    quicksort(local_data, 0, local_n-1);
    int* sorted_chunk_array = new int[local_n];
    int* sorted_array = new int[n];;
    int count[num_process];
    int displacement[num_process];
    
    for(int i=0; i<local_n; i++)
        sorted_chunk_array[i] = local_data[i];         

    if(process_id==0){
        count[0] = local_n;
        displacement[0] = 0;

        for(int i=1; i<num_process; i++)
            MPI_Recv(&count[i], 1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);   
        
        int temp = 0;
        for(int i=1; i<num_process; i++){
            temp += count[i-1];
            displacement[i] = temp;                 
        }
    }
    else
        MPI_Send(&local_n, 1, MPI_INT, 0, process_id, MPI_COMM_WORLD);               

    MPI_Gatherv(sorted_chunk_array, local_n, MPI_INT, sorted_array, count, displacement, MPI_INT, 0, MPI_COMM_WORLD);
    double fim = MPI_Wtime();
    
    // for (int i = 0; i < local_data.size(); i++) {
    //     std::cout << local_data[i] << " " << process_id << std::endl;
    // }
    int test = 1;
    if(process_id == 0){
        printf("Tempo: %f\n", fim-inicio);
        for(int i=0;i < n-1;i++){
            // printf("%d ",sorted_array[i]);
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

int hyper_partition(vector<int> &array,  int low,  int high,  int pivot){
    
     int i = low - 1;	
    for( int j=low; j<high; j++){
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

int partition (vector<int> &array,  int low,  int high){
	
     int pivot = array[high];
	 int i = (low-1);
	for ( int j=low; j<=high-1; j++){
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

void quicksort(vector<int> &array,  int low,  int high){
	if (low<high){
		 int pi = partition(array, low, high);
		quicksort(array, low, pi-1);
		quicksort(array, pi+1, high);
	}
}

void swap(int* p1, int* p2)
{
    int temp;
    temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}
