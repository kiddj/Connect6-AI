#include <iostream>
#include <vector>
#include <thread>
#include "tree.h"
using namespace std;

bool avail_flag[4] = { false, };
Channel channel[4];


using namespace std::chrono;
steady_clock::time_point start_time;


int main(int argc, char** argv) {
	initialize();
	Player_type black_ai = NN, white_ai = NN;
	if (argc < 3) {
		cerr << "needs 2 arguement: black, white" << endl;
	}
	
	black_ai = (Player_type)('1' - '0');
	white_ai = (Player_type)('1' - '0');

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
void play_ai(const int in_ch, const int out_ch, const Player_type player_type) {

	if (player_type == HEURISTIC || player_type == NN) {
		
		MCTS mcts((player_type == NN));
		Status status = PLAYING;
		while (status == PLAYING) {
			int x1, y1, x2, y2;
			bool start;
			int new_x1, new_y1, new_x2, new_y2;

			recv_input(x1, y1, x2, y2, start, in_ch);
			start_time = steady_clock::now();
			status = mcts.one_turn(x1, y1, x2, y2, start, new_x1, new_y1, new_x2, new_y2);
			send_output(new_x1, new_y1, new_x2, new_y2, false, out_ch);
		}
		switch (status) {
		case PLAYING:
			cout << "playing" << endl;
			break;
		case BLACK_WON:
			cout << "BLACK WON" << endl;
			break;
		case WHITE_WON:
			cout << "WHITE WON" << endl;
			break;
		case DRAW:
			cout << "DRAW" << endl;
			break;
		default:
			cout << "unknown status after ai's turn: " << status << endl;
		}
		return;
	}
	else if (player_type == PERSON){
		Status status = PLAYING;
		while (status == PLAYING) {
			int x1, y1, x2, y2;
			bool start;
			int new_x1, new_y1, new_x2, new_y2;

			recv_input(x1, y1, x2, y2, start, in_ch);
			
			cout << endl;
			cout << "Input your position (type 4 integers with spacing x1 y1 x2 y2): ";
			cin >> new_x1 >> new_y1 >> new_x2 >> new_y2;

			send_output(new_x1, new_y1, new_x2, new_y2, false, out_ch);
		}
		return;
	}
	else {
		cerr << "unknown player_type!" << endl;
		exit(1);
	}

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