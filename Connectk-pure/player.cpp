#include "mcts.h"
#include "shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctime>
#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <string>

#pragma warning (disable:4996)

#define		BUF_SIZE	1024
#define		IPAddress	"127.0.0.1"
#define		PORT		8053	// white: 8052 black: 8053
#define		WIDTH		19
#define		HEIGHT		19
#define		CNT			2
#define		CORD_X(X)	((WIDTH - 1) - X)
#define		CORD_Y(Y)	(Y)
#define		CORD(X,Y)	{(X)^=(Y); (Y)^=(X); (X)^=(Y);}

using namespace std;

int op_x[2];
int op_y[2];
int x[2];
int y[2];

bool is_white_first = false;

void print_board(Board* b) {
	for(int i = 0; i < 19; i++) {
		for(int j = 0; j < 19; j++) {
			cout << b->data[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}

void put_stone(Board* b) {
	print_board(b);
	if(b->turn == WHITE && is_white_first) {
		x[0] = 8;
		y[0] = 9;
		x[1] = 9;
		y[1] = 10;
		is_white_first = false;
		cout << "first turn" << endl;
		return;
	}	
	for(int i = 0; i < 2; i++){		
		Moves* moves = monte_carlo(b);
		sort_moves(moves);
		if(moves->len > 0) {
			x[i] = moves->list[0].x;
			y[i] = 18 - moves->list[0].y;
			place_piece(b, moves->list[0].x, moves->list[0].y);
			cout << "piece" << i << " "<< x[i] << " " << 18 - y[i] << endl;
		}
		free(moves);
		b->moves_left -= 1;
	}
}

int main() {

	freopen("log.txt", "a", stdout);

	WORD		wVersionRequested;
	WSADATA		wsaData;
	SOCKADDR_IN servAddr, cliAddr; //Socket address information
	int			err;
	char        buf[BUF_SIZE];

	srand((unsigned int)time(0));

	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);

	if (err != 0) {
		cout << "WSAStartup error " << WSAGetLastError() << endl;
		WSACleanup();
		return false;
	}

	servAddr.sin_family = AF_INET; // address family Internet
	servAddr.sin_port = htons(PORT); //Port to connect on
	servAddr.sin_addr.s_addr = inet_addr(IPAddress); //Target IP

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
	if (s == INVALID_SOCKET)
	{
		cout << "Socket error " << WSAGetLastError() << endl;
		WSACleanup();
		return false; //Couldn't create the socket
	}

	if (bind(s, reinterpret_cast<SOCKADDR *>(&servAddr), sizeof(servAddr)) == SOCKET_ERROR){
		cout << "Binding failed. Error code: " << WSAGetLastError() << endl;
		WSACleanup();
		return false; //Couldn't connect
	}

	cout << "Waiting for client..." << endl;

    Board* board = board_new();
	while (true)
	{
		listen(s, 5);
		int cSize = sizeof(cliAddr);
		SOCKET s2 = accept(s, reinterpret_cast<SOCKADDR *>(&cliAddr), &cSize);
		//cout << "Connection established. New socket num is " << s2 << endl << endl;

		int n;
		n = recv(s2, buf, sizeof(buf), 0);
		if (n <= 0) { cout << "Got nothing" << endl; break; }
		buf[n] = 0;

		for (int i = 18; i >= 0; i--)
			for (int j = 0; j < 19; j++)
				board->data[i][j] = buf[(18 - i) * 19 + j] - '0';
        board_turn_init(board);
        cout << board->turn << endl;

		op_x[0] = (buf[361]-'0')* 10 + (buf[362] - '0');
		op_y[0] = (buf[363] - '0') * 10 + (buf[364] - '0');
		op_x[1] = (buf[365] - '0') * 10 + (buf[366] - '0');
		op_y[1] = (buf[367] - '0') * 10 + (buf[368] - '0');

		printf("\nOpposite: %d %d / %d %d\n", op_x[0], op_y[0], op_x[1], op_y[1]);

		put_stone(board);
		//print_board();

		char sbuf[8];
		sbuf[0] = x[0] / 10 + '0';
		sbuf[1] = x[0] % 10 + '0';
		sbuf[2] = y[0] / 10 + '0';
		sbuf[3] = y[0] % 10 + '0';
		sbuf[4] = x[1] / 10 + '0';
		sbuf[5] = x[1] % 10 + '0';
		sbuf[6] = y[1] / 10 + '0';
		sbuf[7] = y[1] % 10 + '0';

		send(s2, sbuf, sizeof(sbuf), 0);
		cout << sbuf << endl;
		closesocket(s2);
	}
	closesocket(s);

	WSACleanup();
	return 0;
}