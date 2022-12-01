#include "threadtools.h"
//be aware of base cases

// Please complete this three functions. You may refer to the macro function defined in "threadtools.h"

// Mountain Climbing
// You are required to solve this function via iterative method, instead of recursion.
void MountainClimbing(int thread_id, int number){
	ThreadInit(thread_id, number);
	if(!setjmp(Current->Environment))
		longjmp(MAIN, 1);
	for(Current->i;Current->i <= Current->N;Current->i++){
		sleep(1);
		Current->w = Current->x + Current->y;
		Current->y = Current->x;
		Current->x = Current->w;

		printf("Mountain Climbing: %d\n", Current->w);
		if(switchmode) sigpending(&waiting_mask);
		if(!setjmp(Current->Environment) && (!switchmode || sigismember(&waiting_mask, SIGALRM) || sigismember(&waiting_mask, SIGTSTP)))
			ThreadYield();
	}
	ThreadExit();
}

// Reduce Integer
// You are required to solve this function via iterative method, instead of recursion.
void ReduceInteger(int thread_id, int number){
	ThreadInit(thread_id, number);
	if(!setjmp(Current->Environment))
		longjmp(MAIN, 1);
	if(Current->x == 1){
		sleep(1);
		printf("Reduce Integer: %d\n", Current->w);
		if(!setjmp(Current->Environment))	
			ThreadYield();
	}
	for(Current->x;Current->x > 1;){
		sleep(1);
		if(!(Current->x % 2)) Current->x /= 2;
		else if(Current->x % 4 == 1 || Current->x == 3) Current->x--;
		else Current->x++;
		Current->w++;

		printf("Reduce Integer: %d\n", Current->w);
		if(switchmode) sigpending(&waiting_mask);
		if(!setjmp(Current->Environment) && (!switchmode || sigismember(&waiting_mask, SIGALRM) || sigismember(&waiting_mask, SIGTSTP)))	
			ThreadYield();
	} 
	ThreadExit();
}

// Operation Count
// You are required to solve this function via iterative method, instead of recursion.
void OperationCount(int thread_id, int number){
	ThreadInit(thread_id, number);
	if(!setjmp(Current->Environment))
		longjmp(MAIN, 1);
	for(Current->i;Current->i <= (Current->N - 1) / 2;Current->i++){
		sleep(1);
		Current->w += Current->x - 2 * Current->i;

		printf("Operation Count: %d\n", Current->w);
		if(switchmode) sigpending(&waiting_mask);
		if(!setjmp(Current->Environment) && (!switchmode || sigismember(&waiting_mask, SIGALRM) || sigismember(&waiting_mask, SIGTSTP)))
			ThreadYield();
	} 
	ThreadExit();
}
