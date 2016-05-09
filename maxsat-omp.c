#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>


void MAXSAT(int cur_var, int* cur_comb, int root, int* sat_clauses);
int** parseFile(char name_of_the_file[40]);
int* get_cur_comb(int cur_var, int* prev_comb);
int* alloc_int_array(int n);
int clauses_satisfied(int cur_var, int* cur_comb, int* n_clauses_unsatisfied, int* sat_clauses);
void print_sol();
void free_matrix();
void copy_array(int* dest, int* src);
int are_there_idle_threads(int cur_lvl);
int* get_cur_sat_clauses(int cur_var, int* prev_sat_clauses);


int cur_maxsat=0; 			//contains current best score
int n_solutions=0; 			//contains number of solutions that reach the best score
int* cur_sol; 				//contains one combination that reaches the best score
int* thread_status;			//vector of thread status. each thread can be idle(0) or active(1)
int n_threads;				//number of threads
int n_clauses;				//number of clauses
int n_vars;					//number of variables
int** clause_matrix;		//matrix with all the clauses to be tested

int main(int argc, char** argv){

	n_threads = omp_get_max_threads();

	if(argc!=2){
		printf("Usage: maxsat-omp input-file.in");
		exit(1);
	}

	thread_status = (int *) malloc(n_threads*sizeof(int));
	for(int i=0; i<n_threads; i++)
		thread_status[i]=0;

	clause_matrix = parseFile(argv[1]);

	//allocate needed arrays
	int *first_comb = alloc_int_array(n_vars);
	first_comb[0]=1;

	int *first_comb2 = alloc_int_array(n_vars);
	first_comb2[0]=-1;

	int* sat_clauses_1 = alloc_int_array(n_clauses);
	int* sat_clauses_2 = alloc_int_array(n_clauses);

	cur_sol = alloc_int_array(n_vars);


	//root node of the binary tree.
	#pragma omp parallel
	{
		#pragma omp single nowait
		{
			//here the two children nodes are spawned. if there are avaiable threads, a task is created.
			//otherwise, this thread will process both nodes
			if(n_threads>1){
				#pragma omp task
				{
					MAXSAT(-1, first_comb2, 1, sat_clauses_2); //branch with first variable set to false
				}
			}
			else{
				MAXSAT(-1, first_comb2, 1, sat_clauses_2);
			}

			MAXSAT(1, first_comb, 1, sat_clauses_1); //branch with first variable set to true
		}
	}

	free(thread_status);


	printf("%d %d\n", cur_maxsat, n_solutions);
	print_sol(cur_sol, n_vars);


	free_matrix();
	free(cur_sol);
	exit(0);
}

void MAXSAT(int cur_var, int* cur_comb, int root, int* sat_clauses){
	int n_clauses_satisfied;
	int n_clauses_unsatisfied=0;
	int next_var;
	int tid = omp_get_thread_num();
	int* next_comb, *next_sat_clauses;
	int* next_comb2, *next_sat_clauses2;
	#pragma atomic write
		thread_status[tid] = abs(cur_var);

	n_clauses_satisfied = clauses_satisfied(cur_var, cur_comb, &n_clauses_unsatisfied, sat_clauses); //calculate number of clauses satisfied and unsatisfied by current combination

	if(abs(cur_var)<n_vars){ //we're not on the last variable -> we're not on a leaf

		//prune condition
		if(n_clauses - n_clauses_unsatisfied < cur_maxsat){
		//no need to go further, no better solution we'll be found -> suggestion from the project sheet
			#pragma atomic write
					thread_status[tid]=0;

			free(cur_comb);
			free(sat_clauses);
			return;
		}
		else{
			//continue going down the tree
			next_var = abs(cur_var)+1;
			next_comb = get_cur_comb(-next_var, cur_comb);
			next_comb2 = get_cur_comb(next_var, cur_comb);
			next_sat_clauses = get_cur_sat_clauses(-next_var, sat_clauses);
			next_sat_clauses2 = get_cur_sat_clauses(next_var, sat_clauses);

			//here the two children nodes are spawned. if there are avaiable threads, a task is created.
			//otherwise, this thread will process both nodes
			if(are_there_idle_threads(abs(cur_var))){
				#pragma omp task
				{
					MAXSAT(-next_var, next_comb, 1, next_sat_clauses); //branch with next var set to false and "root" set to true.
												 //this means the thread will return up until here and then become idle again.
				}
			}
			else
				MAXSAT(-next_var, next_comb, 0, next_sat_clauses); //branch with next var set to false


			MAXSAT(next_var, next_comb2, 0, next_sat_clauses2); //branch with next var set to true


			if(root){ //if this is a root node, the thread becomes idle here. otherwise, it still has to return until its root node
				#pragma atomic write
					thread_status[tid]=0;
			}

			return;
		}
	}
	else{ //we're in a leaf
		#pragma omp critical (solutions)
		{
			if(n_clauses_satisfied == cur_maxsat){ //if this combination satisfies the same number of clauses as the current best, increase the number of solutions
				n_solutions++;
			}
			else{
				if(n_clauses_satisfied > cur_maxsat){ //if this combination satisfies more clauses than the previous best...
					copy_array(cur_sol,cur_comb); //store the solution
					cur_maxsat = n_clauses_satisfied; //update the best score
					n_solutions=1;
				}
			}
		}

		if(root){ //if this is a root node, the thread becomes idle here. otherwise, it still has to return until its root node
			#pragma atomic write
				thread_status[tid]=0;
		}
		free(cur_comb);
		free(sat_clauses);
		return;
	}
}












// ##################################################################################################################################
// ##################################################################################################################################
// ##################################################################################################################################
// ##################################################################################################################################
// ##################################################################################################################################
// ##################################################################################################################################
// ##################################################################################################################################





// Utility functions down there


int* get_cur_sat_clauses(int cur_var, int* prev_sat_clauses){
	int* cur_sat_clauses;

	if(cur_var<0)
		cur_sat_clauses = prev_sat_clauses;
	else{
		cur_sat_clauses = (int *) malloc(n_clauses*sizeof(int));
		for(int i=0; i<n_clauses; i++)
			cur_sat_clauses[i]=prev_sat_clauses[i];
	}

	return cur_sat_clauses;
}


int are_there_idle_threads(int cur_lvl){
	int idles = 0;
	int max = 0;
	int tid = omp_get_thread_num();

	for(int i=0; i<n_threads; i++){
		if(i==tid)
			continue;

		if(thread_status[i]==0)
			idles = 1;
		else{
			if(thread_status[i]>=max)
				max = thread_status[i];
		}
	}

	return (cur_lvl>=max);
}

void copy_array(int* dest, int* src){
	for(int i=0; i<n_vars; i++){
		dest[i]=src[i];
	}
}



int* alloc_int_array(int n){
	int* first_comb = (int *) malloc(n*sizeof(int));

	for(int i=0; i<n; i++)
		first_comb[i]=0;

	return first_comb;
}


//to obtain the combination, we copy the previous combination and assign the current variable accordingly. actually we only copy half of the time. half of the children reuse the parent's array, the other's copy it.
int* get_cur_comb(int cur_var, int* prev_comb){
	int* cur_comb;

	if(cur_var<0){
		cur_comb = (int *) malloc(n_vars*sizeof(int));
		copy_array(cur_comb, prev_comb);
	}
	else
		cur_comb=prev_comb;

	cur_comb[abs(cur_var)-1]=cur_var;

	return cur_comb;
}

//for a given combination and current variable, calculate the number of clauses satisfied (and unsatisfied)
int clauses_satisfied(int cur_var, int* cur_comb, int* unsatisfied, int* sat_clauses){
	int i, j, n_clauses_satisfied=0;
	int unsat = 0;
	int var;

	for(i=0; i<n_clauses; i++){ //for each clause
		if(sat_clauses[i]==-1){
			*unsatisfied = *unsatisfied + 1;
			unsat=0;
			continue;
		}
		if(sat_clauses[i]==1){
			n_clauses_satisfied++;
			unsat=0;
			continue;
		}

		for(j=0; j<20; j++){ //for each variable
			var = abs(clause_matrix[i][j]);

			if(var==0 || var>abs(cur_var)) // end of clause or we don't know the next variable assignments
				break;

			if(cur_comb[var-1] == clause_matrix[i][j]){ //if the variable corresponds
				sat_clauses[i]=1;
				n_clauses_satisfied++;
				unsat=0;
				break; //only one variable needs to match for the clause to be satisfied
			}
			else
				unsat=1;
		}

		if(unsat==1 && clause_matrix[i][j]==0){
			sat_clauses[i]=-1;
			*unsatisfied = *unsatisfied + 1;
			unsat=0;
		}
	}

	return n_clauses_satisfied;
}

void print_sol(){
	int i;
	for(i=0; i<n_vars; i++){
		printf("%d ", cur_sol[i]);
		fflush(stdout);
	}

	printf("\n");
}

void free_matrix(){
	int i;
	for(i=0; i<n_clauses; i++)
		free(clause_matrix[i]);

	free(clause_matrix);
}

int **parseFile(char name_of_the_file[40]){
	int n,j,i = 0;
  	char line[80];
   	int matrix_line = 0, matrix_column = 0;
   	int field = -1;
	FILE *fr;
	char *start;
	int ** clause_matrix;
   	fr = fopen (name_of_the_file, "rt");  /* open the file for reading */

   	/*Read n_vars and n_clauses*/
	fgets(line,80,fr);
	sscanf(line, "%d" "%d", &n_vars, &n_clauses);
	//printf("N_vars =  %d  N_clauses = %d \n",*n_vars,*n_clauses);



    clause_matrix = (int**)malloc( n_clauses * sizeof( int* ));

    for (i = 0; i < n_clauses; i++){
    	clause_matrix[i] = (int *)malloc(20 * sizeof(int));
 	}

 	for (i = 0; i < n_clauses; i++){
 		for (j = 0; j < 20; j++)
 		{
 			clause_matrix[i][j] = 0;
 		}
 	}
 	//printf("-------clauses---------------\n");
   	while(fgets(line, 80, fr) != NULL){
   		start = line;

   		while (sscanf(start, "%d%n", &field, &n) == 1 && field != 0) {
   			clause_matrix[matrix_line][matrix_column] = field;
        	//printf("%d ", clause_matrix[matrix_line][matrix_column]);
        	start += n;
        	matrix_column ++;
    	}

    //printf("\n");
    matrix_column= 0;
    matrix_line ++;
   	}
   	//printf("----------------------------\n" );

   	fclose(fr);

   	return clause_matrix;
}
