#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>

char *board[2], mode;
int row, col, epoch, Round, thread_num, done;
pthread_t ids[100];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct data{
    pthread_mutex_t mlock;
    pthread_cond_t cv;
    char board_p0[13769230];
    char board_p1[13769230];
    int Done;
}Data;

bool valid(int r, int c){
    return (r >= 0 && r < row && c >= 0 && c < col);
}

//I don't know why spliting the loop into "if"s speeds up the process
int count(int pos){
    int sum = 0, r = pos / col, c = pos % col, pr = pos - col, nr = pos + col;
    if(valid(r - 1, c - 1) && board[Round][pr - 1] == 'O')
        sum++;
    if(valid(r - 1, c) && board[Round][pr] == 'O')
        sum++;
    if(valid(r - 1, c + 1) && board[Round][pr + 1] == 'O')
        sum++;
    if(valid(r, c - 1) && board[Round][pos - 1] == 'O')
        sum++;
    if(valid(r, c + 1) && board[Round][pos + 1] == 'O')
        sum++;
    if(valid(r + 1, c - 1) && board[Round][nr - 1] == 'O')
        sum++;
    if(valid(r + 1, c) && board[Round][nr] == 'O')
        sum++;
    if(valid(r + 1, c + 1) && board[Round][nr + 1] == 'O')
        sum++;
    return sum;
}

void update(int pos, int next_round){
    int alive = count(pos);
    if(board[Round][pos] == 'O'){
        if(alive < 2 || alive > 3)
            board[next_round][pos] = '.';
        else
            board[next_round][pos] = 'O';
    }
    else{
        if(alive == 3)
            board[next_round][pos] = 'O';
        else
            board[next_round][pos] = '.';
    }
    return;
}

void *play(void *arg){
    int start = (int) arg, next_round;
    int n = row * col / thread_num + ((start / (row * col / thread_num) < row * col % thread_num)? 1:0); 
    for(int t = 0;t < epoch;t++){
        next_round = (Round + 1) % 2;
        for(int i = start;i < start + n;i++){
            update(i, next_round);
        }

        pthread_mutex_lock(&lock);
        done++;
        if(done < thread_num)
            pthread_cond_wait(&cond, &lock);
        else{
            done = 0;
            Round = next_round;
            pthread_cond_broadcast(&cond);
        }
        pthread_mutex_unlock(&lock);
    }
    return ((void *)0);
}

int count_p(int pos, Data *ptr){
    int sum = 0, r = pos / col, c = pos % col, pr = pos - col, nr = pos + col;
    char *temp;
    if(Round == 0) temp = ptr->board_p0;
    else temp = ptr->board_p1;

    if(valid(r - 1, c - 1) && temp[pr - 1] == 'O')
        sum++;
    if(valid(r - 1, c) && temp[pr] == 'O')
        sum++;
    if(valid(r - 1, c + 1) && temp[pr + 1] == 'O')
        sum++;
    if(valid(r, c - 1) && temp[pos - 1] == 'O')
        sum++;
    if(valid(r, c + 1) && temp[pos + 1] == 'O')
        sum++;
    if(valid(r + 1, c - 1) && temp[nr - 1] == 'O')
        sum++;
    if(valid(r + 1, c) && temp[nr] == 'O')
        sum++;
    if(valid(r + 1, c + 1) && temp[nr + 1] == 'O')
        sum++;
    return sum;
}

void update_p(int pos, Data *ptr){
    int alive = count_p(pos, ptr);
    //printf("Round %d, process %d, r = %d c = %d alive = %d\n", Round, pos / (row * col / 2), pos / col, pos % col, alive);
    char *temp, *nt;
    if(Round == 0){
        temp = ptr->board_p0;
        nt = ptr->board_p1;
    }
    else{
        temp = ptr->board_p1;
        nt = ptr->board_p0;
    }

    if(temp[pos] == 'O'){
        if(alive < 2 || alive > 3)
            nt[pos] = '.';
        else
            nt[pos] = 'O';
    }
    else{
        if(alive == 3)
            nt[pos] = 'O';
        else
            nt[pos] = '.';
    }
    return;
}

void play_p(Data *ptr, int start){
    int next_round;
    int n = row * col / 2 + ((start == 0)? (row * col % 2):0); 
    for(int t = 0;t < epoch;t++){
        next_round = (Round + 1) % 2;
        for(int i = start;i < start + n;i++){
            update_p(i, ptr);
        }

        pthread_mutex_lock(&ptr->mlock);
        ptr->Done++;
        if(ptr->Done < 2)
            pthread_cond_wait(&ptr->cv, &ptr->mlock);
        else{
            ptr->Done = 0;
            pthread_cond_broadcast(&ptr->cv);
        }
        Round = next_round;
        pthread_mutex_unlock(&ptr->mlock);
    }
    return;
}

int main(int argc, char *argv[]){
    mode = argv[1][1];
    thread_num = atoi(argv[2]);
    FILE *fp_in = fopen(argv[3], "r"), *fp_out = fopen(argv[4], "w");

    char buf[64];
    fgets(buf, 64, fp_in);
    sscanf(buf, "%d%d%d", &row, &col, &epoch);
    
    if(mode == 't'){
        board[0] = (char *)malloc(sizeof(char) * (row * col));
        board[1] = (char *)malloc(sizeof(char) * (row * col));
        char *s = (char *)malloc(sizeof(char) * (col + 3));
        for(int i = 0;i < row;i++){
            fgets(s, col + 3, fp_in);
            for(int j = 0;j < col;j++) board[0][i * col + j] = s[j];
        }

        int start = 0;
        for(int i = 0;i < thread_num;i++){
            pthread_create(ids + i, NULL, play, (void *)start);
            start += row * col / thread_num + ((i < row * col % thread_num)? 1:0);
        }

        for(int i = 0;i < thread_num;i++) pthread_join(ids[i], NULL);

        for(int i = 0;i < row;i++){
            for(int j = 0;j < col;j++) fputc(board[Round][i * col + j], fp_out);
            if(i < row - 1) fputc('\n', fp_out);
        }
    }
    else{
        pthread_mutexattr_t mattr;
        pthread_condattr_t cattr;
        Data *ptr = (Data *) mmap(NULL, sizeof(Data), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        ptr->Done = 0;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&(ptr->mlock), &mattr);
        pthread_condattr_init(&cattr);
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&(ptr->cv), &cattr);

        char *s = (char *)malloc(sizeof(char) * (col + 3));
        for(int i = 0;i < row;i++){
            fgets(s, col + 3, fp_in);
            for(int j = 0;j < col;j++) ptr->board_p0[i * col + j] = s[j];
        }

        if(fork() == 0){
            play_p(ptr, (row * col / 2 + row * col % 2));
            pthread_mutexattr_destroy(&mattr);
            pthread_condattr_destroy(&cattr);
        }
        else{
            play_p(ptr, 0);
            pthread_mutexattr_destroy(&mattr);
            pthread_condattr_destroy(&cattr);
            wait(NULL);

            for(int i = 0;i < row;i++){
                if(Round == 0){
                    for(int j = 0;j < col;j++) fputc(ptr->board_p0[i * col + j], fp_out);
                }
                else{
                    for(int j = 0;j < col;j++) fputc(ptr->board_p1[i * col + j], fp_out);
                }
                if(i < row - 1) fputc('\n', fp_out);
            }
        }
    }
    return 0;
}