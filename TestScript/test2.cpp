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
int K = 2;                       // k is used to denote how many neighborhoods would be used
int SHAKE_STRENGTH = 12;         // the strength of shaking
struct solution_struct best_sln; // global best solution
int NUM_OF_RUNS = 1;             // number of runs
int MAX_TIME = 30;               // max amount of time permited (in sec)
int num_of_problems;             // number of problems

/*
 * This is a global boolean that is used to tell if the the current neighborhood's local search 
 * have improvement. Because though it is improving, the objective would probably not drop. So
 * can not express whether it is an improvement just through changes on objective.
*/
bool isImproving;                

// Sort the items in descending order.
int cmpfunc(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return -1;
    if(item1->size<item2->size) return 1;
    return 0;
}

// Return a random nunber ranging from min to max (inclusive).
int rand_int(int min, int max)
{
    int div = max-min+1;
    int val =rand() % div + min;
    //printf("rand_range= %d \n", val);
    return val;
}

// According to the given parameter, initialize and return a new problem.
void init_problem(char* name, int n, int capacities, int known_best, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = (struct problem_struct*) malloc(sizeof(struct problem_struct));
    strcpy(new_prob->name, name);

    new_prob->n=n; new_prob->capacities=capacities; new_prob->known_best=known_best;
    new_prob->items= (struct item_struct*) malloc(sizeof(struct item_struct)*n);
    *my_prob = new_prob;
}

// Free the problem and its space.
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

// Copy a solution from another solution.
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

// Update global best solution according to current solution.
void update_best_solution(struct solution_struct* sln)
{
    if(best_sln.objective >= sln->objective) { 
        copy_solution(&best_sln, sln);
    }
}

// Load problem from data file.
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

        // Initialize problem
        init_problem(name, n, capacities, known_best, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].index=j;
            fscanf(pfile, "%d", &(my_problems[k]->items[j].size)); //read profit data
        }
    }
    fclose(pfile); //close file

    return my_problems;
}

// Output a given solution to a file.
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

/*
 * This method tell whether in the provided bins, there have items allows 
 * specified swap. If there exists, store the item index in curt_move
 * and return the fitness value (how suitable is this swap).
*/ 
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

// Copy the current move to best move.
void copy_move(int* curt_move, int* best_move) {
    for (int i=0; i<6; i++) {
        best_move[i] = curt_move[i];
    }
}

/* 
 * According to the index of bins/items stored in best_move
 * apply specified swap on those bins/items
*/ 
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

    /* 
     * curt_move and best_move are arrays to store neighbourhood moves
     * to better understand this, you could view curt_move as:
     * [bin1_index, item1_index,  bin2_index, item2_index,  bin3_index, item3_index...]
    */ 
    int curt_move[] ={-1,-1, -1,-1, -1,-1, -1,-1, -1, -1}, best_move []={-1,-1, -1,-1, -1,-1, -1,-1,  -1,-1};
    int delta=0, best_delta=0;  
    
    bin_struct* bin1;
    bin_struct* bin2;
    bin_struct* bin3;
    bin_struct* bin4;

    item_struct item1, item2, item3, item4, item5;
    
    int binIndex1, binIndex2, binIndex3, itemIndex1, itemIndex2, itemIndex3, itemIndex4, itemIndex5;
    switch (nb_indx)
    {
        /*
         * The main idea of case 1 is to move an item from one bin to another bin.
         * Case 1 try to find one items and two bins, which satisfy condition that the 
         * item1 from bin1 could be directly put into bin2. The fitness value is 
         * item1's size, which is also the changed capacity on bin2
        */
        case 1: {
            // travel through all the bins
            for (int i=0; i<bins->size(); i++ ) { 
                // use a pointer to point to the current bin
                bin1 = &(*(bins->begin() + i));
                // if current bin is full
                if (bin1->cap_left == 0) continue;

                // travel through all the bins, start from bin1 + 1
                for (int j=i+1; j<bins->size(); j++ ) { 
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->cap_left == 0) continue;

                    // record the two bins selected
                    curt_move[0] = i;
                    curt_move[2] = j;

                    // calculate the possible delta in these two bins
                    delta = can_move(bins, &curt_move[0], nb_indx);

                    if (delta == -2) {
                        // this case means after swap the bin1's capacity left could be exactly 0
                        // so it would be executed immediately
                        isImproving = true;
                        apply_move(nb_indx, &curt_move[0], best_neighb);
                        return best_neighb;
                    }

                    if (delta > best_delta) {
                        // if have a better solution, update best_move
                        isImproving = true;
                        best_delta = delta;
                        copy_move(&curt_move[0], &best_move[0]);
                    }
                }
            }

            if (best_delta > 0) {
                // apply move 
                isImproving = true;
                apply_move(nb_indx, &best_move[0], best_neighb);
            }
            break;
        }

        /*
         * The main idea of case 2 is to swap two items in two bins.
         * Case 2 try to find two items from two bins, which satisfy condition that the 
         * item1 from bin1 could be replaced with item2 from bin2, in other words, swap two items. 
         * The fitness value is item2's size minus item1's size, it is also the changed 
         * capacity on bin1.
        */

        case 2: {
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

        /* 
         * NOTES:
         * For case 3 and 4, I do to not encapsulate the algorithm through methods can_move and 
         * apply_move. That is because I'm afraid that encapsulation may reduce the execution 
         * efficiency, though encapsulation dose make codes more human friendly.(Later I found 
         * these reduction on performace could be ignored) 
        */


        /*
         * Case 3 is actaully a conbination of 1-2 swap and 1-1-1 swap.
         *
         * 1-2 swap selects two bins and swap one item from bin1 with another two items from bin2.
         * Fitness value for 1-2 swap is item2.size + item3.size - item1's size, 
         * i.e. the capacity change on bin1.
         *
         * 1-1-1 swap is a little bit more complex. It takes three bins, bin1, bin2, and bin3. 
         * From bin1 it take out item1, from bin2 take out item2, and from bin3 take out item3. 
         * Then put the item2 and item3 into bin1, and put the item2 into bin1. The fitness 
         * value for 1-1-1 swap is the same with 1-2 swap, item2.size + item3.size - item1.size.
        */
        case 3: {
            
            for(int i=0; i<bins->size(); i++){
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left==0) { continue; }

                for (int j = i + 1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->cap_left==0) { continue; }
                
                    // here is a trick, let k start with j
                    // when k == j, it is actually searching for 2-1 swap
                    // when k > j, it is searching for 1-1-1 swap
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

                                    // make sure that these three items could swap, and its delta is greater than current best delta
                                    if (item1.size < item2.size + item3.size && item2.size + item3.size - item1.size <= bin1->cap_left && item2.size + item3.size-item1.size > best_delta){
                                        if (item1.size-item2.size <= bin2->cap_left
                                        || j==k && item1.size - item2.size - item3.size <= bin2->cap_left) {

                                            // store the bins/items index in best_move
                                            best_move[0] = i;
                                            best_move[2] = j;
                                            best_move[4] = k;

                                            best_move[1] = a;
                                            best_move[3] = b;
                                            best_move[5] = c;

                                            // update best fitness value
                                            best_delta = item2.size+item3.size-item1.size;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // update best neighbor to the neighbor which has the best fitness value so far
            if (best_delta > 0) {

                isImproving = true;

                bin1 = &(*(bins->begin() + best_move[0]));
                bin2 = &(*(bins->begin() + best_move[2]));
                bin3 = &(*(bins->begin() + best_move[4]));

                item1 = bin1->packed_items[best_move[1]];
                item2 = bin2->packed_items[best_move[3]];
                item3 = bin3->packed_items[best_move[5]];
                
                // update bins capacity
                // if is 2-1 swap, bin2 and bin3 denotes the same bin, it works as well
                bin1->cap_left -= item2.size + item3.size - item1.size;
                bin2->cap_left -= item1.size - item2.size;
                bin3->cap_left += item3.size;

                bin1->packed_items.erase(bin1->packed_items.begin() + best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[3]);

                // when erasing item2 and item3 from in bin2, the order of items should be taken into consideration
                if (best_move[2] == best_move[4] && best_move[3] < best_move[5])
                {
                    bin3->packed_items.erase(bin3->packed_items.begin()+best_move[5]-1);
                } else {
                    bin3->packed_items.erase(bin3->packed_items.begin()+best_move[5]);
                }

                // put the items into bins
                bin1->packed_items.push_back(item2);
                bin1->packed_items.push_back(item3);
                bin2->packed_items.push_back(item1);
                
                // if after swap bin3 contains no item, delete the bin
                if (bin3->packed_items.size() == 0)
                {
                    best_neighb->bins.erase(best_neighb->bins.begin() + best_move[4]);
                    best_neighb->objective--;
                }
            }
            break;
        }

        /*
         * The main idea of case 4 is 2-2 swap.
         * Pick two bins, and take out two items from each bin.
         * Check if two items from bin1 could swap with two items from bin2.
         * The fitness value is item3.size + item4.size - item1.size - item2.size, 
         * i.e. the capacity change on bin1.
        */
            
        case 4: {
            // pick two bins with free space
            for (int i = 0; i < bins->size(); i++) {
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left==0 || bin1->packed_items.size() < 2) { continue; }
                for (int j = i+1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->packed_items.size() < 2) { continue; }

                    // pick two items from each bin
                    for (int a = 0; a < bin1->packed_items.size(); a++) {
                        for (int b = a+1; b < bin1->packed_items.size(); b++) {
                            for (int c = 0; c < bin2->packed_items.size(); c++) {
                                for (int d = c+1; d < bin2->packed_items.size(); d++) {

                                    delta = bin2->packed_items[c].size+bin2->packed_items[d].size-bin1->packed_items[a].size-bin1->packed_items[b].size;

                                    if (delta > best_delta && delta <= bin1->cap_left) {
                                        
                                        // store the bins/items index in best_move
                                        best_move[0] = i;
                                        best_move[2] = j;

                                        best_move[1] = a;
                                        best_move[3] = b;
                                        best_move[5] = c;
                                        best_move[7] = d;

                                        // update best delta
                                        best_delta = bin2->packed_items[c].size+bin2->packed_items[d].size-bin1->packed_items[a].size-bin1->packed_items[b].size;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (best_delta > 0) {

                isImproving = true;

                bin1 = &(*(bins->begin() + best_move[0]));
                bin2 = &(*(bins->begin() + best_move[2]));

                item1 = bin1->packed_items[best_move[1]];
                item2 = bin1->packed_items[best_move[3]];
                item3 = bin2->packed_items[best_move[5]];
                item4 = bin2->packed_items[best_move[7]];

                // update bins capacity
                bin1->cap_left-=item4.size+item3.size-item1.size-item2.size;
                bin2->cap_left+=item4.size+item3.size-item1.size-item2.size;

                // erase items
                bin1->packed_items.erase(bin1->packed_items.begin()+best_move[3]);
                bin1->packed_items.erase(bin1->packed_items.begin()+best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[7]);
                bin2->packed_items.erase(bin2->packed_items.begin()+best_move[5]);
                
                // put items back to bins
                bin1->packed_items.push_back(item3);
                bin1->packed_items.push_back(item4);
                bin2->packed_items.push_back(item1);
                bin2->packed_items.push_back(item2);
            }
            break;
        }
        
             
        /*
         * The main idea of case 5 is 2-3 swap.
         * It selects two bins, from bin1 it take out two item and from bin2 it take out 
         * three items, then swap the three items with two items.
        */
        case 5:
            for (int i = 0; i < bins->size(); i++) {
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left == 0) { continue; }

                for (int j = i+1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->packed_items.size() < 3) { continue; }

                    for (int a = 0; a < bin1->packed_items.size(); a++) {
                        for (int b = 0; b < bin1->packed_items.size(); b++) {
                            for (int c = 0; c < bin2->packed_items.size(); c++) {
                                for (int d = c+1; d < bin2->packed_items.size(); d++) {
                                    for (int e = d+1; e < bin2->packed_items.size(); e++) {

                                        item1 = bin1->packed_items[a];
                                        item2 = bin2->packed_items[b];

                                        item3 = bin2->packed_items[c];
                                        item4 = bin2->packed_items[d];
                                        item5 = bin2->packed_items[e];

                                        delta = item3.size + item4.size + item5.size - item1.size - item2.size;
                                        
                                        if (delta > best_delta && delta <= bin1->cap_left)
                                        {
                                            best_delta = delta;

                                            best_move[0] = i;
                                            best_move[2] = j;

                                            best_move[1] = a;
                                            best_move[3] = b;
                                            best_move[5] = c;
                                            best_move[7] = d;
                                            best_move[9] = e;
                                        }
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
                item2 = bin2->packed_items[best_move[3]];
                item3 = bin2->packed_items[best_move[5]];
                item4 = bin2->packed_items[best_move[7]];
                item5 = bin2->packed_items[best_move[9]];

                bin1->cap_left -= item3.size + item4.size + item5.size - item1.size - item2.size; 
                bin2->cap_left += item3.size + item4.size + item5.size - item1.size - item2.size; 

                bin1->packed_items.erase(bin1->packed_items.begin() + best_move[3]);
                bin1->packed_items.erase(bin1->packed_items.begin() + best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[9]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[7]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[5]);
                
                bin1->packed_items.push_back(item3);
                bin1->packed_items.push_back(item4);
                bin1->packed_items.push_back(item5);
                bin2->packed_items.push_back(item1);
                bin2->packed_items.push_back(item2);

            }
            break;
            
        /*
         * The main idea of case 6 is 1-3 swap.
         * It selects two bins, from bin1 it take out one item and from bin2 it take out 
         * three items, then swap the three items with one item.
        */
        case 6:
            for (int i = 0; i < bins->size(); i++) {
                bin1 = &(*(bins->begin() + i));
                if (bin1->cap_left == 0) { continue; }

                for (int j = i+1; j < bins->size(); j++) {
                    bin2 = &(*(bins->begin() + j));
                    if (bin2->packed_items.size() < 3) { continue; }

                    for (int a = 0; a < bin1->packed_items.size(); a++) {
                        for (int c = 0; c < bin2->packed_items.size(); c++) {
                            for (int d = c+1; d < bin2->packed_items.size(); d++) {
                                for (int e = d+1; e < bin2->packed_items.size(); e++) {
                                    item1 = bin1->packed_items[a];

                                    item2 = bin2->packed_items[c];
                                    item3 = bin2->packed_items[d];
                                    item4 = bin2->packed_items[e];

                                    delta = item2.size + item3.size + item4.size - item1.size;
                                    
                                    if (delta > best_delta && delta <= bin1->cap_left)
                                    {
                                        best_delta = delta;

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
                    }
                }
            }
            if (best_delta>0)
            {
                isImproving = true; 

                bin1 = &(*(bins->begin() + best_move[0]));
                bin2 = &(*(bins->begin() + best_move[2]));

                item1 = bin1->packed_items[best_move[1]];
                item2 = bin2->packed_items[best_move[3]];
                item3 = bin2->packed_items[best_move[5]];
                item4 = bin2->packed_items[best_move[7]];

                bin1->cap_left -= item2.size + item3.size +item4.size - item1.size; 
                bin2->cap_left += item2.size + item3.size +item4.size - item1.size; 

                bin1->packed_items.erase(bin1->packed_items.begin() + best_move[1]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[7]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[5]);
                bin2->packed_items.erase(bin2->packed_items.begin() + best_move[3]);
                
                bin1->packed_items.push_back(item2);
                bin1->packed_items.push_back(item3);
                bin1->packed_items.push_back(item4);
                bin2->packed_items.push_back(item1);

            }
            break;
    }
    return best_neighb;
}


 /* 
  * Use best fit algorithm to get the initial solution. Every time 
  * to pack an item, find the bins that could contain it, and select the 
  * bin with the minimum capacity left. If not find a bin could contain 
  * the item, then create a new bin, and pack the item
 */
struct solution_struct* greedy_heuristic (struct problem_struct* prob) {
    int n = prob->n;                            // unpacked items number
    int cap = prob->capacities;                 // capacities of each bin
    struct item_struct* items = prob->items;    // all items of the problem
    int bin_num = 1;                            // number of bin currently created

    // initialize the solution
    struct solution_struct* sln = new solution_struct(); 
    sln->prob = prob;

    // sort the items in descending order
    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc); 

    // initialize a bin, and push it into vector
    bin_struct bin;
    bin.cap_left = cap;
    sln->bins.push_back(bin);

    // travel through all the items
    for (int i=0; i<n; i++) {
        int index = 0, target_index = -1; int left_cap = cap+1;

        // travel through all the bins
        while(index < bin_num) {
            // if the item could be 
            if (sln->bins[index].cap_left >= prob->items[i].size && left_cap > sln->bins[index].cap_left) {
                target_index = index;
                left_cap = sln->bins[index].cap_left;
            }
            index ++;
        }

        if (target_index==-1) {
            // if the target_index is not updated, which means do not find a suitable bin
            // then create a new bin and store the item into that bin
            bin_struct new_bin;
            new_bin.packed_items.push_back(prob->items[i]);
            new_bin.cap_left = cap-prob->items[i].size;
            sln->bins.push_back(new_bin);
            bin_num ++;
            
        } else {
            // if is updated, push the item into the target bin
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

        // make sure that the items could swap
        if (sln->bins[bin_index2].packed_items[item_indx2].size - sln->bins[bin_index1].packed_items[item_indx1].size <= sln->bins[bin_index1].cap_left
            && sln->bins[bin_index1].packed_items[item_indx1].size-sln->bins[bin_index2].packed_items[item_indx2].size <= sln->bins[bin_index2].cap_left)
        {

            // temporarily store the items
            item1 = sln->bins[bin_index1].packed_items[item_indx1];
            item2 = sln->bins[bin_index2].packed_items[item_indx2];

            // firstly update the bins left capacity
            sln->bins[bin_index1].cap_left -= item2.size-item1.size;
            sln->bins[bin_index2].cap_left += item2.size-item1.size;
            
            // then put the items into the bins
            sln->bins[bin_index1].packed_items.push_back(item2);
            sln->bins[bin_index2].packed_items.push_back(item1);

            // finally delete the items from the bins
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
    int nb_indx = 0;                // neighbourhood index
    bool isBestSolution = false;    // this is to denote if the current solution is already best

    best_sln.prob = prob;           

    // get an initial solution using greedy heuristic
    struct solution_struct* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln); // update best solution

    // Test code here
    cout << "Initialize a possible answer: " << endl;
    cout << "Objectives: " << best_sln.objective << endl;
    cout << "Known best: " << prob->known_best << endl << endl;

    int shaking_count =0;
    while(time_spent < MAX_TIME) {

        while(nb_indx < K) {
            isImproving = false;

            // search for a better solution in current neighborhood
            struct solution_struct* neighb_s=best_descent_vns(nb_indx+1, curt_sln);

            if (isImproving) {
                // if have improvement, update current solution, and restart vns
                copy_solution(curt_sln, neighb_s);
                nb_indx=0;
            }
            else {
                // if have no improvement, search next neighborhood
                nb_indx ++;
            }

            // if the current objective already equal to best obejective, then stop 
            if (curt_sln->objective == curt_sln->prob->known_best) {
                isBestSolution = true;
                cout << "current is the best solution" << endl;
                break;
            }
        }

        update_best_solution(curt_sln);         // update best solution
        
        if (isBestSolution) {
            break;
        }

        /* Diversification part */
        double gap = 1000;
        gap = (double)(best_sln.objective - best_sln.prob->known_best)*100 / best_sln.prob->known_best;

        printf("shaking_count=%d, curt obj =%d, known best obj=%d, gap= %0.1f%%\n",shaking_count, curt_sln->objective, prob->known_best, gap);

        copy_solution(curt_sln, &best_sln);
        vns_shaking(curt_sln, SHAKE_STRENGTH);  // start shaking
        shaking_count ++;                       // shaking count + 1
        nb_indx = 0;                            // restart vns from neighbor 1 

        /* 
         * if already shaked for 1000 times, it would probably have no improvement for 
         * later shaking, so just stop it
        */
        if (shaking_count > 1000) { 
            break;
        }

        time_fin = clock();                     // record the time spent
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
    // srand(3); //best, medium have 4
    // srand(10); //best, medium have 4
    // srand(20); // not bad, but would have 1 for binpack1, sometimes, binpack3 would be 3, mostly 4
    // srand(30); // not bad, but would have 1 for binpack1, sometimes, binpack3 would be 3, mostly 4
    srand(100); // not bad, but would have 1 for binpack1, sometimes, binpack3 would be 3, mostly 4
    
    
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
        free_problem(my_problems[k]); //free problem data memory
    }

    free(my_problems); //free problems array
    
    return 0;
}
