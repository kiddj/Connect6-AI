#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"

#include "tree.h"
#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <chrono>
using namespace std;

MCTS::MCTS(const bool _use_NN) :
	use_NN(_use_NN),
	cur_node(NULL),
	playing(NONE),
	first(true),
	turns(0),
	black_log(TURN_HISTORY_NUM, NULL),
	white_log(TURN_HISTORY_NUM, NULL){
	cur_node = alloc_Node();
	for (int i = 0; i < FIRST_NUM_TURN; i++) {
		expand(cur_node, 0, true, this);
		set_new_root(cur_node->children[0]);
		
	}
}

void MCTS::set_new_root(Node* new_root) {
	set_root_node(new_root);

	if (cur_node->last_piece == BLACK) {
		black_log.push_front(cur_node);
		black_log.pop_back();
	}
	else if (cur_node->last_piece == WHITE) {
		white_log.push_front(cur_node);
		white_log.pop_back();
	}

	recursive_free(cur_node, false);
	cur_node = new_root;
}

Status MCTS::one_turn(const int x1, const int y1,
	const int x2, const int y2,
	const bool start,
	int& new_x1, int& new_y1, int& new_x2, int& new_y2) {

	static const char* piece_to_str[] = { "NONE", "BLACK", "WHITE" };

	if (start) {
		playing = WHITE;
	}
	else {
		if (first) {
			playing = BLACK;
		}
		const Move opp_moves[NUM_TURN] = { { x1, y1 },{ x2, y2 } };
		for (int opp_move_cnt = 0; opp_move_cnt < NUM_TURN; opp_move_cnt++) {
			const int len = cur_node->moves.size();
			const Move& cur_opp_move = opp_moves[opp_move_cnt];
			Node* next_node = NULL;
			int idx_to_expand = -1;
			for (int i = 0; i < len; i++) {
				if (cur_opp_move == cur_node->moves[i]) {
					next_node = cur_node->children[i];
					idx_to_expand = i;
					break;
				}
			}
			if (next_node == NULL) {
				if (idx_to_expand == -1) {
					cur_node->moves.resize(1);
					cur_node->moves[0] = cur_opp_move;
					idx_to_expand = 0;
				}
				next_node = expand(cur_node, idx_to_expand, (opp_move_cnt == NUM_TURN - 1), this);
			}
			set_new_root(next_node);
		}
	}
	int x[NUM_TURN], y[NUM_TURN];
	for (int i = 0; i < NUM_TURN; i++) {
		pair<Move, Node* > result = get_best_move_child(cur_node, PLAYTHROUGH_LIMIT, TIME_LIMIT_SEC);
		set_new_root(result.second);

		x[i] = result.first.x;
		y[i] = result.first.y;

		cout << piece_to_str[(int)playing] << " " << (turns++) << ": " << (use_NN ? "using NN" : "heuristic") << endl;
		display_board_status(*cur_node);

		/* vector<float> flattened_block;
		generate_block(cur_node, this, &flattened_block);
		for(int i = 0; i < flattened_block.size(); i++)
			cout << flattened_block.at(i);
		cout << endl; */
	}

	new_x1 = x[0], new_y1 = y[0], new_x2 = x[1], new_y2 = y[1];

	first = false;

	return cur_node->status;
}

pair<Move, Node* > MCTS::get_best_move_child(Node* cur_node, int num_playout, int seconds) {

	int begin_time = (int)time(NULL);

	int total_tree_height = 0;
	int max_tree_height = -1;
	int playout = 1;
	int mask = num_playout == -1 ? 0 : 1;
	num_playout = num_playout == -1 ? 1 : num_playout;

	using namespace std::chrono;
	steady_clock::time_point loop_start_time = steady_clock::now();
	for (; playout * mask < num_playout; playout++) {
		
		steady_clock::time_point cur_time = steady_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(cur_time - start_time);
		double spent_time = time_span.count();
		if (spent_time > 3.8)
			break;

		int end_time = (int)time(NULL);
		if (end_time - begin_time >= seconds)
			break;
		
		pair <pair <Node*, int>, int > select_result_pair = node_select(cur_node); // leaf: children not all visited OR terminal state
		pair <Node*, int> leaf = select_result_pair.first;
		
		int cur_tree_height = select_result_pair.second;
		total_tree_height += cur_tree_height;
		max_tree_height = max_tree_height > cur_tree_height ?
			max_tree_height : cur_tree_height;
		
		if (leaf.first->status == PLAYING) {
			Node* new_leaf = expand(leaf.first, leaf.second, true, this);
			update(new_leaf);
		}
		else {
			update(leaf.first);
		}
	}

	steady_clock::time_point loop_end_time = steady_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(loop_end_time - loop_start_time);
	double spent_time = time_span.count();

	cout << endl;
	cout << "average tree search depth: " << (double)total_tree_height / playout << endl;
	cout << "playout #: " << playout << endl;
	cout << "average time(ms): " << 1000 * spent_time / playout << endl;
	cout << "max tree search depth: " << max_tree_height << endl;
	double max_win_rate = -2.;
	int res = -1;
	int len = cur_node->moves.size();
	for (int i = 0; i < len; i++) {
		if (cur_node->children[i] == NULL)
			continue;

		Node& cur_child = *(cur_node->children[i]);
		double win_rate = (double)cur_child.win_cnt / cur_child.visit_cnt;
		// double win_rate = cur_child.visit_cnt;
		if (max_win_rate < win_rate) {
			res = i;
			max_win_rate = win_rate;
		}
	}

	if (res != -1)
		cout << "win_rate: " << (int)((cur_node->children[res]->win_cnt / cur_node->children[res]->visit_cnt + 1) / 2 * 100) << "%" << endl;
	return { cur_node->moves[res], cur_node->children[res] };
}

pair <pair <Node*, int>, int > node_select(Node* root) {
	Node* cur_node = root;
	int tree_height = 1;
	
	while (cur_node->status == PLAYING) {
	
		int best_next_node_idx = -2;
		double best_score = -999.;
		Node* best_next_node = NULL;
		const int parent_visit_cnt =  cur_node->visit_cnt;
		const int len = cur_node->moves.size();
		for (int i = 0; i < len; i++) {
			Node* next_node = cur_node->children[i];
			Move& move = cur_node->moves[i];
			const float prior_prob = cur_node->policy[move.x][move.y];
			const double score = get_score(next_node, prior_prob, parent_visit_cnt);
			if (best_score < score) {
				best_next_node_idx = i;
				best_score = score;
				best_next_node = next_node;
			}
		}
		if (best_next_node == NULL) {
			
			return { { cur_node, best_next_node_idx}, tree_height };
		}
		cur_node = best_next_node;
		tree_height++;
	}
	
	return { { cur_node, -1 }, tree_height };
}

Node* expand(Node* old_leaf, const int idx_to_expand, const bool use_NN, MCTS* mcts) {

	const Move move = old_leaf->moves[idx_to_expand];

	Node* new_leaf = copy_Node(*old_leaf);
	place_piece(*new_leaf, move, use_NN, mcts);
	add_child(old_leaf, new_leaf, idx_to_expand);

	return new_leaf;
}

void update(Node* leaf) {
	const Piece about_to = about_to_play(leaf);
	const float value = leaf->value;

	Node* cur_node = leaf;
	while (cur_node != NULL) {

		cur_node->visit_cnt++;

		if (about_to == cur_node->last_piece)
			cur_node->win_cnt += value;
		else
			cur_node->win_cnt -= value;


		/*if (cur_node->visit_cnt < abs(cur_node->win_cnt)) {
			explore_tree(*cur_node);
		}*/

		cur_node = cur_node->parent;
	}
}

#define C 1
inline double get_score(const Node* node, const float prior_prob, const int parent_visit) {
	
	if (node == NULL) { // Q == N == 0, return only U (upper bound) value
		return (double)C * prior_prob * sqrt(parent_visit);
	}

	const int child_visit = node->visit_cnt;
	const float child_win = node->win_cnt;
	//return (double)child_win / child_visit + (double)2 * C * sqrt(2 * log(parent_visit) / child_visit);
	return (double)child_win / child_visit + 
		(double)C * prior_prob *  sqrt(parent_visit) / (1 + child_visit);
}

vector<float> policy_network(vector<float> flattened_block, 
		const bool use_NN) {

	if (use_NN == false) {
		vector<float> policy_1d(BOARD_WIDTH * BOARD_WIDTH, 0.);
		float(*policy_2d)[BOARD_WIDTH] = (float(*)[BOARD_WIDTH])policy_1d.data();

		float total = 0.;
		for (int x = 0; x < BOARD_WIDTH; x++)
			for (int y = 0; y < BOARD_WIDTH; y++)
				total += (policy_2d[x][y] = (float)(1));

		for (int x = 0; x < BOARD_WIDTH; x++)
			for (int y = 0; y < BOARD_WIDTH; y++)
				policy_2d[x][y] /= total;

		return policy_1d;
	}
	const auto shared = fdeep::shared_float_vec(fplus::make_shared_ref<fdeep::float_vec>(flattened_block));
    fdeep::tensor3 input = fdeep::tensor3(fdeep::shape_hwc(19, 19, 3), shared); // converted to tensor form
static fdeep::model model = fdeep::load_model("resnet3.json"); // load a model only once
	fdeep::tensor3s results = model.predict({input});  // tensor3s(input) -> NN -> tensor3s(output)
    fdeep::tensor3 result = results[0];  // tensor3s -> tensor3
    vector<float> result_vec = *result.as_vector();  // tensor3 -> vector<float>

    return result_vec;
}
float value_network(vector<float> flattened_block, void* node, const bool use_NN) {

	if (use_NN == false) {
		const pair<float, int> result_pair = random_play(*(Node*)node, false);
		const float value = result_pair.first;

		Piece about_to_play_who = about_to_play((Node*)node);

		return about_to_play_who == BLACK ? value : - value;
	}

	const auto shared = fdeep::shared_float_vec(fplus::make_shared_ref<fdeep::float_vec>(flattened_block));
    fdeep::tensor3 input = fdeep::tensor3(fdeep::shape_hwc(19, 19, 4), shared);
    static fdeep::model value_model = fdeep::load_model("value.json"); // load value net only once
    
    fdeep::tensor3s results = value_model.predict({input});  // tensor3s(input) -> NN -> tensor3s(output)
    
    fdeep::tensor3 result = results[0];  // tensor3s -> tensor3
    std::vector<float> result_vec = *result.as_vector();  // tensor3 -> vector<float>

    return result_vec[0];



	// const pair<float, int> result_pair = random_play(*(Node*)node, false);
	// const float value = result_pair.first;

	// Piece about_to_play_who = about_to_play((Node*)node);

	// return about_to_play_who == BLACK ? value : - value;
}
	