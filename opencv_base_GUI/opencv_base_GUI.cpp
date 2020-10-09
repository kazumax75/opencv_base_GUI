#include "stdafx.h"
#include "opencv_base_GUI.h"

using namespace std;
using namespace cv;

#define SE_PLAYER_CAPTION "HSHS"
#define SE_PLAYER_EXE "hsptmp.exe"
#define DEATH_TEMPLATE_IMAGE "splat_temp.bmp" 
#define MAP_TEMPLATE_IMAGE   "map_batu.bmp"

//デバッグ文字列表示用マクロ
#define dprintf( str, ... ) \
	{ \
		TCHAR c[2560]; \
		_stprintf_s( c, str, __VA_ARGS__ ); \
		OutputDebugString( c ); \
	}
HWND hwnd;
HINSTANCE hInstance;
BITMAPFILEHEADER srcBit_H;
BITMAPINFO srcBit_Info;
int dsk_width, dsk_height;
HWND desktop;
BYTE *lpPixel;
BITMAPINFO bmpInfo;
Mat screen_mat;
Mat death_tmp_img, map_tmp_img;

/*
一人のプレイヤーの生存秒数、(デス状態なら)デス秒数、デス回数を管理するクラス。
このクラスは味方、敵合わせたプレイヤー8人分の8コの配列変数で保持する。
*/
class PLAYER_DATA {
public:
	Mat img;
	int lr;
	int x, w;
	int num;//左からの順番、番号
	int stat = 0;//0 = 生存; 1 = デス
	int stat_buf;
	int prev_stat;
	int live_time, dead_time, stat_time;
	int death_cnt = 0;
	int live_time_sum;

	std::chrono::system_clock::time_point  start;
	bool is_count;
	PLAYER_DATA(int a, int n, int st) {
		lr = a;
		num = n;
		live_time = dead_time = stat_time = 0;
	}
	//デス回数、生存秒数を0にリセットする関数。
	void reset() {
		live_time = dead_time = stat_time = 0;
		death_cnt = 0;
		start = chrono::system_clock::now();
		live_time_sum = 0;
	}
	//生存秒数orデスしてる秒数をシステムクロックで返す関数。
	double getStatTime() {
		if (stat == 0) {
			return live_time;
		}
		else {
			return dead_time;
		}
	}
	//毎フレーム呼び出される関数。
	void update(int st) {
		//前フレーム呼び出し時と現在のミリ秒の時間差計測をする。
		//差を出すことでプログラムの処理速度の影響なく正確な時間を出せる。
		double dt = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start).count();
		start = chrono::system_clock::now();
		stat_buf = st;

		if (stat == 0) {
			live_time += dt;
			live_time_sum += dt;
		}
		if (stat == 1) {
			dead_time += dt;
		}
		if (prev_stat != stat_buf) {

			is_count = true;
			stat_time = 0;
			dt = 0;
		}
		if (is_count) {
			stat_time += dt;

			/*
			生存→デス or デス→生存にアイコンが変化して、一定フレーム経過したら効果音を再生する。
			誤認識を検知させないための処理。

			ここではしきい値を60フレームとする
			*/

			if (stat_time > 60) {
				//状態変化！！！！！！
				stat = stat_buf;
				is_count = false;

				if (stat == 0) {
					live_time = stat_time;
				}
				if (stat == 1) {
					death_cnt++;
					dead_time = stat_time;
					HWND hw = FindWindowA(NULL, "HSHS");
					if (hw) {
						//効果音を再生する。効果音を鳴らすアプリケーションへメッセージを送る。
						if (lr == 0) {
							//味方がデスした効果音。
							SendMessage(hw, YARARE_MYMESSAGE, lr, num);
						}
						else {
							//敵がデスした効果音。
							SendMessage(hw, YARI_MYMESSAGE, lr, num);
						}
					}

				}
			}
		}
		prev_stat = stat_buf;
	}
};
//敵、味方の合計8人分のクラスをインスタンス化。
PLAYER_DATA  ally_icon[4] = { PLAYER_DATA(0, 0, 0),PLAYER_DATA(0, 1, 0),PLAYER_DATA(0, 2, 0),PLAYER_DATA(0, 3, 0) };
PLAYER_DATA enemy_icon[4] = { PLAYER_DATA(1, 0, 0),PLAYER_DATA(1, 1, 0),PLAYER_DATA(1, 2, 0),PLAYER_DATA(1, 3, 0) };

class ENEMY_WAPON_ICON {
public:
	int x;
	int stat;//0 = 生存; 1 = デス

	ENEMY_WAPON_ICON(int x, int stat) {
		this->x = x;
		this->stat = stat;
	}
	bool operator<(const ENEMY_WAPON_ICON& another) const {
		return x < another.x;
	}
};
class ALLY_WAPON_ICON {
public:
	int x;
	int stat;//0 = 生存; 1 = デス

	ALLY_WAPON_ICON(int x, int stat) {
		this->x = x;
		this->stat = stat;
	}
	bool operator<(const  ALLY_WAPON_ICON& another) const {
		return x > another.x;
	}
};

vector<ALLY_WAPON_ICON>  ally_wapons;
vector<ENEMY_WAPON_ICON> enemy_wapons;


//byte配列ピクセル列から座標指定でポインタ返す関数。
BYTE * GetBits(BITMAPINFO *st, BYTE *pix, int x, int y) {
	if (x < 0) {
		return NULL;
	}
	if (x >= st->bmiHeader.biWidth) {
		return NULL;
	}
	if (y < 0) {
		return NULL;
	}
	if (y >= st->bmiHeader.biHeight) {
		return NULL;
	}
	BYTE * lp;
	lp = pix;
	lp += (st->bmiHeader.biHeight - y - 1) * ((3 * st->bmiHeader.biWidth + 3) / 4) * 4;
	lp += 3 * x;
	return lp;
}
/*
	24bitビットマップをOpenCVの画像フォーマットに変換
*/
IplImage* Convert_DIB_to_IPL(BITMAPINFO *bi, BYTE *pix) {
	IplImage* IplTestImage;
	CvSize size;
	size.width = bi->bmiHeader.biWidth;
	size.height = bi->bmiHeader.biHeight;
	int index = 0;
	int d = 0;
	int c = 0;
	int dOrg = 0;
	BYTE *p;

	IplTestImage = cvCreateImage(size, IPL_DEPTH_8U, 3);
	int WidthStep = IplTestImage->widthStep;
	for (int j = 0; j < (size.height); j++) {
		for (int i = 0; i < (size.width); i++) {
			if ((p = GetBits(bi, pix, i, j))) {
				IplTestImage->imageData[d + 0] = *(p + 0);
				IplTestImage->imageData[d + 1] = *(p + 1);
				IplTestImage->imageData[d + 2] = *(p + 2);
			}
			d += 3;
			c += 3;
		}
		dOrg += WidthStep;
		while (d<dOrg) {
			IplTestImage->imageData[d] = 0;
			d++;
		}
		d = dOrg;
	}
	return IplTestImage;
}
void CallBackFunc(int event, int x, int y, int flags, void* param) {
	cv::Mat* image = static_cast<cv::Mat*>(param);
	switch (event) {
	case cv::EVENT_RBUTTONUP:
		ally_icon[0].reset();
		ally_icon[1].reset();
		ally_icon[2].reset();
		ally_icon[3].reset();
		enemy_icon[0].reset();
		enemy_icon[1].reset();
		enemy_icon[2].reset();
		enemy_icon[3].reset();
		break;
	}
}
//マップモードかどうか判定＆処理
int splat_map() {
	Mat upper_left_mat = Mat(screen_mat, cv::Rect(51, 0, 111, 180));
	Mat mask_image, output_image;
	//左上のXマークのしろのみに限定してマスク。
	Scalar s_min = Scalar(220, 220, 220);
	Scalar s_max = Scalar(255, 255, 255);
	inRange(upper_left_mat, s_min, s_max, mask_image);
	upper_left_mat.copyTo(output_image, mask_image);

	int ret;
	//Xマークテンプレートマッチング
	//int ret = MatMatching_base(output_image, map_tmp_img, 0.6);
	// テンプレートマッチング
	cv::Mat result_img;
	cv::matchTemplate(output_image, map_tmp_img, result_img, CV_TM_CCOEFF_NORMED);
	cv::Point max_pt;
	double maxVal;
	cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
	// 一定スコア以下の場合は処理終了
	if (maxVal < 0.6) {
		ret = -1;
	} else {
		ret = 0;
	}
	//テンプレマッチ結果からマップモードかどうか判定
	if (ret != 0)return 1;
	return 0;
}
//黒ボーダーライン領域のラベリング
int border_labeling(Mat src_img, Mat mask_img) {
	class LABELINGED_IMG {
	public:
		/*
		切り抜かれた画像mat
		ラベリング領域
		重心
		面積
		*/
		Mat mat;
		cv::Rect rc;
		Point centroids;
		int area;

		void cut_img(Mat src_img, int x, int y) {
			x -= rc.x;
			y -= rc.y;
			Vec3b *pv;
			pv = mat.ptr<Vec3b>(y);
			pv[x] = cv::Vec3b(255, 0, 0);
		}
	};
	LABELINGED_IMG border_labeling[500];
	// グレースケールで画像読み込み
	Mat src = mask_img;
	//ラべリング処理
	cv::Mat LabelImg;
	cv::Mat stats;
	cv::Mat centroids;
	int nLab = cv::connectedComponentsWithStats(src, LabelImg, stats, centroids);
	// ラベリング結果の描画色を決定
	std::vector<cv::Vec3b> colors(nLab);
	colors[0] = cv::Vec3b(0, 0, 0);
	for (int i = 1; i < nLab; ++i) {
		colors[i] = cv::Vec3b((rand() & 255), (rand() & 255), (rand() & 255));
	}
	cv::Mat Dst(src.size(), CV_8UC3);
	Dst = Scalar(0, 0, 0);
	int val[500] = { 0 };
	int cnt = 0;
	//ROIの設定
	for (int i = 1; i < nLab; ++i) {
		int *param = stats.ptr<int>(i);
		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
		int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];
		border_labeling[i - 1].mat = Mat(cv::Size(width, height), CV_8UC3);
		border_labeling[i - 1].mat = Scalar(0, 0, 0);
		border_labeling[i - 1].rc = cv::Rect(x, y, width, height);
		//位置、サイズによるフィルタリングを試す。
		//if(y <= 110 && width >= )
		if (width >= 31 && width <= 88 && height >= 14) {
			val[i - 1] = 1;
			if (x < BORDER_CENTER) {
				ally_wapons.push_back(ALLY_WAPON_ICON(x + (width / 2), 0));
			} else {
				enemy_wapons.push_back(ENEMY_WAPON_ICON(x + (width / 2), 0));
			}
			cnt++;
		} else {
			val[i - 1] = 0;
		}
	}
	// ラベリング結果の描画
	for (int i = 0; i < Dst.rows; ++i) {
		int *lb = LabelImg.ptr<int>(i);
		cv::Vec3b *pix = Dst.ptr<cv::Vec3b>(i);
		for (int j = 0; j < Dst.cols; ++j) {
			int idx = lb[j];
			pix[j] = colors[lb[j]];
			if (idx != 0)border_labeling[idx - 1].cut_img(src_img, j, i);
		}
	}
	//ROIの設定
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		int *param = stats.ptr<int>(i);
		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
		int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];
		cv::rectangle(Dst, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);
	}
	//重心の出力
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		double *param = centroids.ptr<double>(i);
		int x = static_cast<int>(param[0]);
		int y = static_cast<int>(param[1]);
		border_labeling[i - 1].centroids = Point(x, y);
		cv::circle(Dst, cv::Point(x, y), 3, cv::Scalar(0, 0, 255), -1);
	}
	//面積値の出力
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		int *param = stats.ptr<int>(i);
		border_labeling[i - 1].area = param[cv::ConnectedComponentsTypes::CC_STAT_AREA];
		//ROIの左上に番号を書き込む
		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
	}
	return 0;
}

//敵、味方のデスしたときの検知のみの簡易処理の関数。
void splat_image_recognition() {
	IplImage *ipl;
	Mat mask_image;

	
	ally_wapons.clear();
	enemy_wapons.clear();
	/*
	マップを開いてる時は、イカアイコンが表示されない。
	キャプチャ画像から。マップを開いてるかどうかをチェックする。
	*/
	if (splat_map() == 0) {
		return;
	}
	Mat output_image, output_image2;
	Mat enemy_icons_mat;
	Mat ally_icons_mat;
	Mat border_mat;
	
	//敵のイカマーク領域を切り取る
	enemy_icons_mat = screen_mat(cv::Rect(1034, 23, 392, 91));
	//味方のイカマーク領域を切り取る
	ally_icons_mat = screen_mat(cv::Rect(493, 23, 392, 91));
	//ボーダーラインの領域を切り取る
	border_mat = screen_mat(cv::Rect(501, 80, 914, 18));
	
	//マスク処理。罰点印(=デスを表す)のグレー色のみを抽出し、それ以外のピクセルを黒に(マスク)する。
	//RGBの最小、最大を設定しその範囲のピクセル色のみを拾う。
	Scalar s_min = Scalar(55, 55, 55);
	Scalar s_max = Scalar(99, 99, 99);
	inRange( enemy_icons_mat, s_min, s_max, mask_image);
	enemy_icons_mat.copyTo(output_image, mask_image);
	//テンプレートマッチングの精度をあげるためにマスク後の画像のサイズを2倍にリサイズ。
	resize(output_image, output_image, cv::Size(), 2, 2);
	//★味方のイカマーク領域からも罰点印(=デスを表す)のグレー色のみを抽出する。
	inRange( ally_icons_mat, s_min, s_max, mask_image);
	ally_icons_mat.copyTo(output_image2, mask_image);
	//テンプレートマッチングの精度をあげるためにマスク後の画像のサイズを2倍にリサイズ。
	resize(output_image2, output_image2, cv::Size(), 2, 2);

	/*
	OpenCVのテンプレートマッチングで敵4人のアイコンが罰点印(=デスした)が有るか探索
	*/
	cv::Mat result_img;
	int enemy_dead_num = 0;
	int ally_dead_num = 0;
	double batu_rate = 0.65;
	for (int i = 0; i < 4; i++) {
		// テンプレートマッチング
		cv::matchTemplate(output_image, death_tmp_img, result_img, CV_TM_CCOEFF_NORMED);
		// 最大のスコアの場所を探す
		cv::Rect roi_rect(0, 0, death_tmp_img.cols, death_tmp_img.rows);
		cv::Point max_pt;
		double maxVal;
		cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
		// 一定スコア以下の場合は処理終了
		if (maxVal < batu_rate) {
			break;
		} else {
			enemy_dead_num++;
		}
		roi_rect.x = max_pt.x;
		roi_rect.y = max_pt.y;
		
		double wr = (0.5);
		double hr = (0.5);
		double xx = (double)roi_rect.x * wr;
		double yy = (double)roi_rect.y * hr;
		double ww = (double)roi_rect.width * wr;
		double hh = (double)roi_rect.height * hr;

		xx += 533;
		yy -= 57;
		hh += 30;
		xx -= 10;
		ww += 13;
		cv::Rect roi_rect2 = cv::Rect((int)xx, (int)yy, (int)ww, (int)hh);
		cv::rectangle(border_mat, roi_rect2, cv::Scalar(0, 0, 0), CV_FILLED);
		// 探索結果の場所に矩形を描画
		cv::rectangle(output_image, roi_rect, cv::Scalar(0, 255, 255), 3);
		//敵アイコンなので右側へ
		enemy_wapons.push_back(ENEMY_WAPON_ICON(xx + (ww / 2), 1));
	}
	//味方プレイヤー4人のアイコンの中にデスマークが存在するかマッチングさせて探索
	for (int i = 0; i < 4; i++) {
		// テンプレートマッチング
		cv::matchTemplate(output_image2, death_tmp_img, result_img, CV_TM_CCOEFF_NORMED);
		// 最大のスコアの場所を探す
		cv::Rect roi_rect(0, 0, death_tmp_img.cols, death_tmp_img.rows);
		cv::Point max_pt;
		double maxVal;
		cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
		// 一定スコア以下の場合は処理終了
		if (maxVal < batu_rate) {
			break;
		} else {
			ally_dead_num++;
		}
		roi_rect.x = max_pt.x;
		roi_rect.y = max_pt.y;
		double wr = (0.5);
		double hr = (0.5);
		double xx = (double)roi_rect.x * wr;
		double yy = (double)roi_rect.y * hr;
		double ww = (double)roi_rect.width * wr;
		double hh = (double)roi_rect.height * hr;
		xx -= 8;
		yy -= 57;
		hh += 30;
		xx -= 10;
		ww += 13;

		cv::Rect roi_rect2 = cv::Rect((int)xx, (int)yy, (int)ww, (int)hh);
		cv::rectangle(border_mat, roi_rect2, cv::Scalar(0, 0, 0), CV_FILLED);
		// 探索結果の場所に矩形を描画
		cv::rectangle(output_image2, roi_rect, cv::Scalar(255, 255, 0), 3);
		//味方アイコンなので左側へ
		ally_wapons.push_back(ALLY_WAPON_ICON(xx + (ww / 2), 1));
	}

	inRange(border_mat, Scalar(0, 0, 0), Scalar(51, 51, 51), mask_image);
	Mat m1, m2, mask_image2, mask_image3;
	bitwise_not(mask_image, mask_image);
	//アイコンのバツのグレー色もマスクする
	s_min = Scalar(60, 60, 60);
	s_max = Scalar(95, 95, 95);
	inRange(border_mat, s_min, s_max, mask_image2);
	dilate(mask_image2, mask_image2, cv::Mat());
	bitwise_xor(mask_image2, mask_image, m1);

	Mat output_image3;
	border_mat.copyTo(output_image3, m1);


	border_labeling(border_mat, m1);


	//ソート開始。アイコンの完全に状況把握
	sort(ally_wapons.begin(), ally_wapons.end());
	sort(enemy_wapons.begin(), enemy_wapons.end());

	//アイコンの情報が8人分取得できていない場合は関数終了。
	if (ally_wapons.size() + enemy_wapons.size() < 8) {
		return ;
	}
	ally_icon[0].img = screen_mat(cv::Rect(501 + ally_wapons[3].x - 63 / 2, 45, 63, 53));
	ally_icon[1].img = screen_mat(cv::Rect(501 + ally_wapons[2].x - 63 / 2, 45, 63, 53));
	ally_icon[2].img = screen_mat(cv::Rect(501 + ally_wapons[1].x - 63 / 2, 45, 63, 53));
	ally_icon[3].img = screen_mat(cv::Rect(501 + ally_wapons[0].x - 63 / 2, 45, 63, 53));
	enemy_icon[0].img = screen_mat(cv::Rect(501 + enemy_wapons[0].x - 63 / 2, 45, 63, 53));
	enemy_icon[1].img = screen_mat(cv::Rect(501 + enemy_wapons[1].x - 63 / 2, 45, 63, 53));
	enemy_icon[2].img = screen_mat(cv::Rect(501 + enemy_wapons[2].x - 63 / 2, 45, 63, 53));
	enemy_icon[3].img = screen_mat(cv::Rect(501 + enemy_wapons[3].x - 63 / 2, 45, 63, 53));
	ally_icon[0].update(ally_wapons[3].stat);
	ally_icon[1].update(ally_wapons[2].stat);
	ally_icon[2].update(ally_wapons[1].stat);
	ally_icon[3].update(ally_wapons[0].stat);
	enemy_icon[0].update(enemy_wapons[0].stat);
	enemy_icon[1].update(enemy_wapons[1].stat);
	enemy_icon[2].update(enemy_wapons[2].stat);
	enemy_icon[3].update(enemy_wapons[3].stat);

	//ゲームの戦況をモニタに出力するための描画処理をしていく。
	Mat window_mat = Mat::zeros(cv::Size(1280, 768), CV_8UC3);
	window_mat = Scalar(255, 255, 255);
	for (int i = 0; i < 4; i++) {
		int dx = i * 320;
		std::stringstream num;
		double du = enemy_icon[i].getStatTime() / 1000.0;
		char s[555];
		double time_rate = enemy_icon[i].getStatTime() / 35000 * 768;
		if (enemy_icon[i].stat == 1) {
			cv::rectangle(window_mat, cv::Rect(dx, window_mat.rows - time_rate, 320, time_rate), cv::Scalar(35, 35, 35), CV_FILLED);
		} else {
			cv::rectangle(window_mat, cv::Rect(dx, window_mat.rows - time_rate, 320, time_rate), cv::Scalar(180, 204, 86), CV_FILLED);
		}
		sprintf_s(s, "   %.1f", du);
		cv::putText(window_mat, s, cv::Point(dx, 768 - 70), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 0), 3);
		sprintf_s(s, " %d", enemy_icon[i].death_cnt);
		cv::putText(window_mat, s, cv::Point(dx, 768 - 370), cv::FONT_HERSHEY_SIMPLEX, 5, cv::Scalar(0, 0, 0), 7);
		//sprintf_s(s, "%.2f", (enemy_icon[i].live_time_sum / 1000.0));
	}
	cv::line(window_mat, Point(0, 0), Point(0, 768), Scalar(0, 00, 0), 5, 8);
	cv::line(window_mat, Point(320, 0), Point(320, 768), Scalar(0, 0, 0), 5, 8);
	cv::line(window_mat, Point(640, 0), Point(640, 768), Scalar(0, 00, 0), 5, 8);
	cv::line(window_mat, Point(960, 0), Point(960, 768), Scalar(0, 00, 0), 5, 8);
	//敵4人のブキアイコンを拡大し、ウィンドウ表示用のMatに転写する
	int tx = 160 - 107;
	int ty = 768 - 300;
	for (int i = 0; i < 4; i++) {
		resize(enemy_icon[i].img, enemy_icon[i].img, cv::Size(), 3.4, 3.4);
		cv::Mat mat = (cv::Mat_<double>(2, 3) << 1.0, 0.0, tx, 0.0, 1.0, ty);
		cv::warpAffine(enemy_icon[i].img, window_mat, mat, window_mat.size(), CV_INTER_LINEAR, cv::BORDER_TRANSPARENT);
		tx += 320;
	}
	//戦況の可視化ウィンドウを表示
	cv::imshow("戦況", window_mat);
	//ウィンドウの位置を指定、ここでは2台目のモニタに表示されるよう数値を指定
	cv::moveWindow("戦況", -1280, 170);
	//ウィンドウのクリックイベントのコールバック関数を設定
	cv::setMouseCallback("戦況", CallBackFunc, (void *)&window_mat);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	static HDC hdc;
	static HBITMAP hBitmap;
	static HDC hMemDC;
	static PAINTSTRUCT ps;
	static IplImage *ipl;
	static int cnt = 0;

	switch (msg) {
	case WM_TIMER: {
		///16ms毎に呼ばれる
		
		//スクリーンキャプチャで、OBSから出力されたゲーム画面の画像を取得する。
		hdc = GetDC(desktop);
		BitBlt(hMemDC, 0, 0, dsk_width, dsk_height, hdc, 0, 0, SRCCOPY);
		ReleaseDC(desktop, hdc);
		//スクリーンキャプチャ画像(DIB)をOpenCVで扱えるようMat型に変換しておく。
		ipl = Convert_DIB_to_IPL(&bmpInfo, lpPixel);
		screen_mat = cvarrToMat(ipl);
		//OpenCVでの画像認識処理の開始。
		splat_image_recognition();
		cvReleaseImage(&ipl);
		return 0;
	}
	case WM_DESTROY: {
		DeleteDC(hMemDC);
		DeleteObject(hBitmap);
		PostQuitMessage(0);
		return 0;
	}
	case WM_CREATE: {
		SetTimer(hwnd, 2525, 1, NULL);
		RECT rc;
		desktop = GetDesktopWindow();
		GetWindowRect(desktop, &rc);
		dsk_width = rc.right;
		dsk_height = rc.bottom;

		//テンプレートマッチング用の画像を読み込む。
		death_tmp_img = cv::imread(DEATH_TEMPLATE_IMAGE, 1);
		if(!death_tmp_img.data){
			dprintf(L"テンプレートマッチング画像の読み込みに失敗");
			SendMessage(hwnd, WM_CLOSE, 0, 0);
		}
		map_tmp_img = cv::imread(MAP_TEMPLATE_IMAGE, 1);
		if (!map_tmp_img.data)   dprintf(L"テンプレートマッチング画像の読み込みに失敗");

		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		//DIBの情報を設定する
		bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth = dsk_width;
		bmpInfo.bmiHeader.biHeight = dsk_height;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 24;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		//DIBSection作成
		hdc = GetDC(hwnd);
		hBitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, (void**)&lpPixel, NULL, 0);
		hMemDC = CreateCompatibleDC(hdc);
		SelectObject(hMemDC, hBitmap);
		ReleaseDC(hwnd, hdc);

		//効果音再生用のアプリケーションが、既に起動されていなければ起動する。
		HWND hw = FindWindowA(NULL, SE_PLAYER_CAPTION);
		if (!hw) {
			ShellExecute(NULL, L"open", _T(SE_PLAYER_EXE), NULL, NULL, SW_SHOWNORMAL);
		}
		//ウィンドウ表示
		Mat mat(Size(1280, 768), CV_8U, Scalar::all(0));
		cv::imshow("戦況", mat);
		//カウントを初期化しておく。
		ally_icon[0].reset();
		ally_icon[1].reset();
		ally_icon[2].reset();
		ally_icon[3].reset();
		enemy_icon[0].reset();
		enemy_icon[1].reset();
		enemy_icon[2].reset();
		enemy_icon[3].reset();
		return 0;
	}
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;                                 // 定義用の構造体
	LPTSTR pClassName = TEXT("MWIN");      // クラス名
	hInstance = hInst;

	wc.cbSize = sizeof(WNDCLASSEX);         // 構造体サイズ
	wc.style = CS_HREDRAW | CS_VREDRAW;    // クラススタイル
	wc.lpfnWndProc = (WNDPROC)WndProc;       // プロシージャ
	wc.cbClsExtra = 0;                          // 補足メモリブロック
	wc.cbWndExtra = 0;                          //   のサイズ
	wc.hInstance = hInst;                      // インスタンス
	wc.hIcon = NULL;   // アイコン
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); // カーソル
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);   // 背景色
	wc.lpszMenuName = NULL;                   // メニュー
	wc.lpszClassName = pClassName;                 // クラス名
	wc.hIconSm = NULL;    // 小さいアイコン
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, TEXT("RegisterClass Error."), TEXT(""), MB_OK | MB_ICONWARNING);
		return -1;
	}
	hwnd = CreateWindow(pClassName, TEXT("メインウィンドウ"),
		WS_OVERLAPPEDWINDOW,
		0, 300,
		640, 480,
		NULL, NULL, hInst, NULL);

	if (!hwnd) {
		MessageBox(NULL, TEXT("CreateWindow Error."), TEXT(""), MB_OK | MB_ICONWARNING);
		return -1;
	}
	ShowWindow(hwnd, SW_SHOW);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
