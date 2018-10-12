#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"
#include "tree.h"

#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

using namespace std;

MCTS::MCTS() :
	cur_node(NULL),
	playing(NONE),
	first(true),
	turns(0),
	black_log(TURN_HISTORY_NUM, NULL),
	white_log(TURN_HISTORY_NUM, NULL) {

	cur_node = alloc_Node();
	for (int i = 0; i < FIRST_NUM_TURN; i++) {
		pair<Move, Node* > result = get_best_move_child(cur_node, PLAYTHROUGH_LIMIT, TIME_LIMIT_SEC);
		set_new_root(result.second);
	}
}

void MCTS::set_new_root(Node*& new_root) {
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
		playing = BLACK;
	}
	else {
		if (first) {
			playing = WHITE;
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
				next_node = expand(cur_node, idx_to_expand, false, this);
			}
			set_new_root(next_node);
		}
	}

	cout << "model before" << endl;
	int x[NUM_TURN], y[NUM_TURN];
	const fdeep::model model = fdeep::load_model("model_50.json"); // load a model only once
	cout << "model after" << endl;
	for (int i = 0; i < NUM_TURN; i++) {
		vector<float> flattened_block; // input blocks in 1D-vector form
		flattened_block.reserve(19 * 19 * 5);
		cout << "generate before" << endl;
		generate_block(cur_node, this, &flattened_block); // generate input blocks
		cout << "generate after" << endl;

		// put block into the network and get top 5 good candidates
		vector<size_t> top5_index = policy_network(model, flattened_block);
		for(int i = 0; i < 5; i++)
			cout << top5_index[i] << " ";
		cout << endl;

		pair<Move, Node* > result = get_best_move_child(cur_node, PLAYTHROUGH_LIMIT, TIME_LIMIT_SEC);
		set_new_root(result.second);

		x[i] = result.first.x;
		y[i] = result.first.y;

		cout << piece_to_str[(int)playing] << " " << (turns++) << endl;
		display_board_status(*cur_node);		
	}

	new_x1 = x[0], new_y1 = y[0], new_x2 = x[1], new_y2 = y[1];

	first = false;

	return cur_node->status;
}

pair<Move, Node* > MCTS::get_best_move_child(Node* cur_node, int num_playout, int seconds) {

	int begin_time = (int)time(NULL);

	int total_tree_height = 0;
	int max_tree_height = -1;
	int playout = 0;
	int mask = num_playout == -1 ? 0 : 1;
	num_playout = num_playout == -1 ? 1 : num_playout;
	for (; playout * mask < num_playout; playout++) {
		
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
			Node* new_leaf = expand(leaf.first, leaf.second, false, this);
			update(new_leaf);
		}
		else {
			update(leaf.first);
		}
		
	}
	cout << endl;
	cout << "average tree search depth: " << (double)total_tree_height / playout << endl;
	cout << "playout #: " << playout << endl;
	cout << "average time(ms): " << (double)1000 * seconds / playout << endl;
	cout << "max tree search depth: " << max_tree_height << endl;
	double max_win_rate = -1.;
	int res = -1;
	int len = cur_node->moves.size();
	for (int i = 0; i < len; i++) {
		if (cur_node->children[i] == NULL)
			continue;

		Node& cur_child = *(cur_node->children[i]);
		//double win_rate = (double)cur_child.win_cnt / cur_child.visit_cnt;
		double win_rate = cur_child.visit_cnt;
		if (max_win_rate < win_rate) {
			res = i;
			max_win_rate = win_rate;
		}
	}

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

vector<size_t> policy_network(const fdeep::model& model, vector<float>& block) {
	const auto shared = fdeep::shared_float_vec(fplus::make_shared_ref<fdeep::float_vec>(block));
    fdeep::tensor3 input = fdeep::tensor3(fdeep::shape_hwc(19, 19, 5), shared); // converted to tensor form

	fdeep::tensor3s results = model.predict({input});  // tensor3s(input) -> NN -> tensor3s(output)
    fdeep::tensor3 result = results[0];  // tensor3s -> tensor3
    vector<float> result_vec = *result.as_vector();  // tensor3 -> vector<float>
    vector<size_t> sorted_index = sort_index(result_vec); // sort with keeping track of indexes
	vector<size_t> top5_index;
	for(int i = 0; i < 5; i++)
		top5_index.push_back(sorted_index[i]);
	
	return top5_index;	
}

float value_network(vector<float> flattened_block, void* node) {
	const pair<float, int> result_pair = random_play(*(Node*)node, false);
	const float value = result_pair.first;

	Piece about_to_play_who = about_to_play((Node*)node);

	return about_to_play_who == BLACK ? value : - value;
}

template <typename T>
std::vector<size_t> sort_index(const std::vector<T> &v) {
    // Initialize original index locations
    std::vector<size_t> idx(v.size());
    std::iota(idx.begin(), idx.end(), 0);

    // Sort indexes based on comparing values in v
    std::sort(idx.begin(), idx.end(),
        [&v] (size_t i, size_t j) {
            return v[i] > v[j];
        });
    return idx;
}