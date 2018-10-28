#pragma once

#define EMPTY 0
#define BLACK 1
#define WHITE 2

#define board_size 19
#define connect_k 6
#define place_p 2

#define MAX_MOVES_LEN 360

extern bool is_white_first;

typedef struct _Board {
    int data[19][19];
    unsigned int moves_left;
    int turn;
} Board;

typedef struct _Move {
    int weight;
    unsigned int x;
    unsigned int y;
} Move;

typedef struct _Moves {
    unsigned int len;
    int utility;
    Move list[360];
} Moves;

