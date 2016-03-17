#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int* cur_ass, int cur_maxsat, int* sol, int* n_solutions);
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int *cur_ass);
void parseFile(int* n_clauses, int* n_vars, int** clause_matrix, char* filename);

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

}


//get nth bit of an integer
int get_bit(int decimal, int N){
	int constant = 1 << (N-1);

	if( decimal & constant )
		return 1;
	else
		return 0;
}


//each combination can be described by a bit array. a bit array can be described by an integer.
int get_cur_comb(int cur_var, int prev_comb){
	if(cur_var<0)
		return prev_comb << 1
	else
		return (prev_comb << 1) + 1
}



int MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int prev_comb){
	int n_clauses_satisfied;
	int next_var;
	int tru, fal;

	int cur_comb = get_cur_comb(cur_var, prev_comb);

	if(cur_var<n_vars){ //we're not on the last variable -> we're not on a leaf
		n_clauses_satisfied = clauses_satisfied(n_clauses, clause_matrix, cur_var, cur_comb);
		if(n_clauses - n_clauses_satisfied < cur_maxsat){
			return 0; //no need to go further, no better solution we'll be found
		}
		else{ //continue the tree
			next_var = cur_var+1;

			tru = MAXSAT(n_clauses, n_vars, clause_matrix,  next_var, cur_comb); //branch with next var set to true
			fal = MAXSAT(n_clauses, n_vars, clause_matrix, -next_var, cur_comb); //branch with next var set to false
			
			if(tru>fal)
				return tru;
			else
				return fal;
		}
	}
	else{ //we're in a leaf
		if(n_clauses_satisfied > cur_maxsat){
			cur_sol = cur_comb;
			cur_maxsat = n_clauses_satisfied;
			n_solutions=1;
		}
		
		if(n_clauses_satisfied == cur_maxsat)
			n_solutions++;	
		
		return n_clauses_satisfied;
	}
}


int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int cur_comb){
	int i, j, n_clauses_satisfied=0;

	for(i=0; i<n_clauses; i++) //for each clause
		for(j=1; j<=abs(cur_var); j++){ //for each variable -> each variable is a bit of cur_ass
			
			if( (get_bit(cur_comb,j) == 1 && clause_matrix[i][j] > 0) || (get_bit(cur_comb,j) == 0 && clause_matrix[i][j] < 0) ){ //if the variable corresponds
				n_clauses_satisfied++;
				break; //only one variable needs to match for the clause to be satisfied
			}
		}
	
	return n_clauses_satisfied;
}