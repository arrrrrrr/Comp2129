#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WIDTH 3
#define HEIGHT WIDTH

/* | |X
  -----
   |O|
  -----
   | |
*/

int has_win(int *board, int id, int x0, int y0, int x1, int y1, int x2, int y2);
void print_board(int *board);
int check_win_state(int player, int *board);
int has_free_cells(int *board);

void print_board(int *board) {
    for (int row = 0; row < HEIGHT; row++) {
        for (int i = 0; i < WIDTH; i++) {
            int idx = row * WIDTH + i;
            char c = (board[idx] == 0) ? ' ' : (board[idx] == 1) ? 'X' : 'O';

            printf("%s%c",(i == 0) ? "" : "|",c);
        }

        printf("\n");

        if (row != HEIGHT-1)
            printf("-----\n");
    }
}

int check_win_state(int player, int *board) {
    int mapped = player+1;

    // printf("[");
    // for (int i = 0; i < WIDTH*HEIGHT; i++) {
    //     printf(" %d%s", board[i], (i == WIDTH*HEIGHT-1) ? " " : ",");
    // }
    // printf("]\n");

    // check rows for win
    for (int i = 0; i < HEIGHT; i++)
        if (has_win(board, mapped, 0, i, 1, i, 2, i))
            return 1;

    // check cols for win
    for (int i = 0; i < WIDTH; i++)
        if (has_win(board, mapped, i, 0, i, 1, i, 2))
            return 1;

    // check diagonals for win
    if (has_win(board, mapped, 0, 0, 1, 1, 2, 2) ||
        has_win(board, mapped, 0, 2, 1, 1, 2, 0))
        return 1;

    return 0;
}

int has_win(int *board, int id, int x0, int y0, int x1, int y1, int x2, int y2) {
    int owned = 0;

    if (board[y0 * WIDTH + x0] == id)
        owned++;

    if (board[y1 * WIDTH + x1] == id)
        owned++;

    if (board[y2 * WIDTH + x2] == id)
        owned++;

    //printf("has_win: (x0,y0) = (%d,%d), (x1,y1) = (%d,%d), (x2,y2) = (%d,%d), [%d] owned = %d\n",x0,y0,x1,y1,x2,y2,id,owned);

    return owned == 3;
}

int has_free_cells(int *board) {
    int free = 0;
    for (int i = 0; i < WIDTH*HEIGHT; i++)
        if (board[i] == 0)
            free++;

    return (free > 0);
}

int main(void) {
    int players[2] = {0}, turn = 0, curr_player = 0;

    int *board = malloc(sizeof *board * WIDTH * HEIGHT);
    memset(board, 0, sizeof *board * WIDTH * HEIGHT);

    int x = 0, y = 0, won = 0;

    while (1) {
        //printf("\nTURN %d\n\n",turn);
        curr_player = turn % 2;
        scanf("%d %d",&x,&y);

        board[y*WIDTH+x] = curr_player+1;
        players[curr_player]++;

        if (players[curr_player] >= 3)
            won = check_win_state(curr_player, board);

        //printf("current player = %d, owned = %d, won = %d\n", curr_player, players[curr_player], won);

        if (won) {
            printf("%s wins!\n",(curr_player == 0) ? "X" : "O");
        } else {
            if (!has_free_cells(board)) {
                printf("Draw\n");
                won = 1;
            }
        }

        printf("\n");
        print_board(board);
        printf("\n");

        if (won)
            break;

        turn++;
    }

    free(board);
}