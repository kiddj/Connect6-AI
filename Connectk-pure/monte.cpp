#include <stdlib.h>
#include <iostream>

#include "mcts.h"
#include "shared.h"

#define CANDIDATES 8
#define MONTE_ITERS 600

using namespace std;

void print_board(Board* b);

Moves *empty_cells(Board *b) {
    Moves *empties = moves_new();
    Move move;

    for (int i = 0; i < board_size; i++) {
        for (int j = 0; j < board_size; j++) {
            if (piece_at(b, i, j) == EMPTY) {
                move.x = i;
                move.y = j;
                move.weight = 0.f;
                add_move(empties, &move);
            }
        }
    }
    return empties;
}

int monte_carlo_run(Board *b) {
    Board *new_board = (Board *)malloc(sizeof(Board));
    board_copy(b, new_board);   

    Move move;
    Moves *empties;
    empties = empty_cells(new_board);
    int i;

    while(true) {
        if(empties->len == 0) {
            return 0;
        }
        i = rand() % empties->len;
        move = empties->list[i];
        erase_move(empties, i);
        place_piece(new_board, move.x, move.y);
        if(check_win(new_board, move.x, move.y)) {
            if(new_board->turn == b->turn) {
                free(new_board);
                free(empties);
                return 1;
            }
            else {
                free(new_board);
                free(empties);
                return 0;
            }
        }
        new_board->moves_left--;
        if(new_board->moves_left == 0) {
            if(new_board->turn == BLACK)
                new_board->turn = WHITE;
            else if(new_board->turn == WHITE)
                new_board->turn = BLACK;
            new_board->moves_left = place_p; 
        }
    }
}

Moves* monte_carlo(Board* b) {
    int i, k, wins, len;

    Board* new_board = board_new();

    Move move;
    Moves* moves = move_utilities(b);
    moves->utility = 0;
    sort_moves(moves);
    crop_moves(moves, CANDIDATES);
    len = moves->len;

    for(i = 0; i < len; i++){
        move = moves->list[i];
        cout << move.x << " " << move.y << " " << move.weight << endl;
        cout << "------------------------";
        board_copy(b, new_board);
        place_piece(new_board, move.x, move.y);
        if(check_win(new_board, move.x, move.y)) {
            move.weight = move.weight * 40 + MONTE_ITERS;
            moves->list[i] = move;
            moves->utility += MONTE_ITERS;
        }
        else {
            wins = 0;
            for(k = 0; k < MONTE_ITERS; k++){
                wins += monte_carlo_run(new_board);
            }
            move.weight = move.weight * 40 + wins;
            moves->list[i]= move;
            moves->utility += wins;
        }
        cout << move.x << " " << move.y << " " << move.weight << endl;
    }
    free(new_board);
    return moves;
}