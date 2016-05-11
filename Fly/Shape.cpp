/*
#include <algorithm>
#include <vector>
#include <queue>

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

class ShapeHandle
{
	struct poi
	{
		int x, y;
		poi(){}
		poi(int _x, int _y) { x = _x; y = _y; }
	};
	int n, m, stay_cnt = 0;
	unsigned char *rgb;
	unsigned char *color;
	unsigned char *image;
	Action action;
	vector<poi> cl;
	//ColorHandle ch = new ColorHandle();
	int tcnt = 0;
public:
	void Reset(int row, int col, unsigned char* img, unsigned char* num)
	{
		n = row; m = col;
		rgb = new unsigned char[n * m * 3];
		color = new unsigned char[n * m];
		memcpy(img, rgb, n * m * 3);
		memcpy()
		for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
		for (int k = 0; k < 3; k++)
			rgb[i, j, k] = img[(i * m + j) * 3 + k];
		for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
			color[i, j] = num[i * m + j];
		action = Stay;
		image = img;
		tcnt = 0;
	}

	Action Calc(cv::Mat& frame, cv::Mat& marker)
	{
		int cnt1 = 0, cnt2 = 0;
		poi c1, c2;
		int tot = TotalArea();
		result = image;
		if (tot > n * m * 3 / 4)
		{
			stay_cnt = 0;
			return Down;
		}
		vector<poi> cl;
		SearchPart(2,  c2,  cnt2);
		if (cnt2 != 0)
		{
			c2.x /= cnt2; c2.y /= cnt2;
			if (cnt2 == 4)
			{
				Link(cl, c2);
				MarkCenter(c2);
				result = image;
			}
			int LR = 0, FB = 0;
			if (c2.y < m / 2 - 50) LR = 1;
			if (c2.y > m / 2 + 50) LR = 2;
			if (c2.x < n / 2 - 50) FB = 1;
			if (c2.x > n / 2 + 50) FB = 2;
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
				if (abs(cl[0].x - cl[2].x) < abs(cl[0].y - cl[2].y))
				{
					if (cl[0].x - cl[2].x < -15 && cl[1].y - cl[3].y < -15)
						action = cl[0].y < cl[2].y ? Counterclockwise : Clockwise;
					else if (cl[0].x - cl[2].x > 15 && cl[1].y - cl[3].y > 15)
						action = cl[0].y < cl[2].y ? Clockwise : Counterclockwise;
				}
				else
				{
					if (cl[0].y - cl[2].y < -15 && cl[1].x - cl[3].x > 15)
						action = cl[0].x < cl[2].x ? Clockwise : Counterclockwise;
					else if (cl[0].y - cl[2].y > 15 && cl[1].x - cl[3].x < -15)
						action = cl[0].x < cl[2].x ? Counterclockwise : Clockwise;
				}
			}
			if (action == Stay)
			{
				SearchPart(1, c1, cnt1);
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
		if (action == Stay)
		{
			stay_cnt++;
			if (stay_cnt > 5) action = Up;
		}
		if (action != Stay) stay_cnt = 0;
		return action;
	}

private:
	const int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	const int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	void SearchPart(int COLOR, poi& center, int& count)
	{
		count = 0;
		center.x = 0;
		center.y = 0;
		bool *vis = new bool[n * m];
		queue<poi> q;
		vector<poi> list;
		for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
		{
			if (color[i, j] != COLOR || vis[i * m + j]) continue;
			list.clear();
			q.push(poi(i, j));
			vis[i * m + j] = true;
			while (q.size() > 0)
			{
				poi now = q.front();
				q.pop();
				list.push_back(now);
				for (int k = 0; k < 8; k++)
				{
					poi next(now.x + dx[k], now.y + dy[k]);
					if (!IsValid(next) || color[next.x, next.y] != COLOR || vis[next.x * m + next.y]) continue;
					q.push(next);
					vis[next.x * m + next.y] = true;
				}
			}
			poi c;
			int result = IsSquare(list, &c);
			if (result == 0) continue;
			//if (result == 1 && COLOR == 2) action = Counterclockwise;
			//if (result == 2 && COLOR == 2) action = Clockwise;
			center.x += c.x;
			center.y += c.y;
			if (COLOR == 2) cl.push_back(c);
			count++;
		}
		delete[] vis;
	}

	// 判断点集是否构成正方形，返回：0不是、1逆时针旋转、2顺时针旋转、3已摆正
	int IsSquare(vector<poi> list, poi *center)
	{
		// 求出连通块的几何中心center、面积area、四个方向边界直线、左上与右下角坐标
		center->x = 0;
		center->y = 0;
		poi a = list[0], c = list[0], b, d;
		int lb = a.y, rb = a.y, ub = a.x, db = a.x;
		int area = 0, len;
		for_each(list.begin(), list.end(), [&](poi& now)
		{
			center->x += now.x; center->y += now.y;
			if (now.x < a.x || now.x == a.x && now.y < a.y) a = now;
			if (now.x > c.x || now.x == c.x && now.y > c.y) c = now;
			area++;
			lb = min(lb, now.y); rb = max(rb, now.y);
			ub = min(ub, now.x); db = max(db, now.x);
		});
		center->x /= area; center->y /= area;
		if (area < 1000) return 0;

		// 判断正方形是否平行于坐标轴，并求出四个顶点a,b,c,d
		if (abs(area * 1.0 / ((rb - lb) * (db - ub)) - 1) < 0.4)
		{
			len = sqrt(area);
			a = poi(ub, lb); b = poi(db, lb);
			c = poi(db, rb); d = poi(ub, rb);
		}
		else {
			len = dist(a, c) / 2 * sqrt(2);
			if (abs(mul(*center, a, c)) * 1.0 / dist(a, c) > 15) return 0;
			if (abs(len * len - area) * 1.0 / len / len > 0.15) return 0;
			b = poi(center->x - a.y + center->y, center->y + a.x - center->x);
			d = poi(center->x - c.y + center->y, center->y + c.x - center->x);
		}

		// 检查正方形形状
		int inner = 0;
		for_each(list.begin(), list.end(), [&](poi& now)
		{
			if (mul(now, a, b) >= 0 && mul(now, b, c) >= 0 && mul(now, c, d) >= 0 && mul(now, d, a) >= 0) inner++;
		});
		if (inner < len * len * 0.8) return 0;
		//if (Math.Abs(a.x - c.x) > Math.Abs(a.y - c.y) * 1.2) return 1; //counterclockwise
		//if (Math.Abs(a.x - c.x) < Math.Abs(a.y - c.y) * 0.8) return 2; //clockwise
		return 3;
	}

	void Link(vector<poi>& cl, poi center)
	{
		for (int i = 0; i < 4; i++)
		for (int j = i + 2; j < 4; j++)
		if (mul(center, cl[i], cl[j]) > mul(center, cl[i], cl[i + 1]))
		{
			poi p = cl[i + 1]; cl[i + 1] = cl[j]; cl[j] = p;
		}
		cl.push_back(cl[0]);
		for (int i = 0; i < 4; i++)
		{
			poi a = cl[i], b = cl[i + 1];
			if (a.x > b.x) { poi c = a; a = b; b = c; }
			for (int x = a.x, y = a.y; x <= b.x; x++)
			{
				int ny = a.x == b.x ? b.y :a.y + 1.0 * (b.y - a.y) / (b.x - a.x) * (x - a.x);
				for (int j = min(y, ny); j <= max(y, ny); j++)
				{
					if (!IsValid(poi(x, j))) continue;
					color[x, j] = 0;
					ch.SetPixel(image, (x * m + j) * 3, Color.FromArgb(0, 0, 0));
				}
				y = ny;
			}
		}
	}

	void MarkCenter(poi center)
	{
		for (int i = -5; i <= 5; i++)
		for (int j = -5; j <= 5; j++)
		{
			int x = center.x + i, y = center.y + j;
			if (IsValid(poi(i, j))) ch.SetPixel(image, (x * m + y) * 3, Color.FromArgb(0, 255, 255));
		}
	}

	int TotalArea()
	{
		int area = 0;
		for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
		if (color[i, j] != 0) area++;
		return area;
	}

	int mul(poi a, poi b, poi c)
	{
		return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
	}

	double dist(poi a, poi b)
	{
		return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	}

	poi turn(poi a)
	{
		return poi(-a.y, a.x);
	}

	bool IsValid(poi a)
	{
		return a.x >= 0 && a.y >= 0 && a.x < n && a.y < m;
	}
};

*/