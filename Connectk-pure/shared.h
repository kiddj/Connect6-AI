#include <stdio.h>
#include <stdlib.h>

#include "mcts.h"

static Board* board_new(void) {
    Board* b = (Board*) malloc(sizeof(Board));
    b->moves_left = 0;
    b->turn = 0;
    for(int i = 0; i < 19; i++)
        for(int j = 0; j < 19; j++)
            b->data[i][j] = 0;
    return b;
}

static void board_turn_init(Board* b) {
    b->moves_left = 2;

    int black = 0, white = 0;
    for(int i = 0; i < 19; i++) {
        for(int j = 0; j < 19; j++) {
            if(b->data[i][j] == BLACK)
                black++;
            if(b->data[i][j] == WHITE)
                white++;
        }
    }
    if(white >= black)
        b->turn = BLACK;
    else {
        if(white == 0)
            is_white_first = true;
        b->turn = WHITE;
    }
}

static inline void board_copy(const Board* from, Board* to) {
    to->turn = from->turn;
    to->moves_left = from->moves_left;
    for(int i = 0; i < 19; i++)
        for(int j = 0; j < 19; j++)
            to->data[i][j] = from->data[i][j];
}

inline Moves* moves_new(void) {
    Moves* m = (Moves*) malloc(sizeof(Moves));
    m->len = 0;
    m->utility = 0;
    for(int i = 0; i < MAX_MOVES_LEN; i++) {
        m->list[i].weight = 0;
        m->list[i].x = 0;
        m->list[i].y = 0;
    }
    return m;
}

static inline int find_move(Moves* moves, unsigned int x, unsigned int y) {
    if(moves) {
        for(int i = 0; i < moves->len; i++) {
            Move *m = moves->list + i;
            if(m->x == x && m->y == y)
                return i;
        }
    }
    return -1;
}

static inline void add_move(Moves* moves, Move* move) {
    int i = find_move(moves, move->x, move->y);
    if(i < 0) {
        if(moves->len >= 19 * 19)
            printf("Adding too much to moves.\n");
        else
            moves->list[moves->len++] = *move;
    }
    else {
        moves->list[i].weight += move->weight;
    }
}

static inline int compare_moves(const void *a, const void *b) {
    return ((Move*) b)->weight - ((Move*) a)->weight;
}

static inline void erase_move(Moves* moves, int i) {
    if(moves->len > i)
        moves->list[i] = moves->list[moves->len -1];
    moves->len--;
}

static inline void sort_moves(Moves* moves) {
    qsort(moves->list, moves->len, sizeof(Move), compare_moves);
}

static inline void crop_moves(Moves* moves, unsigned int n) {
    if(moves->len < n)
        return;
    moves->len = n;
}

static inline int piece_at(const Board* b, unsigned int x, unsigned int y) {
    return b->data[y][x];
}

static inline void place_piece_type(Board *b, unsigned int x, unsigned int y, unsigned int type) {
    b->data[y][x] = type;
}

static inline void place_piece(Board *b, unsigned int x, unsigned int y) {
    place_piece_type(b, x, y, b->turn);
}

Moves* move_utilities(const Board* b);

static int count_pieces(const Board *b, unsigned int x, unsigned int y, unsigned int type, int dx, int dy,
                 unsigned int *out) {
    int i;
    unsigned int p = EMPTY;

    if (!dx && !dy)
            return piece_at(b, x, y) == type ? 1 : 0;
    for (i = 0; x >= 0 && x < board_size && y >= 0 && y < board_size; i++) {
        p = piece_at(b, x, y);
        if (p != type)
                break;
        x += dx;
        y += dy;
    }
    if (out)
        *out = p;
    return i;
}

static inline bool check_win_full(const Board *b, unsigned int x, unsigned int y,
                        unsigned int *x1, unsigned int *y1, unsigned int *x2, unsigned int *y2) {
    int i, c1, c2, xs[] = {1, 1, 0, -1}, ys[] = {0, 1, 1, 1};
    unsigned int type = piece_at(b, x, y);
    if (type != BLACK && type != WHITE)
        return false;
    for (i = 0; i < 4; i++) {
        c1 = count_pieces(b, x, y, type, xs[i], ys[i], NULL);
        c2 = count_pieces(b, x, y, type, -xs[i], -ys[i], NULL);
        if (c1 + c2 > connect_k) {
            if (x1)
                *x1 = x + xs[i] * (c1 - 1);
            if (y1)
                *y1 = y + ys[i] * (c1 - 1);
            if (x2)
                *x2 = x - xs[i] * (c2 - 1);
            if (y2)
                *y2 = y - ys[i] * (c2 - 1);
            return true;
        }
    }
    return false;
}

static inline bool check_win(Board* b, unsigned int x, unsigned int y) {
    return check_win_full(b, x, y, 0, 0, 0, 0);
}

Moves* monte_carlo(Board* b);