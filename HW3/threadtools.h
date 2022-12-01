#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

extern int timeslice, switchmode;

typedef struct TCB_NODE *TCB_ptr;
typedef struct TCB_NODE{
    jmp_buf  Environment;
    int      Thread_id;
    TCB_ptr  Next;
    TCB_ptr  Prev;
    int i, N;
    int w, x, y, z;
} TCB;

extern jmp_buf MAIN, SCHEDULER;
extern TCB_ptr Head;
extern TCB_ptr Current;
extern TCB_ptr Work;
extern sigset_t base_mask, waiting_mask, tstp_mask, alrm_mask;

void sighandler(int signo);
void scheduler();

// Call function in the argument that is passed in
#define ThreadCreate(function, thread_id, number)                                         \
{                                                                                         \
	if(!setjmp(MAIN)) \
    function(thread_id, number);												  \
}

// Build up TCB_NODE for each function, insert it into circular linked-list
#define ThreadInit(thread_id, number)                                                     \
{                                                                                 \
	TCB_ptr temp = (TCB_ptr) malloc(sizeof(TCB));    \
    temp->N = number;   \
    temp->Thread_id = thread_id;  \
    temp->Next = NULL;    \
    temp->Prev = NULL;  \
    temp->w = 0;    \
    \
    if(Head == NULL){   \
        Head = temp;    \
        Current = temp; \
    }   \
    else{   \
        Current->Next = temp;   \
    }   \
    Head->Prev = temp;  \
    temp->Prev = Current;   \
    temp->Next = Head;   \
    \
    if(thread_id == 1){ \
        temp->x = number;    \
    }	\
    else if(thread_id == 2){    \
        temp->x = 0;    \
        temp->y = 1;    \
        temp->i = 0;    \
    }   \
    else{   \
        temp->i = 0;    \
        temp->x = number - 1;   \
    }								  \
    Current = temp; \
}

// Call this while a thread is terminated
#define ThreadExit()                                                                      \
{                                                                                         \
	longjmp(SCHEDULER, 2);												  \
}

// Decided whether to "context switch" based on the switchmode argument passed in main.c
#define ThreadYield()                                                                     \
{                                                                                         \
	if(!switchmode) \
        longjmp(SCHEDULER, 1);												  \
    else{   \
        sigprocmask(SIG_UNBLOCK, &tstp_mask, NULL);  \
        sigprocmask(SIG_SETMASK, &base_mask, NULL);  \
        sigprocmask(SIG_UNBLOCK, &alrm_mask, NULL);  \
        sigprocmask(SIG_SETMASK, &base_mask, NULL);  \
    }   \
}
