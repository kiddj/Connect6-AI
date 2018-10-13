#include <iostream>
#include <vector>
#include <thread>
#include "tree.h"
using namespace std;

bool avail_flag[4] = { false, };
Channel channel[4];

int main() {
	initialize();
	thread black_player(play_ai, 0, 2);
	thread white_player(play_ai, 1, 3);

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

void play_ai(const int in_ch, const int out_ch) {
	
	MCTS* mcts = new MCTS();
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