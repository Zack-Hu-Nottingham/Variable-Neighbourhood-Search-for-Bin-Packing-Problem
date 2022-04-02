#include <iostream>
#include <vector>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

using namespace std;

struct item_struct{
    int size;
    int index;
    bool isPacked;
};

struct bin_struct {
    vector<item_struct> packed_items;
    int cap_left;
};

struct problem_struct{
    int n; //number of items
    int capacities;  //knapsack capacities
    int known_best;
    struct item_struct* items;
};

struct solution_struct {
    struct problem_struct* prob;
    int objective; // float objective;
    int feasibility;
    vector<bin_struct> bins;
};

/* declare parameters for variable neighbourhood search here*/
int K= 3; // k-opt is used
int SHAKE_STRENGTH = 12;
struct solution_struct best_sln;  //global best solution
int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 1;
int MAX_TIME = 30;  //max amount of time permited (in sec)
int num_of_problems;


int cmpfunc1(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return -1;
    if(item1->size<item2->size) return 1;
    return 0;
}

int cmpfunc2(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return 1;
    if(item1->size<item2->size) return -1;
    return 0;
}

void init_problem(int n, int capacities, int known_best, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = (struct problem_struct*) malloc(sizeof(struct problem_struct));
    new_prob->n=n; new_prob->capacities=capacities; new_prob->known_best=known_best;
    new_prob->items= (struct item_struct*) malloc(sizeof(struct item_struct)*n);
    // for(int j=0; j<n; j++)
        // new_prob->items[j].index = j;
    *my_prob = new_prob;
}

void free_problem(struct problem_struct* prob)
{
    if(prob!=NULL)
    {
        // if(prob->capacities !=NULL) free(prob->capacities);
        if(prob->items!=NULL)
        {
            free(prob->items);
        }
        free(prob);
    }
}

//copy a solution from another solution
bool copy_solution(struct solution_struct* dest_sln, struct solution_struct* source_sln)
{
    // if(source_sln ==NULL) return false;
    // if(dest_sln==NULL)
    // {
    //     dest_sln = malloc(sizeof(struct solution_struct));
    // }
    // else{
    //     free(dest_sln->cap_left);
    //     free(dest_sln->x);
    // }
    // int n = source_sln->prob->n;
    // int m =source_sln->prob->dim;
    // dest_sln->x = malloc(sizeof(int)*n);
    // dest_sln->cap_left=malloc(sizeof(int)*m);
    // for(int i=0; i<m; i++)
    //     dest_sln->cap_left[i]= source_sln->cap_left[i];
    // for(int j=0; j<n; j++)
    //     dest_sln->x[j] = source_sln->x[j];
    // dest_sln->prob= source_sln->prob;
    // dest_sln->feasibility=source_sln->feasibility;
    // dest_sln->objective=source_sln->objective;
    return true;
}

//update global best solution from sln
void update_best_solution(struct solution_struct* sln)
{
    if(best_sln.objective > sln->objective)
    copy_solution(&best_sln, sln);
}

struct problem_struct** load_problems(char* data_file)
{
    int i,j,k;

    // Open data file and read out data
    FILE* pfile = fopen(data_file, "r");
    if(pfile==NULL) {
        printf("Data file %s does not exist. Please check!\n", data_file); exit(2); 
    }

    fscanf (pfile, "%d", &num_of_problems);

    // For test purpose
    printf("Number of problem: %d\n", num_of_problems);
    
    struct problem_struct** my_problems = (struct problem_struct**) malloc(sizeof(struct problem_struct*)*num_of_problems);

    for(k=0; k<num_of_problems; k++)
    {
        int capacities, n, known_best;
        char name[10];

        // Read in relevant data of a problem
        fscanf(pfile, "%s", name);
        fscanf (pfile, "%d", &capacities);
        fscanf (pfile, "%d", &n); 
        fscanf (pfile, "%d", &known_best);
        // printf("Name: %s, capacities: %d, item number: %d, known_best: %d\n", name, capacities, n, known_best);
        
        // Initialize problem
        init_problem(n, capacities, known_best, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].index=j;
            my_problems[k]->items[j].isPacked = false;
            fscanf(pfile, "%d", &(my_problems[k]->items[j].size)); //read profit data
            // printf("item[%d].p=%d\n", j, my_problems[k]->items[j].size);
        }
    }
    fclose(pfile); //close file

    return my_problems;
}


//output a given solution to a file
void output_solution(struct solution_struct* sln, char* out_file) {}

struct solution_struct* initialize_empty_sol (struct problem_struct* prob) {
    
    struct solution_struct* sol = (struct solution_struct*) malloc (sizeof(solution_struct));
    sol->prob = prob;
    sol->objective = 0;
    sol->feasibility = 0;
    // sol->bins = vector<bin_struct>;

    return sol;
}

// void free_solution(struct solution_struct* sln)
// {
//     if(sln!=NULL)
//     {
//         sln->objective=0;
//         sln->prob=NULL;
//         sln->feasibility=false;
//     }
// }


/*  first descent
struct solution_struct* greedy_heuristic (struct problem_struct* prob) {
    int total_n = prob->n;
    int n = prob->n; // unpacked items number
    int cap = prob->capacities; // capacities of each bin
    struct item_struct* items = prob->items; // all items of the problem

    struct solution_struct* sol = initialize_empty_sol(prob); // initialize the solution

    // initialize a bin
    bin_struct *bin = new bin_struct();
    bin->cap_left = cap;

    bool isFound = false;

    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc1);
    cout << prob->items[0].size << endl;
    cout << prob->items[n-1].size << endl;

    // let it run infinitely until some conditions are reached
    while(1) {
        // cout << "unpacked items num: " << n << endl;
        if (n == 0) {
            break; // if all items packed
        }
        
        for (int i=0; i<total_n; i++) {
            // if the item is small enough to be added into the bin
            if (!items[i].isPacked && bin->cap_left >= items[i].size) {
                bin->packed_items.push_back(items[i]);
                bin->cap_left -= items[i].size;
                items[i].isPacked = true;
                isFound = true;
                n--; // minus the total unpacked item number

                // cout << "Find "
                break; // break the for loop
            }
        }
        if (!isFound) { 
            // the current bin is too small to hold any item, push back the bin
            sol->bins.push_back(*bin);
            sol->objective += 1;

            // cout << "Create new bin" << endl;

            // initialize a new empty bin
            bin = new bin_struct();
            bin->cap_left = cap;

        }
    
        isFound = false;

    }
    return sol;
    
}
*/

//  best descent
struct solution_struct* greedy_heuristic (struct problem_struct* prob) {
    int total_n = prob->n;
    int n = prob->n; // unpacked items number
    int cap = prob->capacities; // capacities of each bin
    struct item_struct* items = prob->items; // all items of the problem

    struct solution_struct* sol = new solution_struct(); // initialize the solution
    sol->prob = prob;
    // struct solution_struct* sol = initialize_empty_sol(prob); // initialize the solution

    // initialize a bin
    bin_struct *bin = new bin_struct();
    bin->cap_left = cap;

    bool isFound = false;

    // qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc1); // descending
    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc2); // ascending

    // let it run infinitely until some conditions are reached
    while(1) {
        // cout << "unpacked items num: " << n << endl;
        if (n == 0) {
            break; // if all items packed
        }
        
        item_struct* choice = NULL; // record the best descent

        for (int i=0; i<total_n; i++) {
            // if the item is small enough to be added into the bin
            if (!items[i].isPacked && bin->cap_left >= items[i].size) {
                isFound = true; 

                if (choice == NULL) { 
                    choice = & items[i]; 
                }

                // compate with the current known best descent                
                if (items[i].size > choice->size) {
                    choice = & items[i];
                }
            }
        }

        if (isFound) {
            
            bin->packed_items.push_back(*choice);
            bin->cap_left -= choice->size;
            choice->isPacked = true;
            n--; // minus the total unpacked item number

        } else { 
            
            // the current bin is too small to hold any item, push back the bin
            sol->bins.push_back(*bin);
            delete(bin);
            sol->objective += 1;

            // initialize a new empty bin
            bin = new bin_struct();
            bin->cap_left = cap;
        }
    
        isFound = false;

    }

    delete(bin);
    return sol;
    
}

// void varaible_neighbourhood_search(struct problem_struct* prob) {
void varaible_neighbourhood_search(struct problem_struct* prob){
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int nb_indx =0; //neighbourhood index
    
    best_sln.prob = prob;
    struct solution_struct* curt_sln = greedy_heuristic(prob);

    // Test code here
    cout << "Initialize a possible answer: " << endl;
    cout << "Objectives: " << curt_sln->objective << endl;
    cout << "Known best: " << prob->known_best << endl << endl;

    update_best_solution(curt_sln);

    // int cnt = 0;
    // while(!(curt_sln->bins.empty())) {
        // cout << "bin " << cnt << endl;
        // cout << curt_sln->bins.at(cnt).cap_left << endl;
        // cnt ++;
    // }

/* 
    
    int shaking_count =0;
    while(time_spent < MAX_TIME) //note that final computational time can be way beyond the MAX_TIME if best_descent is time consuming
    {
        while(nb_indx<K){
            struct solution_struct* neighb_s=best_descent_vns(nb_indx+1, curt_sln); //best solution in neighbourhood nb_indx
            if(neighb_s->objective > curt_sln->objective){
                copy_solution(curt_sln, neighb_s);
                nb_indx=1;
            }
            else nb_indx++;
            free_solution(neighb_s);free(neighb_s);
        }
        update_best_solution(curt_sln);
        double gap=1000; //set to an arbitrarily large number if best known solution is not availabe.
        if(best_sln.prob->best_obj!=0) gap=  100*(best_sln.prob->best_obj - best_sln.objective)/best_sln.prob->best_obj;
        printf("shaking_count=%d, curt obj =%0.0f,\t best obj=%0.0f,\t gap= %0.2f%%\n",shaking_count, curt_sln->objective, best_sln.objective, gap);
        vns_shaking(curt_sln, SHAKE_STRENGTH); //shaking at a given strength. This can be made adaptive
        //vns_shaking(curt_sln, shaking_count/100+1); //re-active shaking
        shaking_count++;
        nb_indx=0;
        
        time_fin=clock();
        time_spent = (double)(time_fin-time_start)/CLOCKS_PER_SEC;
    }

*/
    // output_solution(&best_sln, "vns_results.txt");
    // free_solution(curt_sln); 
    delete(curt_sln);

}


int main(int argc, const char * argv[]) {
    
    printf("Starting the run...\n");

    char data_file[50]={"somefile"}, out_file[50]={}, solution_file[50]={};  //max 50 problem instances per run

    if(argc<3)
    {
        printf("Insufficient arguments. Please use the following options:\n   -s data_file (compulsory)\n   -o out_file (default my_solutions.txt)\n   -c solution_file_to_check\n   -t max_time (in sec)\n");
        return 1;
    }
    else if(argc>9)
    {
        printf("Too many arguments.\n");
        return 2;
    }
    else
    {
        for(int i=1; i<argc; i=i+2)
        {
            if(strcmp(argv[i],"-s")==0)
                strcpy(data_file, argv[i+1]);
            else if(strcmp(argv[i],"-o")==0)
                strcpy(out_file, argv[i+1]);
            else if(strcmp(argv[i],"-c")==0)
                strcpy(solution_file, argv[i+1]);
            else if(strcmp(argv[i],"-t")==0)
                MAX_TIME = atoi(argv[i+1]);
        }
        printf("data_file= %s, output_file= %s, sln_file=%s, max_time=%d\n", data_file, out_file, solution_file, MAX_TIME);
    }
    struct problem_struct** my_problems = load_problems(data_file);
    
    if(strlen(solution_file)<=0)
    {
        if(strcmp(out_file,"")==0) strcpy(out_file, "my_solutions.txt"); //default output
        FILE* pfile = fopen(out_file, "w"); //open a new file

        fprintf(pfile, "%d\n", num_of_problems); fclose(pfile);

        for(int k=0; k<num_of_problems; k++)
        {
            cout << "Number of question: " << k << endl;
            best_sln.objective=0; best_sln.feasibility=0;
            for(int run=0; run<NUM_OF_RUNS; run++) {
                printf("Running VNS...\n");
                varaible_neighbourhood_search(my_problems[k]);
            }
            // output_solution(&best_sln,out_file);
        }
    }

    for(int k=0; k<num_of_problems; k++)
    {
        // printf("free problem %d\n", k);
        free_problem(my_problems[k]); //free problem data memory
    }

    free(my_problems); //free problems array
    
    // if (best_sln.x!=NULL && best_sln.cap_left != NULL) {
    //     free(best_sln.cap_left); 
    //     free(best_sln.x);
    // } 
    
    //free global
    return 0;
}
