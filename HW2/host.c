#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

void read_arguments(int *host_id, int *depth, int *lucky_number, char *argv[]){
    for(int i = 1;i <= 5;i += 2){
        if(argv[i][1] == 'm'){
            (*host_id) = atoi(argv[i + 1]);
        }
        else if(argv[i][1] == 'd'){
            (*depth) = atoi(argv[i + 1]);
        }
        else{
            (*lucky_number) = atoi(argv[i + 1]);
        }
    }
}

void exec_new_host(int host_id, int depth, int lucky_number){
    char buf[3][64];
    sprintf(buf[0], "%d", host_id);
    sprintf(buf[1], "%d", depth);
    sprintf(buf[2], "%d", lucky_number);
    execl("./host", "./host", "-m", buf[0], "-d", buf[1], "-l", buf[2], (char *)0);
}

void make_children(int fd[][2][2], int host_id, int depth, int lucky_number){
    for(int i = 0;i < 2;i++){
        pipe(fd[i][0]);
        pipe(fd[i][1]);
        if(fork() == 0){
            close(fd[i][0][1]);
            close(fd[i][1][0]);
            dup2(fd[i][0][0], 0);
            dup2(fd[i][1][1], 1);
            
            exec_new_host(host_id, depth + 1, lucky_number);
        }
        else{
            close(fd[i][0][0]);
            close(fd[i][1][1]);
        }
    }
    return;
}

int compare(int guess[], int lucky_number){
    if(abs(lucky_number - guess[0]) > abs(lucky_number - guess[1]))
        return 1;
    else
        return 0;
}

void wait_children(){
    for(int i = 0;i < 2;i++)
        wait(NULL);
}

void terminate(){
    for(int i = 0;i < 2;i++)
        wait(NULL);
    exit(0);
}

void transport_data(int n, int wfd[], int player_ids[]){
    char buf[64];
    for(int i = 0;i < 2;i++){
        if(n == 8) sprintf(buf, "%d %d %d %d\n", player_ids[i * 4], player_ids[i * 4 + 1], player_ids[i * 4 + 2], player_ids[i * 4 + 3]);
        else if(n == 4) sprintf(buf, "%d %d\n", player_ids[i * 2], player_ids[i * 2 + 1]);
        write(wfd[i], buf, strlen(buf));
    }
}

void report_10rounds(FILE *fp[], int lucky_number){
    int candidate[2], guess[2], win_index;
    for(int i = 0;i < 10;i++){
        for(int j = 0;j < 2;j++){
            fscanf(fp[j], "%d%d", &candidate[j], &guess[j]);
        }
        
        win_index = compare(guess, lucky_number);
        printf("%d %d\n", candidate[win_index], guess[win_index]);
        fflush(stdout);
    }
}

void clear_score(int score[]){
    for(int i = 0;i < 8;i++) score[i] = 0;
}

void add_score(int winner, int score[], int player_ids[]){
    for(int i = 0;i < 8;i++){
        if(player_ids[i] == winner){
            score[i] += 10;
            return;
        }
    }
}

void root_host(int fd[][2][2], int lucky_number, int host_id){
    int player_ids[8], wfd[2], candidate[2], guess[2], winner, score[8] = {0};
    FILE *fp[2];

    for(int i = 0;i < 2;i++){
        wfd[i] = fd[i][0][1];
        fp[i] = fdopen(fd[i][1][0], "r");
    }

    char buf[64];
    sprintf(buf, "fifo_%d.tmp", host_id);
    int fifo_fd[2];
    fifo_fd[0] = open("fifo_0.tmp", O_WRONLY);
    fifo_fd[1] = open(buf, O_RDONLY);
    FILE *fifo_p[2];
    fifo_p[0] = fdopen(fifo_fd[0], "w");
    fifo_p[1] = fdopen(fifo_fd[1], "r");
    while(1){
        for(int i = 0;i < 8;i++) fscanf(fifo_p[1], "%d", &player_ids[i]);
        transport_data(8, wfd, player_ids);

        if(player_ids[0] < 0)
            terminate();

        for(int i = 0;i < 10;i++){
            for(int j = 0;j < 2;j++){
                fscanf(fp[j], "%d%d", &candidate[j], &guess[j]);
            }
            
            winner = compare(guess, lucky_number);
            add_score(candidate[winner], score, player_ids);
        }

        fprintf(fifo_p[0], "%d\n", host_id);
        for(int i = 0;i < 8;i++){
            //fprintf(stderr, "%d %d\n", player_ids[i], score[i]);
            fprintf(fifo_p[0], "%d %d\n", player_ids[i], score[i]);
        }
        fflush(fifo_p[0]);
        clear_score(score);
    }
}

void child_host(int fd[][2][2], int lucky_number){
    int player_ids[4], wfd[2];
    FILE *fp[2];

    for(int i = 0;i < 2;i++){
        wfd[i] = fd[i][0][1];
        fp[i] = fdopen(fd[i][1][0], "r");
    }
    
    while(1){
        for(int i = 0;i < 4;i++) scanf("%d", &player_ids[i]);
        transport_data(4, wfd, player_ids);

        if(player_ids[0] < 0)
            terminate();

        report_10rounds(fp, lucky_number);
    }
}

void leaf_host(int lucky_number){
    int player_ids[2], fd[2][2];
    char buf[64];
    FILE *fp[2];

    while(1){
        scanf("%d%d", &player_ids[0], &player_ids[1]);

        if(player_ids[0] < 0)
            exit(0);
        
        pipe(fd[0]);
        pipe(fd[1]);
        for(int i = 0;i < 2;i++){
            if(fork() == 0){
                close(fd[i][0]);
                dup2(fd[i][1], 1);
                sprintf(buf, "%d", player_ids[i]);
                execl("./player", "./player", "-n", buf, (char *)0);
            }
            else{
                close(fd[i][1]);
                fp[i] = fdopen(fd[i][0], "r");
            }
        }

        report_10rounds(fp, lucky_number);
        wait_children();
    }
    return;
}

int main(int argc, char *argv[]){
    int host_id, depth, lucky_number;
    read_arguments(&host_id, &depth, &lucky_number, argv);

    int fd[2][2][2];    //fd[][0][] parent write to child
    if(depth < 2){
        make_children(fd, host_id, depth, lucky_number);
    }

    if(depth == 0)
        root_host(fd, lucky_number, host_id);
    else if(depth == 1)
        child_host(fd, lucky_number);
    else
        leaf_host(lucky_number);
    
    return 0;
}