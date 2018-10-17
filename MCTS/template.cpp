//
//#include "fdeep/fdeep.hpp"
//#include "fplus/fplus.hpp"
//
//#include <WinSock2.h>
//#include <iostream>
//#include <stdio.h>
//#include <string>
//#include <ctime>
//#include "tree.h"
//#define		BUF_SIZE	1024
//#define		IPAddress	"127.0.0.1"
//#define		PORT		8052	// white: 8052 black: 8053
//#define		WIDTH		19
//#define		HEIGHT		19
//#define		CNT			2
//#define		CORD_X(X)	((WIDTH - 1) - X)
//#define		CORD_Y(Y)	(Y)
//#define		CORD(X,Y)	{(X)^=(Y); (Y)^=(X); (X)^=(Y);}
//
//#pragma warning(disable: 4996)
//
//using namespace std;
//
//fdeep::model model = fdeep::load_model("model_50.json"); // load a model only once
//MCTS mcts(true);
//
//int board[WIDTH][HEIGHT];
//int op_x[2];
//int op_y[2];
//int x[2];
//int y[2];
//
//void print_board() {
//
//	printf("\n    ---------------------------------------------------------\n");
//
//	for (int i = 0; i < WIDTH; i++) {
//		printf("%3d", 18 - i);
//		for (int j = 0; j < HEIGHT; j++) {
//			printf("|%c", board[i][j] == 1 ? 'O' : board[i][j] == 2 ? 'X' : ' ');
//		}
//		printf("|\n");
//	}
//	printf("   ----------------------------------------------------------\n   ");
//	for (int i = 0; i < WIDTH; i++) printf("%3d", i);
//
//}
//
//int isFree(int X, int Y) {
//	return X >= 0 && Y >= 0 && X < WIDTH && Y < HEIGHT && !board[CORD_X(X)][CORD_Y(Y)];
//}
//
//void put_stone() {
//
//	// YOUT ALGORITHM
//	for (int i = 0; i < CNT; i++) {
//		do {
//			x[i] = rand() % WIDTH;
//			y[i] = rand() % HEIGHT;
//			CORD(x[i], y[i]);
//		} while (!isFree(x[i], y[i]));
//		CORD(x[i], y[i]);
//
//		if (x[1] == x[0] && y[1] == y[0]) i--;
//	}
//}
//
//int main() {
//
//	WORD		wVersionRequested;
//	WSADATA		wsaData;
//	SOCKADDR_IN servAddr, cliAddr; //Socket address information
//	int			err;
//	char        buf[BUF_SIZE];
//
//	srand((unsigned int)time(0));
//
//	wVersionRequested = MAKEWORD(1, 1);
//	err = WSAStartup(wVersionRequested, &wsaData);
//
//	if (err != 0) {
//		cout << "WSAStartup error " << WSAGetLastError() << endl;
//		WSACleanup();
//		return false;
//	}
//
//	servAddr.sin_family = AF_INET; // address family Internet
//	servAddr.sin_port = htons(PORT); //Port to connect on
//	servAddr.sin_addr.s_addr = inet_addr(IPAddress); //Target IP
//
//	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
//	if (s == INVALID_SOCKET)
//	{
//		cout << "Socket error " << WSAGetLastError() << endl;
//		WSACleanup();
//		return false; //Couldn't create the socket
//	}
//
//	if (::bind(s, reinterpret_cast<SOCKADDR *>(&servAddr), sizeof(servAddr)) == SOCKET_ERROR) {
//		cout << "Binding failed. Error code: " << WSAGetLastError() << endl;
//		WSACleanup();
//		return false; //Couldn't connect
//	}
//
//	cout << "Waiting for client..." << endl;
//
//	cout << "Starting initialization" << endl;
//	cout << "0" << endl;
//	
//	cout << "0.5" << endl;
//	Status status = PLAYING;
//	cout << "Initialization complete" << endl;
//
//	while (true) {
//
//		listen(s, 5);
//		cout << "1" << endl;
//		int cSize = sizeof(cliAddr);
//		SOCKET s2 = accept(s, reinterpret_cast<SOCKADDR *>(&cliAddr), &cSize);
//		cout << "2" << endl;
//		//cout << "Connection established. New socket num is " << s2 << endl << endl;
//
//		int n;
//		n = recv(s2, buf, sizeof(buf), NULL);
//		if (n <= 0) { cout << "Got nothing" << endl; break; }
//		buf[n] = 0;
//
//		for (int i = 0; i < 19; i++)
//			for (int j = 0; j < 19; j++)
//				board[18 - i][j] = buf[i * 19 + j] - '0';
//
//		op_x[0] = (buf[361] - '0') * 10 + (buf[362] - '0');
//		op_y[0] = (buf[363] - '0') * 10 + (buf[364] - '0');
//		op_x[1] = (buf[365] - '0') * 10 + (buf[366] - '0');
//		op_y[1] = (buf[367] - '0') * 10 + (buf[368] - '0');
//
//		printf("\n\nOpposite: %d %d / %d %d", op_x[0], op_y[0], op_x[1], op_y[1]);
//
//		put_stone();
//		//print_board();
//
//		char sbuf[8];
//		sbuf[0] = x[0] / 10 + '0';
//		sbuf[1] = x[0] % 10 + '0';
//		sbuf[2] = y[0] / 10 + '0';
//		sbuf[3] = y[0] % 10 + '0';
//		sbuf[4] = x[1] / 10 + '0';
//		sbuf[5] = x[1] % 10 + '0';
//		sbuf[6] = y[1] / 10 + '0';
//		sbuf[7] = y[1] % 10 + '0';
//
//		send(s2, sbuf, sizeof(sbuf), NULL);
//		closesocket(s2);
//	}
//
//	closesocket(s);
//	WSACleanup();
//
//	return 0;
//}

#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"

#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <ctime>
#include "tree.h"

#define		BUF_SIZE	1024
#define		IPAddress	"127.0.0.1"
#define		PORT		8052	//white:8052, black:8053
#define		WIDTH		19
#define		HEIGHT		19
#define		CNT			2
#define		CORD_X(X)	((WIDTH - 1) - X)
#define		CORD_Y(Y)	(Y)
#define		CORD(X,Y)	{(X)^=(Y); (Y)^=(X); (X)^=(Y);}

#pragma warning(disable: 4996)

using namespace std;

fdeep::model model = fdeep::load_model("model_50.json"); // load a model only once
MCTS mcts(true);

int board[WIDTH][HEIGHT];
int op_x[2];
int op_y[2];
int x[2];
int y[2];


void print_board() {

	printf("\n    ---------------------------------------------------------\n");

	for (int i = 0; i < WIDTH; i++) {
		printf("%3d", 18-i);
		for (int j = 0; j < HEIGHT; j++) {
			printf("|%c", board[i][j] == 1 ? 'O' : board[i][j] == 2 ? 'X' : ' ');
		}
		printf("|\n");
	}
	printf("   ----------------------------------------------------------\n   ");
	for (int i = 0; i < WIDTH; i++) printf("%3d", i);

}

int isFree(int X, int Y) {
	return X >= 0 && Y >= 0 && X < WIDTH && Y < HEIGHT && !board[CORD_X(X)][CORD_Y(Y)];
}

void put_stone() {

	// YOUT ALGORITHM
	for (int i = 0; i < CNT; i++) {
		do {
			x[i] = rand() % WIDTH;
			y[i] = rand() % HEIGHT;
			CORD(x[i], y[i]);
		} while (!isFree(x[i], y[i]));
		CORD(x[i], y[i]);

		if (x[1] == x[0] && y[1] == y[0]) i--;
	}
}

int main() {

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

	if (::bind(s, reinterpret_cast<SOCKADDR *>(&servAddr), sizeof(servAddr)) == SOCKET_ERROR){
		cout << "Binding failed. Error code: " << WSAGetLastError() << endl;
		WSACleanup();
		return false; //Couldn't connect
	}

	cout << "Waiting for client..." << endl;

	cout << "Starting initialization" << endl;
	Status status = PLAYING;
	cout << "Initialization complete" << endl;

	while (true) {
		listen(s, 5);
		int cSize = sizeof(cliAddr);
		SOCKET s2 = accept(s, reinterpret_cast<SOCKADDR *>(&cliAddr), &cSize);
		//cout << "Connection established. New socket num is " << s2 << endl << endl;

		int n;
		n = recv(s2, buf, sizeof(buf), NULL);

		if (n <= 0) { cout << "Got nothing" << endl; break;
		}
		buf[n] = 0;

		for (int i = 0; i < 19; i++)
			for (int j = 0; j < 19; j++)
				board[18-i][j] = buf[i * 19 + j] - '0';

		op_x[0] = (buf[361]-'0')*10 + (buf[362] - '0');
		op_y[0] = (buf[363] - '0') * 10 + (buf[364] - '0');
		op_x[1] = (buf[365] - '0') * 10 + (buf[366] - '0');
		op_y[1] = (buf[367] - '0') * 10 + (buf[368] - '0');

		printf("\n\nOpposite: %d %d / %d %d", op_x[0], op_y[0], op_x[1], op_y[1]);

		cout << "choosing position..(put_stone() will not be used)" << endl;
		int x1 = op_x[0], y1 = op_y[0], x2 = op_x[1], y2 = op_y[1];
		bool start = op_x[0] == 99;
		int new_x1, new_y1, new_x2, new_y2;
		
		status = mcts.one_turn(x1, y1, x2, y2, start, new_x1, new_y1, new_x2, new_y2);
		
		x[0] = new_x1, y[0] = new_y1, x[1] = new_x2, y[1] = new_y2;
		cout << "choosing finished" << endl;

		//put_stone();
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

		send(s2, sbuf, sizeof(sbuf), NULL);
		closesocket(s2);
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

	closesocket(s);
	WSACleanup();

	return 0;
}

void log_str(char * msg) {
	cout << msg << endl;
	FILE *fp = fopen("log_aaaaa.txt", "a");
	fprintf(fp, "%s\n", msg);
	fclose(fp);
}
