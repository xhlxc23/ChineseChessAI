#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<cstring>
#include<vector>
#include<map>
#include<string>
#include<algorithm>
#include<chrono>
#include<sstream>

using namespace std;

struct Move {
	int fx, fy, tx, ty;
	Move(int ffx, int ffy, int ttx, int tty) :fx(ffx), fy(ffy), tx(ttx), ty(tty) {}
};

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
vector<Move> generate();
void output_board(int fx, int fy, int tx, int ty);
char board[10][9];
bool is_red;
int move_count = 0;
std::chrono::steady_clock::time_point end_time;

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
	if (s == "-1") return -1;
	int y = 0;
	if (s.size() >= 2) y = stoi(s.substr(1));
	return 9 - y;
}

int coord_to_col(const string& s) {
	if (s == "-1") return -1;
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
	board[tr][tc] = board[fr][fc];
	board[fr][fc] = '.';
	move_count++;
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
		size_t q2 = arr.find('"', q1 + 1);
		string src = arr.substr(q1 + 1, q2 - q1 - 1);

		p = arr.find("\"target\"", q2);
		if (p == string::npos) break;
		size_t q3 = arr.find('"', p + 8);
		size_t q4 = arr.find('"', q3 + 1);
		string tgt = arr.substr(q3 + 1, q4 - q3 - 1);

		out_moves.push_back({ src, tgt });
		p = q4;
	}
}

string game_history = "";

void getInputBotzone() {
	string str;
	getline(cin, str);

	init_board();
	move_count = 0;
	is_red = true;
	game_history = "";

	if (str.empty()) return;

	vector<pair<string, string>> reqs, resps;
	parse_moves(str, "requests", reqs);
	parse_moves(str, "responses", resps);

	if (!reqs.empty() || !resps.empty()) {
		int turnID = resps.size();
		for (int i = 0; i < turnID; i++) {
			if (reqs[i].first != "-1") {
				int fr = coord_to_row(reqs[i].first), fc = coord_to_col(reqs[i].first);
				int tr = coord_to_row(reqs[i].second), tc = coord_to_col(reqs[i].second);
				apply_move_rc(fr, fc, tr, tc);
				game_history += reqs[i].first + reqs[i].second;
			}
			if (resps[i].first != "-1") {
				int fr = coord_to_row(resps[i].first), fc = coord_to_col(resps[i].first);
				int tr = coord_to_row(resps[i].second), tc = coord_to_col(resps[i].second);
				apply_move_rc(fr, fc, tr, tc);
				game_history += resps[i].first + resps[i].second;
			}
		}

		if (turnID < (int)reqs.size()) {
			if (reqs[turnID].first != "-1") {
				int fr = coord_to_row(reqs[turnID].first), fc = coord_to_col(reqs[turnID].first);
				int tr = coord_to_row(reqs[turnID].second), tc = coord_to_col(reqs[turnID].second);
				apply_move_rc(fr, fc, tr, tc);
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
		size_t q2 = str.find('"', q1 + 1);
		string s = str.substr(q1 + 1, q2 - q1 - 1);

		size_t q3 = str.find('"', t_pos + 8);
		size_t q4 = str.find('"', q3 + 1);
		string t = str.substr(q3 + 1, q4 - q3 - 1);

		if (s == "-1") {
			is_red = true;
		}
		else {
			int fr = coord_to_row(s), fc = coord_to_col(s);
			int tr = coord_to_row(t), tc = coord_to_col(t);
			apply_move_rc(fr, fc, tr, tc);
			game_history += s + t;
			is_red = false;
		}
	}
}

int get_score(char ch) {
	switch (ch) {
	case 'K': case 'k': return 99999;
	case 'R': case 'r': return 100;
	case 'N': case 'n': return 45;
	case 'C': case 'c': return 50;
	case 'B': case 'b': return 20;
	case 'A': case 'a': return 20;
	case 'P': case 'p': return 10;
	default: return 0;
	}
}

// 位置附加分表 (PST - Piece Square Tables)，以红方视角为准（黑方只需翻转行坐标）
// 数值表示该棋子在该位置的额外奖励分，提升 AI 的走位意识
const int PST_PAWN[10][9] = {
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 0: 底线
	{10, 15, 20, 25, 25, 20, 15, 10, 10}, // 1: 接近兵临城下
	{10, 15, 20, 25, 25, 20, 15, 10, 10}, // 2
	{ 5, 10, 15, 20, 20, 15, 10,  5,  5}, // 3
	{ 5,  5,  5,  5,  5,  5,  5,  5,  5}, // 4: 刚过河
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 5: 自己河界
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 6: 初始位置
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 7
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 8
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0}, // 9
};

const int PST_KNIGHT[10][9] = {
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  5,  5,  5,  5,  5,  5,  5,  0},
	{ 0,  5, 10, 10, 10, 10, 10,  5,  0},
	{ 0,  5, 10, 15, 15, 15, 10,  5,  0},
	{ 0,  5, 10, 15, 15, 15, 10,  5,  0},
	{ 0,  5, 10, 10, 10, 10, 10,  5,  0},
	{ 0,  5,  5,  5,  5,  5,  5,  5,  0},
	{ 0,  0,  5,  5,  5,  5,  5,  0,  0},
	{-5,  0,  0,  0,  0,  0,  0,  0, -5},
	{-5, -5, -5, -5, -5, -5, -5, -5, -5},
};

const int PST_ROOK[10][9] = {
	{ 5,  5,  5, 10, 10, 10,  5,  5,  5},
	{ 5, 10, 10, 15, 15, 15, 10, 10,  5},
	{ 5, 10, 10, 15, 15, 15, 10, 10,  5},
	{ 5, 10, 10, 15, 15, 15, 10, 10,  5},
	{10, 15, 15, 20, 20, 20, 15, 15, 10},
	{10, 15, 15, 20, 20, 20, 15, 15, 10},
	{ 0,  5,  0, 10, 10, 10,  0,  5,  0},
	{-5,  5,  0, 10, 10, 10,  0,  5, -5},
	{-5,  0,  0, 10,  0, 10,  0,  0, -5},
	{-5,  0,  0, 10,  0, 10,  0,  0, -5},
};

const int PST_CANNON[10][9] = {
	{ 5,  5,  0, -5, -5, -5,  0,  5,  5},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  5,  5,  5, 10,  5,  5,  5,  0},
	{ 0,  0,  0,  0, 10,  0,  0,  0,  0},
	{ 0,  0,  5,  0, 10,  0,  5,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  5,  5, 10,  5,  5,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  5,  5,  5,  0,  0,  0},
};

// 评估棋盘总分,红正黑负
int evaluate() {
	int cnt = 0;
	for (int x = 0; x < 10; x++) {
		for (int y = 0; y < 9; y++) {
			if (board[x][y] != '.') cnt++;
		}
	}

	int score = 0;
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 9; j++) {
			char c = board[i][j];
			if (c == '.') continue;

			int val = get_score(c);
			// 炮残局减分（由于残局炮威力下降）
			if ((c == 'C' || c == 'c') && cnt <= 12) val -= 10;

			int pst_val = 0;
			if (c >= 'A' && c <= 'Z') {
				// 红方
				if (c == 'P') pst_val = PST_PAWN[i][j];
				else if (c == 'N') pst_val = PST_KNIGHT[i][j];
				else if (c == 'R') pst_val = PST_ROOK[i][j];
				else if (c == 'C') pst_val = PST_CANNON[i][j];
				score += (val + pst_val);
			}
			else {
				// 黑方（黑方的行坐标要镜像翻转后查表）
				if (c == 'p') pst_val = PST_PAWN[9 - i][j];
				else if (c == 'n') pst_val = PST_KNIGHT[9 - i][j];
				else if (c == 'r') pst_val = PST_ROOK[9 - i][j];
				else if (c == 'c') pst_val = PST_CANNON[9 - i][j];
				score -= (val + pst_val);
			}
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
				if (p == king && c == tc) {
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

bool is_legal_move(const Move& m) {
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

	int kr = -1, kc = -1;
	bool red_side = is_red;
	// 模拟走子后，再去寻找老将的位置
	bool ok = find_king(red_side, kr, kc) != 0;
	bool illegal = true;
	if (ok) {
		illegal = attacked_by(!red_side, kr, kc);
	}

	board[m.fx][m.fy] = old_f;
	board[m.tx][m.ty] = old_t;

	return ok && !illegal;
}

int minmax(int depth, int alpha, int beta) {
	if (time_up()) return evaluate();
	if (depth == 0) return evaluate();

	vector<Move> moves = generate();
	if (moves.empty()) return -999999;

	sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
		char ta = board[a.tx][a.ty];
		char tb = board[b.tx][b.ty];
		return get_score(ta) > get_score(tb);
		});

	int best = -999999;
	for (size_t i = 0; i < moves.size(); i++) {
		if (time_up()) break;
		Move m = moves[i];
		char old_f = board[m.fx][m.fy];
		char old_t = board[m.tx][m.ty];
		board[m.tx][m.ty] = old_f;
		board[m.fx][m.fy] = '.';
		is_red = !is_red;

		int val = -minmax(depth - 1, -beta, -alpha);

		is_red = !is_red;
		board[m.fx][m.fy] = old_f;
		board[m.tx][m.ty] = old_t;

		if (val > best) best = val;
		if (val > alpha) alpha = val;
		if (alpha >= beta) break;
	}
	return best;
}

string query_opening_book(const string& history, bool is_red) {
	map<string, string> book;
	// 红方经典：中炮巡河车（仙人指路等）
	book[""] = "h2e2"; // 炮二平五
	book["h2e2h9g7"] = "h0g2"; // 对马8进7，我马八进七
	book["h2e2h9g7h0g2i9h9"] = "i0h0"; // 对车9平8，我车九平八
	book["h2e2h9g7h0g2i9h9i0h0b9c7"] = "g3g4"; // 对马2进3，我兵七进一 (挺兵制马)
	book["h2e2h9g7h0g2i9h9i0h0b9c7g3g4a9a8"] = "h0h6"; // 对车1进1，我车八进六 (巡河车)

	// 黑方经典：屏风马应对中炮
	book["h2e2"] = "h9g7"; // 对炮二平五，我马8进7
	book["h2e2h0g2"] = "i9h9"; // 对马八进七，我车9平8
	book["h2e2h0g2i9h9i0h0"] = "b9c7"; // 对车九平八，我马2进3 (形成屏风马)
	book["h2e2h0g2i9h9i0h0b9c7g3g4"] = "c6c5"; // 对兵七进一，我卒3进1
	book["h2e2h0g2i9h9i0h0b9c7g3g4c6c5h0h6"] = "b7b6"; // 对车八进六，我炮2退1 (平炮兑车准备)

	if (book.count(history)) {
		return book[history];
	}
	return "";
}

// 最终AI：搜索
void ai() {
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
		if (valid) {
			output_board(fr, fc, tr, tc);
			return;
		}
	}

	end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(800);
	Move best_move(0, 0, 0, 0);
	bool has_best = false;

	for (int depth = 1; depth <= 4; depth++) {
		if (time_up()) break;
		vector<Move> moves = generate();
		if (moves.empty()) break;

		sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
			char ta = board[a.tx][a.ty];
			char tb = board[b.tx][b.ty];
			return get_score(ta) > get_score(tb);
			});

		int best_score = -999999;
		Move cur_best = moves[0];

		for (size_t i = 0; i < moves.size(); i++) {
			Move m = moves[i];
			char old_f = board[m.fx][m.fy];
			char old_t = board[m.tx][m.ty];

			board[m.tx][m.ty] = old_f;
			board[m.fx][m.fy] = '.';
			is_red = !is_red;

			int score = -minmax(depth - 1, -999999, 999999);

			is_red = !is_red;
			board[m.fx][m.fy] = old_f;
			board[m.tx][m.ty] = old_t;

			if (score > best_score) {
				best_score = score;
				cur_best = m;
			}
			if (time_up()) break;
		}

		best_move = cur_best;
		has_best = true;
	}

	if (!has_best) {
		vector<Move> moves = generate();
		if (moves.empty()) {
			if (is_red) output_board(7, 7, 7, 4); // 红方默认炮二平五
			else output_board(2, 1, 2, 4);        // 黑方默认炮8平5
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
	getInputBotzone();
	ai();
	return 0;
}
