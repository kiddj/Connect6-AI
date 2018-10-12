#pragma once

#include <iostream>
#include <vector>
#include <deque>
using namespace std;

#define BOARD_WIDTH 19
#define CONNECT_K 6
#define FIRST_NUM_TURN 1
#define NUM_TURN 2
#define PLAYTHROUGH_LIMIT -1
#define TIME_LIMIT_SEC 2

#define TURN_HISTORY_NUM 2
#define TOTAL_HISTORY_NUM (TURN_HISTORY_NUM * 2)
#define CHANNEL_NUM (TOTAL_HISTORY_NUM + 1)

typedef enum { NONE = 0, BLACK, WHITE, BASE_CASE } Piece; // BASE_CASE used only in update for default value before entering loop.
typedef Piece Player;
typedef enum { PLAYING = 0, BLACK_WON, WHITE_WON, DRAW } Status;

#define FIRST_PIECE BLACK

typedef struct Move {
	int x, y;
} Move;
inline bool operator==(const Move& lhs, const Move& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

class Node {
public:
	// tree related members
	Node *parent;

	vector<Move> moves;
	vector<Node * > children;

	int num_child, num_child_visited;

	// board related members
	Piece board_state[BOARD_WIDTH][BOARD_WIDTH];
	float black_state[BOARD_WIDTH][BOARD_WIDTH];
	float white_state[BOARD_WIDTH][BOARD_WIDTH];
	Piece last_piece;
	int nth_turn;
	int num_pieces;
	Status status;

	// MCTS related members
	float win_cnt;
	int visit_cnt;

	// neural net related members
	// N: visit_cnt
	// W: win_cnt
	// Q: calculated on the fly = win_cnt / visit_cnt
	float value;
	float policy[BOARD_WIDTH][BOARD_WIDTH];
};

class Board_record {
	float board_state[BOARD_WIDTH][BOARD_WIDTH];
	Board_record(const Piece new_board_state[BOARD_WIDTH][BOARD_WIDTH], const Piece last_piece) {
		for (int x = 0; x < BOARD_WIDTH; x++)
			for (int y = 0; y < BOARD_WIDTH; y++) {

			}
	}
};

class MCTS {
public:
	Node* cur_node;
	Piece playing;
	bool first;
	int turns;
	deque<Node* > black_log, white_log;

	MCTS();
	void set_new_root(Node*& new_root);
	Status one_turn(const int x1, const int y1,
		const int x2, const int y2,
		const bool start,
		int& new_x1, int& new_y1, int& new_x2, int& new_y2);
	pair<Move, Node* > get_best_move_child(Node* cur_node, int num_playout, int seconds);
};

Node *alloc_Node(); // allocate with default start state
Node *copy_Node(const Node& original); // allocate with copied state
// WARNING! must initialize with place_piece after copied!

void simple_free(Node* n); // free single node
void recursive_free(Node* n, const bool free_root); // free the node and its child recursively
// ignore NULL ptrs in children vector (unvisited children)
// if free_root == true, free the root node.

void set_root_node(Node* new_root); // make a node a root.
// parent -> children[...](of new_root) = NULL (make it invisible to recursive_free())
// parent = NULL
// root = new_root

Status place_piece(Node& node, const Move& move, const bool use_NN, MCTS* mcts); // play move in node. mutates node
// mutate status member to resulting Status and return it too
// initialize parent to NULL. Have to set it with add_child().
// does not check whether node.status != PLAYING
// --> evaluate selected node with NN. store value in Node member.store policy in parent. 
// --> NN policy is used for get_legal_moves() (if use_NN == true)
// --> use_NN: if true, use the help of NN related value.(assuming they're set)
// --> else, only use classical heuristic. (NN or NN related values are not used)
// -->       also, mcts is not used
void add_child(Node* parent, Node* child, int idx); // mutate both parent and child
// idx is index for children vector. caller is responsible for finding right idx.
// increment num_child_visited.

pair<float, int> random_play(Node start, bool verbous); // random playout. mutate start
// no mutation outside this function.
// return final status.
// print intermediate steps if verbous == true.
// --> returns as v black_won:1.0, draw:0, white_won:-1.0

vector<Move> get_legal_moves(const Node& node, const bool use_NN); // get all legal(good) moves
// does not mutate node
// if node's status is not PLAYING (finished) it returns empty vector
// --> use_NN: if true, use the help of NN related value.(assuming they're set)
// --> else, only use classical heuristic. (NN or NN related values are not used)

Status get_status(const Node& node, const Move& last_move); // get status(draw, playing, or b or w win)
// does not mutate node
// use num_pieces to determine whether it's a DRAW efficiently
// last_move: last placed piece's position, used to determine whose turn it just was.
int get_longest_connect(const Node& node, const Move& last_move);
// get longest streak of pieces

inline bool on_board(const int x, const int y);
// determins if within board

void explore_tree(const Node& start_node); // tree explorer.
// loops through:
// print output (is_root?, board state, status, last_piece, nth_turn, child_num/child_num_visit, win/visit
// prompt input (show moves, move to a child, move to parent)

void display_board_status(const Node& node); // display board state & game status
void display_move(const Move& move); // display move

template <typename T>
T pick_random(vector<T>& items); // pick single random element

void initialize(void); // initialize few things e.g. seed for rand()

pair<Move, Node* > get_best_move_child(Node* cur_node, int num_playout, int seconds);

pair <pair <Node* , int> , int > node_select(Node* root); // starting from root, find terminal state, OR node with not all children visited
// always choose child with largest score.
// --> find node that is terminal OR not evaluated with NN(no children at all)
// --> so unless it is terminal, it is not yet allocated.
// --> returns { {parent_node, idx_to_expand}, tree_depth }
// --> idx_to_expand == -1 means terminal node
Node* expand(Node* old_leaf, const int idx_to_expand, const bool use_NN, MCTS* mcts); 
// add a child to selected node. does not check whether the selected is terminal.
// use add_child()
// --> return parent(expanded just now) as new_leaf
// -->(in place_piece()) evaluate selected node with NN. store value in Node member.store policy in parent. 
// --> if use_NN, use NN to evaluate new_leaf, by using mcts.<piece>_log members.
void update(Node* leaf); 
// increment win/visit counts from the expanded to root.
// --> if last_turn == about_to_play(leaf)
// --> else, -=result

inline double get_score(const Node* node, const float prior_prob, const int parent_visit); // get score. does not check whether it is root (aka have no parent)
// --> accepts a prior probability and parent_visit_cnt.
// --> check whether or not node == NULL

void generate_block(Node* node, MCTS* mcts, vector<float>* flattened_block);
// generated 1d-flattened vector representation of a 19x19x5 3d block

vector<float> policy_network(vector<float> flattened_block); // using policy NN, record node.policy matrix
// in terms of player who is ABOUT to play
float value_network(vector<float> flattened_block, void* node); // using value NN, record node.value float
// in terms of player who is ABOUT to play

inline Piece about_to_play(const Node* node); // who is ABOUT to play

void play_ai(const int in_ch, const int out_ch); // no parameter. used when simulating network based playing with multithreading.
// whether player is white or black is determined by the first network message it receives.

typedef struct Channel {
	int x1, y1, x2, y2;
	bool start;
} Channel;
void recv_input(int& x1, int& y1, int& x2, int& y2, bool& start, const int ch);
// if after the call start==true, it is first turn for black(2 placement). x1,y1,x2,y2 is not affected
// get other player's latest choice otherwise
void send_output(const int x1,  const int y1, const int x2, const int y2, const bool start, const int ch);
