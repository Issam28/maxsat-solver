#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int* cur_ass, int cur_maxsat, int* sol, int* n_solutions);
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int *cur_ass);
int getVarIdx(int var);
void parseFile(int* n_clauses, int* n_vars, int** clause_matrix, char* filename);




void main(){
	int n_clauses, n_vars;
	int MAXSAT;
	int ** clause_matrix;

	parseFile(&n_clauses, &n_vars, clause_matrix, "name_of_the_file.txt");
}




int MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int* cur_ass, int cur_maxsat, int* sol, int* n_solutions){
	int n_clauses_satisfied;
	int tru, fal;
	int *sol;
	int *n_solutions;

	cur_ass[getVarIdx(cur_var)] = cur_var;

	if(cur_var<n_vars-1){ //we're not on the last variable -> we're not on a leaf
		n_clauses_satisfied = clauses_satisfied(n_clausses, clause_matrix, cur_var, cur_ass);
		if(n_clauses - n_clauses_satisfied < cur_maxsat){
			//no need to go further, no better solution we'll be found
		}
		else{ //continue the tree
			tru = MAXSAT(n_clauses, n_vars, clause_matrix, cur_var+1, cur_ass, cur_maxsat);
			fal = MAXSAT(n_clauses, n_vars, clause_matrix, -cur_var-1, cur_ass, cur-maxsat);
			if(tru>fal)
				return tru;
			else
				return fal;
		}
	}
	else{ //we're in a leaf

	}
}

int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int *cur_ass){
	int i, j, n_clauses_satisfied=0;

	for(i=0; i<n_clauses; i++)
		for(j=0; j<=getVarIdx(cur_var); j++)
			if(cur_ass[j]!=0)
				if(cur_ass[j]==clause_matrix[i][j]){
					n_clauses_satisfied++;
					break; //only one variable needs to match for the clause to be satisfied
				}
	
	return n_clauses_satisfied;
}



int getVarIdx(int var){
	return var/abs(var) - 1; //arrays go from 0 to size-1. Var can be a positive or negative integer, never 0.
}