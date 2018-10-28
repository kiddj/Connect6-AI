#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "mcts.h"
#include "shared.h"

using namespace std;

int utility_upper_bound;

/* arrays for holding the utility and threat values for each window */
int white_window_sequence_array[19][19][4];
int black_window_sequence_array[19][19][4];
int white_window_threat_array[19][19][4];
int black_window_threat_array[19][19][4];

/* arrays for holding the marks used for counting threats */
int whites_marks[19][19];
int blacks_marks[19][19];

/* the sum of all the windows' values for threats and utility */
int white_window_sequence_count;
int black_window_sequence_count;
int white_threat_count;
int black_threat_count;

/* the max sequence on a board */
int max_white_sequence;
int max_black_sequence;

static void clear_all(void) {
    for (int x = 0; x < 19; x++) {
		for (int y = 0; y < 19; y++) {

			blacks_marks[x][y] = 0;
			whites_marks[x][y] = 0;

			for (int d = 0; d < 4; d++) {
				white_window_sequence_array[x][y][d] = 0;
				black_window_sequence_array[x][y][d] = 0;
				white_window_threat_array[x][y][d] = 0;
				black_window_threat_array[x][y][d] = 0;
			}
		}
	}
	white_window_sequence_count = 0;
	black_window_sequence_count = 0;
	white_threat_count = 0;
	black_threat_count = 0;
	max_black_sequence = 0;
	max_white_sequence = 0;
}

int window_id(int x, int y, int i) {
	return x * board_size * 4 + y * 4 + i + 1;
}

void window_sequence(const Board *b, unsigned int x, unsigned int y, int i, unsigned int player)
{
	int sequence, count_sequence, count_threat, dx, dy, ddx, ddy, j, len;
	int ddxs[] = {1, 0, 1,  1};
	int ddys[] = {0, 1, 1, -1};
	int to_mark[6];

	ddx = ddxs[i];
	ddy = ddys[i];

	len = 0;
	count_sequence = 0;
	count_threat = 0;
	sequence = 0;

	if (player == WHITE) {
		// We are recalculating this window's threat level.
		// Start be taking this window's previous threat out of the sum
		// and reset it to 0.
		white_threat_count -= white_window_threat_array[x][y][i];
		white_window_threat_array[x][y][i] = 0;

		// Do the same for the sequence count.
		white_window_sequence_count -= white_window_sequence_array[x][y][i];
		white_window_sequence_array[x][y][i] = 0;

		// step through the window.
		for (dx = 0, dy = 0; dx < connect_k && dy < (int)connect_k; dx += ddx, dy += ddy) {
			if (x + dx < 0 || y + dy < 0 || x + dx >= board_size || y + dy >= board_size) {
				break;
			}
			// If this window was the one that marked this spot, unmark it so we can recheck.
			if (whites_marks[x + dx][y + dy] == window_id(x, y, i)) {
				whites_marks[x + dx][y + dy] = 0;
			}

			if (piece_at(b, x + dx, y + dy) == EMPTY) {
				count_sequence++;
				if (whites_marks[x + dx][y + dy] == 0) {
					count_threat++;
					to_mark[len++] = x + dx;
					to_mark[len++] = y + dy;
				}
			} else if (piece_at(b, x + dx, y + dy) == player) {
				count_sequence++;
				count_threat++;
				sequence++;
			}
		}

		if (count_threat == connect_k) {
			/* if this is a threat, update the threat array */
			if (sequence >= (connect_k - place_p)) {
				for (j = 0; j < len; j+=2) {
					whites_marks[to_mark[j]][to_mark[j+1]] = window_id(x, y, i);
				}
				white_window_threat_array[x][y][i] = 1;
				white_threat_count += 1;
			}
		}

		if (count_sequence == connect_k) {

			/* if this is the max sequence of the board, update the max_sequence value */
			if (sequence > max_white_sequence) {
				max_white_sequence = sequence;
			}

			/* if the sequence is greater than connect_k - place_p, set it equal to connect_k - place_p.
			This prevents it from giving a sequence of 5 a higher score than a sequence of 4 in the default game.  */
			if (sequence > (connect_k - place_p)) {
				sequence = (connect_k - place_p);
			}

			/* update the utility array values */
			white_window_sequence_array[x][y][i] = sequence * sequence;
			white_window_sequence_count += white_window_sequence_array[x][y][i];
		}

		/* Do it all again if we are black.  There must be a better way to do this */
	} else if (player == BLACK) {
		// We are recalculating this window's threat level.
		// Start be taking this window's previous threat out of the sum
		// and reset it to 0.
		black_threat_count -= black_window_threat_array[x][y][i];
		black_window_threat_array[x][y][i] = 0;

		// Do the same for the sequence count.
		black_window_sequence_count -= black_window_sequence_array[x][y][i];
		black_window_sequence_array[x][y][i] = 0;

		// step through the window.
		for (dx = 0, dy = 0; dx < connect_k && dy < (int)connect_k; dx += ddx, dy += ddy) {
			if (x + dx < 0 || y + dy < 0 || x + dx >= board_size || y + dy >= board_size) {
				break;
			}
			// If this window was the one that marked this spot, unmark it so we can recheck.
			if (blacks_marks[x + dx][y + dy] == window_id(x, y, i)) {
				blacks_marks[x + dx][y + dy] = 0;
			}

			if (piece_at(b, x + dx, y + dy) == EMPTY) {
				count_sequence++;
				if (blacks_marks[x + dx][y + dy] == 0) {
					count_threat++;
					to_mark[len++] = x + dx;
					to_mark[len++] = y + dy;
				}
			} else if (piece_at(b, x + dx, y + dy) == player) {
				count_sequence++;
				count_threat++;
				sequence++;
			}
		}

		if (count_threat == connect_k) {
			/* if this is a threat, update the threat array */
			if (sequence >= (connect_k - place_p)) {
				for (j = 0; j < len; j+=2) {
					blacks_marks[to_mark[j]][to_mark[j+1]] = window_id(x, y, i);
				}
				black_window_threat_array[x][y][i] = 1;
				black_threat_count += 1;
			}
		}

		if (count_sequence == connect_k) {

			/* if this is the max sequence of the board, update the max_sequence value */
			if (sequence > max_black_sequence) {
				max_black_sequence = sequence;
			}

			/* if the sequence is greater than connect_k - place_p, set it equal to connect_k - place_p.
			This prevents it from giving a sequence of 5 a higher score than a sequence of 4 in the default game.  */
			if (sequence > (connect_k - place_p)) {
				sequence = (connect_k - place_p);
			}

			/* update the utility array values */
			black_window_sequence_array[x][y][i] = sequence * sequence;
			black_window_sequence_count += black_window_sequence_array[x][y][i];
		}
	} else {
		printf("windows sequence error!\n");
	}
}

int board_utility(const Board *b, unsigned player) {

	int utility = 0;
	utility_upper_bound = 2 * board_size * board_size * connect_k * connect_k;

	if (b->turn == WHITE) {
		if ((connect_k - max_white_sequence) <= (b->moves_left)) {
			utility = utility_upper_bound;
		} else if (black_threat_count) {
			utility = -utility_upper_bound - black_threat_count;
		} else if (white_threat_count > place_p) {
			utility = utility_upper_bound - 1;
		} else {
			utility = white_window_sequence_count - black_window_sequence_count;
		}
	} else if (b->turn == BLACK) {
		if ((connect_k - max_black_sequence) <= (b->moves_left)) {
			utility = utility_upper_bound;
		} else if (white_threat_count) {
			utility = -utility_upper_bound - white_threat_count;
		} else if (black_threat_count > place_p) {
			utility = utility_upper_bound - 1;
		} else {
			utility = black_window_sequence_count - white_window_sequence_count;
		}
	} else {
		printf("board utility error!\n");
	}

	if (b->turn != player)
		utility = -utility;

	return utility;
}

int board_update(const Board *b, unsigned int player) {
	unsigned int x, y;
	int i;

	clear_all();
	for (y = 0; y < board_size; y++) {
		for (x = 0; x < board_size; x++) {
			for (i = 0; i < 4; i++) {
				window_sequence(b, x, y, i, WHITE);
				window_sequence(b, x, y, i, BLACK);
			}
		}
	}

	return board_utility(b, player);
}

int incremental_update(const Board *b, unsigned int x, unsigned int y, unsigned int player) {
	int dx, dy, ddx, ddy, i, counter;
	int ddxs[] = {1, 0, 1,  1};
	int ddys[] = {0, 1, 1, -1};

	max_black_sequence = 0;
	max_white_sequence = 0;

	for (i = 0; i < 4; i++) {

		ddx = ddxs[i];
		ddy = ddys[i];

		for (counter = 0, dx = ddx * -connect_k, dy = ddy * -connect_k;
		     counter < connect_k + 2;
		     counter ++, dx += ddx, dy += ddy) {
			if (x + dx < 0 || y + dy < 0 || x + dx >= board_size || y + dy >= board_size) {
				continue;
			}

			window_sequence(b, x + dx, y + dy, i, WHITE);
			window_sequence(b, x + dx, y + dy, i, BLACK);
		}
	}

	return board_utility(b, player);
}

Moves* move_utilities(const Board* b) {
    Moves* moves = moves_new();
    Move move;

    moves->utility = board_update(b, b->turn);

    Board *new_board;
	new_board = board_new();

    for (move.y = 0; move.y < board_size; move.y++) {
		for (move.x = 0; move.x < board_size; move.x++) {
			if (piece_at(b, move.x, move.y) != EMPTY)
				continue;
			board_copy(b, new_board);
			place_piece(new_board, move.x, move.y);
			new_board->moves_left--;
			move.weight = incremental_update(new_board, move.x, move.y, new_board->turn);
			add_move(moves, &move);
			place_piece_type(new_board, move.x, move.y, EMPTY);
			new_board->moves_left++;
			incremental_update(new_board, move.x, move.y, new_board->turn);
		}
	}

    if ((b->turn == WHITE && black_threat_count) || (b->turn == BLACK && white_threat_count)) {
		sort_moves(moves);
		crop_moves(moves, 1);
	}

	free(new_board);
	cout << "moves len: " << moves->len << endl;
	return moves;
}