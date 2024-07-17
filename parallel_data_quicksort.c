// OpenMP header
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#define NUM_THREADS 3

struct Group{
    int pivot;
    int range;
    int n_members;
    int count;
    int release;
    int min_index;
    int max_index;
    int* prefix_sum_min;
    int* prefix_sum_max;
    int* vetor_aux;
    omp_lock_t lock;
};

int get_pivot(int low, int range){
    srand(time(0));

    // Gera um nÃºmero aleatÃ³rio dentro do intervalo fornecido
    return (rand() % (range)) + low;
}

void omp_barrier(omp_lock_t* lock, int* counter,int* realease, int num_threads) {
    
    while ((*realease) > 0){
        #pragma omp flush(realease)
    };

    omp_set_lock(lock); 
    (*counter)++; 
    omp_unset_lock(lock); 
    if((*counter) == num_threads){
        (*realease)++;
        // printf("%d %d \n",(*counter),(*realease));
    }

    while ((*realease) < 1){
        #pragma omp flush(realease)
    }; 
    
    omp_set_lock(lock); 
    (*counter)--;
    omp_unset_lock(lock);

    if((*counter) == 0){
        (*realease) = 0;
    }

}

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

void printf_array(int data[],int n){
    for(int i=0;i<n;i++){
        printf("%d ", data[i]);
    }
    printf("\n");
}

int partition(int data[],int pivot, int low, int high) {
    int i = low;
    int data_change = 0;

    for(int j=low;j<=high;j++){
        if(data[j] <= pivot){
            swap(&data[i],&data[j]);
            i = i+1;
            data_change++;
        }   
    }

    return data_change;
}

void quicksort(int arr[],int l,int h){
    if (l < h){
        int x = arr[l];
        int s = l;
        for(int i=s+1; i<=h;i++){
            if (arr[i] < x){
                s = s+1;
                swap(&arr[i],&arr[s]);
            }
        }
        swap(&arr[l],&arr[s]);
        quicksort(arr,l,s-1);
        quicksort(arr,s+1,h);
    }
}

void prefix_sum(int team_id,int member_id,struct Group* groups){
    int offset = 1;
    int rest = 0;

    if((groups[team_id].n_members & 1) != 0){
        rest = 1;
    }

    for(int d=(groups[team_id].n_members)/2; d>0; d>>=1){
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        // #pragma omp barrier  
        if(member_id < d){
            int ai = offset*(2*member_id+1)-1;
            int bi = offset*(2*member_id+2)-1;
            groups[team_id].prefix_sum_min[bi] += groups[team_id].prefix_sum_min[ai];
            groups[team_id].prefix_sum_max[bi] += groups[team_id].prefix_sum_max[ai];
        }
        offset *=2;
    }

    for(int d=2; d<(groups[team_id].n_members-rest);d*=2){
        offset >>= 1;
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        // #pragma omp barrier
        if(member_id < d-1){
            int ai = offset*(member_id+1)-1;
            int bi = offset*(member_id+1)+(offset/2)-1;
            groups[team_id].prefix_sum_min[bi] += groups[team_id].prefix_sum_min[ai];
            groups[team_id].prefix_sum_max[bi] += groups[team_id].prefix_sum_max[ai];
        }
    }
    // #pragma omp barrier
    omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
    if((rest > 0) && member_id +1 == groups[team_id].n_members){
        groups[team_id].prefix_sum_min[member_id] += groups[team_id].prefix_sum_min[member_id-1];
        groups[team_id].prefix_sum_max[member_id] += groups[team_id].prefix_sum_max[member_id-1];
    }
}

void sort_aux_vector(int data[],int team_id, int member_id,int low,int high,struct Group* groups){
    int aux = 0;
    int start_min = 0;
    int start_max = 0;
    if(member_id != 0){
        start_min = groups[team_id].prefix_sum_min[member_id-1];
        // printf("{%d %d}",start_min+low,team_id);
        start_max = groups[team_id].prefix_sum_max[member_id-1];
    }
    for(int i=start_min;i<groups[team_id].prefix_sum_min[member_id];i++){
        groups[team_id].vetor_aux[i] = data[low+aux];
        aux++;
    }
    aux = 0;
    for(int i=groups[team_id].prefix_sum_max[member_id-1];i<groups[team_id].prefix_sum_max[member_id];i++){
        groups[team_id].vetor_aux[groups[team_id].range-i-1] = data[high+aux];
        aux--;
    }
}

void parallel(int data[],int low,int high,int team_id,int member_id,struct Group* groups, int off){
    if(groups[team_id].n_members == 1){
        quicksort(data,low,high);
        return;
    }
    else{
        int data_change = 0;
        int n = groups[team_id].n_members;
        int range = groups[team_id].range;
        int n_members = 0;
        int index_offset = 0; 
        int division;
        int index;
        int previos_id=0;

        if(member_id == 0){
            groups[team_id].pivot = 3 + off;
            // groups[team_id].pivot = get_pivot(low, groups[team_id].range);
            printf("Pivot: %d %d\n", groups[team_id].pivot,groups[team_id].range);
            groups[team_id].vetor_aux = (int *)malloc((groups[team_id].range) * sizeof(int));
        }
        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        
        data_change = partition(data,groups[team_id].pivot,low,high);
        
        groups[team_id].prefix_sum_min[member_id] = data_change;
        groups[team_id].prefix_sum_max[member_id] = (high-low+1) - data_change;
        

        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);

        prefix_sum(team_id,member_id,groups);
        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        sort_aux_vector(data,team_id,member_id,low,high,groups);
        int aux = floor((groups[team_id].range*member_id)/groups[team_id].n_members);
        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        for(int i=low;i<=high;i++){
            data[i] = groups[team_id].vetor_aux[aux];
            aux++;
        }
        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        if(member_id == 0){   
            free(groups[team_id].vetor_aux);
        }
        division = ceil((groups[team_id].prefix_sum_min[groups[team_id].n_members-1]*groups[team_id].n_members)/(float)groups[team_id].range);
        // printf("Division: %d %d %d %d\n", division,groups[team_id].prefix_sum_min[groups[team_id].n_members-1],groups[team_id].n_members, groups[team_id].range);
        previos_id = team_id;
        team_id *=2; 

        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        printf("A\n");
        if(division < member_id+1){
            team_id++;
            member_id = member_id - division;
            n_members =  n - division;
            range = groups[previos_id].prefix_sum_max[n-1] ;
            index_offset = groups[previos_id].prefix_sum_min[n-1];
            if(member_id == 0){
                groups[team_id].n_members = n_members;
                groups[team_id].range = range ;
            }
        }
        else{
            n_members = division;
            range = groups[previos_id].prefix_sum_min[n-1];
            if(member_id == 0){
                groups[team_id].range = range;
                groups[team_id].n_members = n_members;
            }
        }

        low = floor((range*member_id)/n_members) + index_offset;
        high = floor(range*(member_id+1)/n_members)-1+ index_offset; 
        // #pragma omp barrier
        // omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        if(member_id == 0){
            printf("{%d %d %d %d}\n",team_id,n_members,range,division);
        }
        // #pragma omp barrier
        omp_barrier(&groups[team_id].lock,&groups[team_id].count,&groups[team_id].release,groups[team_id].n_members);
        parallel(data,low,high,team_id,member_id,groups,8);

    }
}

int main(int argc, char* argv[])
{
    int data[] = {7,13,18,2,17,1,14,20,6,10,15,9,3,16,19,4,11,12,5,8};
    int low;
    int high;
    int index;
    int team_id;
    int member_id;
    struct Group groups[NUM_THREADS];
    int nthreads = NUM_THREADS;
    int n = (sizeof(data) / sizeof(data[0]));
    groups[0].n_members = NUM_THREADS;
    groups[0].range = n;
    
    // printf_array(data,n);
    #pragma omp parallel shared(data,groups) private(team_id,member_id,low,high) num_threads(NUM_THREADS)
    {
        team_id = 0;
        member_id = omp_get_thread_num();
        omp_init_lock(&(groups[member_id].lock));
        groups[member_id].count = 0;
        groups[member_id].release = 0;
        groups[member_id].prefix_sum_min = (int *)malloc((groups[team_id].n_members) * sizeof(int));
        groups[member_id].prefix_sum_max = (int *)malloc((groups[team_id].n_members) * sizeof(int));
        low = floor((n*member_id)/nthreads);
        high = floor(n*(member_id+1)/nthreads)-1; 

        parallel(data,low,high,team_id,member_id,groups, 0);
        int tid = omp_get_thread_num();
        free(groups[tid].prefix_sum_min);
        free(groups[tid].prefix_sum_max);
        omp_destroy_lock(&(groups[tid].lock));
    }
    printf("\n");
    printf_array(data,n);
    
}

//7 2 1 6 10 18 14 20 13 17 9 3 4 5 8 15 11 12 16 19