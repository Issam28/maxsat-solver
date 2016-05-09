#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

int masterProc(int argc, char** argv );
int slaveProc();
int* MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int* prev_comb, int* cur_sol, int* cur_maxsat, int* n_solutions);
int** parseFile(int * n_clauses, int * n_vars, char name_of_the_file[40]);
int* get_cur_comb(int cur_var, int* prev_comb, int n_vars);
int* get_first_comb(int n_vars);
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int* cur_comb, int* n_clauses_unsatisfied);
void print_sol(int* cur_sol, int n_vars);
void free_matrix(int **clause_matrix, int n_clauses);
int** BCastClauseMat(int **clause_matrix, int* n_vars, int* n_clauses);
void sendProblemParams(int* cur_comb, int cur_var, int n_vars, int proc);
int* recvProblemParams(int* cur_var, int n_vars);
int* distributeNodes(int depth, int cur_var, int* prev_comb, int n_vars, int* idle, int* to_see, int* master_var);

int main(int argc, char** argv ){
    int rank, size;
    double t1,t2;
    MPI_Comm new_comm;

    if(argc!=2){
  		printf("Usage: maxsat-serial input-file.in");
  		exit(1);
  	}

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    MPI_Barrier(MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    if (rank == 0){
			masterProc(argc, argv );
    }
    else{
			slaveProc();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    t2=MPI_Wtime();
    MPI_Finalize( );


    if(rank==0)
      printf("Time elapsed: %f\n", t2-t1);

    return 0;
}

/*
Ran by the master processor.
	->Handles the results from the remaining processors
*/
int masterProc(int argc, char** argv ){
    MPI_Status status;
    int **clause_matrix;
		char buf[1024];
		int size;
    int n_clauses, n_vars;
    int cur_maxsat=0;
    int n_solutions=0;
    int* cur_sol;
    int* final_sol;
    int* first_comb = get_first_comb(n_vars);
    int* cur_comb;
    int idle = 0; //number of processors to which the master didn't send work yet
    int depth;
    int cur_var;
    int to_see;
    int code;

    int temp_n_sols, temp_maxsat;


    clause_matrix = parseFile(&n_clauses, &n_vars, argv[1]);
		BCastClauseMat(clause_matrix, &n_vars, &n_clauses);
    final_sol = (int*) malloc(sizeof(int)*n_vars);
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    idle=size-1;
//    printf("Idle: %d\n", idle);
    depth = (int) floor(log2(size));
//    printf("Depth: %d\n", depth);
    to_see = pow(2,depth);
//    printf("To_see: %d\n", to_see);

    cur_comb = distributeNodes(depth, 0, first_comb, n_vars, &idle, &to_see, &cur_var);
    if(cur_comb!=NULL){
      cur_sol = MAXSAT(n_clauses, n_vars, clause_matrix, abs(cur_var)+1, cur_comb, cur_sol, &cur_maxsat, &n_solutions);
      cur_sol = MAXSAT(n_clauses, n_vars, clause_matrix, -abs(cur_var)-1, cur_comb, cur_sol, &cur_maxsat, &n_solutions);
    }


    for(int i=1; i<size; i++){
        MPI_Recv(&temp_maxsat, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
        if(temp_maxsat==cur_maxsat){
          code=0; //signal slave to send only the number of solutions
          MPI_Send(&code, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Recv(&temp_n_sols, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
          n_solutions=n_solutions+temp_n_sols;
        }else{
          if(temp_maxsat>cur_maxsat){
//          printf("slave %d has a better solution.\n", i);
            code=1; //signal slave to send the number of solutions and one example solution
            MPI_Send(&code, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Recv(&temp_n_sols, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            n_solutions = temp_n_sols;
            cur_maxsat = temp_maxsat;
            MPI_Recv(cur_sol, n_vars, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
          }
          else{
            code=-1; //signal slave to terminate and don't send anything more.
            MPI_Send(&code, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          }
        }
    }
    printf("%d %d\n", cur_maxsat, n_solutions);
  	print_sol(cur_sol, n_vars);
}

/*Ran by the slave processors.
	->Solves part of the problem and sends result to master processor
	->Asks for work after finishing a task(???)
*/
int slaveProc(){
    MPI_Status status;
		int** clause_matrix;
    int  rank;
    int cur_maxsat=0;
    int n_solutions=0;
    int* cur_sol;
    int* cur_comb;
    int n_vars, n_clauses;
    int cur_var;
    int code;

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    clause_matrix = BCastClauseMat(clause_matrix,&n_vars,&n_clauses); //transmit n_vars, n_clauses and the clause_matrix to every node
    cur_comb = recvProblemParams(&cur_var, n_vars);

    cur_sol = MAXSAT(n_clauses, n_vars, clause_matrix, abs(cur_var)+1, cur_comb, cur_sol, &cur_maxsat, &n_solutions);
    cur_sol = MAXSAT(n_clauses, n_vars, clause_matrix, -abs(cur_var)-1, cur_comb, cur_sol, &cur_maxsat, &n_solutions);


    //send results to master
    MPI_Send(&cur_maxsat, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    switch(code){
      case -1:
        break;
      case 0:
        MPI_Send(&n_solutions, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        break;
      case 1:
        MPI_Send(&n_solutions, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(cur_sol, n_vars, MPI_INT, 0, 0, MPI_COMM_WORLD);
        break;
    }

    /*if(rank==1){
      printf("%d %d\n", cur_maxsat, n_solutions);
      print_sol(cur_sol, n_vars);
      printf("\n");
    }*/
		return 0;
}

int** BCastClauseMat(int **clause_matrix, int* n_vars, int* n_clauses){
  int rank;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  MPI_Bcast(n_vars, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(n_clauses, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if(rank!=0){
    clause_matrix = (int**) malloc(*n_clauses * sizeof(int*));
    for(int i=0; i<*n_clauses; i++)
      clause_matrix[i]= (int*) malloc(*n_vars * sizeof(int));
  }

	for(int i=0; i< *n_clauses; i++)
		MPI_Bcast(clause_matrix[i], *n_vars, MPI_INT, 0, MPI_COMM_WORLD); //master sends the matrix, line by line, slaves receive it

  return clause_matrix;
}


int* distributeNodes(int depth, int cur_var, int* prev_comb, int n_vars, int* idle, int* to_see, int* master_var){
    int* cur_comb = get_cur_comb(cur_var, prev_comb, n_vars);
    int* cur_comb2, *cur_comb3;
    int* aux = NULL;
    if(abs(cur_var)<depth && abs(cur_var)<n_vars){
      aux = distributeNodes(depth, abs(cur_var)+1, cur_comb, n_vars, idle, to_see, master_var);
      aux = distributeNodes(depth, -abs(cur_var)-1, cur_comb, n_vars, idle, to_see, master_var);
    }
    else{
      if(*to_see < (*idle+1)){ //if there are more processors to "hang" than nodes on this level, "hang" two processors in each of this node's children
        cur_comb2=get_cur_comb(abs(cur_var)+1, cur_comb, n_vars);
        cur_comb3=get_cur_comb(-abs(cur_var)-1, cur_comb, n_vars);
        sendProblemParams(cur_comb2, abs(cur_var)+1, n_vars, *idle);
        (*idle)=(*idle)-1;
        if(*idle>0){
          sendProblemParams(cur_comb2, abs(cur_var)+1, n_vars, *idle);
          (*idle)=(*idle)-1;
        }
        else{
          *master_var = cur_var;
          aux = cur_comb;
        }
      }
      else{ //if there is one node per processor, "hang" one processor in this node
        if(*idle>0){ //if this node isn't the master
          sendProblemParams(cur_comb, cur_var, n_vars, *idle);
          (*idle)=(*idle)-1;
          free(cur_comb);
        }
        else{
          *master_var = cur_var;
          aux = cur_comb;
        }
      }
      (*to_see)=(*to_see)-1;
    }
    return aux;
}


void sendProblemParams(int* cur_comb, int cur_var, int n_vars, int proc){
  MPI_Send(cur_comb, n_vars, MPI_INT, proc, 0, MPI_COMM_WORLD);
  MPI_Send(&cur_var, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
}

int* recvProblemParams(int* cur_var, int n_vars){
  MPI_Status status;
  int* cur_comb = malloc(n_vars*sizeof(int));
  MPI_Recv(cur_comb, n_vars, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
  MPI_Recv(cur_var, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
  return cur_comb;
}


int* MAXSAT(int n_clauses, int n_vars, int** clause_matrix, int cur_var, int* prev_comb, int* cur_sol, int* cur_maxsat, int* n_solutions){
	int n_clauses_satisfied;
	int n_clauses_unsatisfied=0;
	int next_var;

	int* cur_comb = get_cur_comb(cur_var, prev_comb, n_vars); //knowing the current variable assignment and the previous combination,
													         //get the current combination


	//printf("%f\r", (++prone*100)/pow(2,n_vars) );
	n_clauses_satisfied = clauses_satisfied(n_clauses, clause_matrix, cur_var, cur_comb, &n_clauses_unsatisfied); //calculate number of clauses satisfied and unsatisfied by current combination
	if(abs(cur_var)<n_vars){ //we're not on the last variable -> we're not on a leaf

		if(n_clauses - n_clauses_unsatisfied < *cur_maxsat){ //no need to go further, no better solution we'll be found -> suggestion from the project sheet
			free(cur_comb);
		}
		else{ //continue going down the tree
			next_var = abs(cur_var)+1;

			cur_sol = MAXSAT(n_clauses, n_vars, clause_matrix,  next_var, cur_comb, cur_sol, cur_maxsat, n_solutions); //branch with next var set to true
			cur_sol = MAXSAT(n_clauses, n_vars, clause_matrix, -next_var, cur_comb, cur_sol, cur_maxsat, n_solutions); //branch with next var set to false
		}
		return cur_sol;
	}
	else{ //we're in a leaf

		if(n_clauses_satisfied == *cur_maxsat) //if this combination satisfies the same number of clauses as the current best, increase the number of solutions
			(*n_solutions)++;
		else{
			if(n_clauses_satisfied > *cur_maxsat){ //if this combination satisfies more clauses than the previous best...
				cur_sol = cur_comb; //store the solution
				*cur_maxsat = n_clauses_satisfied; //update the best score
				*n_solutions=1;
				return cur_sol;
			}
		}
		free(cur_comb);
    return cur_sol;
	}
}

int* get_first_comb(int n_vars){
	int* first_comb = (int *) malloc(n_vars*sizeof(int));
	int i;

	for(i=0; i<n_vars; i++)
		first_comb[i]=0;

	return first_comb;
}


//to obtain the combination, we copy the previous combination and assign the current variable accordingly. actually we only copy half of the time. half of the children reuse the parent's array, the other's copy it.
int* get_cur_comb(int cur_var, int* prev_comb, int n_vars){
	int* cur_comb;
	int i;

  if(cur_var==0)
    return prev_comb;

	if(cur_var<0)
		cur_comb = prev_comb;
	else{
		cur_comb = (int *) malloc(n_vars*sizeof(int));
		for(i=0; i<n_vars; i++)
			cur_comb[i]=prev_comb[i];
	}

	cur_comb[abs(cur_var)-1]=cur_var;

	return cur_comb;
}

//for a given combination and current variable, calculate the number of clauses satisfied (and unsatisfied)
int clauses_satisfied(int n_clauses, int** clause_matrix, int cur_var, int* cur_comb, int* unsatisfied){
	int i, j, n_clauses_satisfied=0;
	int unsat = 0;
	int var, bit;

	for(i=0; i<n_clauses; i++){ //for each clause
		for(j=0; j<20; j++){ //for each variable -> each variable is a bit of cur_ass
			var = abs(clause_matrix[i][j]);

			if(var==0 || var>abs(cur_var)) // end of clause or we don't know the next variable assignments
				break;

			if(cur_comb[var-1] == clause_matrix[i][j]){ //if the variable corresponds
				n_clauses_satisfied++;
				unsat=0;
				break; //only one variable needs to match for the clause to be satisfied
			}
			else
				unsat=1;
		}

		if(unsat==1 && clause_matrix[i][j]==0){
			*unsatisfied = *unsatisfied + 1;
			unsat=0;
		}
	}

	return n_clauses_satisfied;
}

void print_sol(int* cur_sol, int n_vars){
	int i;
	for(i=0; i<n_vars; i++)
		printf("%d ", cur_sol[i]);

	printf("\n");
}

void free_matrix(int** clause_matrix, int n_clauses){
	int i;
	for(i=0; i<n_clauses; i++)
		free(clause_matrix[i]);

	free(clause_matrix);
}

int **parseFile(int * n_clauses, int * n_vars, char name_of_the_file[40]){
	int n,j,i = 0;
   	long elapsed_seconds;
  	char line[80];
   	int matrix_line = 0, matrix_column = 0;
   	int field = -1;
	FILE *fr;
	char *start;
	int ** clause_matrix;
   	fr = fopen (name_of_the_file, "rt");  /* open the file for reading */

   	/*Read n_vars and n_clauses*/
	fgets(line,80,fr);
	sscanf(line, "%d" "%d", n_vars, n_clauses);
	//printf("N_vars =  %d  N_clauses = %d \n",*n_vars,*n_clauses);



    clause_matrix = (int**)malloc( *n_clauses * sizeof( int* ));

    for (i = 0; i < *n_clauses; i++){
    	clause_matrix[i] = (int *)malloc(20 * sizeof(int));
 	}

 	for (i = 0; i < *n_clauses; i++){
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
