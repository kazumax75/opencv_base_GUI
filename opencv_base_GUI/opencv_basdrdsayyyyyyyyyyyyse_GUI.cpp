/*
	OpenCVの関数からキャプチャボードのイメージを取得するのではなく、
	キャプチャボードで取得したゲーム画面を、OBS Studioでモニタにフルスクリーンで表示させ、
	それをスクリーンキャプチャで取得し、認識させる　という方法を採用しています。
*/
#include "stdafx.h"
#include "opencv_base_GUI.h"

#include <Shellapi.h>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace cv;

//デバッグ文字列表示用マクロ
#define dprintf( str, ... ) \
	{ \
		TCHAR c[2560]; \
		_stprintf_s( c, str, __VA_ARGS__ ); \
		OutputDebugString( c ); \
	}
HWND hwnd;
HINSTANCE hInstance;

/*
	WINAPIでは、効果音の複数同時再生ができないようなので、
	効果音はHSPで作成した外部のプログラムに再生させることにする。
	効果音を再生したい時に、以下のウィンドウメッセージをその外部プログラムに送り再生させる。
*/
#define YARI_MYMESSAGE (WM_APP + 1)
#define YARARE_MYMESSAGE (WM_APP + 2)


BITMAPFILEHEADER srcBit_H;
BITMAPINFO srcBit_Info;
BYTE *pix;

int dsk_width, dsk_height;
HWND desktop;

BITMAPINFO  trim_bi;
BYTE *trim_pix;
BYTE *lpPixel;
BITMAPINFO bmpInfo;

#define BORDER_CENTER 456

Mat screen_mat;
cv::Mat tmp_img, BATSU_TMP_IMG;

int teki_dead_num_prev;
int mikata_dead_num_prev;

/*
	一人のプレイヤーの生存秒数、(デス状態なら)デス秒数、デス回数を管理するクラス。
	このクラスは味方、敵合わせたプレイヤー8人分の8コの配列変数で保持する。
*/
class WAPON_ICON {
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
	WAPON_ICON(int a, int n, int st) {
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
		} else {
			return dead_time;
		}
	}
	//毎フレーム呼び出される関数。
	void update(int st){
		//前フレーム呼び出し時と現在のミリ秒の時間差計測をする。
		//差を出すことでプログラムの処理速度の影響なく正確な時間を出せる。
		double dt = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now()- start).count();
		start = chrono::system_clock::now();
		stat_buf = st;
		
		if(stat == 0){
			live_time += dt;
			live_time_sum += dt;
		}
		if(stat == 1){
			dead_time += dt;
		}
		if(prev_stat != stat_buf){
			
			is_count = true;
			stat_time = 0;
			dt = 0;
		}
		if(is_count){
			stat_time += dt;
			
			/*
				生存→デス or デス→生存にアイコンが変化して、一定フレーム経過したら効果音を再生する。
				誤認識を検知させないための処理。
			
				ここではしきい値を60フレームとする
			*/
				
			if(stat_time > 60){
				//状態変化！！！！！！
				stat = stat_buf;
				is_count = false;
				
				if(stat == 0){
					live_time = stat_time;
				}
				if(stat == 1){
					death_cnt++;
					dead_time = stat_time;
					HWND hw = FindWindowA(NULL, "HSHS");
					if (hw) {
						//効果音を再生する。効果音を鳴らすアプリケーションへメッセージを送る。
						if(lr == 0){
							//味方がデスした効果音。
							SendMessage(hw, YARARE_MYMESSAGE, lr, num);
						} else {
							//敵がデスした効果音。
							SendMessage(hw, YARI_MYMESSAGE,   lr, num);
						}
					}
					
				}
			}
		}
		prev_stat = stat_buf;
	}
};

//敵、味方の合計8人分のクラスをインスタンス化。
WAPON_ICON mikata_icon[4] = { WAPON_ICON(0, 0, 0),WAPON_ICON(0, 1, 0),WAPON_ICON(0, 2, 0),WAPON_ICON(0, 3, 0) };
WAPON_ICON teki_icon[4]   = { WAPON_ICON(1, 0, 0),WAPON_ICON(1, 1, 0),WAPON_ICON(1, 2, 0),WAPON_ICON(1, 3, 0) };

class RIGHT_WAPON_ICON {
public:
	int x;
	int stat;//0 = 生存; 1 = デス

	RIGHT_WAPON_ICON(int x, int stat) {
		this->x = x;
		this->stat = stat;
	}

	bool operator<(const RIGHT_WAPON_ICON& another) const {
		return x < another.x;
	}
};
class LEFT_WAPON_ICON {
public:
	int x;
	int stat;//0 = 生存; 1 = デス

	LEFT_WAPON_ICON(int x, int stat) {
		this->x = x;
		this->stat = stat;
	}

	bool operator<(const  LEFT_WAPON_ICON& another) const {
		return x > another.x;
	}
};
vector<LEFT_WAPON_ICON>   left_labeling_center;
vector<RIGHT_WAPON_ICON> right_labeling_center;


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

int MatMatching_base(Mat output_image ,Mat  tmp_img, double rate){
	Mat search_img0 = output_image.clone();
	Mat search_img;
	search_img0.copyTo( search_img );
	Mat result_img;
	
	// テンプレートマッチング
	cv::matchTemplate(search_img, tmp_img, result_img, CV_TM_CCOEFF_NORMED);
	cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
	cv::Point max_pt;
	double maxVal;
	cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
	// 一定スコア以下の場合は処理終了
	if (maxVal < rate) {
		return -1;
	} else {
		roi_rect.x = max_pt.x;
		roi_rect.y = max_pt.y;
		cv::rectangle(search_img0, roi_rect, cv::Scalar(0,255,255), 3);
		cv::rectangle(search_img, roi_rect, cv::Scalar(0,0,255), CV_FILLED);
	}
	return 0;
}

void CallBackFunc(int event, int x, int y, int flags, void* param) {
	cv::Mat* image = static_cast<cv::Mat*>(param);
	switch (event) {
		//ウィンドウ内で右クリックで、敵のデス数、秒数カウントを0にリセット
		case cv::EVENT_RBUTTONUP:
			mikata_icon[0].reset();
			mikata_icon[1].reset();
			mikata_icon[2].reset();
			mikata_icon[3].reset();
			teki_icon[0].reset();
			teki_icon[1].reset();
			teki_icon[2].reset();
			teki_icon[3].reset();
			break;
	}

}
//マップモードかどうか判定＆処理
int splat_map(){
	Mat upper_left_mat = Mat(screen_mat, cv::Rect(51, 0, 111, 180));
	Mat mask_image, output_image;
	//左上のXマークのしろのみに限定してマスク。
	Scalar s_min = Scalar(220, 220, 220);
	Scalar s_max = Scalar(255, 255, 255);
	inRange(upper_left_mat, s_min, s_max, mask_image);
	upper_left_mat.copyTo(output_image, mask_image);

	//Xマークテンプレートマッチング
	int ret = MatMatching_base(output_image, BATSU_TMP_IMG, 0.6);
	//テンプレマッチ結果からマップモードかどうか判定
	if(ret != 0)return 1;
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
	Dst =  Scalar(0, 0, 0);
	int val[500] = {0};
	int cnt = 0;
	//ROIの設定
	for (int i = 1; i < nLab; ++i) {
		int *param = stats.ptr<int>(i);

		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
		int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];

		border_labeling[i - 1].mat = Mat(cv::Size(width, height),  CV_8UC3);
		border_labeling[i - 1].mat =  Scalar(0, 0, 0);
		border_labeling[i - 1].rc = cv::Rect(x, y, width, height);

		
		//位置、サイズによるフィルタリングを試す。
		if (width >= 31 && width <= 88 && height >= 14) {
			val[i - 1] = 1;
			if(x < BORDER_CENTER){
				left_labeling_center.push_back( LEFT_WAPON_ICON(x + (width/2), 0) );
			} else {
				right_labeling_center.push_back( RIGHT_WAPON_ICON(x + (width/2), 0) );
			}
			cnt++;
		}else {
			val[i - 1] =0;
		}
	}
	// ラベリング結果の描画
	for (int i = 0; i < Dst.rows; ++i) {
		int *lb = LabelImg.ptr<int>(i);
		cv::Vec3b *pix = Dst.ptr<cv::Vec3b>(i);
		for (int j = 0; j < Dst.cols; ++j) {
			int idx = lb[j];
			//if (val[idx] == 0)continue;
			pix[j] = colors[lb[j]];
			
			//val[idx] == 1 && 
			if(idx != 0)border_labeling[idx-1].cut_img(src_img, j, i);
		}
	}
	//ROIの設定
	for (int i = 1; i < nLab; ++i) {
		if (val[i-1] == 0)continue;
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
		std::stringstream num;
		num << i;
		cv::putText(Dst, num.str(), cv::Point(x + 5, y + 20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
	}
	return 0;
}
/*
	この関数で
	ピクセルデータから
	イカマークのアイコンの画像認識をし
	それぞれのプレイヤーの生存、デス状況を解析する。
*/
void Splat_Image_Recognition(){
	IplImage *ipl;
	Mat mask_image;
	
	right_labeling_center.clear();
	left_labeling_center.clear();
	
	/*
		マップを開いてる時は、イカアイコンが表示されない。
		キャプチャ画像から。マップを開いてるかどうかをチェックする。
		マップ表示中は画像認識処理を中断することにする。リターンする。
	*/
	if (splat_map() == 0) {
		SetWindowText(hwnd, L"マップモードです！！！ブキアイコン調べないよ！");
		return;
	}
	
	Mat output_image, output_image2;
	Mat teki_icons_mat;
	Mat mikata_icons_mat;
	Mat border_mat;
	
	//敵のイカマーク領域を切り取る
	teki_icons_mat = screen_mat(cv::Rect(1034, 23, 392, 91));
	//味方のイカマーク領域を切り取る
	mikata_icons_mat = screen_mat(cv::Rect(493, 23, 392, 91));
	//ボーダーラインの領域を切り取る
	border_mat = screen_mat(cv::Rect(501, 80, 914, 18));
	
	//マスク処理。罰点印(=デスを表す)のグレー色のみを抽出し、それ以外のピクセルを黒に(マスク)する。
	//RGBの最小、最大を設定しその範囲のピクセル色のみを拾う。
	Scalar s_min = Scalar(55, 55, 55);
	Scalar s_max = Scalar(99, 99, 99);
	inRange( teki_icons_mat, s_min, s_max, mask_image);
	
	//
	teki_icons_mat.copyTo(output_image, mask_image);
	
	//テンプレートマッチングの精度をあげるためにマスク後の画像のサイズを2倍にリサイズ。
	resize(output_image, output_image, cv::Size(), 2, 2);
	
	//★味方のイカマーク領域からも罰点印(=デスを表す)のグレー色のみを抽出する。
	inRange( mikata_icons_mat, s_min, s_max, mask_image);
	mikata_icons_mat.copyTo(output_image2, mask_image);
	//テンプレートマッチングの精度をあげるためにマスク後の画像のサイズを2倍にリサイズ。
	resize(output_image2, output_image2, cv::Size(), 2, 2);
	

	cv::Mat result_img;

	int teki_dead_num = 0;
	int mikata_dead_num = 0;
	double batu_rate = 0.65;
	
	//debug
	//imshow("バツテンプレ", tmp_img);
	//imshow("灰消し味方", output_image2);
	//imshow("灰消しの敵", output_image);
	

	
	/*
		OpenCVのテンプレートマッチングで敵プレイヤー4人のアイコンの中に罰点印(=デスした)が有るか探索
		敵プレイヤー4人のアイコンの中にデスマークが存在するかマッチングさせて探索
	*/
	for (int i = 0; i < 4; i++) {
		// テンプレートマッチング
		cv::matchTemplate(output_image, tmp_img, result_img, CV_TM_CCOEFF_NORMED);
		// 最大のスコアの場所を探す
		cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
		cv::Point max_pt;
		double maxVal;
		cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
		// マッチング結果が一定スコア以下の場合は処理終了
		if (maxVal < batu_rate) {
			break;
		} else {
			teki_dead_num++;
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
		
		//4回マッチングを繰り返すので一度マッチした場所は塗りつぶしておく
		cv::rectangle(output_image, roi_rect, cv::Scalar(0,255,255), 3);
		//ボーダーライン部分にもマッチしたところは塗りつぶす
		cv::rectangle(border_mat, roi_rect2, cv::Scalar(0, 0, 0), CV_FILLED);
		
		right_labeling_center.push_back( RIGHT_WAPON_ICON(xx + (ww / 2), 1) );
	}
	
	
	//敵プレイヤー4人のアイコンの中にデスマークが存在するかマッチングさせて探索
	for (int i = 0; i < 4; i++) {
		// テンプレートマッチング
		cv::matchTemplate(output_image2, tmp_img, result_img, CV_TM_CCOEFF_NORMED);
		// 最大のスコアの場所を探す
		cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
		cv::Point max_pt;
		double maxVal;
		cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
		// 一定スコア以下の場合は処理終了
		if (maxVal < batu_rate) {
			break;
		} else {
				mikata_dead_num++;
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
		
		
		//4回マッチングを繰り返すので一度マッチした場所は塗りつぶしておく
		cv::rectangle(output_image2, roi_rect, cv::Scalar(255,255,0), 3);
		//ボーダーライン部分にもマッチしたところは塗りつぶす
		cv::Rect roi_rect2 = cv::Rect((int)xx, (int)yy, (int)ww, (int)hh);
		cv::rectangle(border_mat, roi_rect2, cv::Scalar(0, 0, 0), CV_FILLED);
		
		left_labeling_center.push_back( LEFT_WAPON_ICON(xx + (ww / 2), 1) );
	}
	
	
	Mat output_image3;
	Mat m1, m2,mask_image2, mask_image3;
	
	
	//ボーダーラインの黒色と、アイコンのバツのグレー色をマスクする
	inRange(border_mat, Scalar(0, 0, 0), Scalar(51, 51, 51),    mask_image);
	inRange(border_mat, Scalar(60, 60, 60), Scalar(95, 95, 95), mask_image2);
	
	bitwise_not(mask_image, mask_image);
	dilate(mask_image2, mask_image2, cv::Mat());
	bitwise_xor(mask_image2, mask_image, m1);
	
	
	border_mat.copyTo(output_image3, m1);
	
	//ボーダーライン領域にラベリング処理
	border_labeling(border_mat, m1);
	
	//ソート開始。アイコンの完全に状況把握
	sort(left_labeling_center.begin(), left_labeling_center.end());
	sort(right_labeling_center.begin(), right_labeling_center.end());
	
	
	if(left_labeling_center.size() + right_labeling_center.size() < 8){
		return ;
	}
	//ゲージ上に表示するためのイカアイコンを切り取ってMat型に格納
	mikata_icon[0].img = screen_mat(cv::Rect(501 + left_labeling_center[3].x - 63/2, 45,  63, 53));
	mikata_icon[1].img = screen_mat(cv::Rect(501 + left_labeling_center[2].x - 63/2, 45,  63, 53));
	mikata_icon[2].img = screen_mat(cv::Rect(501 + left_labeling_center[1].x - 63/2, 45,  63, 53));
	mikata_icon[3].img = screen_mat(cv::Rect(501 + left_labeling_center[0].x - 63/2, 45,  63, 53));
	teki_icon[0].img =   screen_mat(cv::Rect(501 + right_labeling_center[0].x - 63/2, 45,  63, 53));
	teki_icon[1].img =   screen_mat(cv::Rect(501 + right_labeling_center[1].x - 63/2, 45,  63, 53));
	teki_icon[2].img =   screen_mat(cv::Rect(501 + right_labeling_center[2].x - 63/2, 45,  63, 53));
	teki_icon[3].img =   screen_mat(cv::Rect(501 + right_labeling_center[3].x - 63/2, 45,  63, 53));
	
	//生存・デス状態の変化をアップデート
	mikata_icon[0].update(left_labeling_center[3].stat);
	mikata_icon[1].update(left_labeling_center[2].stat);
	mikata_icon[2].update(left_labeling_center[1].stat);
	mikata_icon[3].update(left_labeling_center[0].stat);
	teki_icon[0].update(right_labeling_center[0].stat);
	teki_icon[1].update(right_labeling_center[1].stat);
	teki_icon[2].update(right_labeling_center[2].stat);
	teki_icon[3].update(right_labeling_center[3].stat);
	
	
	//todo 
	teki_dead_num = teki_icon[0].stat +  teki_icon[1].stat +  teki_icon[2].stat +  teki_icon[3].stat;
	
	Mat matt = Mat::zeros(cv::Size(1280, 768), CV_8UC3);
	matt = Scalar(255, 255, 255);
		
		
	//ゲームの戦況をモニタに出力するための描画処理をしていく。
	for(int i = 0; i < 4; i++){
		int dx = i*320;
		
		std::stringstream num;
		double du = teki_icon[i].getStatTime() / 1000.0;
		char s[555];
		
		double time_rate = teki_icon[i].getStatTime() / 35000 * 768;
		
		if (teki_icon[i].stat == 1) {
			cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(35, 35, 35), CV_FILLED);
		} else {
			cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(180, 204, 86), CV_FILLED);
		}
		sprintf_s(s, "   %.1f", du);
		cv::putText(matt, s, cv::Point(dx, 768 -70), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 0), 3);
		
		sprintf_s(s, " %d", teki_icon[i].death_cnt);
		cv::putText(matt, s, cv::Point(dx, 768 -370), cv::FONT_HERSHEY_SIMPLEX, 5, cv::Scalar(0, 0, 0), 7);
		
		sprintf_s(s, "%.2f",( teki_icon[i].live_time_sum/ 1000.0));
		//cv::putText(matt, s, cv::Point(dx, 768 -330), cv::FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 0, 127), 5);
	}
	cv::line(matt, Point(0, 0), Point(0, 768), Scalar(0, 00, 0), 5, 8);
	cv::line(matt, Point(320, 0), Point(320, 768), Scalar(0, 0, 0), 5, 8);
	cv::line(matt, Point(640, 0), Point(640, 768), Scalar(0, 00, 0), 5, 8);
	cv::line(matt, Point(960, 0), Point(960, 768), Scalar(0, 00, 0), 5, 8);
	
	//敵4人のブキアイコンを拡大し、ウィンドウ表示用のMatに転写する
	int tx = 160 - 107;
	int ty = 768 - 300;
	for(int i = 0; i < 4; i++){
		resize(teki_icon[i].img, teki_icon[i].img, cv::Size(), 3.4, 3.4);
		cv::Mat mat = (cv::Mat_<double>(2,3)<<1.0, 0.0, tx, 0.0, 1.0, ty);
		cv::warpAffine(teki_icon[i].img, matt, mat, matt.size(), CV_INTER_LINEAR, cv::BORDER_TRANSPARENT);
		tx += 320;
	}
	
	//戦況の可視化ウィンドウを表示
	cv::imshow("戦況", matt);
	//ウィンドウの位置を指定、ここでは2台目のモニタに表示されるよう数値を指定
	cv::moveWindow("戦況", -1280, 170);
	//ウィンドウのクリックイベントのコールバック関数を設定
	cv::setMouseCallback("戦況", CallBackFunc, (void *)&matt);
	
	//imshow("味方アイコンX", mikata_icons_mat);
	//imshow("味方アイコンXX", teki_icons_mat);

	teki_dead_num_prev = teki_dead_num;
	mikata_dead_num_prev = mikata_dead_num;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	static HDC hdc;
	static HBITMAP hBitmap;
	static HDC hMemDC;
	static PAINTSTRUCT ps;
	static IplImage *ipl;
	static int frame_cnt = 0;

	switch (msg) {
		case WM_TIMER: {
			///16ms毎に呼ばれる
			
			//スクリーンキャプチャで出力されたゲーム画面の画像を取得する。
			hdc = GetDC(desktop);
			BitBlt(hMemDC, 0, 0, dsk_width, dsk_height, hdc, 0, 0, SRCCOPY);
			ReleaseDC(desktop, hdc);
			
			//スクリーンキャプチャ画像をOpenCVで扱えるようMat型に変換しておく。
			ipl = Convert_DIB_to_IPL(&bmpInfo, lpPixel);
			screen_mat = cvarrToMat(ipl);
cv::imshow("戦況", screen_mat);
			screen_mat
			//OpenCVでの画像認識処理の開始。
			//Splat_Image_Recognition();
			
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
			//60fps ≒ 16msでタイマーセット
			SetTimer(hwnd, 2525, 16, NULL);
			
			
			//テンプレートマッチング用の画像を読み込む。
			tmp_img = cv::imread("E:\\scan5\\splat_temp.bmp", 1);
			if (!tmp_img.data) dprintf(L"よめねええええええええええええええええ");

			BATSU_TMP_IMG = cv::imread("E:\\scan5\\map_batu.bmp", 1);
			if (!BATSU_TMP_IMG.data) dprintf(L"よめねええええええええええええええええ");
			
			
			SetWindowPos(hwnd, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE);
			
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
			//効果音の再生用のアプリケーションを実行させる。
			HWND hw = FindWindowA( NULL, "HSHS");
			if (!hw) {
				ShellExecute(NULL, L"open", L"C:\\Users\\kento\\Documents\\Visual Studio 2015\\Projects\\opencv_base_GUI\\x64\\Release\\hsptmp.exe", NULL, NULL, SW_SHOWNORMAL);
			}
			Mat m1(Size(1280, 768), CV_8U, Scalar::all(0)); 
			cv::imshow("戦況", m1);
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
