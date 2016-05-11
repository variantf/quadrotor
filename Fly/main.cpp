#include <Windows.h>
#include <iostream>
#include <string>
#include "mavlink.h"
#include <conio.h>
#include <consoleapi.h>
#include <sstream>
#include <ctime>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <vector>
#include <queue>

#include "IO.h"

using namespace std;

enum Action
{
	Stay = 0,
	Down = 1,
	Up = 2,
	Left = 3,
	Right = 4,
	Front = 5,
	Back = 6,
	Clockwise = 7,
	Counterclockwise = 8
};

struct Result
{
	Result(){}
	Result(Action a, int b, int c, int d, int e){ act = a, vx = b, vy = c, dx = d, dy = e; }
	Action act;
	int vx, vy;
	int dx, dy;
};

class ShapeHandle
{
	int stay_cnt = 0;
	Action action;
	int stepx = 0, stepy = 0;
	int tcnt = 0;
	cv::Point last_pos = cv::Point(0, 0);
public:
	Result Calc(cv::Mat& frame, cv::Mat& marker)
	{
		vector<cv::Point> cl;
		int cnt1 = 0, cnt2 = 0, dx, dy, vx, vy;
		tcnt = 0;
		action = Stay; stepx = stepy = 0;
		cv::Point c1, c2;
		int tot = cv::countNonZero(marker);
		if (tot > marker.total() * 3 / 4)
		{
			stay_cnt = 0;
			return Result(Down, 0, 0, 0, 0);
		}
		SearchPart(2, c2, cnt2, marker, cl);

		if (cnt2 != 0)
		{
			c2.x /= cnt2; c2.y /= cnt2;
			if (cnt2 == 4)
			{
				Link(cl, c2, marker, frame);
				MarkCenter(c2, frame);
			}
			if (last_pos.x || last_pos.y)
			{
				dx = marker.rows / 2 - c2.x, vx = c2.x - last_pos.x;
				dy = marker.cols / 2 - c2.y, vy = c2.y - last_pos.y;
				stepx = dx - vx, stepy = dy - vy;
			}
			last_pos = c2;

			int LR = 0, FB = 0;
			if (c2.y < marker.cols / 2 - 50) LR = 1;
			if (c2.y > marker.cols / 2 + 50) LR = 2;
			if (c2.x < marker.rows / 2 - 50) FB = 1;
			if (c2.x > marker.rows / 2 + 50) FB = 2;
			if (LR + FB > 0)
			{
				tcnt++;
				if ((tcnt & 1) == 0)
				{
					if (LR == 1) action = Left;
					else if (LR == 2) action = Right;
					else if (FB == 1) action = Front;
					else action = Back;
				}
				else
				{
					if (FB == 1) action = Front;
					else if (FB == 2) action = Back;
					else if (LR == 1) action = Left;
					else action = Right;
				}
			}
			else if (cnt2 == 4)
			{
				int delta = dist(cl[0], cl[1]) / 5;
				delta = max(delta, 15);
				if (abs(cl[0].x - cl[2].x) < abs(cl[0].y - cl[2].y))
				{
					if (cl[0].x - cl[2].x < -delta && cl[1].y - cl[3].y < -delta)
						action = cl[0].y < cl[2].y ? Counterclockwise : Clockwise;
					else if (cl[0].x - cl[2].x > delta && cl[1].y - cl[3].y > delta)
						action = cl[0].y < cl[2].y ? Clockwise : Counterclockwise;
				}
				else
				{
					if (cl[0].y - cl[2].y < -delta && cl[1].x - cl[3].x > delta)
						action = cl[0].x < cl[2].x ? Clockwise : Counterclockwise;
					else if (cl[0].y - cl[2].y > delta && cl[1].x - cl[3].x < -delta)
						action = cl[0].x < cl[2].x ? Counterclockwise : Clockwise;
				}
			}
			if (action == Stay)
			{
				SearchPart(1, c1, cnt1, marker, cl);
				//if (c2.y < m / 2 - 20) action = Left;
				//else if (c2.y > m / 2 + 20) action = Right;
				//else if (c2.x < n / 2 - 20) action = Front;
				//else if (c2.x > n / 2 + 20) action = Back;
				if (cnt2 == 4 && cnt1 == 5 && action == Stay)
				{
					c1.x /= cnt1; c1.y /= cnt1;
					if (abs(c1.x - c2.x) + abs(c1.y - c2.y) < 50) action = Down;
				}
			}
		}
		else last_pos = cv::Point(0, 0);

		if (action == Stay)
		{
			stay_cnt++;
			if (stay_cnt > 5) action = Up;
		}
		if (action != Stay) stay_cnt = 0;
		return Result(action, vx, vy, dx, dy);
	}

private:
	void SearchPart(int COLOR, cv::Point& center, int& count, const cv::Mat& marker, vector<cv::Point>& cl)
	{
		clock_t st = clock();
		static const int dx[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
		static const int dy[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
		count = 0;
		center.x = 0;
		center.y = 0;
		cv::Rect rect(cv::Point(), cv::Point(marker.rows, marker.cols));
		bool *vis = new bool[marker.total()];
		memset(vis, 0, marker.total());
		queue<cv::Point> q;
		vector<cv::Point> list;
		unsigned char *marker_data = (unsigned char*)marker.data;
		for (int i = 0; i < marker.rows; i++) {
			for (int j = 0; j < marker.cols; j++)
			{
				if (marker_data[i * marker.step + j] != COLOR || vis[i * marker.cols + j]) continue;
				list.clear();
				q.push(cv::Point(i, j));
				vis[i * marker.cols + j] = true;
				while (q.size() > 0)
				{
					cv::Point now = q.front();
					q.pop();
					list.push_back(now);
					for (int k = 0; k < 8; k++)
					{
						cv::Point next(now.x + dx[k], now.y + dy[k]);
						if (!rect.contains(next) || marker_data[next.x * marker.step + next.y] != COLOR || vis[next.x * marker.cols + next.y]) continue;
						q.push(next);
						vis[next.x * marker.cols + next.y] = true;
					}
				}
				cv::Point c;
				//clock_t square = clock();
				int result = IsSquare(list, &c);
				//double s_ed = double(clock() - square) / CLOCKS_PER_SEC;
				//cout << "Cycles: " << clock() - square << endl;
				//cout << " =====> isSquare" << fixed << s_ed << endl;
				if (result == 0) continue;
				//if (result == 1 && COLOR == 2) action = Counterclockwise;
				//if (result == 2 && COLOR == 2) action = Clockwise;
				center.x += c.x;
				center.y += c.y;
				if (COLOR == 2) cl.push_back(c);
				count++;
			}
		}
		delete[] vis;
		cout << "bfs ====> " << double(clock() - st) / CLOCKS_PER_SEC << endl;
	}

	// 判断点集是否构成正方形，返回：0不是、1逆时针旋转、2顺时针旋转、3已摆正
	int IsSquare(vector<cv::Point> list, cv::Point *center)
	{
		// 求出连通块的几何中心center、面积area、四个方向边界直线、左上与右下角坐标
		center->x = 0;
		center->y = 0;
		cv::Point a = list[0], c = list[0], b, d;
		int lb = a.y, rb = a.y, ub = a.x, db = a.x;
		int area = 0, len;
		for_each(list.begin(), list.end(), [&](cv::Point& now)
		{
			center->x += now.x; center->y += now.y;
			if (now.x < a.x || now.x == a.x && now.y < a.y) a = now;
			if (now.x > c.x || now.x == c.x && now.y > c.y) c = now;
			area++;
			lb = min(lb, now.y); rb = max(rb, now.y);
			ub = min(ub, now.x); db = max(db, now.x);
		});
		center->x /= area; center->y /= area;

		if (area < 1000) {
			return 0;
		}

		// 判断正方形是否平行于坐标轴，并求出四个顶点a,b,c,d
		if (abs(area * 1.0 / ((rb - lb) * (db - ub)) - 1) < 0.4)
		{
			len = sqrt(area);
			a = cv::Point(ub, lb); b = cv::Point(db, lb);
			c = cv::Point(db, rb); d = cv::Point(ub, rb);
		}
		else {
			len = dist(a, c) / 2 * sqrt(2);
			if (abs(mul(*center, a, c)) * 1.0 / dist(a, c) > 15) return 0;
			if (abs(len * len - area) * 1.0 / len / len > 0.15) return 0;
			b = cv::Point(center->x - a.y + center->y, center->y + a.x - center->x);
			d = cv::Point(center->x - c.y + center->y, center->y + c.x - center->x);
		}

		// 检查正方形形状
		int inner = 0;
		for_each(list.begin(), list.end(), [&](cv::Point& now)
		{
			if (mul(now, a, b) >= 0 && mul(now, b, c) >= 0 && mul(now, c, d) >= 0 && mul(now, d, a) >= 0) inner++;
		});
		if (inner < len * len * 0.8) return 0;
		//if (Math.Abs(a.x - c.x) > Math.Abs(a.y - c.y) * 1.2) return 1; //counterclockwise
		//if (Math.Abs(a.x - c.x) < Math.Abs(a.y - c.y) * 0.8) return 2; //clockwise

		return 3;
	}

	void Link(vector<cv::Point>& cl, cv::Point center, cv::Mat& marker, cv::Mat& frame)
	{
		cv::Rect rect(cv::Point(), cv::Point(frame.rows, frame.cols));
		for (int i = 0; i < 4; i++)
		for (int j = i + 2; j < 4; j++)
		if (mul(center, cl[i], cl[j]) > mul(center, cl[i], cl[i + 1]))
		{
			cv::Point p = cl[i + 1]; cl[i + 1] = cl[j]; cl[j] = p;
		}
		cl.push_back(cl[0]);
		for (int i = 0; i < 4; i++)
		{
			cv::Point a = cl[i], b = cl[i + 1];
			if (a.x > b.x) { cv::Point c = a; a = b; b = c; }
			for (int x = a.x, y = a.y; x <= b.x; x++)
			{
				int ny = a.x == b.x ? b.y : a.y + 1.0 * (b.y - a.y) / (b.x - a.x) * (x - a.x);
				for (int j = min(y, ny); j <= max(y, ny); j++)
				{
					if (!rect.contains(cv::Point(x, j))) continue;
					marker.at<unsigned char>(x, j) = 0;
					frame.at<cv::Vec3b>(x, j) = cv::Vec3b(0, 255, 255);
				}
				y = ny;
			}
		}
	}

	void MarkCenter(cv::Point center, cv::Mat& frame)
	{
		cv::Rect rect(cv::Point(0, 0), cv::Point(frame.rows, frame.cols));
		for (int i = -5; i <= 5; i++)
		for (int j = -5; j <= 5; j++)
		{
			int x = center.x + i, y = center.y + j;
			if (rect.contains(cv::Point(i, j))) frame.at<cv::Vec3b>(x, y) = cv::Vec3b(0, 255, 255);
		}
	}

	int mul(cv::Point a, cv::Point b, cv::Point c)
	{
		return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
	}

	double dist(cv::Point a, cv::Point b)
	{
		return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	}

	cv::Point turn(cv::Point a)
	{
		return cv::Point(-a.y, a.x);
	}
};



#define ROTATE 3
#define COUNTER_CLOCKWISE -1
#define CLOCKWISE 1
#define FRONT_BACK 1
#define FRONT -1
#define BACK 1
#define LEFT_RIGHT 0
#define LEFT -1
#define RIGHT 1

OVERLAPPED read_overlapped;
OVERLAPPED write_overlapped;

void gotoxy(short x, short y)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
	COORD position = { x, y };

	SetConsoleCursorPosition(hStdout, position);
}

double get_time() {
	LARGE_INTEGER tm, counter;
	QueryPerformanceFrequency(&tm);
	QueryPerformanceCounter(&counter);
	return double(counter.QuadPart) / tm.QuadPart;
}

void log(const char* msg) {
	LARGE_INTEGER performanceCount;
	QueryPerformanceCounter(&performanceCount);
	printf("%f %s\n", performanceCount.QuadPart /1000000.0, msg);
}


#define puts printFile
FILE* f = fopen("instruct.txt", "w");
void printFile(char * str) {
	printf("%s\n", str);
	fprintf(f, "%s\n", str);
}



void mark(cv::Mat& img, cv::Mat& marker) {
	const unsigned char g_h1 = 35, g_h2 = 77, g_s1 = 43, g_s2 = 255, g_v1 = 46, g_v2 = 255;
	const unsigned char p_h1 = 125, p_h2 = 155, p_s1 = 43, p_s2 = 255, p_v1 = 46, p_v2 = 255;
	marker = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);
	for (int i = 0; i < img.size[0]; i++) {
		cv::Vec3b *row_img = img.ptr<cv::Vec3b>(i);
		unsigned char *row_marker = marker.ptr<unsigned char>(i);
		for (int j = 0; j < img.size[1]; j++) {
			cv::Vec3b &pixel = row_img[j];
			unsigned char &pixel_marker = row_marker[j];
			if (g_h1 <= pixel.val[0] && pixel.val[0] <= g_h2 && g_s1 <= pixel.val[1] && pixel.val[1] <= g_s2 && g_v1 <= pixel.val[2] && pixel.val[2] <= g_v2) {
				pixel_marker = 1;
				pixel = cv::Vec3b(150, 255, 255);
			}
			else if (p_h1 <= pixel.val[0] && pixel.val[0] <= p_h2 && p_s1 <= pixel.val[1] && pixel.val[1] <= p_s2 && p_v1 <= pixel.val[2] && pixel.val[2] <= p_v2) {
				pixel_marker = 2;
				pixel = cv::Vec3b(30, 255, 255);
			}
		}
	}
}

int main() {
	LARGE_INTEGER freq, tm;
	QueryPerformanceFrequency(&freq);
	IO* io = IO::getIOInstance();
	string mode;
	cin >> mode;
	if (mode == "ping") {
		cout << "start ping." << endl;
		mavlink_message_t msg;
		for (int seq = 0; seq < 1000 ; seq ++) {
			mavlink_msg_ping_pack(1, 1, &msg, 0, seq, 1, 1);
			(*io) << &msg;
			Sleep(25);
		}
		getch();
		return 0;
	}
	else if (mode == "manual") {
		int fb = 1500;
		int lr = 1500;
		while (1) {
			if (GetKeyState('W') < 0){
				fb -= 50;
			}
			else if (GetKeyState('S') < 0) {
				fb += 50;
			}
			else{
				if (fb > 1500) {
					fb -= 50;
				}
				else if (fb < 1500) {
					fb += 50;
				}
			}
			if (GetKeyState('D') < 0) {
				lr += 50;
			}
			else if (GetKeyState('A') < 0) {
				lr -= 50;
			}
			else {
				if (lr > 1500) {
					lr -= 50;
				}
				else if (lr < 1500) {
					lr += 50;
				}
			}
			if (fb > 1800) fb = 1800;
			else if (fb < 1200) fb = 1200;
			if (lr > 1800) lr = 1800;
			else if (lr < 1200) lr = 1200;
			printf("fb: %d, lr: %d\n", fb, lr);
			mavlink_message_t msg;
			mavlink_msg_rc_channels_raw_pack(1, 1, &msg, 0, 1, lr, fb, UINT16_MAX, 1500, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, 255);
			(*io) << &msg;
			Sleep(30);
		}
		return 0;
	}
	cv::VideoCapture vc;

	ShapeHandle sh;
	if (!vc.open(1)) {
		puts("Open Failed.");
		return 0;
	}
	int frame_width = vc.get(CV_CAP_PROP_FRAME_WIDTH);
	int frame_height = vc.get(CV_CAP_PROP_FRAME_HEIGHT);
	int cnt = vc.get(CV_CAP_PROP_FRAME_COUNT);
	//cv::VideoWriter vw("out1.avi", -1, 10, cv::Size(frame_width, frame_height), true);
	cv::namedWindow("frame", 0);
	while (true) {
		gotoxy(0, 0);
		cv::Mat frame;
		//frame = cv::imread("C:\\Downloads\\开飞机\\bin\\Release\\video\\capture385.jpg");
		clock_t st = clock();
		vc >> frame;
		cv::flip(frame, frame, -1);

		if (frame.empty())
			break;

		cv::cvtColor(frame, frame, CV_BGR2HSV);
		cv::Mat marker;
		mark(frame, marker);
		cout << double(clock() - st) / CLOCKS_PER_SEC << endl;

		Result sh_res = sh.Calc(frame, marker);
		Action ac = sh_res.act;
		cout << '(' << sh_res.vx << ',' << sh_res.vy << ") (" << sh_res.dx << ',' << sh_res.dy << ")                     " << endl;

		//cout << double(clock() - st) / CLOCKS_PER_SEC << endl;
		//sendCmd(ac, sh_res.vx - sh_res.dx, sh_res.vy - sh_res.dy);
		//cout << double(clock() - st) / CLOCKS_PER_SEC << endl;

		cv::cvtColor(frame, frame, CV_HSV2BGR);

		cv::putText(frame, to_string(ac), cv::Point(30, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));

		cv::putText(frame, to_string(sh_res.vx - sh_res.dx), cv::Point(120, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));

		cv::putText(frame, to_string(sh_res.vy - sh_res.dy), cv::Point(210, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0));
		

		cv::imshow("frame", frame);

		//vw << frame;

		cout << double(clock() - st) / CLOCKS_PER_SEC << endl;
		if (cv::waitKey(3) >= 0) break;
	}
	cv::waitKey();
}