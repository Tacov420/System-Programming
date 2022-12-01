#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    int guess, player_id = atoi(argv[2]);
    for(int i = 1;i <= 10;i++){
        srand ((player_id + i) * 323);
        guess = rand() % 1001;

        printf("%d %d\n", player_id, guess);
        fflush(stdout);
    }
    return 0;
}