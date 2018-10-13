#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <winsock.h>
#include <ctime>
#include "tree.h"

#pragma warning (disable:4996)
#define height 19
#define width 19
#define cnt 2

using namespace std;

fdeep::model model = fdeep::load_model("model_50.json");
int board[height][width];
int x[2], y[2];

int isFree(int x, int y){
	return x >= 0 && y >= 0 && x < width && y < height && board[18-y][x] == 0;
}

void print_board() {

	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 19; j++) {
			printf("%d ", board[i][j]);
		}
		printf("\n");
	}
}

void put_stone() {
	int new_x1, new_y1, new_x2, new_y2;

	const bool use_NN = true;

	MCTS* mcts = new MCTS(use_NN, board);

	mcts->one_turn(new_x1, new_y1, new_x2, new_y2);

	x[0] = new_x1, y[0] = new_y1, x[1] = new_x2, y[1] = new_y2;
}

int main() {

	srand((unsigned int)time(0));
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAddr;

	char message[500];
	int strlen;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup() errer!" << endl;
		return -1;
	}

	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET) {
		cout << "socket error" << endl;
		return -1;
	}

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAddr.sin_port = htons(8052);

	if (connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		cout << "connect error" << endl;
		return -1;
	}

	while (true)
	{
		strlen = recv(hSocket, message, sizeof(message) - 1, 0);
		if (strlen == 0) continue;

		message[strlen] = 0;
		if (strlen == -1) {
			cout << "read error" << endl;
			return -1;
		}

		for (int i = 18; i >= 0; i--)
			for (int j = 0; j < 19; j++)
				board[i][j] = message[(18 - i) * 19 + j] - '0';
				
		put_stone();
		
		char sendmsg[8];
		sendmsg[0] = x[0] / 10 + '0';
		sendmsg[1] = x[0] % 10 + '0';
		sendmsg[2] = y[0] / 10 + '0';
		sendmsg[3] = y[0] % 10 + '0';
		sendmsg[4] = x[1] / 10 + '0';
		sendmsg[5] = x[1] % 10 + '0';
		sendmsg[6] = y[1] / 10 + '0';
		sendmsg[7] = y[1] % 10 + '0';

		send(hSocket, sendmsg, sizeof(sendmsg), 0);
		break;
	}
	printf("Closing Socket \n");
	closesocket(hSocket);

	WSACleanup();
	return 0;
}