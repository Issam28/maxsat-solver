/*


BRIEF EXPLANATION OF NOTATION Every:


USED variable can be either 0 or 1. So a combination of variable assignments can be described by a bit vector.
And a bit vector can also describe an Integer. Hence, combinations of variable assignments can be described by ints.

Since we're going down a binary tree and looking variable by variable, we have to keep track of which variable we're looking at,
so that we change/evaluate the appropriate bit.

An example with 3 variables: (V_3 V_2 V_1)

<<<<<<< HEAD

                                                                         ____Start____
                                                                        /             \
                                                                 ______/               \______
                                                                /                             \
                                                               /                               \
                                             _________________/                                 \_________________
V1:                              (0: [- - 0])                                                                     (1: [- - 1])
                                  /        \                                                                       /        \
                                 /          \                                                                     /          \
                                /            \                                                                   /            \
                          _____/              \_____                                                        ____/              \_____
V2:            (0: [- 0 0])                         (2: [- 1 0])                                (1: [- 0 1])                         (3: [- 1 1]) 
                /    \                                /        \                                 /    \                                /     \
               /      \                              /          \                               /      \                              /       \
              /        \                            /            \                             /        \                            /         \
             /          \                          /              \                           /          \                          /           \
V3:  (0: [0 0 0])   (4: [1 0 0])          (2: [0 1 0])          (6: [1 1 0])        (1: [0 0 1])       (5: [1 0 1])       (3: [0 1 1])     (7: [1 1 1]) 


=======
															             ____Start____
																        /             \
															     ______/               \______
															    /                             \
														       /                               \
										     _________________/                                 \_________________
V1:								 (0: [- - 0])             				                                          (1: [- - 1])
								  /        \                                                                       /        \
								 /          \					                                                  /          \
								/            \                                                                   /            \
						  _____/              \_____                                                        ____/              \_____
V2:	    	  (0: [- 0 0])                          (2: [- 1 0])                                (1: [- 0 1])                         (3: [- 1 1]) 
				/    \                                /        \						         /    \                                /     \
			   /      \			                     /          \			   				    /      \				              /       \
			  /        \                            /            \			  				   /        \                            /         \
			 /          \                          /              \			   				  /          \                          /           \
V3:  (0: [0 0 0])   (4: [1 0 0])          (2: [0 1 0])          (6: [1 1 0])        (1: [0 0 1])       (5: [1 0 1])       (3: [0 1 1])     (7: [1 1 1]) 



>>>>>>> 8252f34e287b3b305b8e08470600db767fe37c15
As one can see, at each level the combinations are univocal. We just need to know what the order of the last bit we have to check,
which coincides with the variable's id.

As stated in the assignment, each variable is represented by an integer, whose absolute value is the variable's id. If the value is positive,
the variable is set to True, else it is set to Negative. Example: -3 means V_3 -> False. 
FOR THIS REASON, THE FIRST VARIABLE ID IS ONE, NOT ZERO. So in the bit array, bit_x refers to variable_(x+1).


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int prev_comb);
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int *cur_ass);
void parseFile(int* n_clauses, int* n_vars, int** clause_matrix, char* filename);
int get_bit(int decimal, int N);
int get_cur_comb(int cur_var, int prev_comb);
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int cur_comb);
void print_sol(int cur_sol, int n_vars);

int cur_maxsat=0;
int n_solutions=0;
int cur_sol=0;


void main(){
	int n_clauses, n_vars;
	int MAXSAT;
	int ** clause_matrix;

	parseFile(&n_clauses, &n_vars, clause_matrix, "name_of_the_file.txt");

	MAXSAT(n_clauses, n_vars, clause_matrix,  1, 0);
	MAXSAT(n_clauses, n_vars, clause_matrix, -1, 1);

	
	printf("%d %d\n", cur_maxsat, n_solutions);
	print_sol(cur_sol);

}


int MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int prev_comb){
	int n_clauses_satisfied;
	int next_var;
	int tru, fal;

	int cur_comb = get_cur_comb(cur_var, prev_comb); //knowing the current variable assignment and the previous combination,
													 //get the current combination

	if(cur_var<n_vars){ //we're not on the last variable -> we're not on a leaf
		n_clauses_satisfied = clauses_satisfied(n_clauses, clause_matrix, cur_var, cur_comb); //calculate number of clauses satisfied by current combination

		if(n_clauses - n_clauses_satisfied < cur_maxsat)
			return 0; //no need to go further, no better solution we'll be found -> suggestion from the project sheet

		else{ //continue going down the tree
			next_var = abs(cur_var)+1;

			tru = MAXSAT(n_clauses, n_vars, clause_matrix,  next_var, cur_comb); //branch with next var set to true
			fal = MAXSAT(n_clauses, n_vars, clause_matrix, -next_var, cur_comb); //branch with next var set to false
			
			if(tru>fal)
				return tru;
			else
				return fal;
		}
	}
	else{ //we're in a leaf
		if(n_clauses_satisfied > cur_maxsat){ //if this combination satisfies more clauses than the previous best...
			cur_sol = cur_comb; //store the solution
			cur_maxsat = n_clauses_satisfied; //update the best score
			n_solutions=1;
		}
		
		if(n_clauses_satisfied == cur_maxsat) //if this combination satisfies the same number of clauses as the current best, increase the number of solutions
			n_solutions++;	
		
		return n_clauses_satisfied;
	}
}


//to obtain the combination, we have to sum the bit of the new variable to the previous combination
int get_cur_comb(int cur_var, int prev_comb){

	if(cur_var>0) //if the variable is set to true
		return prev_comb + pow(2, abs(cur_var)-1)
	else //if the variable is set to false, its corresponding bit will be 0, so the value of the next combination is equal to the previous one
		return prev_comb
}


//for a given combination and current variable, calculate the number of clauses satisfied
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int cur_comb){
	int i, j, n_clauses_satisfied=0;

	for(i=0; i<n_clauses; i++) //for each clause
		for(j=0; j<abs(cur_var); j++){ //for each variable -> each variable is a bit of cur_ass
			
			if(clause_matrix[i][j]!=0){ //if clause contains the variable

				if( (get_bit(cur_comb,j) == 1 && clause_matrix[i][j] > 0) || (get_bit(cur_comb,j) == 0 && clause_matrix[i][j] < 0) ){ //if the variable corresponds
					n_clauses_satisfied++;
					break; //only one variable needs to match for the clause to be satisfied
				}
			}
		}
	
	return n_clauses_satisfied;
}

//get nth bit of an integer. the order of the first bit is 0, because it corresponds to the weight 2^0.
int get_bit(int decimal, int N){
	int constant = 1 << (N);

	if( decimal & constant )
		return 1;
	else
		return 0;
}

void print_sol(int cur_sol, int n_vars);
	int i;
	for(i=0; i<n_vars; i++){
		if(get_bit(array, i)==1)
			printf("%d ", i+1);
		else
			printf("%d ", -i-1);
	}
	printf("\n");
}