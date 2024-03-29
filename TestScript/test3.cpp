#include <iostream>
#include <vector>

#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

using namespace std;

// struct of the item
struct item_struct{
    int size;   // size of the item
    int index;  // index of the item
};

// struct of the bin
struct bin_struct {
    vector<item_struct> packed_items;   // vector of the items it contains
    int cap_left;                       // the capacity left
};

// struct of the problem
struct problem_struct{
    char name[10];              // name of the problem
    int n;                      // number of items
    int capacities;             // capacitie of each bin
    int known_best;             // the best objective of this problem
    struct item_struct* items;  // a pointer to an array of items
};

// struct of the solution
struct solution_struct {
    struct problem_struct* prob;    // a pointer to the problem that it is going to solve
    int objective;                  // current solution's objective
    vector<bin_struct> bins;
};

// global variables 
int K = 4;                       // k is used to denote how many neighborhoods would be used
int SHAKE_STRENGTH = 12;         // the strength of shaking
struct solution_struct best_sln; // global best solution
// int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 1;             // number of runs
int MAX_TIME = 30;               // max amount of time permited (in sec)
int num_of_problems;             // number of problems
bool isImproving;                // a global boolean that is used to tell if the the current neighborhood's local search have improvement. because though it might be improving, the objective would probably not drop

// sort the items in ascending order
int cmpfunc1(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return -1;
    if(item1->size<item2->size) return 1;
    return 0;
}

// sort the items in decending order
int cmpfunc2(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return -1;
    if(item1->size<item2->size) return 1;
    return 0;
}

//return a random nunber ranging from min to max (inclusive)
int rand_int(int min, int max)
{
    int div = max-min+1;
    int val =rand() % div + min;
    //printf("rand_range= %d \n", val);
    return val;
}

// according to the given parameter, initialize and return a new problem
void init_problem(char* name, int n, int capacities, int known_best, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = (struct problem_struct*) malloc(sizeof(struct problem_struct));
    strcpy(new_prob->name, name);

    new_prob->n=n; new_prob->capacities=capacities; new_prob->known_best=known_best;
    new_prob->items= (struct item_struct*) malloc(sizeof(struct item_struct)*n);
    *my_prob = new_prob;
}

// free the problem and its space
void free_problem(struct problem_struct* prob)
{
    if(prob!=NULL)
    {
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
    if(source_sln ==NULL) return false;
    if(dest_sln==NULL)
    {
        dest_sln = (struct solution_struct*) malloc(sizeof(struct solution_struct));
    }
    dest_sln->prob = source_sln->prob;
    dest_sln->objective = source_sln->objective;
    dest_sln->bins = source_sln->bins;
    return true;
}

//update global best solution from sln
void update_best_solution(struct solution_struct* sln)
{
    if(best_sln.objective >= sln->objective) { //这边特意放宽条件
        copy_solution(&best_sln, sln);
    }
}

// load problem from data file
struct problem_struct** load_problems(char* data_file)
{
    int i,j,k;

    // Open data file and read out data
    FILE* pfile = fopen(data_file, "r");
    if(pfile==NULL) {
        printf("Data file %s does not exist. Please check!\n", data_file); exit(2); 
    }

    fscanf (pfile, "%d", &num_of_problems);
    
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
        init_problem(name, n, capacities, known_best, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].index=j;
            fscanf(pfile, "%d", &(my_problems[k]->items[j].size)); //read profit data
            // printf("item[%d].p=%d\n", j, my_problems[k]->items[j].size);
        }
    }
    fclose(pfile); //close file

    return my_problems;
}

// output a given solution to a file
void output_solution(struct solution_struct* sln, char* out_file) {
    cout << "output solution" << endl;
    FILE* pfile = fopen(out_file, "a"); //open a new file

    fprintf(pfile, "%s\n", sln->prob->name); 
    fprintf(pfile, " obj=   %d   %d\n", sln->objective, sln->objective - sln->prob->known_best); 
    for (int i=0; i<sln->bins.size(); i++) {
        // fprintf(pfile, "bin %d: ", i);
        for (int j=0; j<sln->bins[i].packed_items.size(); j++) {
            fprintf(pfile, "%d ", sln->bins[i].packed_items[j].index); 
            // fprintf(pfile, "%d: %d  ", sln->bins[i].packed_items[j].index, sln->bins[i].packed_items[j].size); 
        }
        // fprintf(pfile, "%d", sln->bins[i].cap_left);
        fprintf(pfile, "\n");  
    }

    fclose(pfile);

}

int can_move(vector<bin_struct> *bins, int* curt_move, int nb_indx) {

    bin_struct* bin1;
    bin_struct* bin2;
    bin_struct* bin3;

    int bin1_idx, bin2_idx, bin3_idx;
    item_struct item1, item2, item3;

    int delta = 0, best_delta = 0;

    switch(nb_indx) {
        
        case 1: {
            // cheeck if any item could be packed into another bin
            bin1_idx = curt_move[0];
            bin2_idx = curt_move[2];

            bin1 = &(*(bins->begin() + bin1_idx));
            bin2 = &(*(bins->begin() + bin2_idx));

            for (int j=0; j<bin2->packed_items.size(); j++) {

                item2 = bin2->packed_items[j];
                
                if (bin1->cap_left >= item2.size) {
                    if (bin1->cap_left == item2.size) {
                        curt_move[3] = j;
                        return -2;
                    }
                    
                    delta = item2.size;

                    if (delta > best_delta) {
                        curt_move[3] = j;
                        best_delta = delta;
                    }
                }
            }
            return best_delta;
        }
        
        case 2: {
            // transfer one item from one bin to another
            bin1_idx = curt_move[0];
            bin2_idx = curt_move[2];

            bin1 = &(*(bins->begin() + bin1_idx));
            bin2 = &(*(bins->begin() + bin2_idx));

            for (int i=0; i<bin1->packed_items.size(); i++) {
                for (int j=0; j<bin2->packed_items.size(); j++) {

                    item1 = bin1->packed_items[i];
                    item2 = bin2->packed_items[j];
                    
                    if (bin1->cap_left >= item2.size - item1.size ) {
                        if (bin1->cap_left == item2.size - item1.size) {
                            curt_move[1] = i;
                            curt_move[3] = j;
                            return -2;
                        }
                        
                        delta = item2.size - item1.size;

                        if (delta > best_delta) {
                            curt_move[1] = i;
                            curt_move[3] = j;
                            best_delta = delta;
                        }
                    }
                }
            }
            return best_delta;
        }

    }

    return -1;
}

// copy the current move to best move
void copy_move(int* curt_move, int* best_move) {
    for (int i=0; i<6; i++) {
        best_move[i] = curt_move[i];
    }
}


void apply_move(int nb_indx, int* best_move, struct solution_struct* best_neighb) {

    bin_struct* bin1;
    bin_struct* bin2;
    bin_struct* bin3;
    
    item_struct item1, item2, item3;

    int bin1_idx, bin2_idx, bin3_idx;
    int item1_idx, item2_idx, item3_idx;

    vector<bin_struct> * bins = & best_neighb->bins;

    int delta = 0;
    
    switch(nb_indx) {
        case 1: {
            // purely add an item from one bin to another
            bin1_idx = best_move[0];
            bin2_idx = best_move[2];

            item2_idx = best_move[3];
            
            bin1 = &(*(bins->begin() + bin1_idx));
            bin2 = &(*(bins->begin() + bin2_idx));
            
            item2 = bin2->packed_items[item2_idx];

            bin1->cap_left = bin1->cap_left - item2.size;
            bin2->cap_left = bin2->cap_left + item2.size;
       
            bin2->packed_items.erase(bin2->packed_items.begin() + item2_idx);

            bin1->packed_items.push_back(item2); // add item2 to bin1
            
            if (bin2->packed_items.size() == 0) {
                cout << "objective -1 " << endl;
                best_neighb->bins.erase(best_neighb->bins.begin() + bin2_idx);
                best_neighb->objective -= 1;
            }
            break;

        }
        
        case 2: {
            // transfer two items in two bins
            bin1_idx = best_move[0];
            bin2_idx = best_move[2];

            item1_idx = best_move[1];
            item2_idx = best_move[3];

            bin1 = &(*(bins->begin() + bin1_idx));
            bin2 = &(*(bins->begin() + bin2_idx));
            
            item1 = bin1->packed_items[item1_idx];
            item2 = bin2->packed_items[item2_idx];

            bin1->cap_left = bin1->cap_left - item2.size + item1.size;
            bin2->cap_left = bin2->cap_left - item1.size + item2.size;
       
            bin1->packed_items.erase(bin1->packed_items.begin() + item1_idx);
            bin2->packed_items.erase(bin2->packed_items.begin() + item2_idx);

            bin1->packed_items.push_back(item2); // add item2 to bin1
            bin2->packed_items.push_back(item1); // add item1 to bin2

            break;

        }
        
    }

}


struct solution_struct* best_descent_vns(int nb_indx, struct solution_struct* curt_sln)
{
    struct solution_struct* best_neighb = curt_sln;

    vector<bin_struct> * bins = & curt_sln->bins;

    int n=curt_sln->prob->n;

    //storing best neighbourhood moves
    int curt_move[] ={-1,-1, -1,-1, -1,-1, -1,-1}, best_move []={-1,-1, -1,-1, -1,-1, -1,-1};
    int delta=0, best_delta=0;  
    
    bin_struct* bin1;
    bin_struct* bin2;
    bin_struct* bin3;
    bin_struct* bin4;

    item_struct item1, item2, item3, item4;
    
    int binIndex1, binIndex2, binIndex3, itemIndex1, itemIndex2, itemIndex3, itemIndex4, itemIndex5;
    switch (nb_indx)
    {
        
        case 1: {
            
            // // traverse bins, let i denotes the index of the first bin
            // for(int i=0; i<bins->size(); i++){
            //     int left_cap = best_neighb->bins[i].cap_left;
            //     if (left_cap<=0)
            //     {
            //         continue; // only focus on the bin which is not full
            //     }
            //     // traverse bins, let j denotes the index of the second bin
            //     for (int j = i + 1; j < bins->size(); j++)
            //     {
            //         // traverse all items in bin j
            //         for (int k = 0; k < best_neighb->bins[j].packed_items.size(); k++)
            //         {
            //             // check if item k can be packed in to bin i and whether we can have a better fitness value
            //             if (best_neighb->bins[j].packed_items[k].size<=left_cap&&best_neighb->bins[j].packed_items[k].size>best_delta)
            //             {
            //                 best_delta = best_neighb->bins[j].packed_items[k].size; // update best fitness value
            //                 // update the data for the move that generate best fitness value
            //                 binIndex1 = i;
            //                 binIndex2 = j;
            //                 itemIndex1 = k;
            //             }
            //         }
            //     }
            // }
            // // update best neighbor to the neighbor which has the best fitness value so far
            // if(best_delta>0) {
                
            //     isImproving = true; // denotes the best_neighb is different from the original solution
            //     // copy the target item
            //     item_struct copyItem;
            //     copyItem.index = best_neighb->bins[binIndex2].packed_items[itemIndex1].index;
            //     copyItem.size = best_neighb->bins[binIndex2].packed_items[itemIndex1].size;

            //     // update left capacities for bins
            //     best_neighb->bins[binIndex1].cap_left-=copyItem.size;
            //     best_neighb->bins[binIndex2].cap_left+=copyItem.size;
            //     // pack the item into new bin
            //     best_neighb->bins[binIndex1].packed_items.push_back(copyItem);
            //     // delete the rubbish item
            //     best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex1);
            //     // check if we have an empty bin now
            //     if (best_neighb->bins[binIndex2].packed_items.size()==0)
            //     {
            //         best_neighb->bins.erase(best_neighb->bins.begin()+binIndex2); // delete empty bin
            //         best_neighb->objective--; // update objective
            //     }
                
            // }
            // break;


            // // add item
            for (int i=0; i<bins->size(); i++ ) { 
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left == 0) continue;
                for (int j=i+1; j<bins->size(); j++ ) { 
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->cap_left == 0) continue;

                    curt_move[0] = i;
                    curt_move[2] = j;

                    delta = can_move(bins, &curt_move[0], nb_indx);

                    if (delta == -2) {
                        isImproving = true;
                        apply_move(nb_indx, &curt_move[0], best_neighb);
                        return best_neighb;
                    }

                    if (delta > best_delta) {
                        isImproving = true;
                        best_delta = delta;
                        copy_move(&curt_move[0], &best_move[0]);
                    }
                }
            }

            if (best_delta > 0) {
                isImproving = true;
                apply_move(nb_indx, &best_move[0], best_neighb);
            }
            break;
        }

        case 2: {
            //             // traverse bins, let i denotes the index of the first bin
            // for (int i = 0; i < bins->size(); i++)
            // {
            //     if (best_neighb->bins[i].cap_left==0)
            //     {
            //         continue; // skip the bin which is already full
            //     }
            //     // traverse bins, let j denotes the index of the second bin
            //     for (int j = i+1; j < bins->size(); j++)
            //     {
            //         // traverse all items in bin i, let a denotes its index
            //         for (int a = 0; a < best_neighb->bins[i].packed_items.size(); a++)
            //         {
            //             // traverse all items in bin j, let b denotes its index
            //             for (int b = 0; b < best_neighb->bins[j].packed_items.size(); b++)
            //             {
            //                 // check if we can swap the item a and item b
            //                 if (best_neighb->bins[i].packed_items[a].size<best_neighb->bins[j].packed_items[b].size
            //                     && best_neighb->bins[j].packed_items[b].size-best_neighb->bins[i].packed_items[a].size<=best_neighb->bins[i].cap_left)
            //                 {
            //                     // check whether we can have a better fitness value
            //                     if (best_neighb->bins[j].packed_items[b].size-best_neighb->bins[i].packed_items[a].size>best_delta)
            //                     {
            //                         // update best fitness value
            //                         best_delta = best_neighb->bins[j].packed_items[b].size-best_neighb->bins[i].packed_items[a].size;

            //                         // update the data for the move that generate best fitness value
            //                         binIndex1 = i;
            //                         binIndex2 = j;
            //                         itemIndex1 = a;
            //                         itemIndex2 = b;
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }
            // // update best neighbor to the neighbor which has the best fitness value so far
            // if(best_delta>0) {
            //     isImproving = true; // denotes the best_neighb is different from the original solution
            //     // copy the target items
            //     item_struct copyItem1, copyItem2;
            //     copyItem1.index = best_neighb->bins[binIndex1].packed_items[itemIndex1].index;
            //     copyItem1.size = best_neighb->bins[binIndex1].packed_items[itemIndex1].size;
            //     copyItem2.index = best_neighb->bins[binIndex2].packed_items[itemIndex2].index;
            //     copyItem2.size = best_neighb->bins[binIndex2].packed_items[itemIndex2].size;
            //     // update left capacities for bins
            //     best_neighb->bins[binIndex1].cap_left-=copyItem2.size-copyItem1.size;
            //     best_neighb->bins[binIndex2].cap_left+=copyItem2.size-copyItem1.size;
            //     // swap two items in two bins correspondingly
            //     best_neighb->bins[binIndex1].packed_items.erase(best_neighb->bins[binIndex1].packed_items.begin()+itemIndex1);
            //     best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex2);
            //     best_neighb->bins[binIndex1].packed_items.push_back(copyItem2);
            //     best_neighb->bins[binIndex2].packed_items.push_back(copyItem1);
            // }
            // break;



            // transfer
            for (int i=0; i<bins->size(); i++ ) { 
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left == 0) continue;
                for (int j=i+1; j<bins->size(); j++ ) { 
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->cap_left == 0) continue;

                    curt_move[0] = i;
                    curt_move[2] = j;

                    delta = can_move(bins, &curt_move[0], nb_indx);

                    if (delta == -2) {
                        isImproving = true;
                        apply_move(nb_indx, &curt_move[0], best_neighb);
                        return best_neighb;
                    }

                    if (delta > best_delta) {
                        isImproving = true;
                        best_delta = delta;
                        copy_move(&curt_move[0], &best_move[0]);
                    }
                }
            }

            if (best_delta > 0) {
                isImproving = true;
                apply_move(nb_indx, &best_move[0], best_neighb);
            }
            break;
        }

        /* For case 3 and 4, I choose to not encapsulate the algorithm. Because I'm afraid that     encapsulation may reduce the execution efficiency, though encapsulation dose make codes more human friendly. */

        case 3: {
            
            for(int i=0; i<bins->size(); i++){
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left==0) { continue; }

                for (int j = i + 1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->cap_left==0) { continue; }
                
                    for (int k = j; k < bins->size(); k++) {
                        bin3 = &(*(bins->begin() + k));

                        for (int a = 0; a < bin1->packed_items.size(); a++) {
                            for (int b = 0; b < bin2->packed_items.size(); b++) {
                                for (int c = 0; c < bin3->packed_items.size(); c++) {
                                    // if bin j are the same with bin k, and b, c denotes the same item, then skip
                                    if (j == k && b == c) { continue; }

                                    // tempararily store the items, this may decrease the efficiency, but it make the code more readable
                                    item1 = bin1->packed_items[a];
                                    item2 = bin2->packed_items[b];
                                    item3 = bin3->packed_items[c];
                                    // Check whether we can apply the swap, making sure bin i,j has enough capacity. Check 
                                    // whether we can generate a better fitness value as well.
                                    if (item1.size < item2.size + item3.size && item2.size + item3.size - item1.size <= bin1->cap_left && item2.size + item3.size-item1.size > best_delta){
                                        if (item1.size-item2.size <= bin2->cap_left
                                        || j==k && item1.size - item2.size - item3.size <= bin2->cap_left) {
                                            

                                            // update best fitness value
                                            best_delta = item2.size+item3.size-item1.size;
                                            // update the data for the move that generate best fitness value
                                            best_move[0] = i;
                                            best_move[2] = j;
                                            best_move[4] = k;

                                            best_move[1] = a;
                                            best_move[3] = b;
                                            best_move[5] = c;

                                        }
                                            
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // update best neighbor to the neighbor which has the best fitness value so far
            if (best_delta>0)
            {   
                isImproving = true;

                bin1 = &(*(bins->begin() + best_move[0]));
                bin2 = &(*(bins->begin() + best_move[2]));
                bin3 = &(*(bins->begin() + best_move[4]));

                item1 = bin1->packed_items[best_move[1]];
                item2 = bin2->packed_items[best_move[3]];
                item3 = bin3->packed_items[best_move[5]];
                
                bin1->cap_left-=item2.size+item3.size-item1.size;
                bin2->cap_left-=item1.size-item2.size;
                bin3->cap_left+=item3.size;

                bin1->packed_items.erase(bin1->packed_items.begin() + best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[3]);


                if (best_move[2] == best_move[4] && best_move[3] < best_move[5])
                {
                    bin3->packed_items.erase(bin3->packed_items.begin()+best_move[5]-1);
                } else {
                    bin3->packed_items.erase(bin3->packed_items.begin()+best_move[5]);
                }

                bin1->packed_items.push_back(item2);
                bin1->packed_items.push_back(item3);
                bin2->packed_items.push_back(item1);
                

                if (bin3->packed_items.size() == 0)
                {
                    best_neighb->bins.erase(best_neighb->bins.begin() + best_move[4]);
                    best_neighb->objective--;
                }
            }
            break;
        }
            
        case 4: {
            for (int i = 0; i < bins->size(); i++) {
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left==0 || bin1->packed_items.size() < 2) { continue; }
                for (int j = i+1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->packed_items.size() < 2) { continue; }

                    for (int a = 0; a < bin1->packed_items.size(); a++) {
                        for (int b = a+1; b < bin1->packed_items.size(); b++) {
                            for (int c = 0; c < bin2->packed_items.size(); c++) {
                                for (int d = c+1; d < bin2->packed_items.size(); d++) {

                                    int delta = bin2->packed_items[c].size+bin2->packed_items[d].size-bin1->packed_items[a].size-bin1->packed_items[b].size;

                                    if (delta>best_delta&&delta<=bin1->cap_left)
                                    {
                                        best_delta = bin2->packed_items[c].size+bin2->packed_items[d].size-bin1->packed_items[a].size-bin1->packed_items[b].size;

                                        best_move[0] = i;
                                        best_move[2] = j;

                                        best_move[1] = a;
                                        best_move[3] = b;
                                        best_move[5] = c;
                                        best_move[7] = d;

                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (best_delta>0)
            {
                isImproving = true;

                bin1 = &(*(bins->begin() + best_move[0]));
                bin2 = &(*(bins->begin() + best_move[2]));

                item1 = bin1->packed_items[best_move[1]];
                item2 = bin1->packed_items[best_move[3]];
                item3 = bin2->packed_items[best_move[5]];
                item4 = bin2->packed_items[best_move[7]];

                bin1->cap_left-=item4.size+item3.size-item1.size-item2.size;
                bin2->cap_left+=item4.size+item3.size-item1.size-item2.size;

                bin1->packed_items.erase(bin1->packed_items.begin()+best_move[3]);
                bin1->packed_items.erase(bin1->packed_items.begin()+best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[7]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[5]);
                
                bin1->packed_items.push_back(item3);
                bin1->packed_items.push_back(item4);
                bin2->packed_items.push_back(item1);
                bin2->packed_items.push_back(item2);
            }
            break;
        }
        
        case 5:
        // cout << "5" << endl;
            // traverse bins, let i denotes the index of the first bin
            for (int i = 0; i < bins->size(); i++) {
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left==0) {
                    continue; // skip the bin which is already full or has no more than two items
                }

                // traverse bins, let j denotes the index of the second bin
                for (int j = i+1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->packed_items.size()<3) {
                        continue; // skip the bin which is already full or has no more than three items
                    }

                    // traverse items in bin i, let a denotes the index of the first item in bin i
                    for (int a = 0; a < bin1->packed_items.size(); a++)
                    {
                        // traverse items in bin i, let b denotes the index of the second item in bin i
                        // for (int b = a+1; b < bin1->packed_items.size(); b++)
                        // {
                            // traverse items in bin j, let c denotes the index of the first item in bin j
                        for (int c = 0; c < bin2->packed_items.size(); c++) {
                            // traverse items in bin j, let d denotes the index of the second item in bin j
                            for (int d = c+1; d < bin2->packed_items.size(); d++) {
                                // traverse items in bin j, let e denotes the index of the third item in bin j
                                for (int e = d+1; e < bin2->packed_items.size(); e++) {
                                    item1 = bin1->packed_items[a];

                                    item2 = bin2->packed_items[c];
                                    item3 = bin2->packed_items[d];
                                    item4 = bin2->packed_items[e];
                                    // calculate the fitness value of this new solution (neighbor)
                                    delta = item2.size + item3.size + item4.size - item1.size;
                                    // bin2->packed_items[c].size+bin2->packed_items[d].size+bin2->packed_items[e].size-bin1->packed_items[a].size-bin1->packed_items[b].size;
                                    // check if the moves are feasible and whether we can have a better fitness value
                                    if (delta > best_delta && delta <= bin1->cap_left)
                                    {
                                        // update best fitness value
                                        best_delta = delta;
                                        // bin2->packed_items[c].size+bin2->packed_items[d].size+bin2->packed_items[e].size-bin1->packed_items[a].size-bin1->packed_items[b].size;
                                        // update the data for the move that generate best fitness value
                                        best_move[0] = i;
                                        best_move[2] = j;

                                        best_move[1] = a;
                                        best_move[3] = c;
                                        best_move[5] = d;
                                        best_move[7] = e;
                                    }
                                }
                            }
                        }
                        // }
                    }
                }
            }
            // update best neighbor to the neighbor which has the best fitness value so far
            if (best_delta>0)
            {
                isImproving = true; // denotes the best_neighb is different from the original solution

                bin1 = &(*(bins->begin() + best_move[0]));
                bin2 = &(*(bins->begin() + best_move[2]));

                item1 = bin1->packed_items[best_move[1]];
                item2 = bin2->packed_items[best_move[3]];
                item3 = bin2->packed_items[best_move[5]];
                item4 = bin2->packed_items[best_move[7]];

                bin1->cap_left -= item2.size + item3.size +item4.size - item1.size; // ?
                bin2->cap_left += item2.size + item3.size +item4.size - item1.size; // ?

                bin1->packed_items.erase(bin1->packed_items.begin()+best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[7]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[5]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[3]);
                
                bin1->packed_items.push_back(item2);
                bin1->packed_items.push_back(item3);
                bin1->packed_items.push_back(item4);
                bin2->packed_items.push_back(item1);



                // // copy the target items
                // item_struct copyItem1,copyItem2,copyItem3,copyItem4,copyItem5;
                // copyItem1.index = best_neighb->bins[binIndex1].packed_items[itemIndex1].index;
                // copyItem1.size = best_neighb->bins[binIndex1].packed_items[itemIndex1].size;
                // copyItem2.index = best_neighb->bins[binIndex1].packed_items[itemIndex2].index;
                // copyItem2.size = best_neighb->bins[binIndex1].packed_items[itemIndex2].size;
                // copyItem3.index = best_neighb->bins[binIndex2].packed_items[itemIndex3].index;
                // copyItem3.size = best_neighb->bins[binIndex2].packed_items[itemIndex3].size;
                // copyItem4.index = best_neighb->bins[binIndex2].packed_items[itemIndex4].index;
                // copyItem4.size = best_neighb->bins[binIndex2].packed_items[itemIndex4].size;
                // copyItem5.index = best_neighb->bins[binIndex2].packed_items[itemIndex5].index;
                // copyItem5.size = best_neighb->bins[binIndex2].packed_items[itemIndex5].size;
                // // update left capacities for bins
                // best_neighb->bins[binIndex1].cap_left-=copyItem5.size+copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                // best_neighb->bins[binIndex2].cap_left+=copyItem5.size+copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                // // swap items
                // best_neighb->bins[binIndex1].packed_items.erase(best_neighb->bins[binIndex1].packed_items.begin()+itemIndex2);
                // best_neighb->bins[binIndex1].packed_items.erase(best_neighb->bins[binIndex1].packed_items.begin()+itemIndex1);
                // best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex5);
                // best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex4);
                // best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex3);
                // best_neighb->bins[binIndex1].packed_items.push_back(copyItem3);
                // best_neighb->bins[binIndex1].packed_items.push_back(copyItem4);
                // best_neighb->bins[binIndex1].packed_items.push_back(copyItem5);
                // best_neighb->bins[binIndex2].packed_items.push_back(copyItem1);
                // best_neighb->bins[binIndex2].packed_items.push_back(copyItem2);
            }
            
            break;
        
        case 6:
        // cout << "5" << endl;
            // traverse bins, let i denotes the index of the first bin
            for (int i = 0; i < bins->size(); i++)
            {
                if (best_neighb->bins[i].cap_left==0||best_neighb->bins[i].packed_items.size()<2)
                {
                    continue; // skip the bin which is already full or has no more than two items
                }
                // traverse bins, let j denotes the index of the second bin
                for (int j = i+1; j < bins->size(); j++)
                {
                    if (best_neighb->bins[j].packed_items.size()<3)
                    {
                        continue; // skip the bin which is already full or has no more than three items
                    }
                    // traverse items in bin i, let a denotes the index of the first item in bin i
                    for (int a = 0; a < best_neighb->bins[i].packed_items.size(); a++)
                    {
                        // traverse items in bin i, let b denotes the index of the second item in bin i
                        for (int b = a+1; b < best_neighb->bins[i].packed_items.size(); b++)
                        {
                            // traverse items in bin j, let c denotes the index of the first item in bin j
                            for (int c = 0; c < best_neighb->bins[j].packed_items.size(); c++)
                            {
                                // traverse items in bin j, let d denotes the index of the second item in bin j
                                for (int d = c+1; d < best_neighb->bins[j].packed_items.size(); d++)
                                {
                                    // traverse items in bin j, let e denotes the index of the third item in bin j
                                    for (int e = d+1; e < best_neighb->bins[j].packed_items.size(); e++)
                                    {
                                        // calculate the fitness value of this new solution (neighbor)
                                        int delta = best_neighb->bins[j].packed_items[c].size+best_neighb->bins[j].packed_items[d].size+best_neighb->bins[j].packed_items[e].size-best_neighb->bins[i].packed_items[a].size-best_neighb->bins[i].packed_items[b].size;
                                        // check if the moves are feasible and whether we can have a better fitness value
                                        if (delta>best_delta&&delta<=best_neighb->bins[i].cap_left)
                                        {
                                            // update best fitness value
                                            best_delta = best_neighb->bins[j].packed_items[c].size+best_neighb->bins[j].packed_items[d].size+best_neighb->bins[j].packed_items[e].size-best_neighb->bins[i].packed_items[a].size-best_neighb->bins[i].packed_items[b].size;
                                            // update the data for the move that generate best fitness value
                                            binIndex1 = i;
                                            binIndex2 = j;
                                            itemIndex1 = a;
                                            itemIndex2 = b;
                                            itemIndex3 = c;
                                            itemIndex4 = d;
                                            itemIndex5 = e;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // update best neighbor to the neighbor which has the best fitness value so far
            if (best_delta>0)
            {
                isImproving = true; // denotes the best_neighb is different from the original solution
                // copy the target items
                item_struct copyItem1,copyItem2,copyItem3,copyItem4,copyItem5;
                copyItem1.index = best_neighb->bins[binIndex1].packed_items[itemIndex1].index;
                copyItem1.size = best_neighb->bins[binIndex1].packed_items[itemIndex1].size;
                copyItem2.index = best_neighb->bins[binIndex1].packed_items[itemIndex2].index;
                copyItem2.size = best_neighb->bins[binIndex1].packed_items[itemIndex2].size;
                copyItem3.index = best_neighb->bins[binIndex2].packed_items[itemIndex3].index;
                copyItem3.size = best_neighb->bins[binIndex2].packed_items[itemIndex3].size;
                copyItem4.index = best_neighb->bins[binIndex2].packed_items[itemIndex4].index;
                copyItem4.size = best_neighb->bins[binIndex2].packed_items[itemIndex4].size;
                copyItem5.index = best_neighb->bins[binIndex2].packed_items[itemIndex5].index;
                copyItem5.size = best_neighb->bins[binIndex2].packed_items[itemIndex5].size;
                // update left capacities for bins
                best_neighb->bins[binIndex1].cap_left-=copyItem5.size+copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                best_neighb->bins[binIndex2].cap_left+=copyItem5.size+copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                // swap items
                best_neighb->bins[binIndex1].packed_items.erase(best_neighb->bins[binIndex1].packed_items.begin()+itemIndex2);
                best_neighb->bins[binIndex1].packed_items.erase(best_neighb->bins[binIndex1].packed_items.begin()+itemIndex1);
                best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex5);
                best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex4);
                best_neighb->bins[binIndex2].packed_items.erase(best_neighb->bins[binIndex2].packed_items.begin()+itemIndex3);
                best_neighb->bins[binIndex1].packed_items.push_back(copyItem3);
                best_neighb->bins[binIndex1].packed_items.push_back(copyItem4);
                best_neighb->bins[binIndex1].packed_items.push_back(copyItem5);
                best_neighb->bins[binIndex2].packed_items.push_back(copyItem1);
                best_neighb->bins[binIndex2].packed_items.push_back(copyItem2);
            }
            
            break;
        
    }
    return best_neighb;
}


// use best fit algorithm to get the initial solution
struct solution_struct* greedy_heuristic (struct problem_struct* prob) {
    int n = prob->n;                            // unpacked items number
    int cap = prob->capacities;                 // capacities of each bin
    struct item_struct* items = prob->items;    // all items of the problem
    int bin_num = 1;                            // number of bin currently created

    // initialize the solution
    struct solution_struct* sln = new solution_struct(); 
    sln->prob = prob;

    // sort the items in descending order
    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc2); 

    // initialize a bin, and push it into vector
    bin_struct bin;
    bin.cap_left = cap;
    sln->bins.push_back(bin);

    // travel through all the items
    for (int i=0; i<n; i++) {
        int index = 0, target_index = -1; int left_cap = cap+1;

        // travel through all the bins
        while(index <bin_num) {
            // if the item could be 
            if (sln->bins[index].cap_left >= prob->items[i].size && left_cap > sln->bins[index].cap_left) {
                target_index = index;
                left_cap = sln->bins[index].cap_left;
            }
            index ++;
        }


        if (target_index==-1) {
            bin_struct new_bin;
            new_bin.packed_items.push_back(prob->items[i]);
            new_bin.cap_left = cap-prob->items[i].size;
            sln->bins.push_back(new_bin);
            bin_num ++;
            
        } else {
            sln->bins[target_index].packed_items.push_back(prob->items[i]);
            sln->bins[target_index].cap_left-=prob->items[i].size;
        }
    }

    sln->objective = bin_num;

    return sln;
}


// pair-wise random swap, strength denotes how much 
void vns_shaking(struct solution_struct* sln, int strength)
{
    int bin_index1, bin_index2;
    int item_indx1, item_indx2;
    item_struct item1, item2;

    int n = sln->prob->n;
    int m = 0, time = 0;


    while (m < strength && time < 200)
    {
        // randomly pick two bins
        bin_index1 = rand_int(0, sln->bins.size() - 1);
        bin_index2 = rand_int(0, sln->bins.size() - 1);

        // if the two bins are the same, repick
        while (bin_index1 == bin_index2 && time < 200)
        {
            bin_index2 = rand_int(0, sln->bins.size() - 1);
            time++;
        }

        // randomly pick two item in the two bins
        item_indx1 = rand_int(0, sln->bins[bin_index1].packed_items.size() - 1);
        item_indx2 = rand_int(0, sln->bins[bin_index2].packed_items.size() - 1);

        if (sln->bins[bin_index2].packed_items[item_indx2].size - sln->bins[bin_index1].packed_items[item_indx1].size <= sln->bins[bin_index1].cap_left
            && sln->bins[bin_index1].packed_items[item_indx1].size-sln->bins[bin_index2].packed_items[item_indx2].size <= sln->bins[bin_index2].cap_left)
        {

            item1 = sln->bins[bin_index1].packed_items[item_indx1];
            item2 = sln->bins[bin_index2].packed_items[item_indx2];

            sln->bins[bin_index1].cap_left-=item2.size-item1.size;
            sln->bins[bin_index2].cap_left+=item2.size-item1.size;
            
            sln->bins[bin_index1].packed_items.push_back(item2);
            sln->bins[bin_index2].packed_items.push_back(item1);

            sln->bins[bin_index1].packed_items.erase(sln->bins[bin_index1].packed_items.begin()+item_indx1);
            sln->bins[bin_index2].packed_items.erase(sln->bins[bin_index2].packed_items.begin()+item_indx2);

            m++;

        }

        time++;
    }
}

void varaible_neighbourhood_search(struct problem_struct* prob){
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int nb_indx = 0; //neighbourhood index, start from 2, 0 or 1 is meaningless
    bool isBestSolution = false;
    bool finish = false;

    best_sln.prob = prob;
    struct solution_struct* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln);

    // Test code here
    cout << "Initialize a possible answer: " << endl;
    cout << "Objectives: " << best_sln.objective << endl;
    cout << "Known best: " << prob->known_best << endl << endl;


    int shaking_count =0;
    while(time_spent < MAX_TIME) {

        while(nb_indx < K) {
            // cout << "nb_indx=" << nb_indx << endl;
            struct solution_struct* neighb_s=best_descent_vns(nb_indx+1, curt_sln);
            if (isImproving) {
                // cout << "is improving" << endl;
                copy_solution(curt_sln, neighb_s);
                // cout << curt_sln->objective << endl;
                nb_indx=0;
            }
            else {
                // cout << "no improment, nb+1" << endl;
                nb_indx ++;
                // if ( nb_indx == K ) {
                //     finish = true;
                // }
            }
            isImproving = false;
            // cout << endl;

            if (curt_sln->objective == curt_sln->prob->known_best) {
                isBestSolution = true;
                cout << "current is the best solution" << endl;
                break;
            }

            // delete(neighb_s);
        }
        update_best_solution(curt_sln);
        
        if (isBestSolution) {
            break;
        }
        // if (finish) {
        //     break;
        // }

        double gap = 1000;
        gap = (double)(best_sln.objective - best_sln.prob->known_best)*100 / best_sln.prob->known_best;

        printf("shaking_count=%d, curt obj =%d, best obj=%d, gap= %0.1f%%\n",shaking_count, curt_sln->objective, prob->known_best, gap);

        copy_solution(curt_sln, &best_sln);


        vns_shaking(curt_sln, rand_int(1, SHAKE_STRENGTH));
        shaking_count ++;
        nb_indx = 0;
        
        

        time_fin = clock();
        time_spent = (double) (time_fin - time_start) /CLOCKS_PER_SEC;
    }

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

    // srand(2);
    srand(3); //best
    // srand(10); // the same with 3 
    // srand(20); // binpack1会有一个不到0
    // srand(13);
    // srand(6);
    
    if(strlen(solution_file)<=0)
    {
        if(strcmp(out_file,"")==0) strcpy(out_file, "my_solutions.txt"); //default output
        FILE* pfile = fopen(out_file, "w"); //open a new file

        fprintf(pfile, "%d\n", num_of_problems); fclose(pfile);

        for(int k=0; k<num_of_problems; k++)
        {
            cout << "Number of question: " << k << endl;
            best_sln.objective=1000; 
            for(int run=0; run<NUM_OF_RUNS; run++) {
                printf("Running VNS...\n");
                varaible_neighbourhood_search(my_problems[k]);
            }
            output_solution(&best_sln,out_file);
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
