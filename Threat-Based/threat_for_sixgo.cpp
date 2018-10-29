#pragma warning(disable: 4996)
#define _SCL_SECURE_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <ctime>
#include <algorithm>
#include <tuple>
#include <time.h>
#include <vector>

using namespace std;

#define		BUF_SIZE	1024
#define		IPAddress	"127.0.0.1"
#define		PORT		8052	// white: 8052 black: 8053
#define		WIDTH		19
#define		HEIGHT		19
#define		CNT			2
#define		CORD_X(X)	((WIDTH - 1) - X)
#define		CORD_Y(Y)	(Y)
#define		CORD(X,Y)	{(X)^=(Y); (Y)^=(X); (X)^=(Y);}

typedef pair<int, int> POS;
typedef pair<POS, POS> MOVES;
typedef pair<POS, double> POS_SCORE;
typedef pair<MOVES, double> MOVES_SCORE;

int board[WIDTH][HEIGHT];
int op_x[2];
int op_y[2];
int x[2];
int y[2];

// ------------------------------------------------------------------------

#define X first
#define Y second
#define EMPTY 0
#define ME 1
#define OP 2
#define MARK 4 // 처리용 MARK
#define BOARD_SIZE 19

#define TIME_LIMIT 4
#define INF 100000000.0
#define NINF -100000000.0

#define DEPTH 7
#define BREADTH 5
#define BREADTH_FIRST 20

void log_str(char * msg) {
	cout << msg << endl;
	FILE *fp = fopen("log_new.txt", "a");
	fprintf(fp, "%s\n", msg);
	fclose(fp);
}

// 가중치
double PlayerFactor[6] = { 0.0, 1.0, 2.00, 15.00, 0.0, 0.0 };
double OpponentFactor[6] = { 0.0, 1.00, 5.00, 25.00, 0.0, 0.0 };

// 2시, 3시, 5시, 6시만 검사 (나머지는 반대방향으로)
int dir[4][2] = { { 1,1 },{ 1,0 },{ 0,1 },{ 1,-1 } };

// Deep Copy
void CopyBoard(int src[][BOARD_SIZE], int des[][BOARD_SIZE]) {
	copy(&src[0][0], &src[0][0] + BOARD_SIZE * BOARD_SIZE, &des[0][0]);
}

bool isOutOfBounds(int x, int y) {
	if (0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE) {
		return false;
	}
	return true;
}

int CountThreat(int board[][BOARD_SIZE], int player) {
	int op = 3 - player;
	int threat_cnt = 0;
	int board_tmp[BOARD_SIZE][BOARD_SIZE];
	CopyBoard(board, board_tmp);

	for (int x = 0; x < BOARD_SIZE; x++) {
		for (int y = 0; y < BOARD_SIZE; y++) {
			for (int d = 0; d < 4; d++) {
				int ch_x = dir[d][0];
				int ch_y = dir[d][1];

				if (isOutOfBounds(x + 5 * ch_x, y + 5 * ch_y))
					continue;

				int opStone = 0;

				// Threat Window
				for (int i = 0; i < 6; i++) {
					int move_board = board_tmp[x + i * ch_x][y + i * ch_y];
					if (move_board == EMPTY)
						continue;
					else if (move_board == op)
						opStone++;
					else {
						opStone = 0;
						break;
					}
				}

				if (opStone >= 4
					&& (isOutOfBounds(x - ch_x, y - ch_y) || board_tmp[x - ch_x][y - ch_y] != op)   // 전에 있을 경우 (따로 고려할 필요가 없음)
					&& (isOutOfBounds(x + 6 * ch_x, y + 6 * ch_y) || board_tmp[x + 6 * ch_x][y + 6 * ch_y] != op)) {   //7, 8목
					threat_cnt++;
					for (int i = 0; i < 6; i++) {
						if (board_tmp[x + i * ch_x][y + i * ch_y] == EMPTY)
							board_tmp[x + i * ch_x][y + i * ch_y] = MARK;
					}
				}
			}
		}
	}

	return threat_cnt;
}

int ReadMap(int board[][BOARD_SIZE], int me, int &threat_cnt, MOVES &BestMove, vector<MOVES> &MustMove) {
	vector<MOVES> ThreatList;
	int op = 3 - me;
	threat_cnt = 0;
	int board_tmp[BOARD_SIZE][BOARD_SIZE];
	CopyBoard(board, board_tmp);

	threat_cnt = 0;

	for (int x = 0; x < BOARD_SIZE; x++) {
		for (int y = 0; y < BOARD_SIZE; y++) {
			for (int d = 0; d < 4; d++) {
				int ch_x = dir[d][0];
				int ch_y = dir[d][1];
				if (isOutOfBounds(x + 5 * ch_x, y + 5 * ch_y))
					continue;

				int myStone = 0;

				for (int i = 0; i < 6; i++) {
					int board_move = board[x + i * ch_x][y + i * ch_y];
					if (board_move == EMPTY) continue;
					else if (board_move == me) myStone++;
					else {
						myStone = 0;
						break;
					}
				}

				// 이길 수 있는지 확인
				if (myStone >= 4
					&& (isOutOfBounds(x - ch_x, y - ch_y) || board[x - ch_x][y - ch_y] != me)   // 전에 있을 경우 (따로 고려할 필요가 없음)
					&& (isOutOfBounds(x + 6 * ch_x, y + 6 * ch_y) || board[x + 6 * ch_x][y + 6 * ch_y] != me)) {   //7, 8목

					POS tmp[2] = { { 0, 0 },{ 0, 0 } };
					int idx = 0;

					for (int i = 0; i < 6; i++) {
						if (board[x + i * ch_x][y + i * ch_y] == EMPTY) {
							board[x + i * ch_x][y + i * ch_y] = me;
							board_tmp[x + i * ch_x][y + i * ch_y] = me;
							tmp[idx++] = { x + i * ch_x, y + i * ch_y };
						}
					}

					// 한 개 둬서 6개 완성한 경우 나머지 하나는 피해야함
					if (idx == 1) {
						for (int i = 0; i < BOARD_SIZE; i++) {
							for (int j = 0; j < BOARD_SIZE; j++) {
								if (board[i][j] != EMPTY
									|| (i == x + 6 * ch_x && j == y + 6 * ch_y)
									|| (i == x - ch_x && j == y - ch_y))
									continue;

								tmp[1] = { i, j };
								BestMove = { tmp[0], tmp[1] };
								return 1;   // 게임 이김
							}
						}
						tmp[0] = { -1, -1 };
						tmp[1] = { -1, -1 };
					}

					BestMove = { tmp[0], tmp[1] };

					if (BestMove.first.X >= 0) return 1;   // 게임 이김
				}

				int opStone = 0;

				for (int i = 0; i < 6; i++) {
					int board_move = board_tmp[x + i * ch_x][y + i * ch_y];
					if (board_move == EMPTY) continue;
					else if (board_move == op) opStone++;
					else {
						opStone = 0;
						break;
					}
				}

				// Threat Update 
				if (opStone >= 4 // op 4개 이상 존재
					&& (isOutOfBounds(x - ch_x, y - ch_y) || board_tmp[x - ch_x][y - ch_y] != op)   // 전에 있을 경우 (따로 고려할 필요가 없음)
					&& (isOutOfBounds(x + 6 * ch_x, y + 6 * ch_y) || board_tmp[x + 6 * ch_x][y + 6 * ch_y] != op)) {   //7, 8목

					threat_cnt++;

					// 무조건 짐
					if (threat_cnt > 2) {
						MustMove.push_back({ ThreatList[0].first, ThreatList[1].first });
						return 2;
					}

					MOVES tmpMove = { { -1, -1 },{ -1, -1 } };

					for (int i = 0; i < 6; i++) {
						if (board_tmp[x + i * ch_x][y + i * ch_y] == EMPTY) {
							board_tmp[x + i * ch_x][y + i * ch_y] = MARK;
							if (tmpMove.first.X == -1) {
								tmpMove.first = { x + i * ch_x, y + i * ch_y };
							}
							else {
								tmpMove.second = { x + i * ch_x, y + i * ch_y };
							}
						}
					}

					ThreatList.push_back(tmpMove);
				}
			}
		}
	}

	if (threat_cnt == 0) return 0;
	if (threat_cnt == 1) { // threat이 1개이면
		board[ThreatList[0].first.X][ThreatList[0].first.Y] = me;
		if (CountThreat(board, me) == 0)
			MustMove.push_back({ ThreatList[0].first,{ -1, -1 } });
		board[ThreatList[0].first.X][ThreatList[0].first.Y] = EMPTY;
		return 0;
	}
	if (threat_cnt == 2) { // threat이 2개이면
		POS tmp1[2] = { ThreatList[0].first, ThreatList[0].second };
		POS tmp2[2] = { ThreatList[1].first, ThreatList[1].second };

		for (int i = 0; i < 2; i++) {
			if (tmp1[i].X == -1)
				continue;

			for (int j = 0; j < 2; j++) {
				if (tmp2[j].X == -1)
					continue;

				board[tmp1[i].X][tmp1[i].Y] = me;
				board[tmp2[j].X][tmp2[j].Y] = me;

				if (CountThreat(board, me) == 0)
					MustMove.push_back({ tmp1[i], tmp2[j] });

				board[tmp1[i].X][tmp1[i].Y] = EMPTY;
				board[tmp2[j].X][tmp2[j].Y] = EMPTY;
			}
		}
		return 0;
	}

	return 0;
}

double getSingleScore(int board[][BOARD_SIZE], POS myMove, int me) {
	int op = 3 - me;
	double score = 0.0;
	int board_tmp[BOARD_SIZE][BOARD_SIZE];
	CopyBoard(board, board_tmp);

	board_tmp[myMove.X][myMove.Y] = me;

	for (int d = 0; d < 4; d++) {
		int ch_x = dir[d][0];
		int ch_y = dir[d][1];

		for (int i = 0; i < 6; i++) {
			int x = myMove.X - i * ch_x;
			int y = myMove.Y - i * ch_y;

			if (isOutOfBounds(x, y) || isOutOfBounds(x + 5 * ch_x, y + 5 * ch_y))
				continue;

			bool isThreat = true;

			int myStone = 0;
			int opStone = 0;

			for (int j = 0; j < 6; j++) {
				int board_move = board_tmp[x + j * ch_x][y + j * ch_y];

				if (board_move == op) {
					opStone++;
					isThreat = false;
				}
				else if (board_move == me) {
					myStone++;
				}
				else if (board_move == MARK) {
					isThreat = false;
				}
			}

			if (isThreat && myStone == 4
				&& (isOutOfBounds(x - ch_x, y - ch_y) || board[x - ch_x][y - ch_y] != me)
				&& (isOutOfBounds(x + 6 * ch_x, y + 6 * ch_y) || board[x + 6 * ch_x][y + 6 * ch_y] != me)) {

				score += 10000.0;

				for (int j = 0; j < 6; j++) {
					if (board_tmp[x + j * ch_x][y + j * ch_y] == EMPTY) {
						board_tmp[x + j * ch_x][y + j * ch_y] = me;
					}
				}
			}

			if (opStone == 0
				&& (isOutOfBounds(x - ch_x, y - ch_y) || board[x - ch_x][y - ch_y] != me)
				&& (isOutOfBounds(x + 6 * ch_x, y + 6 * ch_y) || board[x + 6 * ch_x][y + 6 * ch_y] != me)) {
				score += PlayerFactor[myStone];
			}

			if (myStone == 1
				&& (isOutOfBounds(x - ch_x, y - ch_y) || board[x - ch_x][y - ch_y] != op)
				&& (isOutOfBounds(x + 6 * ch_x, y + 6 * ch_y) || board[x + 6 * ch_x][y + 6 * ch_y] != op)) {
				score += OpponentFactor[opStone];
			}
		}
	}

	return score;
}

double getDoubleScore(int board[][BOARD_SIZE], MOVES myMove, int me) {
	double score = 0.0;

	score += getSingleScore(board, myMove.first, me);
	board[myMove.first.X][myMove.first.Y] = me;
	score += getSingleScore(board, myMove.second, me);
	board[myMove.first.X][myMove.first.Y] = EMPTY;

	return score;
}

vector<MOVES_SCORE> findDoubleCandidate(int board[][BOARD_SIZE], int me, int N_ALL, int N_SINGLE) {   // 후보는 BREADTH개로 제한
	vector<MOVES_SCORE> candidate(N_ALL, { { { -1, -1 },{ -1, -1 } }, NINF });
	int op = 3 - me;
	int myThreat = 0;

	MOVES bestMove = { { -1, -1 },{ -1, -1 } };
	vector<MOVES> mustMove;
	int mapResult = ReadMap(board, me, myThreat, bestMove, mustMove);

	// 이김
	if (bestMove.first.X >= 0) {
		candidate[0] = { bestMove, INF };
		return candidate;
	}

	// 짐
	if (myThreat >= 3) {
		candidate[0] = { mustMove[0], NINF };
		return candidate;
	}

	// 그 외
	if (myThreat == 2) {
		for (auto const& canMoves : mustMove) {
			double canScore = getDoubleScore(board, canMoves, me);

			// 올바른 위치에 삽입
			int idx = N_ALL - 1;
			while (idx >= 0 && canScore > candidate[idx].second)
				idx--;
			if (idx == N_ALL - 1)
				continue;
			for (int i = N_ALL - 2; i >= idx + 1; i--)
				candidate[i + 1] = candidate[i];

			candidate[idx + 1] = { canMoves, canScore };
		}
	}
	else if (myThreat == 1) {   // 하나는 두고 하나는 빈칸 중에 탐색
		for (auto const& canMoves : mustMove) {
			POS firstMove = canMoves.first;

			// 나머지 하나는 빈칸 탐색
			for (int x = 0; x < BOARD_SIZE; x++) {
				for (int y = 0; y < BOARD_SIZE; y++) {
					if (board[x][y] != EMPTY || (x == firstMove.X && y == firstMove.Y)) continue;

					MOVES tmpMoves = { firstMove,{ x, y } };
					double canScore = getDoubleScore(board, tmpMoves, me);

					int idx = N_ALL - 1;
					while (idx >= 0 && canScore > candidate[idx].second)
						idx--;
					if (idx == N_ALL - 1)
						continue;
					for (int k = N_ALL - 2; k >= idx + 1; k--)
						candidate[k + 1] = candidate[k];

					candidate[idx + 1] = { tmpMoves, canScore };
				}
			}
		}
	}
	else if (myThreat == 0) {   // 두 개 다 탐색
		vector<POS_SCORE> candidate_first(N_SINGLE, { { -1, -1 }, -1.0 });
		int firstcnt = 0;

		for (int x = 0; x < BOARD_SIZE; x++) {
			for (int y = 0; y < BOARD_SIZE; y++) {
				if (board[x][y] != EMPTY) continue;

				double firstScore = getSingleScore(board, { x,y }, me);

				int idx = N_SINGLE - 1;
				while (idx >= 0 && firstScore > candidate_first[idx].second)
					idx--;
				if (idx == N_SINGLE - 1)
					continue;
				for (int k = N_SINGLE - 2; k >= idx + 1; k--) {
					candidate_first[k + 1] = candidate_first[k];
				}

				candidate_first[idx + 1] = { { x,y }, firstScore };
			}
		}

		for (int i = 0; i < N_SINGLE / 2; i++) {
			for (int j = i + 1; j < N_SINGLE; j++) {
				MOVES tmpMoves = { candidate_first[i].first, candidate_first[j].first };
				double canScore = getDoubleScore(board, tmpMoves, me);

				int idx = N_ALL - 1;
				while (idx >= 0 && canScore > candidate[idx].second)
					idx--;
				if (idx == N_ALL - 1)
					continue;
				for (int k = N_ALL - 2; k >= idx + 1; k--)
					candidate[k + 1] = candidate[k];

				candidate[idx + 1] = { tmpMoves, canScore };
			}
		}
	}
	return candidate;
}

MOVES_SCORE SearchBestMove(int board[][BOARD_SIZE], int me, int depth, long st_time) {
	MOVES_SCORE maxMove = { { { -1, -1 },{ -1, -1 } }, NINF };
	vector<MOVES_SCORE> candidate = findDoubleCandidate(board, me, BREADTH, BREADTH_FIRST);

	// 최선의 수
	MOVES_SCORE bestMove = candidate[0];

	// 종료 조건
	if (bestMove.second <= NINF || bestMove.second >= INF) return bestMove;
	if (depth <= 0) return bestMove;
	if (1.0 * (clock() - st_time) / (CLOCKS_PER_SEC) > TIME_LIMIT) return bestMove;

	int op = 3 - me;

	for (auto const& currentCandidate : candidate) {
		if (currentCandidate.first.first.X == -1) {
			//printf("break\n");
			break;
		}

		MOVES currentMove = currentCandidate.first;
		double currentScore = currentCandidate.second;

		//printf("CurrentMove : %d %d %d %d\n", currentMove.first.X, currentMove.first.Y, currentMove.second.X, currentMove.second.Y);

		board[currentMove.first.X][currentMove.first.Y] = me;
		board[currentMove.second.X][currentMove.second.Y] = me;

		MOVES_SCORE bestNode = SearchBestMove(board, op, depth - 1, st_time);

		double score = currentScore - bestNode.second;

		if (maxMove.second < score) {
			maxMove = { currentMove, score };
		}

		board[currentMove.first.X][currentMove.first.Y] = EMPTY;
		board[currentMove.second.X][currentMove.second.Y] = EMPTY;
	}

	return maxMove;
}

// ------------------------------------------------------------------------

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

	if (bind(s, reinterpret_cast<SOCKADDR *>(&servAddr), sizeof(servAddr)) == SOCKET_ERROR){
		cout << "Binding failed. Error code: " << WSAGetLastError() << endl;
		WSACleanup();
		return false; //Couldn't connect
	}

	cout << "Waiting for client..." << endl;

	while (true) {

		listen(s, 5);
		int cSize = sizeof(cliAddr);
		SOCKET s2 = accept(s, reinterpret_cast<SOCKADDR *>(&cliAddr), &cSize);
		//cout << "Connection established. New socket num is " << s2 << endl << endl;

		int n;
		n = recv(s2, buf, sizeof(buf), NULL);
		if (n <= 0) { cout << "Got nothing" << endl; break; }
		buf[n] = 0;

		for (int i = 0; i < 19; i++)
			for (int j = 0; j < 19; j++)
				board[i][j] = buf[i * 19 + j] - '0';

		op_x[0] = (buf[361]-'0')*10 + (buf[362] - '0');
		op_y[0] = (buf[363] - '0') * 10 + (buf[364] - '0');
		op_x[1] = (buf[365] - '0') * 10 + (buf[366] - '0');
		op_y[1] = (buf[367] - '0') * 10 + (buf[368] - '0');

		printf("\n\nOpposite: %d %d / %d %d", op_x[0], op_y[0], op_x[1], op_y[1]);

		cout << "Search Start" << endl;

		long st_time = clock();

		int board_tmp[BOARD_SIZE][BOARD_SIZE];
		CopyBoard(board, board_tmp);

		MOVES_SCORE myMove = SearchBestMove(board_tmp, (PORT==8052?2:1), DEPTH, st_time);
		
		x[0] = myMove.first.first.X;
		x[1] = myMove.first.second.X;
		y[0] = myMove.first.first.Y;
		y[1] = myMove.first.second.Y;

		cout << "Search End" << endl;
		printf("(%d, %d), (%d, %d)\n", x[0], y[0], x[1], y[1]);
			
		//put_stone();
		//print_board();

		char sbuf[8];
		sbuf[0] = y[0] / 10 + '0';
		sbuf[1] = y[0] % 10 + '0';
		sbuf[2] = x[0] / 10 + '0';
		sbuf[3] = x[0] % 10 + '0';
		sbuf[4] = y[1] / 10 + '0';
		sbuf[5] = y[1] % 10 + '0';
		sbuf[6] = x[1] / 10 + '0';
		sbuf[7] = x[1] % 10 + '0';

		send(s2, sbuf, sizeof(sbuf), NULL);
		closesocket(s2);
	}

	closesocket(s);
	WSACleanup();

	return 0;
}