#include <iostream>
#include <vector>
#include <thread>
#include "tree.h"
using namespace std;

bool avail_flag[4] = { false, };
Channel channel[4];



int main(int argc, char** argv) {
	initialize();
	bool black_ai = false, white_ai = false;
	if (argc < 3) {
		cerr << "needs 2 arguement: black, white" << endl;
	}
	
	black_ai = argv[1][0] == '1';
	white_ai = argv[2][0] == '1';

	thread black_player(play_ai, 0, 2, black_ai);
	thread white_player(play_ai, 1, 3, white_ai);

	int cur_player = 0; 
	send_output(-1, -1, -1, -1, true, cur_player); // to black

	while (true) {
		int x1, y1, x2, y2;
		bool dummy = false;
		recv_input(x1, y1, x2, y2, dummy, cur_player + 2); // from a
		send_output(x1, y1, x2, y2, false, 1 - cur_player); // to b
		cur_player = 1 - cur_player;
	}

	white_player.join();
	black_player.join();

	return 0;
}
void play_ai(const int in_ch, const int out_ch, const bool use_NN) {
	const fdeep::model model = fdeep::load_model("model_50.json"); // load a model only once
	MCTS* mcts = new MCTS(use_NN, &model);
	Status status = PLAYING;
	while (status == PLAYING) {
		int x1, y1, x2, y2;
		bool start;
		int new_x1, new_y1, new_x2, new_y2;

		recv_input(x1, y1, x2, y2, start, in_ch);
		status = mcts->one_turn(x1, y1, x2, y2, start, new_x1, new_y1, new_x2, new_y2);
		send_output(new_x1, new_y1, new_x2, new_y2, false, out_ch);

		if (status != PLAYING)
			exit(0);
	}
	return;


}

void recv_input(int& x1, int& y1, int& x2, int& y2, bool& start, const int ch) {
	while (avail_flag[ch] == false)
		this_thread::sleep_for(chrono::milliseconds(100));
	

	x1 = channel[ch].x1;
	y1 = channel[ch].y1;
	x2 = channel[ch].x2;
	y2 = channel[ch].y2;
	start = channel[ch].start;
	
	avail_flag[ch] = false;
}

void send_output(const int x1, const int y1, const int x2, const int y2, const bool start, const int ch) {
	while (avail_flag[ch] == true)
		this_thread::sleep_for(chrono::milliseconds(100));

	channel[ch].x1 = x1;
	channel[ch].y1 = y1;
	channel[ch].x2 = x2;
	channel[ch].y2 = y2;
	channel[ch].start = start;
	
	avail_flag[ch] = true;
}