#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<cstring>
#include<vector>
#include<map>
#include<string>
#include<algorithm>
#include<chrono>
#include<sstream>
#include<cstdint>
#include<random>

using namespace std;

/*
红方（大写）     黑方（小写）
帅  K        将  k
士  A        士  a
象  B        象  b
马  N        马  n
车  R        车  r
炮  C        炮  c
兵  P        卒  p
*/
char board[10][9];
bool is_red;
bool engine_is_red = true;
int move_count = 0;
int r_king_r = -1, r_king_c = -1, b_king_r = -1, b_king_c = -1; // 全局将帅位置
int red_major_count = 0, black_major_count = 0; // 全局大子计数

const int EXACT = 0, ALPHA = 1, BETA = 2;

struct Move {
	int fx, fy, tx, ty;
	Move(int ffx = -1, int ffy = -1, int ttx = -1, int tty = -1) :fx(ffx), fy(ffy), tx(ttx), ty(tty) {}
	bool operator==(const Move& other) const {
		return fx == other.fx && fy == other.fy && tx == other.tx && ty == other.ty;
	}
};

struct TTEntry {
	uint64_t key;
	int depth;
	int score;
	int flag;
	Move best_move;
	int age; // 增加 age 字段，用于逻辑清理
};

const int TT_SIZE = 1 << 20;
TTEntry transposition_table[TT_SIZE];
int tt_current_age = 0;

uint64_t zobrist_table[14][10][9];
uint64_t zobrist_side;
uint64_t current_zobrist_key;

int piece_to_idx(char c) {
	switch (c) {
	case 'K': return 0; case 'A': return 1; case 'B': return 2; case 'N': return 3; case 'R': return 4; case 'C': return 5; case 'P': return 6;
	case 'k': return 7; case 'a': return 8; case 'b': return 9; case 'n': return 10; case 'r': return 11; case 'c': return 12; case 'p': return 13;
	default: return -1;
	}
}

// 提前声明
void update_king_positions();

void init_zobrist() {
	mt19937_64 rng(12345);
	for (int p = 0; p < 14; p++) {
		for (int r = 0; r < 10; r++) {
			for (int c = 0; c < 9; c++) {
				zobrist_table[p][r][c] = rng();
			}
		}
	}
	zobrist_side = rng();
}

uint64_t compute_zobrist() {
	uint64_t key = 0;
	for (int r = 0; r < 10; r++) {
		for (int c = 0; c < 9; c++) {
			int p = piece_to_idx(board[r][c]);
			if (p != -1) key ^= zobrist_table[p][r][c];
		}
	}
	if (is_red) key ^= zobrist_side;
	return key;
}

void update_tt(uint64_t key, int depth, int score, int flag, Move best_move) {
	int idx = key % TT_SIZE;
	// 覆盖策略：如果 age 不同（说明是旧局面的）或者是更深的搜索
	if (transposition_table[idx].age != tt_current_age || transposition_table[idx].depth <= depth) {
		transposition_table[idx] = { key, depth, score, flag, best_move, tt_current_age };
	}
}

bool query_tt(uint64_t key, int depth, int& alpha, int& beta, int& score, Move& best_move) {
	int idx = key % TT_SIZE;
	if (transposition_table[idx].key == key && transposition_table[idx].age == tt_current_age) {
		best_move = transposition_table[idx].best_move;
		if (transposition_table[idx].depth >= depth) {
			int tt_score = transposition_table[idx].score;
			if (transposition_table[idx].flag == EXACT) {
				score = tt_score;
				return true;
			}
			if (transposition_table[idx].flag == ALPHA && tt_score <= alpha) {
				score = alpha;
				return true;
			}
			if (transposition_table[idx].flag == BETA && tt_score >= beta) {
				score = beta;
				return true;
			}
		}
	}
	return false;
}

struct PositionRecord {
	string key;
	bool moved_red;
	bool gave_check;
	PositionRecord(const string& k, bool mr, bool gc) : key(k), moved_red(mr), gave_check(gc) {}
};

vector<Move> generate();
void output_board(int fx, int fy, int tx, int ty);
int find_king(bool red_side, int& kr, int& kc);
bool attacked_by(bool attacker_red, int tr, int tc);

std::chrono::steady_clock::time_point end_time;
const int INF = 100000000;
const int MATE_SCORE = 1000000;

bool time_up() {
	return std::chrono::steady_clock::now() >= end_time;
}

//判断红黑
void side() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 9; j++) {
			if (board[i][j] == 'K') {
				is_red = true;//红方
				return;
			}
			else if (board[i][j] == 'k') {
				is_red = false;//黑方
				return;
			}
		}
	}
}

bool is_red_piece(char c) { return c >= 'A' && c <= 'Z'; }
bool is_black_piece(char c) { return c >= 'a' && c <= 'z'; }

int coord_to_row(const string& s) {
	if (s.size() < 2 || s == "-1") return -1;
	try {
		int y = stoi(s.substr(1));
		return 9 - y;
	} catch (...) { return -1; }
}

int coord_to_col(const string& s) {
	if (s.empty() || s == "-1") return -1;
	return s[0] - 'a';
}

string rowcol_to_coord(int r, int c) {
	string s;
	s.push_back(char('a' + c));
	s += to_string(9 - r);
	return s;
}

void init_board() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 9; j++) board[i][j] = '.';
	}
	string red0 = "RNBAKABNR";
	string blk0 = "rnbakabnr";
	for (int j = 0; j < 9; j++) {
		board[9][j] = red0[j];
		board[0][j] = blk0[j];
	}
	board[7][1] = 'C'; board[7][7] = 'C';
	board[2][1] = 'c'; board[2][7] = 'c';
	for (int j = 0; j < 9; j += 2) {
		board[6][j] = 'P';
		board[3][j] = 'p';
	}
}

void apply_move_rc(int fr, int fc, int tr, int tc) {
	if (fr < 0 || fr >= 10 || fc < 0 || fc >= 9 || tr < 0 || tr >= 10 || tc < 0 || tc >= 9) return;
	board[tr][tc] = board[fr][fc];
	board[fr][fc] = '.';
	move_count++;
	update_king_positions(); // 确保历史对局重演时，全局将帅位置是最新的
}

void parse_moves(const string& str, const string& array_name, vector<pair<string, string>>& out_moves) {
	size_t pos = str.find("\"" + array_name + "\"");
	if (pos == string::npos) return;
	size_t start = str.find('[', pos);
	size_t end = str.find(']', start);
	if (start == string::npos || end == string::npos) return;

	string arr = str.substr(start, end - start);
	size_t p = 0;
	while (true) {
		p = arr.find("\"source\"", p);
		if (p == string::npos) break;
		size_t q1 = arr.find('"', p + 8);
		if (q1 == string::npos) break;
		size_t q2 = arr.find('"', q1 + 1);
		if (q2 == string::npos) break;
		string src = arr.substr(q1 + 1, q2 - q1 - 1);

		p = arr.find("\"target\"", q2);
		if (p == string::npos) break;
		size_t q3 = arr.find('"', p + 8);
		if (q3 == string::npos) break;
		size_t q4 = arr.find('"', q3 + 1);
		if (q4 == string::npos) break;
		string tgt = arr.substr(q3 + 1, q4 - q3 - 1);

		out_moves.push_back({ src, tgt });
		p = q4;
	}
}

string game_history = "";
vector<PositionRecord> actual_position_records;
vector<PositionRecord> search_position_records;

string serialize_position(bool side_to_move_red) {
	string s;
	s.reserve(92);
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 9; j++) s.push_back(board[i][j]);
	}
	s.push_back(side_to_move_red ? 'R' : 'B');
	return s;
}

bool side_gives_check(bool attacker_red) {
	int kr = -1, kc = -1;
	if (!find_king(!attacker_red, kr, kc)) return true;
	return attacked_by(attacker_red, kr, kc);
}

void record_historical_position(bool moved_red) {
	bool next_side_red = (move_count % 2 == 0);
	actual_position_records.push_back(PositionRecord(serialize_position(next_side_red), moved_red, side_gives_check(moved_red)));
}

bool last_move_by_side_gave_check(bool moved_red) {
	for (int i = (int)search_position_records.size() - 1; i >= 0; i--) {
		if (search_position_records[i].moved_red == moved_red) return search_position_records[i].gave_check;
	}
	for (int i = (int)actual_position_records.size() - 1; i >= 0; i--) {
		if (actual_position_records[i].moved_red == moved_red) return actual_position_records[i].gave_check;
	}
	return false;
}

int count_matching_check_positions(const string& key, bool moved_red) {
	int cnt = 0;
	for (size_t i = 0; i < actual_position_records.size(); i++) {
		if (actual_position_records[i].moved_red == moved_red && actual_position_records[i].gave_check && actual_position_records[i].key == key) cnt++;
	}
	for (size_t i = 0; i < search_position_records.size(); i++) {
		if (search_position_records[i].moved_red == moved_red && search_position_records[i].gave_check && search_position_records[i].key == key) cnt++;
	}
	return cnt;
}

bool move_causes_long_check(const Move& m) {
	bool mover_red = is_red;
	char old_f = board[m.fx][m.fy];
	char old_t = board[m.tx][m.ty];
	board[m.tx][m.ty] = old_f;
	board[m.fx][m.fy] = '.';

	bool gives_check = side_gives_check(mover_red);
	bool risky = false;
	if (gives_check) {
		string key = serialize_position(!mover_red);
		int repeat_check_count = count_matching_check_positions(key, mover_red);
		bool just_checked = last_move_by_side_gave_check(mover_red);
		risky = (repeat_check_count >= 2) || (repeat_check_count >= 1 && just_checked);
	}

	board[m.fx][m.fy] = old_f;
	board[m.tx][m.ty] = old_t;
	return risky;
}

void avoid_long_check_if_possible(vector<Move>& moves) {
	vector<Move> safe_moves;
	safe_moves.reserve(moves.size());
	for (size_t i = 0; i < moves.size(); i++) {
		if (!move_causes_long_check(moves[i])) safe_moves.push_back(moves[i]);
	}
	if (!safe_moves.empty()) moves.swap(safe_moves);
}

void getInputBotzone() {
	string str;
	getline(cin, str);

	init_board();
	move_count = 0;
	is_red = true;
	engine_is_red = true;
	game_history = "";
	actual_position_records.clear();
	search_position_records.clear();

	if (str.empty()) return;

	vector<pair<string, string>> reqs, resps;
	parse_moves(str, "requests", reqs);
	parse_moves(str, "responses", resps);
	if (!reqs.empty()) engine_is_red = (reqs[0].first == "-1");

	if (!reqs.empty() || !resps.empty()) {
		int turnID = resps.size();
		for (int i = 0; i < turnID; i++) {
			if (i < (int)reqs.size() && reqs[i].first != "-1" && reqs[i].second != "-1") {
				int fr = coord_to_row(reqs[i].first), fc = coord_to_col(reqs[i].first);
				int tr = coord_to_row(reqs[i].second), tc = coord_to_col(reqs[i].second);
				bool mover_red = (move_count % 2 == 0);
				apply_move_rc(fr, fc, tr, tc);
				record_historical_position(mover_red);
				game_history += reqs[i].first + reqs[i].second;
			}
			if (i < (int)resps.size() && resps[i].first != "-1" && resps[i].second != "-1") {
				int fr = coord_to_row(resps[i].first), fc = coord_to_col(resps[i].first);
				int tr = coord_to_row(resps[i].second), tc = coord_to_col(resps[i].second);
				bool mover_red = (move_count % 2 == 0);
				apply_move_rc(fr, fc, tr, tc);
				record_historical_position(mover_red);
				game_history += resps[i].first + resps[i].second;
			}
		}

		if (turnID < (int)reqs.size()) {
			if (reqs[turnID].first != "-1" && reqs[turnID].second != "-1") {
				int fr = coord_to_row(reqs[turnID].first), fc = coord_to_col(reqs[turnID].first);
				int tr = coord_to_row(reqs[turnID].second), tc = coord_to_col(reqs[turnID].second);
				bool mover_red = (move_count % 2 == 0);
				apply_move_rc(fr, fc, tr, tc);
				record_historical_position(mover_red);
				game_history += reqs[turnID].first + reqs[turnID].second;
			}
		}

		is_red = (move_count % 2 == 0);
		return;
	}

	// 兼容可能存在的单步 {"source":"..", "target":".."} 格式
	size_t s_pos = str.find("\"source\"");
	size_t t_pos = str.find("\"target\"");
	if (s_pos != string::npos && t_pos != string::npos) {
		size_t q1 = str.find('"', s_pos + 8);
		if (q1 != string::npos) {
			size_t q2 = str.find('"', q1 + 1);
			if (q2 != string::npos) {
				string s = str.substr(q1 + 1, q2 - q1 - 1);

				size_t q3 = str.find('"', t_pos + 8);
				if (q3 != string::npos) {
					size_t q4 = str.find('"', q3 + 1);
					if (q4 != string::npos) {
						string t = str.substr(q3 + 1, q4 - q3 - 1);

						if (s == "-1") {
							engine_is_red = true;
							is_red = true;
						}
						else {
							engine_is_red = false;
							int fr = coord_to_row(s), fc = coord_to_col(s);
							int tr = coord_to_row(t), tc = coord_to_col(t);
							bool mover_red = (move_count % 2 == 0);
							apply_move_rc(fr, fc, tr, tc);
							record_historical_position(mover_red);
							game_history += s + t;
							is_red = false;
						}
					}
				}
			}
		}
	}
}

int get_score(char ch) {
	switch (ch) {
	case 'K': case 'k': return 100000;
	case 'R': case 'r': return 600;
	case 'N': case 'n': return 270;
	case 'C': case 'c': return 300;
	case 'B': case 'b': return 120;
	case 'A': case 'a': return 120;
	case 'P': case 'p': return 70;
	default: return 0;
	}
}

bool same_side_piece(char a, char b) {
	if (a == '.' || b == '.') return false;
	return (is_red_piece(a) && is_red_piece(b)) || (is_black_piece(a) && is_black_piece(b));
}

int guard_bonus(char ch) {
	if (ch == 'A' || ch == 'a') return 18;
	if (ch == 'B' || ch == 'b') return 16;
	return 0;
}

int piece_safety_penalty(int value, bool defended) {
	if (value >= 600) return defended ? 55 : 140;
	if (value >= 300) return defended ? 28 : 95;
	if (value >= 120) return defended ? 18 : 55;
	return defended ? 8 : 24;
}

int piece_safety_bonus(int value) {
	if (value >= 600) return 18;
	if (value >= 300) return 12;
	if (value >= 120) return 8;
	return 4;
}

// 位置附加分表 (PST - Piece Square Tables)，以红方视角为准（黑方只需翻转行坐标）
// 数值表示该棋子在该位置的额外奖励分，提升 AI 的走位意识
const int PST_PAWN[10][9] = {
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 0: 底线
	{15, 25, 35, 45, 45, 35, 25, 15, 15}, // 1: 兵临城下奖励加大
	{15, 25, 30, 40, 40, 30, 25, 15, 15}, // 2
	{10, 20, 25, 35, 35, 25, 20, 10, 10}, // 3
	{ 8, 12, 18, 25, 25, 18, 12,  8,  8}, // 4: 刚过河
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
};

const int PST_KNIGHT[10][9] = {
	{ 0,  5, 10, 15, 15, 15, 10,  5,  0}, // 对面底线
	{ 5, 15, 25, 25, 25, 25, 25, 15,  5},
	{ 5, 20, 30, 35, 35, 35, 30, 20,  5},
	{ 5, 20, 30, 40, 40, 40, 30, 20,  5},
	{ 5, 15, 25, 35, 35, 35, 25, 15,  5},
	{ 5, 10, 20, 20, 20, 20, 20, 10,  5},
	{ 0,  5, 10, 10, 10, 10, 10,  5,  0},
	{ 0,  0,  5,  5,  5,  5,  5,  0,  0},
	{-5,  0,  0,  0,  0,  0,  0,  0, -5},
	{-5, -5, -5, -5, -5, -5, -5, -5, -5},
};

const int PST_ROOK[10][9] = {
	{15, 15, 15, 25, 25, 25, 15, 15, 15},
	{20, 25, 25, 35, 35, 35, 25, 25, 20},
	{15, 25, 25, 30, 30, 30, 25, 25, 15},
	{15, 25, 25, 30, 30, 30, 25, 25, 15},
	{20, 30, 30, 40, 40, 40, 30, 30, 20},
	{20, 30, 30, 40, 40, 40, 30, 30, 20},
	{ 5, 15,  5, 20, 20, 20,  5, 15,  5},
	{ 0, 10,  0, 20, 20, 20,  0, 10,  0},
	{-5,  5,  0, 20,  0, 20,  0,  5, -5},
	{-5,  0,  0, 20,  0, 20,  0,  0, -5},
};

const int PST_CANNON[10][9] = {
	{10, 15, 10,  5,  5,  5, 10, 15, 10},
	{ 5, 10, 10, 10, 10, 10, 10, 10,  5},
	{ 5, 10, 15, 15, 15, 15, 15, 10,  5},
	{ 5, 15, 20, 20, 25, 20, 20, 15,  5},
	{ 5, 10, 15, 20, 25, 20, 15, 10,  5},
	{ 0,  5, 10, 10, 20, 10, 10,  5,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  5,  5, 10,  5,  5,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  5,  5,  5,  0,  0,  0},
};

// 评估棋盘总分,红正黑负
int evaluate() {
	int cnt = 0;
	int red_king_r = r_king_r, red_king_c = r_king_c;
	int black_king_r = b_king_r, black_king_c = b_king_c;

	// 统计棋子数量（用于残局判断）
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 9; y++) {
			if (board[x][y] != '.') cnt++;
		}
	}

	int score = 0;
	bool is_endgame = cnt <= 12; // 残局判断

	// 统计守卫数量
	int red_guards = 0, black_guards = 0;
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 9; y++) {
			char c = board[x][y];
			if (c == 'A' || c == 'B') red_guards++;
			if (c == 'a' || c == 'b') black_guards++;
		}
	}

	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 9; j++) {
			char c = board[i][j];
			if (c == '.') continue;

			bool red_piece = is_red_piece(c);
			int val = get_score(c);
			
			// 炮残局减分（由于残局炮威力下降）
			if ((c == 'C' || c == 'c') && is_endgame) val -= 10;

			int pst_val = 0;
			int tactics_val = 0;
			
			// 计算攻击和防御状态（避免重复计算）
			bool attacked = attacked_by(!red_piece, i, j);
			bool defended = attacked_by(red_piece, i, j);

			// 马腿阻塞惩罚
			if (c == 'N' || c == 'n') {
				int blocks = 0;
				int mobility = 0;
				int dr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
				int dc[8] = {-1, 1, -2, 2, -2, 2, -1, 1};
				int br[8] = {-1, -1, 0, 0, 0, 0, 1, 1};
				int bc[8] = {0, 0, -1, 1, -1, 1, 0, 0};
				for(int d=0; d<8; d++) {
					int nr = i + dr[d], nc = j + dc[d];
					if(nr >= 0 && nr < 10 && nc >= 0 && nc < 9) {
						int br_idx = i + br[d], bc_idx = j + bc[d];
						if(br_idx >= 0 && br_idx < 10 && bc_idx >= 0 && bc_idx < 9 && board[br_idx][bc_idx] == '.') {
							mobility++;
						} else {
							blocks++;
						}
					}
				}
				tactics_val -= blocks * 8;
				tactics_val += mobility * 4;
			}
			
			// 沉底车奖励（控制底线）
			if (c == 'R' && i <= 2 && j >= 3 && j <= 5) tactics_val += 25;
			if (c == 'r' && i >= 7 && j >= 3 && j <= 5) tactics_val += 25;

			// 车路通畅度
			if (c == 'R' || c == 'r') {
				int mobility = 0;
				for(int d=0; d<4; d++) {
					int dr4[4] = {-1, 1, 0, 0}, dc4[4] = {0, 0, -1, 1};
					int nr = i + dr4[d], nc = j + dc4[d];
					while(nr >= 0 && nr < 10 && nc >= 0 && nc < 9) {
						if (board[nr][nc] == '.') {
							mobility++;
							nr += dr4[d]; nc += dc4[d];
						} else {
							// 发现棋子，若是对方棋子，且受保护较弱，则有一定的压制分
							if (is_red_piece(board[nr][nc]) != red_piece) mobility++;
							break;
						}
					}
				}
				tactics_val += mobility * 3;
			}

			// 进攻九宫格奖励 (增强攻击性)
			if (red_piece) {
				// 红方棋子靠近对方九宫格
				if (i <= 4) {
					if (c == 'R' || c == 'C' || c == 'N') {
						// 大子进入对方半场奖励
						tactics_val += (4 - i) * 6;
						// 靠近对方将位奖励
						if (black_king_r != -1 && black_king_c != -1) {
							int dist = abs(i - black_king_r) + abs(j - black_king_c);
							if (dist <= 3) tactics_val += 25;
						}
					}
					if (c == 'P' && i <= 2) tactics_val += 20; // 兵临城下
				}
				// 兵过河后的灵活性
				if (c == 'P' && i <= 4) tactics_val += 10;
			} else {
				// 黑方棋子靠近对方九宫格
				if (i >= 5) {
					if (c == 'r' || c == 'c' || c == 'n') {
						tactics_val += (i - 5) * 6;
						if (red_king_r != -1 && red_king_c != -1) {
							int dist = abs(i - red_king_r) + abs(j - red_king_c);
							if (dist <= 3) tactics_val += 25;
						}
					}
					if (c == 'p' && i >= 7) tactics_val += 20;
				}
				if (c == 'p' && i >= 5) tactics_val += 10;
			}

			// 空头炮/沉底炮威胁评估与炮架质量
			if (c == 'C' || c == 'c') {
				int target_kr = red_piece ? black_king_r : red_king_r;
				int target_kc = red_piece ? black_king_c : red_king_c;
				if (target_kc != -1 && j == target_kc) {
					int pieces_between = 0;
					int step = (i < target_kr) ? 1 : -1;
					for (int r = i + step; r != target_kr; r += step) {
						if (board[r][j] != '.') pieces_between++;
					}
					if (pieces_between == 0) tactics_val += 50; // 空头炮
					else if (pieces_between == 1) tactics_val += 25; // 有炮架
				}
				// 炮架灵活性与炮的威慑
				for(int d=0; d<4; d++) {
					int dr4[4] = {-1, 1, 0, 0}, dc4[4] = {0, 0, -1, 1};
					int nr = i + dr4[d], nc = j + dc4[d];
					int mount_count = 0;
					while(nr >= 0 && nr < 10 && nc >= 0 && nc < 9) {
						if(board[nr][nc] != '.') {
							mount_count++;
							if(mount_count == 1) tactics_val += 4; // 发现一个炮架
							if(mount_count == 2) {
								if(is_red_piece(board[nr][nc]) != red_piece) {
									tactics_val += 15; // 威胁对方棋子
								}
								break;
							}
						}
						nr += dr4[d]; nc += dc4[d];
					}
				}
			}

			// 位置附加分（PST）
			if (red_piece) {
				switch (c) {
					case 'P': pst_val = PST_PAWN[i][j]; break;
					case 'N': pst_val = PST_KNIGHT[i][j]; break;
					case 'R': pst_val = PST_ROOK[i][j]; break;
					case 'C': pst_val = PST_CANNON[i][j]; break;
				}
				score += (val + pst_val + guard_bonus(c) + tactics_val);
				
				// 安全性评估
				if (attacked) score -= piece_safety_penalty(val, defended);
				else if (defended) score += piece_safety_bonus(val);
			} else {
				// 黑方镜像处理
				switch (c) {
					case 'p': pst_val = PST_PAWN[9 - i][j]; break;
					case 'n': pst_val = PST_KNIGHT[9 - i][j]; break;
					case 'r': pst_val = PST_ROOK[9 - i][j]; break;
					case 'c': pst_val = PST_CANNON[9 - i][j]; break;
				}
				score -= (val + pst_val + guard_bonus(c) + tactics_val);
				
				// 安全性评估
				if (attacked) score += piece_safety_penalty(val, defended);
				else if (defended) score -= piece_safety_bonus(val);
			}
		}
	}

	// 将帅安全评估
	if (red_king_r != -1) {
		if (attacked_by(false, red_king_r, red_king_c)) score -= 250; // 加大将军惩罚
		// 缺少守卫惩罚
		if (red_guards < 4) score -= (4 - red_guards) * 30;
		// 将帅周围格子受攻击惩罚
		for (int dc = -1; dc <= 1; dc++) {
			int c = red_king_c + dc;
			if (c >= 0 && c < 9 && attacked_by(false, red_king_r, c)) score -= 15;
		}
	}
	
	if (black_king_r != -1) {
		if (attacked_by(true, black_king_r, black_king_c)) score += 250;
		if (black_guards < 4) score += (4 - black_guards) * 30;
		// 将帅周围格子受攻击奖励（对黑方不利）
		for (int dc = -1; dc <= 1; dc++) {
			int c = black_king_c + dc;
			if (c >= 0 && c < 9 && attacked_by(true, black_king_r, c)) score += 15;
		}
	}

	return is_red ? score : -score;
}

int find_king(bool red_side, int& kr, int& kc) {
	char k = red_side ? 'K' : 'k';
	for (int r = 0; r < 10; r++) {
		for (int c = 0; c < 9; c++) {
			if (board[r][c] == k) {
				kr = r;
				kc = c;
				return 1;
			}
		}
	}
	return 0;
}

bool in_palace(bool red_side, int r, int c) {
	if (c < 3 || c > 5) return false;
	if (red_side) return r >= 7 && r <= 9;
	return r >= 0 && r <= 2;
}

bool attacked_by(bool attacker_red, int tr, int tc) {
	if (tr < 0 || tr >= 10 || tc < 0 || tc >= 9) return false;

	char rook = attacker_red ? 'R' : 'r';
	char cannon = attacker_red ? 'C' : 'c';
	char knight = attacker_red ? 'N' : 'n';
	char bishop = attacker_red ? 'B' : 'b';
	char assistant = attacker_red ? 'A' : 'a';
	char king = attacker_red ? 'K' : 'k';
	char pawn = attacker_red ? 'P' : 'p';

	int dr4[4] = { -1,1,0,0 };
	int dc4[4] = { 0,0,-1,1 };

	for (int d = 0; d < 4; d++) {
		int r = tr + dr4[d], c = tc + dc4[d];
		while (r >= 0 && r < 10 && c >= 0 && c < 9) {
			char p = board[r][c];
			if (p != '.') {
				if (p == rook) return true;
				if (p == king && c == tc && r != tr) {
					bool clear = true;
					int step = (r > tr) ? 1 : -1;
					for (int rr = tr + step; rr != r; rr += step) {
						if (board[rr][tc] != '.') { clear = false; break; }
					}
					if (clear) return true;
				}
				break;
			}
			r += dr4[d];
			c += dc4[d];
		}
	}

	for (int d = 0; d < 4; d++) {
		int r = tr + dr4[d], c = tc + dc4[d];
		while (r >= 0 && r < 10 && c >= 0 && c < 9 && board[r][c] == '.') {
			r += dr4[d];
			c += dc4[d];
		}
		r += dr4[d];
		c += dc4[d];
		while (r >= 0 && r < 10 && c >= 0 && c < 9) {
			char p = board[r][c];
			if (p != '.') {
				if (p == cannon) return true;
				break;
			}
			r += dr4[d];
			c += dc4[d];
		}
	}

	int kdr[8] = { -2,-2,-1,-1,1,1,2,2 };
	int kdc[8] = { -1,1,-2,2,-2,2,-1,1 };
	int ldr[8] = { -1,-1,0,0,0,0,1,1 };
	int ldc[8] = { 0,0,-1,1,-1,1,0,0 };
	for (int i = 0; i < 8; i++) {
		int sr = tr + kdr[i], sc = tc + kdc[i];
		int lr = tr + ldr[i], lc = tc + ldc[i];
		if (sr < 0 || sr >= 10 || sc < 0 || sc >= 9) continue;
		if (lr < 0 || lr >= 10 || lc < 0 || lc >= 9) continue;
		if (board[lr][lc] != '.') continue;
		if (board[sr][sc] == knight) return true;
	}

	int bdr[4] = { -2,-2,2,2 };
	int bdc[4] = { -2,2,-2,2 };
	int edr[4] = { -1,-1,1,1 };
	int edc[4] = { -1,1,-1,1 };
	for (int i = 0; i < 4; i++) {
		int sr = tr + bdr[i], sc = tc + bdc[i];
		int er = tr + edr[i], ec = tc + edc[i];
		if (sr < 0 || sr >= 10 || sc < 0 || sc >= 9) continue;
		if (er < 0 || er >= 10 || ec < 0 || ec >= 9) continue; // 修复：增加象眼越界检查
		if (board[er][ec] != '.') continue;
		if (attacker_red && sr < 5) continue;
		if (!attacker_red && sr > 4) continue;
		if (board[sr][sc] == bishop) return true;
	}

	int adr[4] = { -1,-1,1,1 };
	int adc[4] = { -1,1,-1,1 };
	for (int i = 0; i < 4; i++) {
		int sr = tr + adr[i], sc = tc + adc[i];
		if (sr < 0 || sr >= 10 || sc < 0 || sc >= 9) continue;
		if (!in_palace(attacker_red, sr, sc)) continue;
		if (board[sr][sc] == assistant) return true;
	}

	int kx[4] = { -1,1,0,0 };
	int ky[4] = { 0,0,-1,1 };
	for (int i = 0; i < 4; i++) {
		int sr = tr + kx[i], sc = tc + ky[i];
		if (sr < 0 || sr >= 10 || sc < 0 || sc >= 9) continue;
		if (!in_palace(attacker_red, sr, sc)) continue;
		if (board[sr][sc] == king) return true;
	}

	if (attacker_red) {
		int sr = tr + 1, sc = tc;
		if (sr >= 0 && sr < 10 && board[sr][sc] == pawn) return true;
		if (tr <= 4) {
			if (tc - 1 >= 0 && board[tr][tc - 1] == pawn) return true;
			if (tc + 1 < 9 && board[tr][tc + 1] == pawn) return true;
		}
	}
	else {
		int sr = tr - 1, sc = tc;
		if (sr >= 0 && sr < 10 && board[sr][sc] == pawn) return true;
		if (tr >= 5) {
			if (tc - 1 >= 0 && board[tr][tc - 1] == pawn) return true;
			if (tc + 1 < 9 && board[tr][tc + 1] == pawn) return true;
		}
	}

	return false;
}

bool is_in_check(bool red_side) {
	int kr = -1, kc = -1;
	if (!find_king(red_side, kr, kc)) return true;
	return attacked_by(!red_side, kr, kc);
}

bool is_capture_move(const Move& m) {
	return board[m.tx][m.ty] != '.';
}

bool move_gives_check(const Move& m) {
	char old_f = board[m.fx][m.fy];
	char old_t = board[m.tx][m.ty];
	board[m.tx][m.ty] = old_f;
	board[m.fx][m.fy] = '.';

	bool opponent_red = !is_red;
	int kr = -1, kc = -1;
	bool gives_check = false;
	if (find_king(opponent_red, kr, kc)) {
		gives_check = attacked_by(!opponent_red, kr, kc);
	}
	else {
		gives_check = true;
	}

	board[m.fx][m.fy] = old_f;
	board[m.tx][m.ty] = old_t;
	return gives_check;
}

int defensive_bias(bool mover_red, bool root_context) {
	int bias = 1;
	// 适当降低防守偏好，鼓励反击
	if (!engine_is_red && !mover_red && move_count <= 12) bias += 1;
	return bias;
}

int move_safety_score(const Move& m, bool root_context = false) {
	bool mover_red = is_red;
	char mover = board[m.fx][m.fy];
	char target = board[m.tx][m.ty];
	bool from_attacked = attacked_by(!mover_red, m.fx, m.fy);
	bool from_defended = attacked_by(mover_red, m.fx, m.fy);
	bool from_hanging = from_attacked && !from_defended;

	char old_f = board[m.fx][m.fy];
	char old_t = board[m.tx][m.ty];
	board[m.tx][m.ty] = old_f;
	board[m.fx][m.fy] = '.';

	bool to_attacked = attacked_by(!mover_red, m.tx, m.ty);
	bool to_defended = attacked_by(mover_red, m.tx, m.ty);
	bool to_hanging = to_attacked && !to_defended;
	int bias = defensive_bias(mover_red, root_context);

	int score = 0;
	if (from_hanging && !to_hanging) score += bias * (220 + get_score(mover) / 2);
	if (!from_hanging && to_hanging) score -= bias * (90 + get_score(mover) / 3);

	if (target != '.') {
		if (!to_attacked) score += 180;
		else if (to_defended) score += 70;
		else score -= bias * (get_score(mover) / 2);
		if (to_hanging && get_score(target) < get_score(mover)) {
			score -= bias * (get_score(mover) - get_score(target));
		}
		if (!to_hanging && get_score(target) >= get_score(mover)) {
			score += 60;
		}
	}
	else {
		if (from_hanging && (!to_attacked || to_defended)) score += bias * (get_score(mover) / 3);
		if (to_hanging) score -= bias * (get_score(mover) / 3);
	}

	board[m.fx][m.fy] = old_f;
	board[m.tx][m.ty] = old_t;
	return score;
}

const int MAX_PLY = 48;
int history_table[2][10][9][10][9];
Move killer_moves[MAX_PLY][2] = { {Move(-1, -1, -1, -1), Move(-1, -1, -1, -1)} };

void clear_heuristics() {
	memset(history_table, 0, sizeof(history_table));
	for (int i = 0; i < MAX_PLY; i++) {
		killer_moves[i][0] = Move(-1, -1, -1, -1);
		killer_moves[i][1] = Move(-1, -1, -1, -1);
	}
}

int move_order_score(const Move& m, const Move* pv_move = nullptr, bool root_context = false, int ply = 0) {
	if (pv_move && m.fx == pv_move->fx && m.fy == pv_move->fy && m.tx == pv_move->tx && m.ty == pv_move->ty) {
		return INF / 2;
	}

	char mover = board[m.fx][m.fy];
	char target = board[m.tx][m.ty];
	int score = 0;

	if (target != '.') {
		score += 100000 + get_score(target) * 16 - get_score(mover);
	}
	else {
		// 杀手启发
		if (ply < MAX_PLY) {
			if (m.fx == killer_moves[ply][0].fx && m.fy == killer_moves[ply][0].fy && m.tx == killer_moves[ply][0].tx && m.ty == killer_moves[ply][0].ty) {
				score += 50000;
			}
			else if (m.fx == killer_moves[ply][1].fx && m.fy == killer_moves[ply][1].fy && m.tx == killer_moves[ply][1].tx && m.ty == killer_moves[ply][1].ty) {
				score += 40000;
			}
		}
		// 历史表启发
		score += history_table[is_red ? 1 : 0][m.fx][m.fy][m.tx][m.ty];
	}

	if (move_gives_check(m)) {
		score += 15000; // 提高将军走法的优先级
	}
	
	// 进攻倾向排序：奖励靠近对方将帅的走法
	int opponent_kr = is_red ? b_king_r : r_king_r;
	int opponent_kc = is_red ? b_king_c : r_king_c;
	if (opponent_kr != -1) {
		int old_dist = abs(m.fx - opponent_kr) + abs(m.fy - opponent_kc);
		int new_dist = abs(m.tx - opponent_kr) + abs(m.ty - opponent_kc);
		if (new_dist < old_dist) score += 100; // 靠近对方将帅奖励
	}

	score += move_safety_score(m, root_context);

	int center_dist = abs(m.tx - 4) + abs(m.ty - 4);
	score += 20 - center_dist;
	return score;
}

void sort_moves_by_priority(vector<Move>& moves, const Move* pv_move = nullptr, bool root_context = false, int ply = 0) {
	vector<pair<int, Move>> scored;
	scored.reserve(moves.size());
	for (size_t i = 0; i < moves.size(); i++) {
		scored.push_back({ move_order_score(moves[i], pv_move, root_context, ply), moves[i] });
	}
	sort(scored.begin(), scored.end(), [](const pair<int, Move>& a, const pair<int, Move>& b) {
		return a.first > b.first;
		});
	for (size_t i = 0; i < moves.size(); i++) {
		moves[i] = scored[i].second;
	}
}

bool is_legal_move(const Move& m) {
	update_king_positions();   // 添加：强制同步将帅位置
	if (m.fx < 0 || m.fx >= 10 || m.fy < 0 || m.fy >= 9 ||
		m.tx < 0 || m.tx >= 10 || m.ty < 0 || m.ty >= 9) return false;
	char p = board[m.fx][m.fy];
	if (p == '.') return false;
	if (is_red && !is_red_piece(p)) return false;
	if (!is_red && !is_black_piece(p)) return false;

	char t = board[m.tx][m.ty];
	if (t != '.') {
		if (is_red && is_red_piece(t)) return false;
		if (!is_red && is_black_piece(t)) return false;
	}

	char old_f = board[m.fx][m.fy];
	char old_t = board[m.tx][m.ty];
	board[m.tx][m.ty] = old_f;
	board[m.fx][m.fy] = '.';

	bool illegal = false;
	if (is_red) {
		int kr = r_king_r, kc = r_king_c;
		if (p == 'K') { kr = m.tx; kc = m.ty; }
		if (kr == -1) illegal = true; // 防御老将丢失
		else illegal = attacked_by(false, kr, kc);
	} else {
		int kr = b_king_r, kc = b_king_c;
		if (p == 'k') { kr = m.tx; kc = m.ty; }
		if (kr == -1) illegal = true;
		else illegal = attacked_by(true, kr, kc);
	}

	board[m.fx][m.fy] = old_f;
	board[m.tx][m.ty] = old_t;

	return !illegal;
}

void update_major_counts() {
	red_major_count = 0; black_major_count = 0;
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 9; j++) {
			char c = board[i][j];
			if (c == 'R' || c == 'N' || c == 'C') red_major_count++;
			if (c == 'r' || c == 'n' || c == 'c') black_major_count++;
		}
	}
}

void update_king_positions() {
	r_king_r = -1; r_king_c = -1; b_king_r = -1; b_king_c = -1;
	for (int r = 0; r < 10; r++) {
		for (int c = 0; c < 9; c++) {
			if (board[r][c] == 'K') { r_king_r = r; r_king_c = c; }
			if (board[r][c] == 'k') { b_king_r = r; b_king_c = c; }
		}
	}
}

void make_move(const Move& m, uint64_t& key) {
	char mover = board[m.fx][m.fy];
	char target = board[m.tx][m.ty];
	int p_idx = piece_to_idx(mover);
	if (p_idx != -1) key ^= zobrist_table[p_idx][m.fx][m.fy];
	if (target != '.') {
		int t_idx = piece_to_idx(target);
		if (t_idx != -1) key ^= zobrist_table[t_idx][m.tx][m.ty];
		if (target == 'R' || target == 'N' || target == 'C') red_major_count--;
		if (target == 'r' || target == 'n' || target == 'c') black_major_count--;
	}
	if (p_idx != -1) key ^= zobrist_table[p_idx][m.tx][m.ty];
	board[m.tx][m.ty] = mover;
	board[m.fx][m.fy] = '.';
	
	// 更新将帅位置
	if (mover == 'K') { r_king_r = m.tx; r_king_c = m.ty; }
	if (mover == 'k') { b_king_r = m.tx; b_king_c = m.ty; }
	
	is_red = !is_red;
	key ^= zobrist_side;
}

void unmake_move(const Move& m, char old_t, uint64_t& key) {
	char mover = board[m.tx][m.ty];
	
	// 还原将帅位置
	if (mover == 'K') { r_king_r = m.fx; r_king_c = m.fy; }
	if (mover == 'k') { b_king_r = m.fx; b_king_c = m.fy; }

	key ^= zobrist_side;
	is_red = !is_red;
	board[m.fx][m.fy] = mover;
	board[m.tx][m.ty] = old_t;
	
	// 严格镜像撤销哈希，增加索引安全检查
	int p_idx = piece_to_idx(mover);
	if (p_idx != -1) key ^= zobrist_table[p_idx][m.tx][m.ty];
	if (old_t != '.') {
		int t_idx = piece_to_idx(old_t);
		if (t_idx != -1) key ^= zobrist_table[t_idx][m.tx][m.ty];
		if (old_t == 'R' || old_t == 'N' || old_t == 'C') red_major_count++;
		if (old_t == 'r' || old_t == 'n' || old_t == 'c') black_major_count++;
	}
	if (p_idx != -1) key ^= zobrist_table[p_idx][m.fx][m.fy];
}

int quiescence(int alpha, int beta, int depth_left, uint64_t& key, int ply) {
	bool in_check = is_in_check(is_red);
	if (!in_check) {
		int stand_pat = evaluate();
		if (stand_pat >= beta) return beta;
		if (alpha < stand_pat) alpha = stand_pat;
	}
	if (time_up() || depth_left == 0 || ply >= MAX_PLY - 4) return in_check ? evaluate() : alpha;

	vector<Move> moves = generate();
	vector<Move> candidates;
	candidates.reserve(moves.size());
	for (size_t i = 0; i < moves.size(); i++) {
		if (!in_check && !is_capture_move(moves[i]) && !move_gives_check(moves[i])) continue;
		candidates.push_back(moves[i]);
	}
	avoid_long_check_if_possible(candidates);
	sort_moves_by_priority(candidates, nullptr, false, ply);

	if (in_check && candidates.empty()) return -MATE_SCORE + ply;

	for (size_t i = 0; i < candidates.size(); i++) {
		if (time_up()) break;
		Move m = candidates[i];
		char old_t = board[m.tx][m.ty];
		bool mover_red = is_red;
		
		make_move(m, key);
		bool gives_check = side_gives_check(mover_red);
		search_position_records.push_back(PositionRecord(serialize_position(is_red), mover_red, gives_check));

		int val = -quiescence(-beta, -alpha, depth_left - 1, key, ply + 1);

		search_position_records.pop_back();
		unmake_move(m, old_t, key);

		if (val >= beta) return beta;
		if (val > alpha) alpha = val;
	}
	return alpha;
}

bool has_major_pieces(bool red_side) {
	return red_side ? (red_major_count > 0) : (black_major_count > 0);
}

int minmax(int depth, int alpha, int beta, int ply, uint64_t& key, bool is_null = false) {
	if (time_up()) return 0; // 快速退出
	if (ply >= MAX_PLY - 1) return evaluate();
	if (depth <= 0) return quiescence(alpha, beta, 6, key, ply);

	int old_alpha = alpha;
	Move tt_move;
	int tt_score;
	if (query_tt(key, depth, alpha, beta, tt_score, tt_move)) {
		return tt_score;
	}

	bool in_check = is_in_check(is_red);

	// 空步裁剪 (Null Move Pruning)
	if (depth >= 3 && !in_check && !is_null && ply > 0 && has_major_pieces(is_red)) {
		is_red = !is_red;
		key ^= zobrist_side;
		int null_val = -minmax(depth - 3, -beta, -beta + 1, ply + 1, key, true);
		key ^= zobrist_side;
		is_red = !is_red;
		if (time_up()) return 0; // 超时直接退出，不执行 evaluate 避免耗时
		if (null_val >= beta) return beta;
	}

	vector<Move> moves = generate();
	if (moves.empty()) {
		if (in_check) return -MATE_SCORE + ply;
		return 0;
	}

	avoid_long_check_if_possible(moves);
	sort_moves_by_priority(moves, (tt_move.fx != -1) ? &tt_move : nullptr, false, ply);

	int best_score = -INF;
	Move best_move;

	for (size_t i = 0; i < moves.size(); i++) {
		if (time_up()) break;
		Move m = moves[i];
		char old_t = board[m.tx][m.ty];
		bool mover_red = is_red;
		
		make_move(m, key);
		bool gives_check = side_gives_check(mover_red);
		search_position_records.push_back(PositionRecord(serialize_position(is_red), mover_red, gives_check));

		int extension = (gives_check && ply < MAX_PLY - 2) ? 1 : 0;
		int val = -minmax(depth - 1 + extension, -beta, -alpha, ply + 1, key, false);

		search_position_records.pop_back();
		unmake_move(m, old_t, key);

		if (val > best_score) {
			best_score = val;
			best_move = m;
			if (old_t == '.') {
				history_table[is_red ? 1 : 0][m.fx][m.fy][m.tx][m.ty] += depth * depth;
				if (ply < MAX_PLY && !(killer_moves[ply][0] == m)) {
					killer_moves[ply][1] = killer_moves[ply][0];
					killer_moves[ply][0] = m;
				}
			}
		}
		if (val > alpha) alpha = val;
		if (alpha >= beta) break;
	}

	if (!time_up()) {
		int flag = EXACT;
		if (best_score <= old_alpha) flag = ALPHA;
		else if (best_score >= beta) flag = BETA;
		update_tt(key, depth, best_score, flag, best_move);
	}

	return best_score;
}

string query_opening_book(const string& history, bool is_red) {
	map<string, string> book;
	// 网战常见开局：中炮局 (h2e2)
	book[""] = "h2e2"; // 默认起手中炮 (炮二平五)
	
	// --- 红方视角：中炮对屏风马 ---
	book["h2e2h9g7"] = "h0g2"; // 对马8进7，我马八进七
	book["h2e2h9g7h0g2i9h9"] = "i0h0"; // 对车9平8，我车九平八
	book["h2e2h9g7h0g2i9h9i0h0b9c7"] = "g3g4"; // 对马2进3，我兵七进一 (挺兵制马)
	book["h2e2h9g7h0g2i9h9i0h0b9c7g3g4a9a8"] = "h0h6"; // 对车1进1，我车八进六 (巡河车)

	// --- 黑方视角：应对中炮 (h2e2) ---
	book["h2e2"] = "h9g7"; // 对炮二平五，我马8进7 (屏风马)
	book["h2e2h0g2"] = "i9h9"; // 对马八进七，我车9平8
	book["h2e2h0g2i9h9i0h0"] = "b9c7"; // 对车九平八，我马2进3 (形成屏风马)
	book["h2e2h0g2i9h9i0h0b9c7g3g4"] = "c6c5"; // 对兵七进一，我卒3进1
	book["h2e2h0g2i9h9i0h0b9c7g3g4c6c5h0h6"] = "b7b6"; // 对车八进六，我炮2退1 (平炮兑车准备)

	// --- 黑方视角：应对顺炮 (h2e2 h7e7) ---
	book["h2e2h7e7"] = "h0g2"; // 红马八进七
	book["h2e2h7e7h0g2"] = "h9g7"; // 黑马8进7
	book["h2e2h7e7h0g2h9g7"] = "i0h0"; // 红车九平八

	// --- 黑方视角：应对仙人指路 (g3g4) ---
	book["g3g4"] = "c6c5"; // 卒底炮 (对挺兵)
	book["g3g4c6c5"] = "h0g2"; // 红马八进七
	book["g3g4c6c5h0g2"] = "h9g7"; // 黑马8进7
	book["g3g4c6c5h0g2h9g7"] = "b2d2"; // 红炮八平六
	
	// --- 黑方视角：应对飞相局 (c0e2) ---
	book["c0e2"] = "h7e7"; // 应对飞相，黑左中炮
	book["c0e2h7e7"] = "h0g2"; // 红马八进七
	book["c0e2h7e7h0g2"] = "h9g7"; // 黑马8进7

	if (book.count(history)) {
		return book[history];
	}
	return "";
}

// 最终AI：搜索
void ai() {
	current_zobrist_key = compute_zobrist();
	update_major_counts(); // 初始化大子计数器
	update_king_positions(); // 初始化将帅位置
	tt_current_age++;      // 逻辑清理置换表

	string book_move = query_opening_book(game_history, is_red);
	if (!book_move.empty()) {
		int fr = coord_to_row(book_move.substr(0, 2));
		int fc = coord_to_col(book_move.substr(0, 2));
		int tr = coord_to_row(book_move.substr(2, 2));
		int tc = coord_to_col(book_move.substr(2, 2));

		// 校验开局库走法是否合法（安全兜底）
		vector<Move> moves = generate();
		bool valid = false;
		for (size_t i = 0; i < moves.size(); i++) {
			if (moves[i].fx == fr && moves[i].fy == fc && moves[i].tx == tr && moves[i].ty == tc) {
				valid = true; break;
			}
		}
		if (valid && !move_causes_long_check(Move(fr, fc, tr, tc))) {
			output_board(fr, fc, tr, tc);
			return;
		}
	}

	// 动态时间管理：根据对局阶段分配时间，增加安全冗余
	int time_limit = 700;
	if (move_count > 40) time_limit = 850; // 残局留出更多冗余
	if (move_count < 10) time_limit = 500; 

	end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(time_limit);
	Move best_move(0, 0, 0, 0);
	bool has_best = false;
	clear_heuristics();
	// 移除 memset，改用 age 机制

	for (int depth = 1; depth <= 16; depth++) { // 提高上限，靠时间退出
		if (time_up()) break;
		
		int alpha = -INF, beta = INF;
		uint64_t key_copy = current_zobrist_key;
		minmax(depth, alpha, beta, 0, key_copy);

		if (!time_up()) {
			int idx = current_zobrist_key % TT_SIZE;
			if (transposition_table[idx].key == current_zobrist_key) {
				best_move = transposition_table[idx].best_move;
				has_best = true;
			}
		}
	}

	if (!has_best) {
		vector<Move> moves = generate();
		if (moves.empty()) {
			output_board(-1, -1, -1, -1);
		}
		else output_board(moves[0].fx, moves[0].fy, moves[0].tx, moves[0].ty);
		return;
	}

	output_board(best_move.fx, best_move.fy, best_move.tx, best_move.ty);
}

vector<Move> generate() {
	vector<Move> local_moves;

	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 9; y++) {
			char ch = board[x][y];

			if (is_red && !(ch >= 'A' && ch <= 'Z'))continue;
			if (!is_red && !(ch >= 'a' && ch <= 'z'))continue;

			//車R/r
			if (ch == 'R' || ch == 'r') {
				int dx[] = { -1 ,1,0,0 };
				int dy[] = { 0,0,-1,1 };

				for (int d = 0; d < 4; d++) {
					for (int k = 1; k < 10; k++) {
						int xx = x + dx[d] * k, yy = y + dy[d] * k;
						if (xx < 0 || xx >= 10 || yy < 0 || yy >= 9)break;

						char tar = board[xx][yy];
						if (tar == '.') {
							local_moves.emplace_back(x, y, xx, yy);
						}
						else {
							if (is_red && tar >= 'a' && tar <= 'z') {
								local_moves.emplace_back(x, y, xx, yy);
							}
							else if (!is_red && tar >= 'A' && tar <= 'Z') {
								local_moves.emplace_back(x, y, xx, yy);
							}
							break;
						}
					}
				}
			}

			//马N/n
			else if (ch == 'N' || ch == 'n') {
				int dx[] = { -2,-2,-1,-1,1,1,2,2 };
				int dy[] = { -1,1,-2,2,-2,2,-1,1 };
				int bx[] = { -1,-1,0,0,0,0,1,1 };
				int by[] = { 0,0,-1,1,-1,1,0,0 };

				for (int d = 0; d < 8; d++) {
					int xx = x + dx[d], yy = y + dy[d];
					int bx2 = x + bx[d], by2 = y + by[d];

					if (xx < 0 || xx >= 10 || yy < 0 || yy >= 9)continue;
					if (bx2 < 0 || bx2 >= 10 || by2 < 0 || by2 >= 9) continue; // 防御：检查马腿坐标越界
					if (board[bx2][by2] != '.')continue;

					char tar = board[xx][yy];
					if (tar == '.' || (is_red && tar >= 'a' && tar <= 'z') || (!is_red && tar >= 'A' && tar <= 'Z')) {
						local_moves.emplace_back(x, y, xx, yy);
					}
				}
			}

			//炮C/c
			else if (ch == 'C' || ch == 'c') {
				int dx[] = { -1,1,0,0 };
				int dy[] = { 0,0,-1,1 };
				for (int d = 0; d < 4; d++) {
					int step = 1;
					int platform = 0;
					while (true) {
						int xx = x + dx[d] * step;
						int yy = y + dy[d] * step;
						if (xx < 0 || xx >= 10 || yy < 0 || yy >= 9) break;
						char tar = board[xx][yy];
						if (tar == '.') {
							if (platform == 0) local_moves.emplace_back(x, y, xx, yy);
							step++;
							continue;
						}
						platform++;
						if (platform == 1) { step++; continue; }
						if (platform == 2) {
							if ((is_red && tar >= 'a' && tar <= 'z') || (!is_red && tar >= 'A' && tar <= 'Z')) {
								local_moves.emplace_back(x, y, xx, yy);
							}
							break;
						}
						break;
					}
				}
			}

			//士A/a
			else if (ch == 'A' || ch == 'a') {
				int dx[] = { -1,-1,1,1 };
				int dy[] = { -1,1,-1,1 };
				int lx = 7, rx = 9, ly = 3, ry = 5;
				if (!is_red) lx = 0, rx = 2;

				for (int d = 0; d < 4; d++) {
					int xx = x + dx[d], yy = y + dy[d];
					if (xx<lx || xx>rx || yy<ly || yy>ry) continue;
					char tar = board[xx][yy];
					if (tar == '.' || (is_red && tar >= 'a' && tar <= 'z') || (!is_red && tar >= 'A' && tar <= 'Z'))
						local_moves.emplace_back(x, y, xx, yy);
				}
			}

			//象B/b
			else if (ch == 'B' || ch == 'b') {
				int dx[] = { -2,-2,2,2 };
				int dy[] = { -2,2,-2,2 };
				int bx[] = { -1,-1,1,1 };
				int by[] = { -1,1,-1,1 };
				int line = 4; if (!is_red) line = 5;

				for (int d = 0; d < 4; d++) {
					int xx = x + dx[d], yy = y + dy[d];
					int bx2 = x + bx[d], by2 = y + by[d];
					if (xx < 0 || xx >= 10 || yy < 0 || yy >= 9) continue;
					if (bx2 < 0 || bx2 >= 10 || by2 < 0 || by2 >= 9) continue; // 防御：检查象眼坐标越界
					if ((is_red && xx < line) || (!is_red && xx >= line)) continue;
					if (board[bx2][by2] != '.') continue;

					char tar = board[xx][yy];
					if (tar == '.' || (is_red && tar >= 'a' && tar <= 'z') || (!is_red && tar >= 'A' && tar <= 'Z'))
						local_moves.emplace_back(x, y, xx, yy);
				}
			}

			//帅K/k
			else if (ch == 'K' || ch == 'k') {
				int dx[] = { -1,1,0,0 };
				int dy[] = { 0,0,-1,1 };
				int lx = 7, rx = 9, ly = 3, ry = 5;
				if (!is_red) lx = 0, rx = 2;

				for (int d = 0; d < 4; d++) {
					int xx = x + dx[d], yy = y + dy[d];
					if (xx<lx || xx>rx || yy<ly || yy>ry) continue;
					char tar = board[xx][yy];
					if (tar == '.' || (is_red && tar >= 'a' && tar <= 'z') || (!is_red && tar >= 'A' && tar <= 'Z'))
						local_moves.emplace_back(x, y, xx, yy);
				}
			}

			//兵P/p
			else if (ch == 'P' || ch == 'p') {
				int dx[3], dy[3];
				int cnt = 0;

				if (is_red) {
					dx[cnt] = -1; dy[cnt++] = 0;
					if (x <= 4) {
						dx[cnt] = 0; dy[cnt++] = -1;
						dx[cnt] = 0; dy[cnt++] = 1;
					}
				}
				else {
					dx[cnt] = 1; dy[cnt++] = 0;
					if (x >= 5) {
						dx[cnt] = 0; dy[cnt++] = -1;
						dx[cnt] = 0; dy[cnt++] = 1;
					}
				}

				for (int i = 0; i < cnt; i++) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (xx < 0 || xx >= 10 || yy < 0 || yy >= 9) continue;

					char tar = board[xx][yy];
					if (tar == '.'
						|| (is_red && tar >= 'a' && tar <= 'z')
						|| (!is_red && tar >= 'A' && tar <= 'Z'))
					{
						local_moves.emplace_back(x, y, xx, yy);
					}
				}
			}
		}
	}

	vector<Move> legal;
	legal.reserve(local_moves.size());
	for (size_t i = 0; i < local_moves.size(); i++) {
		if (is_legal_move(local_moves[i])) legal.push_back(local_moves[i]);
	}
	return legal;
}

void output_board(int fx, int fy, int tx, int ty) {
	if (fx < 0 || fy < 0 || tx < 0 || ty < 0) {
		cout << "{\"response\":{\"source\":\"-1\",\"target\":\"-1\"}}" << endl;
	}
	else {
		cout << "{\"response\":{\"source\":\"" << rowcol_to_coord(fx, fy) << "\",\"target\":\"" << rowcol_to_coord(tx, ty) << "\"}}" << endl;
	}
}

int main() {
	init_zobrist();
	getInputBotzone();
	ai();
	return 0;
}
