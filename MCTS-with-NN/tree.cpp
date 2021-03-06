#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <mutex>
#include "tree.h"
#include <algorithm>
using namespace std;

Node *alloc_Node() {
	Node* new_node = new Node;
	Node& node = *new_node;
	node.parent = NULL;

	// MUST initialize vectors members with constructor!
	node.moves.push_back(Move{ BOARD_WIDTH / 2, BOARD_WIDTH / 2 });
	int len = node.moves.size();
	node.children = vector<Node* >(len, NULL);

	node.num_child = len;
	node.num_child_visited = 0;

	for (int x = 0; x < BOARD_WIDTH; x++)
		for (int y = 0; y < BOARD_WIDTH; y++) {
			node.board_state[x][y] = NONE;
			node.black_state[x][y] = 0.;
			node.white_state[x][y] = 0.;
		}
	node.last_piece = NONE;
	node.nth_turn = -1;
	node.num_pieces = 0;
	node.status = PLAYING;

	node.win_cnt = 0.;
	node.visit_cnt = 0;

	return new_node;
}

Node *copy_Node(const Node& original) {
	return new Node(original);
}

void simple_free(Node* n) {
	delete n;
}
void recursive_free(Node* n, const bool free_root) {
	
	for (Node* child : n->children)
		if (child)
			recursive_free(child, true);
	if (free_root == true)
		simple_free(n);
}

void set_root_node(Node* new_root) {
	if (new_root->parent != NULL) {
		vector<Node* >& children = new_root->parent->children;
		int len = children.size();
		for (int i = 0; i < len; i++)
			if (children[i] == new_root)
				children[i] = NULL;
	}
	new_root->parent = NULL;
}

Status place_piece(Node& node, const Move& move, const bool use_NN, MCTS* mcts) {
	node.parent = NULL;

	if (node.last_piece == BLACK) {
		if (node.nth_turn == NUM_TURN) {
			node.last_piece = WHITE;
			node.nth_turn = 1;
		}
		else
			node.nth_turn++;
	}
	else if (node.last_piece == WHITE) {
		if (node.nth_turn == NUM_TURN) {
			node.last_piece = BLACK;
			node.nth_turn = 1;
		}
		else
			node.nth_turn++;
	}
	else { // node.last_piece == NONE (start state)
		node.last_piece = FIRST_PIECE;
		node.nth_turn = NUM_TURN - FIRST_NUM_TURN + 1;
	}

	node.board_state[move.x][move.y] = node.last_piece;
	if (node.last_piece == BLACK) {
		node.black_state[move.x][move.y] = 1.;
	}
	else {
		node.white_state[move.x][move.y] = 1.;
	}
	node.num_pieces++;

	if (use_NN) {
		vector<float> flattened_block;
		generate_block(&node, mcts, &flattened_block);
		vector<float> policy_1d = policy_network(flattened_block, mcts->use_NN);
		float(*policy_2d)[BOARD_WIDTH] = 
			(float(*)[BOARD_WIDTH])policy_1d.data();
		for (int x = 0; x < BOARD_WIDTH; x++) {
			for (int y = 0; y < BOARD_WIDTH; y++) {
				node.policy[x][y] = policy_2d[x][y];
			}
		}

		vector<float> value_flattened_block;
		generate_block_value(&node, mcts, &value_flattened_block);
		float value = value_network(value_flattened_block, &node, mcts->use_NN);
		node.value = value;
	}
	node.moves.clear();
	node.moves = get_legal_moves(node, use_NN);

	int len = node.moves.size();
	node.children.clear();
	node.children = vector<Node* >(len, NULL);

	node.num_child = len;
	node.num_child_visited = 0;

	node.win_cnt = 0.;
	node.visit_cnt = 0;

	return node.status = get_status(node, move);
}

void add_child(Node* parent, Node* child, int idx) {
	parent->children[idx] = child;
	child->parent = parent;
	parent->num_child_visited++;
}

pair<float, int> random_play(Node start, bool verbous) {

	// TEST:
	//return { 1.f, 100 };

	Status result = start.status;
	Node& node = start;

	if (verbous)
		display_board_status(node);

	int depth = 0;

	while (result == PLAYING) {
		Move move = pick_random(node.moves);
		MCTS* dummy = NULL;
		place_piece(node, move, false, dummy);
		result = node.status;

		depth++;

		if (verbous) {
			display_board_status(node);
			display_move(move);
		}
	}

	float v = NONE;
	if (result == DRAW)
		v = 0.f;
	else if (result == BLACK_WON)
		v = 1.f;
	else if (result == WHITE_WON)
		v = -1.f;

	return { v, depth };
}

#define CROP 360 // only consider top CROP number of options
vector<Move> get_legal_moves(const Node& node, const bool use_NN) {

	
	vector<pair<Move, int> > move_vs_score;
	vector<Move> imm_threat, imm_util;

	for (int x = 0; x < BOARD_WIDTH; x++) {
		for (int y = 0; y < BOARD_WIDTH; y++) {

			if (node.board_state[x][y] == NONE) { // where empty! 

				static const int dx[] = { 1, 1, 0, -1 }, dy[] = { 0, 1, 1, 1 };
				int score = 0;
				bool forbidden = false;
					
				for (int dir = 0; dir < 4; dir++) {

					const int dxx = dx[dir], dyy = dy[dir];

					// [0]: minus-, [1]: plus+ (follow idx)
					Piece piece[2] = { NONE, NONE };
					int indirect[2] = { 0, 0 };
					int direct[2] = { 0, 0 };

					// sign == -1 OR sign == 1
					// idx == 0 OR idx == 1
					for (int sign = -1, idx = 0; sign <= 1; sign += 2, idx++) {

						bool discontinued = false;

						for (int offset = 1; offset <= 6; offset++) {
							const int cur_x = x + sign * dxx * offset;
							const int cur_y = y + sign * dyy * offset;

							if (on_board(cur_x, cur_y) == false)
								break;

							const Piece cur_piece = node.board_state[cur_x][cur_y];

							if (cur_piece == NONE) {
								discontinued = true;
								indirect[idx]++;
							}
							else if (cur_piece == BLACK) {
								if (piece[idx] == NONE) { // first
									piece[idx] = BLACK;
									indirect[idx]++;
									if (discontinued == false)
										direct[idx]++;
								}
								else if (piece[idx] == BLACK) {
									indirect[idx]++;
									if (discontinued == false)
										direct[idx]++;
								}
								else { // piece[idx] == WHITE
									break;
								}
							}
							else { // cur_piece == WHITE
								if (piece[idx] == NONE) { // first
									piece[idx] = WHITE;
									indirect[idx]++;
									if (discontinued == false)
										direct[idx]++;
								}
								else if (piece[idx] == WHITE) {
									indirect[idx]++;
									if (discontinued == false)
										direct[idx]++;
								}
								else { // piece[idx] == BLACK
									break;
								}
							} // if end
						} // offset
					} // sign

					Piece my_piece = NONE;
					int my_nth_turn = -1;
					if (node.last_piece == BLACK) {
						if (node.nth_turn == NUM_TURN) {
							my_piece = WHITE;
							my_nth_turn = 1;
						}
						else {
							my_piece = BLACK;
							my_nth_turn = node.nth_turn + 1;
						}
					}
					else if (node.last_piece == WHITE) {
						if (node.nth_turn == NUM_TURN) {
							my_piece = BLACK;
							my_nth_turn = 1;
						}
						else {
							my_piece = WHITE;
							my_nth_turn = node.nth_turn + 1;
						}
					}
					else { // node.last_piece == NONE (start state)
						my_piece = BLACK;
						my_nth_turn = NUM_TURN - FIRST_NUM_TURN + 1;
					}

					Piece opp_piece = my_piece == BLACK ? WHITE : BLACK;

					int my_direct = 0, my_indirect = 0;
					int opp_direct = 0, opp_indirect = 0;

					//cout << direct[0] << ' ' << direct[1] << ' ';
					//int check = 0;

					if (piece[0] == piece[1]) {
						int total_direct = 1 + direct[0] + direct[1];
						int total_indirect = 1 + indirect[0] + indirect[1];

						if (piece[0] == my_piece) {
							my_direct = total_direct;
							my_indirect = total_indirect;
						}
						else if (piece[0] == opp_piece) {
							opp_direct = total_direct;
							opp_indirect = total_indirect;
						}
						else { // piece[0] == NONE (empty vicinity)
								// empty
								//check = 1;
						}
					}
					else if (piece[0] == NONE) {
						int bigger_direct = 1 + direct[0] + direct[1];
						int bigger_indirect = 1 + indirect[0] + indirect[1];

						int smaller_direct = 1 + direct[0];
						int smaller_indirect = 1 + indirect[0];

						if (piece[1] == my_piece) {
							my_direct = bigger_direct;
							my_indirect = bigger_indirect;

							opp_direct = smaller_direct;
							opp_indirect = smaller_indirect;
						}
						else { // piece[1] == opp_piece
							opp_direct = bigger_direct;
							opp_indirect = bigger_indirect;

							my_direct = smaller_direct;
							my_indirect = smaller_indirect;
						}
					}
					else if (piece[1] == NONE) {
						int bigger_direct = 1 + direct[0] + direct[1];
						int bigger_indirect = 1 + indirect[0] + indirect[1];

						int smaller_direct = 1 + direct[1];
						int smaller_indirect = 1 + indirect[1];

						if (piece[0] == my_piece) {
							my_direct = bigger_direct;
							my_indirect = bigger_indirect;

							opp_direct = smaller_direct;
							opp_indirect = smaller_indirect;
						}
						else { // piece[0] == opp_piece
							opp_direct = bigger_direct;
							opp_indirect = bigger_indirect;

							my_direct = smaller_direct;
							my_indirect = smaller_indirect;
						}
					}
					else if ((piece[0] == BLACK && piece[1] == WHITE) || (piece[0] == WHITE && piece[1] == BLACK)) {
						if (piece[0] == my_piece) {
							my_direct = 1 + direct[0];
							my_indirect = 1 + indirect[0];

							opp_direct = 1 + direct[1];
							opp_indirect = 1 + indirect[1];
						}
						else { // piece[1] == my_piece

							my_direct = 1 + direct[1];
							my_indirect = 1 + indirect[1];

							opp_direct = 1 + direct[0];
							opp_indirect = 1 + indirect[0];
						}
					}
					else {
						cout << "unexpected branching!!!" << endl;
						exit(0);
					}


					if (my_direct > CONNECT_K) { // forbidden move
						forbidden = true;
						break;
					}
					else if (my_indirect >= CONNECT_K && my_direct == CONNECT_K) { // immediate util (conservative)
						imm_util.push_back(Move{ x, y });
					}
					else if (opp_indirect >= CONNECT_K && opp_direct >= CONNECT_K - NUM_TURN + 1) { // immediate threat
						imm_threat.push_back(Move{ x, y });
					}
					else {
						if (my_indirect >= CONNECT_K)
							score += my_direct;
						if (opp_indirect >= CONNECT_K)
							score += opp_direct;
					}

					//cout << my_indirect << ' ' << opp_indirect << ' ';
					//cout << my_direct << ' ' << opp_direct << ' ';
					//cout << check << ' ';

				} // dir
					
				if (forbidden == true) // forbidden move: consider next move (do not add to moves vector)
					break;

				move_vs_score.push_back({ Move{ x, y }, score });
			} // if end
		} // y
	} // x
	
	if (imm_util.size() > 0) {
		return imm_util;
	}
	else if (imm_threat.size() > 0) {
		return imm_threat;
	}

	sort(move_vs_score.begin(), move_vs_score.end(), 
		[](pair<Move, int> & a, pair<Move, int> & b) {return a.second > b.second; } );

	vector<Move> moves;

	const int len = min(CROP, (int)move_vs_score.size());
	for (int i = 0; i < len; i++) {
		moves.push_back(move_vs_score[i].first);
	}

	return moves;
}

Status get_status(const Node& node, const Move& last_move) {

	if (node.nth_turn != NUM_TURN)
		if (node.moves.size() == 0)
			return DRAW;
		else
			return PLAYING;

	Piece last_piece = node.last_piece;

	int len = get_longest_connect(node, last_move);
	if (len > CONNECT_K)
		return node.last_piece == BLACK ? WHITE_WON : BLACK_WON;
	else if (len == CONNECT_K)
		return node.last_piece == BLACK ? BLACK_WON : WHITE_WON;

	else if (node.num_pieces == BOARD_WIDTH * BOARD_WIDTH)
		return DRAW;
	else
		return PLAYING;
}

int get_longest_connect(const Node& node, const Move& last_move) {
	Piece last_piece = node.last_piece;

	static const int dx[] = { 1, 1, 0, -1 }, dy[] = { 0, 1, 1, 1 };
	int max_len = -1;
	for (int dir = 0; dir < 4; dir++) {
		int len = 1;

		int x = last_move.x + dx[dir], y = last_move.y + dy[dir];
		while (on_board(x, y) && node.board_state[x][y] == last_piece) {
			len++;
			x += dx[dir];
			y += dy[dir];
		}

		x = last_move.x - dx[dir], y = last_move.y - dy[dir];
		while (on_board(x, y) && node.board_state[x][y] == last_piece) {
			len++;
			x -= dx[dir];
			y -= dy[dir];
		}

		max_len = max_len < len ? len : max_len;
	}
	return max_len;
}

inline bool on_board(const int x, const int y) {
	return 0 <= x && x < BOARD_WIDTH && 0 <= y && y < BOARD_WIDTH;
}

#define MAX_CMD_LEN 20
void explore_tree(const Node& start_node) {
	static const char* piece_str[] = { "NONE", "BLACK", "WHITE" };
	Node const * cur_node = &start_node;
	while (true) {
		if (cur_node->parent == NULL) cout << "[ROOT_NODE]" << endl;
		display_board_status(*cur_node);
		cout << "last played by: " << piece_str[(int)(cur_node->last_piece)] << endl;
		cout << cur_node->nth_turn << "th placement" << endl;
		cout << "children visited: " << cur_node->num_child_visited << "/" << cur_node->num_child << endl;
		cout << "win/visit: " << cur_node->win_cnt << "/" << cur_node->visit_cnt <<
			" = " << ((double)cur_node->win_cnt / cur_node->visit_cnt) << endl;

		skip_output:
		char input_cmd[MAX_CMD_LEN + 1];
		cout << ">>>";
		cin.getline(input_cmd, MAX_CMD_LEN);
		if (strcmp(input_cmd, "help") == 0 || strcmp(input_cmd, "h") == 0) {
			cout << "commands: parent, moves, (x,y)" << endl;
			goto skip_output;
		}
		else if (strcmp(input_cmd, "parent") == 0) {
			cur_node = cur_node->parent;
		}
		else if (strcmp(input_cmd, "moves") == 0) {
			for (const Move& move : cur_node->moves) {
				cout << "(" << move.x << "," << move.y << ")";
			}
			cout << endl;
			goto skip_output;
		}
		else if (input_cmd[0] == '(') {
			int x, y;
			sscanf(input_cmd, "(%d,%d)", &x, &y);
			int len = cur_node->moves.size();
			int i;
			for (i = 0; i < len; i++) {
				const Move& move = cur_node->moves[i];
				if (x == move.x && y == move.y)
					break;
			}
			if (i < len) {
				cur_node = cur_node->children[i];
			}
			else
				cout << "that move is not possible" << endl;
		}
		else if (input_cmd[0] == 'q') {
			return;
		}
	}
}

void display_board_status(const Node& node) {
	static const char* status_str[] = { "PLAYING", "BLACK_WON", "WHITE_WON", "DRAW" };
	static const char piece_char[] = { '.', 'X', 'O', '?' };

	cout << "    ";
	for (int x = 0; x < BOARD_WIDTH; x++) {
		cout << (x >= 10 ? x - 10 : x) << " ";
	}
	cout << endl;
	for (int y = 0; y < BOARD_WIDTH; y++) {
		cout << "  ";
		cout << (y >= 10 ? y - 10 : y) << " ";
		for (int x = 0; x < BOARD_WIDTH; x++) {
			cout << piece_char[(int)(node.board_state[x][y])] << " ";
		}
		cout << endl;
	}
	cout << status_str[(int)(node.status)] << endl;

}
void display_move(const Move& move) {
	cout << "(" << move.x << ", " << move.y << ")" << endl;
}

template <typename T>
T pick_random(vector<T>& items) {
	return items[rand() % items.size()];
}

void initialize(void) {
	srand((unsigned int)time(NULL));
}

void generate_block(Node* node, MCTS* mcts, vector<float> *flattened_block) {
	Piece whose_turn = about_to_play(node);

	flattened_block->resize(BOARD_WIDTH * BOARD_WIDTH * CHANNEL_NUM);
	float (*internal_array)[BOARD_WIDTH][CHANNEL_NUM] = 
		(float(*)[BOARD_WIDTH][CHANNEL_NUM])(flattened_block->data());


	// 0th channel
	const float zeroth_channel_val = whose_turn == BLACK ? 1. : 0.;
	for (int x = 0; x < BOARD_WIDTH; x++) {
		for (int y = 0; y < BOARD_WIDTH; y++) {
			internal_array[x][y][0] = zeroth_channel_val;
		}
	}

	// 1st channel, 2nd channel
	for (int x = 0; x < BOARD_WIDTH; x++) {
		for (int y = 0; y < BOARD_WIDTH; y++) {
			for (int channel = 0; channel < TURN_HISTORY_NUM; channel++) {
				internal_array[x][y][1 + channel] = node->black_state[x][y];
				internal_array[x][y][1 + TURN_HISTORY_NUM + channel] = node->white_state[x][y];
			}
		}
	}
}

void generate_block_value(Node* node, MCTS* mcts, vector<float> *value_flattened_block) {
	Piece whose_turn = about_to_play(node);

	float (*policy_array)[BOARD_WIDTH] = node->policy;

	value_flattened_block->resize(BOARD_WIDTH * BOARD_WIDTH * (CHANNEL_NUM + 1));
	float (*value_internal_array)[BOARD_WIDTH][CHANNEL_NUM + 1] = 
		(float(*)[BOARD_WIDTH][CHANNEL_NUM + 1])(value_flattened_block->data());


	// 0th channel
	const float zeroth_channel_val = whose_turn == BLACK ? 1. : 0.;
	for (int x = 0; x < BOARD_WIDTH; x++) {
		for (int y = 0; y < BOARD_WIDTH; y++) {
			value_internal_array[x][y][0] = zeroth_channel_val;
		}
	}

	// 1st channel, 2nd channel
	for (int x = 0; x < BOARD_WIDTH; x++) {
		for (int y = 0; y < BOARD_WIDTH; y++) {
			for (int channel = 0; channel < TURN_HISTORY_NUM; channel++) {
				value_internal_array[x][y][1 + channel] = node->black_state[x][y];
				value_internal_array[x][y][1 + TURN_HISTORY_NUM + channel] = node->white_state[x][y];
			}
			value_internal_array[x][y][1 + 2 * TURN_HISTORY_NUM] = policy_array[x][y];
		}
	}
}