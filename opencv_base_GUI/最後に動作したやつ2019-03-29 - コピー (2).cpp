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

//�f�o�b�O������\���p�}�N��
#define dprintf( str, ... ) \
	{ \
		TCHAR c[2560]; \
		_stprintf_s( c, str, __VA_ARGS__ ); \
		OutputDebugString( c ); \
	}
HWND hwnd;
HINSTANCE hInstance;

/*
	WINAPI�ł́A���ʉ��̕��������Đ����ł��Ȃ��悤�Ȃ̂ŁA
	���ʉ���HSP�ō쐬�����O���̃v���O�����ɍĐ������邱�Ƃɂ���B
	���ʉ����Đ����������ɁA�ȉ��̃E�B���h�E���b�Z�[�W�����̊O���v���O������send����B
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
cv::Mat ki_batsu, ao_batsu, siro_batsu;
cv::Mat mika_batsu_mask, teki_batsu_mask;
cv::Mat wapons_tmp[8];
cv::Mat tmp_mikata_batu, tmp_teki_batu;


/*
	��l�̃v���C���[�̐����b���A(�f�X��ԂȂ�)�f�X�b���A�f�X�񐔂��Ǘ�����N���X�B
	���̃N���X�͖����A�G���킹���v���C���[8�l����8�R�̔z��ϐ��ŕێ�����B
*/
class WAPON_ICON {
public:
	Mat img;
	int lr;
	int x, w;
	int num;//������̏��ԁA�ԍ�
	int stat = 0;//0 = ����; 1 = �f�X
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
	//�f�X�񐔁A�����b����0�Ƀ��Z�b�g����֐��B
	void reset() {
		live_time = dead_time = stat_time = 0;
		death_cnt = 0;
		start = chrono::system_clock::now();
		
		live_time_sum = 0;
	}
	//�����b��or�f�X���Ă�b�����V�X�e���N���b�N�ŕԂ��֐��B
	double getStatTime() {
		if (stat == 0) {
			return live_time;
		} else {
			return dead_time;
		}
	}
	//���t���[���Ăяo�����֐��B
	void update(int st){
		//�O�t���[���Ăяo�����ƌ��݂̃~���b�̎��ԍ��v��������B
		//�����o�����ƂŃv���O�����̏������x�̉e���Ȃ����m�Ȏ��Ԃ��o����B
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
				�������f�X or �f�X�������ɃA�C�R�����ω����āA���t���[���o�߂�������ʉ����Đ�����B
				��F�������m�����Ȃ����߂̏����B
			
				�����ł͂������l��60�t���[���Ƃ���
			*/
				
			if(stat_time > 60){
				//��ԕω��I�I�I�I�I�I
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
						//���ʉ����Đ�����B���ʉ���炷�A�v���P�[�V�����փ��b�Z�[�W�𑗂�B
						if(lr == 0){
							//�������f�X�������ʉ��B
							SendMessage(hw, YARARE_MYMESSAGE, lr, num);
						} else {
							//�G���f�X�������ʉ��B
							SendMessage(hw, YARI_MYMESSAGE,   lr, num);
						}
					}
					
				}
			}
		}
		prev_stat = stat_buf;
	}
};

//�G�A�����̍��v8�l���̃N���X���C���X�^���X���B
WAPON_ICON mikata_icon[4] = { WAPON_ICON(0, 0, 0),WAPON_ICON(0, 1, 0),WAPON_ICON(0, 2, 0),WAPON_ICON(0, 3, 0) };
WAPON_ICON teki_icon[4]   = { WAPON_ICON(1, 0, 0),WAPON_ICON(1, 1, 0),WAPON_ICON(1, 2, 0),WAPON_ICON(1, 3, 0) };

class RIGHT_WAPON_ICON {
public:
	int x;
	int stat;//0 = ����; 1 = �f�X

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
	int stat;//0 = ����; 1 = �f�X

	LEFT_WAPON_ICON(int x, int stat) {
		this->x = x;
		this->stat = stat;
	}

	bool operator<(const  LEFT_WAPON_ICON& another) const {
		return x > another.x;
	}
};
vector<LEFT_WAPON_ICON> left_labeling_center;
vector<RIGHT_WAPON_ICON> right_labeling_center;


//byte�z��s�N�Z���񂩂���W�w��Ń|�C���^�Ԃ��֐��B
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
	24bit�r�b�g�}�b�v��OpenCV�̉摜�t�H�[�}�b�g�ɕϊ�
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



int adv;//0-2
BITMAPINFO *Gtrim_bi;
BYTE      *Gtrim_pix;

int MatMatching_base(Mat output_image ,Mat  tmp_img, double rate){
	cv::Mat search_img0 = output_image.clone();
	cv::Mat search_img;
	search_img0.copyTo( search_img );
	cv::Mat result_img;
	// �ő�̃X�R�A�̏ꏊ��T��
	dprintf(L"%d %d aaa\n", tmp_img.cols, tmp_img.rows);
              // �e���v���[�g�}�b�`���O
              cv::matchTemplate(search_img, tmp_img, result_img, CV_TM_CCOEFF_NORMED);
			  //dprintf(L"%d %p Xaaa\n", 26, tmp_img);
              cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
              cv::Point max_pt;
              double maxVal;
              cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
              // ���X�R�A�ȉ��̏ꍇ�͏����I��
			  if (maxVal < rate) {
				  return -1;
			  }else {
				  roi_rect.x = max_pt.x;
	              roi_rect.y = max_pt.y;
	          	//dprintf(L"idx: %d [%d/%d]\n", i, max_pt.x, max_pt.y);
	              //std::cout << "(" << max_pt.x << ", " << max_pt.y << "), score=" << maxVal << std::endl;
	              // �T�����ʂ̏ꏊ�ɋ�`��`��
	              cv::rectangle(search_img0, roi_rect, cv::Scalar(0,255,255), 3);
	              cv::rectangle(search_img, roi_rect, cv::Scalar(0,0,255), CV_FILLED);
			
			  	  //cv::imshow("�}�b�`���O", search_img0 );
			  }
	
	return 0;
}


void CallBackFunc(int event, int x, int y, int flags, void* param) {
	cv::Mat* image = static_cast<cv::Mat*>(param);

	switch (event) {
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
		
//�}�b�v���[�h���ǂ������聕����
int splat_map() {

	Mat upper_left_mat;
	
	try{
				
	upper_left_mat = Mat(screen_mat, cv::Rect(51, 0, 111, 180));

	}catch (cv::Exception& e){
		std::cout << e.what() << std::endl;
		//Sleep(15555);
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	Mat mask_image, output_image;
	//�����X�}�[�N�̂���݂̂Ɍ��肵�ă}�X�N�B
	Scalar s_min = Scalar(220, 220, 220);
	Scalar s_max = Scalar(255, 255, 255);
	inRange(upper_left_mat, s_min, s_max, mask_image);
	upper_left_mat.copyTo(output_image, mask_image);
	//imshow("����", output_image);

	//X�}�[�N�e���v���[�g�}�b�`���O
	int ret = MatMatching_base(output_image, BATSU_TMP_IMG, 0.6);
	
	//�e���v���}�b�`���ʂ���}�b�v���[�h���ǂ�������
	if(ret != 0)return 1;
	
	//�G�o�c�m�F, SP�m�F
	//cv::Rect rc_teki   (1632, 51,  208, 58);
	cv::Rect rc_teki_sp(1620, 61,  17 , 34);
	cv::Rect rc_teki[4];
	
	rc_teki[0] = cv::Rect(1632, 51,      208, 58);
	rc_teki[1] = cv::Rect(1632, 51+66,   208, 58);
	rc_teki[2] = cv::Rect(1632, 51+131,  208, 58);
	rc_teki[3] = cv::Rect(1632, 51+196,  208, 58);

	
	
	
	
	Mat teki_batsu_mat[4];
	Mat teki_sp_mat[4];

	for(int i = 0; i < 4; i++){
		teki_batsu_mat[i] = screen_mat(rc_teki[i]);
		teki_sp_mat[i]    = screen_mat(rc_teki_sp);

		
		teki_batsu_mat[i].copyTo(output_image, teki_batsu_mask);
		//output_image = teki_batsu_mat[i];w
		
		 s_min = Scalar(120, 120, 120);
		 s_max = Scalar(255, 255, 255);
		
		Mat mask_image2;
		inRange(output_image, s_min, s_max, mask_image2);
		
		dilate(mask_image2, mask_image2, cv::Mat());
		bitwise_not(mask_image2, mask_image2);
		
		output_image.copyTo(output_image, mask_image2);
		
		
		char s[555];
		sprintf_s(s, "�Ă��΂� %d ch[%d]", i, teki_batsu_mat[i].type());
		//imshow(s, mask_image2);
		sprintf_s(s, "i�Ă��΂� %d ch[%d]", i, teki_batsu_mat[i].type());
		//imshow(s, output_image);

		sprintf_s(s, "SP�̂� %d ch[%d]", i, teki_batsu_mat[i].type());
		//imshow(s, teki_sp_mat[i]);
		/*
		
printf("�}�X�N�Q�̃^�C�v�B%d\n",   mask_image2.depth());
printf("�Ă�Ղ�̃^�C�v�B%d\n", tmp_teki_batu.depth());
try{
	Mat m1, m2;
	tmp_teki_batu.convertTo(m1, CV_32F);
	  mask_image2.convertTo(m2, CV_32F);
	int ret = MatMatching_base(m2, m1, 0.5);
	printf("Teki[%d]ret[%d]\n", i, ret);
} catch (cv::Exception& e){
	std::cout << e.what() << std::endl;
	Sleep(10000);
	return -1;
}*/
		
		
		
	
		
		
		//51
		
		//197
		rc_teki_sp.y += 66;
	}

	//�����o�c�m�F
	Mat mikata_batsu_mat[3];
	Mat mikata_sp_mat[3];
	//����E
	cv::Rect rc_mikata[3];

	rc_mikata[0] = cv::Rect(127, 492, 333, 93);
	rc_mikata[1] = cv::Rect(832, 42, 333, 93);
	rc_mikata[2] = cv::Rect(1537, 492, 333, 93);

	for (int i = 0; i < 3; i++) {
		cv::Rect rc_mikata_sp = rc_mikata[i];
		rc_mikata_sp.x -= 19;
		rc_mikata_sp.width = 17;
		rc_mikata_sp.height = 27;
		mikata_batsu_mat[i] = screen_mat(rc_mikata[i]);
		mikata_sp_mat[i] = screen_mat(rc_mikata_sp);

		mikata_batsu_mat[i].copyTo(output_image, mika_batsu_mask);

		
/*		
		Vec3b *pv, bgr;
for(int y = 0; y < output_image.rows; ++y){
	for(int x = 0; x < output_image.cols; ++x){
		pv = output_image.ptr<cv::Vec3b>(y);
		bgr = pv[x];
		
		if(bgr[2] == 0 && bgr[1] == 0 && bgr[0] == 0){
			pv[x] = cv::Vec3b(254, 254, 254);
		}
		
	}
}*/
		
		
		
		
		 s_min = Scalar(120, 120, 120);
		 s_max = Scalar(255, 255, 255);
		
		Mat mask_image2;
		inRange(output_image, s_min, s_max, mask_image2);
		
		dilate(mask_image2, mask_image2, cv::Mat());
		bitwise_not(mask_image2, mask_image2);
		
		output_image.copyTo(output_image, mask_image2);

		
		//output_image = mikata_batsu_mat[i];
		char s[555];

		
		sprintf_s(s, "�����΂� %d ch[%d]", i, 44);
		//imshow(s, mask_image2);
		/*
		sprintf_s(s, "�����΂� %d ch[%d]", i, 44);
		//imshow(s, mask_image2);*/
		

		sprintf_s(s, "SP���� %d ch[%d]", i, 44);
		//imshow(s, mikata_sp_mat[i]);

	}
	return ret;
}


	
	


//���{�[�_�[���C���̈�̃��x�����O
int border_labeling(Mat src_img, Mat mask_img) {
	class LABELINGED_IMG {
	public:
		/*
		�؂蔲���ꂽ�摜mat
		���x�����O�̈�
		�d�S
		�ʐ�
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
	// �O���[�X�P�[���ŉ摜�ǂݍ���
	 Mat src = mask_img;
	//���׃����O����
	cv::Mat LabelImg;
	cv::Mat stats;
	cv::Mat centroids;
	int nLab = cv::connectedComponentsWithStats(src, LabelImg, stats, centroids);
	// ���x�����O���ʂ̕`��F������
	std::vector<cv::Vec3b> colors(nLab);
	colors[0] = cv::Vec3b(0, 0, 0);
	for (int i = 1; i < nLab; ++i) {
		colors[i] = cv::Vec3b((rand() & 255), (rand() & 255), (rand() & 255));
	}

	cv::Mat Dst(src.size(), CV_8UC3);
	Dst =  Scalar(0, 0, 0);
	int val[500] = {0};
	int cnt = 0;
	//ROI�̐ݒ�
	for (int i = 1; i < nLab; ++i) {
		int *param = stats.ptr<int>(i);

		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
		int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];

		border_labeling[i - 1].mat = Mat(cv::Size(width, height),  CV_8UC3);
		border_labeling[i - 1].mat =  Scalar(0, 0, 0);
		border_labeling[i - 1].rc = cv::Rect(x, y, width, height);

		
		//�ʒu�A�T�C�Y�ɂ��t�B���^�����O�������B
		//if(y <= 110 && width >= )
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
	// ���x�����O���ʂ̕`��
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
	//ROI�̐ݒ�
	for (int i = 1; i < nLab; ++i) {
		if (val[i-1] == 0)continue;
		int *param = stats.ptr<int>(i);

		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
		int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];

		cv::rectangle(Dst, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);
	}

	//cv::rectangle(Dst, cv::Rect(BORDER_CENTER, 0, 6, 10), cv::Scalar(0, 5, 255), 2);

	//�d�S�̏o��
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		double *param = centroids.ptr<double>(i);
		int x = static_cast<int>(param[0]);
		int y = static_cast<int>(param[1]);

		border_labeling[i - 1].centroids = Point(x, y);
		cv::circle(Dst, cv::Point(x, y), 3, cv::Scalar(0, 0, 255), -1);
	}

	//�ʐϒl�̏o��
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		int *param = stats.ptr<int>(i);
		//std::cout << "area " << i << " = " << param[cv::ConnectedComponentsTypes::CC_STAT_AREA] << std::endl;
		border_labeling[i - 1].area = param[cv::ConnectedComponentsTypes::CC_STAT_AREA];

		//ROI�̍���ɔԍ�����������
		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		std::stringstream num;
		num << i;
		cv::putText(Dst, num.str(), cv::Point(x + 5, y + 20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

		char s[256];
		sprintf_s(s, "Src %d", i);
		
		
		
		//cv::imshow(s, border_labeling[i - 1].mat );
	}
	char s[256];
		sprintf_s(s, "AAAAAASrc ");
	//debug
	//cv::imshow("�{�[�_�[���x�����O", Dst);
	return 0;
}



int teki_dead_num_prev;
int mikata_dead_num_prev;


//�G�A�����̃f�X�����Ƃ��̌��m�݂̂̊ȈՏ����̊֐��B
void splat_death(){
	IplImage *ipl;
	Mat mask_image;
	
	right_labeling_center.clear();
	left_labeling_center.clear();
	
	/*
		�}�b�v���J���Ă鎞�́A�C�J�A�C�R�����\������Ȃ��B
		�L���v�`���摜����B�}�b�v���J���Ă邩�ǂ������`�F�b�N����B
	*/
	if (splat_map() == 0) {
		SetWindowText(hwnd, L"�}�b�v���[�h�ł��I�I�I�u�L�A�C�R�����ׂȂ���I");
		return;
	}
	
	SetWindowText(hwnd, L"map����Ȃ���B�u�L�A�C�R����������");
	
	Mat teki_icons_mat, output_image, output_image2;
	
	//�y�G�z�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
	teki_icons_mat = screen_mat(cv::Rect(1034, 23, 392, 91));
	//�y�����z�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
	Mat mikata_icons_mat = screen_mat(cv::Rect(493, 23, 392, 91));
	//�{�[�_�[���C�����畐��̈ʒu�����肷��
	Mat border_mat = screen_mat(cv::Rect(501, 80, 914, 18));
	
	
	Scalar s_min = Scalar(55, 55, 55);
	Scalar s_max = Scalar(99, 99, 99);
	inRange( teki_icons_mat, s_min, s_max, mask_image);
	
	//dilate(mask_image, mask_image, cv::Mat());
	//bitwise_not(mask_image, mask_image);
	
	Mat teki_buf = teki_icons_mat.clone();
	teki_icons_mat.copyTo(output_image, mask_image);
	resize(output_image, output_image, cv::Size(), 2, 2);
	
//////////////////////////////////////////
	
	inRange( mikata_icons_mat, s_min, s_max, mask_image);
	mikata_icons_mat.copyTo(output_image2, mask_image);
	resize(output_image2, output_image2, cv::Size(), 2, 2);
        cv::Mat result_img;
	//�G�z�u�L�e���v���[�g�}�b�`���O

		int teki_dead_num = 0;
	double batu_rate = 0.65;
	
	//debug
	//imshow("�o�c�e���v��", tmp_img);
	//imshow("�D��������", output_image2);
	//imshow("�D�����̓G", output_image);
	
	
	cv::Rect teki_wap[4];
	
	/*
		OpenCV�̃e���v���[�g�}�b�`���O�œG4�l�̃A�C�R�������_��(=�f�X����)���L�邩�T��
	*/
	for (int i = 0; i < 4; i++) {
		// �e���v���[�g�}�b�`���O
		cv::matchTemplate(output_image, tmp_img, result_img, CV_TM_CCOEFF_NORMED);
		// �ő�̃X�R�A�̏ꏊ��T��
		cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
		cv::Point max_pt;
		double maxVal;
		cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
		// ���X�R�A�ȉ��̏ꍇ�͏����I��
		if (maxVal < batu_rate) {
			break;
		}else {
			teki_dead_num++;
		}
		roi_rect.x = max_pt.x;
		roi_rect.y = max_pt.y;
		teki_wap[teki_dead_num-1] =  roi_rect;
		dprintf(L"idx: %d [%d/%d]\n", i, max_pt.x, max_pt.y);
		// �T�����ʂ̏ꏊ�ɋ�`��`��
		cv::rectangle(output_image, roi_rect, cv::Scalar(0,255,255), 3);
		double wr = (0.5);
		double hr = (0.5);

		double xx = (double)roi_rect.x * wr;
		double yy = (double)roi_rect.y * hr;

		double ww = (double)roi_rect.width * wr;
		double hh = (double)roi_rect.height * hr;

		//Mat border_mat = screen_mat(cv::Rect(501, 80, 914, 18));
		//teki_icons_mat = screen_mat(cv::Rect(1034, 20, 392, 112));
		////�y�����z�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
		// Mat mikata_icons_mat = screen_mat(cv::Rect(493, 20, 392, 112));
//
          	//���h�{�b�N�X
	teki_icons_mat = screen_mat(cv::Rect(   1034, 23, 392, 91));
	Mat border_mat = screen_mat(cv::Rect(    501, 80, 914, 18));
	
			  xx += 533;
				  yy  -= 57;

			  hh += 30;

			  xx -= 10;
			  ww += 13;
			  cv::Rect roi_rect2 = cv::Rect((int)xx, (int)yy, (int)ww, (int)hh);

			 // printf("�����ꂦ�ƁI[%f %f]", wr, hr);
			 // printf("�������˂�I[%d %d]", roi_rect.width, roi_rect.height);

			 // printf("�������˂�B[%d %d]", (int)ww, (int)hh);
			//  roi_rect.width = 190;
			//	  roi_rect.height = 50;
			  

			  //cv::rectangle(teki_buf, roi_rect, cv::Scalar(0, 0, 255), CV_FILLED);
			  cv::rectangle(teki_buf, roi_rect, cv::Scalar(0, 0, 255), CV_FILLED);
			  cv::rectangle(border_mat, roi_rect2, cv::Scalar(0, 0, 0), CV_FILLED);

			  //imshow("���T�G�O", teki_buf);
	
			
	//�G�A�C�R���Ȃ̂ŉE����
			
				right_labeling_center.push_back( RIGHT_WAPON_ICON(xx + (ww / 2), 1) );
			
			  //imshow("scc", screen_mat);
			  //imshow("scc2", output_image);
			  //screen_mat(cv::Rect(493, 20, 392, 112));
			  //�{�[�_�[���C�����畐��̈ʒu�����肷��
			 // Mat border_mat = screen_mat(cv::Rect(501, 80, 914, 18));
        }
	
	//�O�t���[���̃f�X�A�C�R���������̂ق����������Ƃ��B
	if(teki_dead_num_prev < teki_dead_num){
		//�G�̃f�X�����m�I
		//printf("�ł��������I");
		HWND hw = FindWindowA( NULL, "HSHS");
		if (hw) {
			//SendMessage(hw, YARI_MYMESSAGE, 0, 0);
			//printf("�������񂎁u�I�ł��������I");
		}
	}
	
	char s[555];
	//printf("teki�f�X�A�C�R�����@%d", teki_dead_num);
	//imshow("�Ă�",output_image);
	
	
	
	
	
	
	
	
	
		
	//�y�����z�u�L�e���v���[�g�}�b�`���O
		int mikata_dead_num = 0;
	
		for (int i = 0; i < 4; i++) {
			// �e���v���[�g�}�b�`���O
			cv::matchTemplate(output_image2, tmp_img, result_img, CV_TM_CCOEFF_NORMED);
			// �ő�̃X�R�A�̏ꏊ��T��
			cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
			cv::Point max_pt;
			double maxVal;
				cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
				// ���X�R�A�ȉ��̏ꍇ�͏����I��
				if (maxVal < batu_rate) {
					break;
				}
			  else {
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

				//Mat border_mat = screen_mat(cv::Rect(501, 80, 914, 18));
				////�y�����z�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
				// Mat mikata_icons_mat = screen_mat(cv::Rect(493, 20, 392, 112));

			 //teki_icons_mat = screen_mat(cv::Rect(1034, 20, 392, 112));
			 //Mat border_mat = screen_mat(cv::Rect(501,  80, 914, 18));
			  //
			
			
	//�y�����z�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
	//Mat mikata_icons_mat = screen_mat(cv::Rect(493, 23, 392, 91));
	//Mat border_mat = screen_ma      t(cv::Rect(501, 80, 914, 18));
				xx -= 8;
				yy -= 57;

				hh += 30;

				xx -= 10;
				ww += 13;
				//�����A�C�R���Ȃ̂ō�����
			
				left_labeling_center.push_back( LEFT_WAPON_ICON(xx + (ww / 2), 1) );

				cv::Rect roi_rect2 = cv::Rect((int)xx, (int)yy, (int)ww, (int)hh);

              // �T�����ʂ̏ꏊ�ɋ�`��`��
              cv::rectangle(output_image2, roi_rect, cv::Scalar(255,255,0), 3);
			  cv::rectangle(border_mat, roi_rect2, cv::Scalar(0, 0, 0), CV_FILLED);
        }
	//�O�t���[���̃f�X�A�C�R���������̂ق����������Ƃ��B
	if(mikata_dead_num_prev < mikata_dead_num){
		//�����̃f�X�����m�I

		HWND hw = FindWindowA(NULL, "HSHS");
		if (hw) {
			//SendMessage(hw, YARARE_MYMESSAGE, 0, 0);
		}
	}
	//printf("mika�f�X�A�C�R�����@%d", mikata_dead_num);


	//imshow("����", output_image2);


//	resize(mikata_icons_mat, mikata_icons_mat, cv::Size(), BIT_BMP_WIDTH / mikata_icons_mat.cols, BIT_BMP_HEIGHT / mikata_icons_mat.rows);
//	resize(teki_icons_mat,     teki_icons_mat, cv::Size(), BIT_BMP_WIDTH / teki_icons_mat.cols, BIT_BMP_HEIGHT / teki_icons_mat.rows);

	
	
	
	//���̉��_
	//resize(border_mat, border_mat, cv::Size(), BIT_BMP_WIDTH / teki_icons_mat.cols, BIT_BMP_HEIGHT / teki_icons_mat.rows);
	
	int v_max = (int)( (double)20.0 * 100.0 / 255.0 );
	int v_min = (int)( (double)0.0 * 100.0 / 255.0 );
	
	Mat hsv, mask;
	//cvtColor(border_mat, hsv, COLOR_BGR2HSV);

	//cv::inRange(hsv, Scalar(0,     0, v_min), 
	//				 Scalar(180, 100, v_max), mask_image);
	inRange(border_mat, Scalar(0, 0, 0), Scalar(51, 51, 51), mask_image);
	//dilate(mask_image, mask_image, cv::Mat());

	Mat m1, m2,mask_image2, mask_image3;
	//dilate(mask_image, mask_image, cv::Mat());
	bitwise_not(mask_image, mask_image);
	//�A�C�R���̃o�c�̃O���[�F���}�X�N����
	s_min = Scalar(60, 60, 60);
	s_max = Scalar(95, 95, 95);
	inRange(border_mat, s_min, s_max, mask_image2);
	dilate(mask_image2, mask_image2, cv::Mat());
	//bitwise_not(mask_image2, mask_image2);

	bitwise_xor(mask_image2, mask_image, m1);
	
	Mat output_image3;
	border_mat.copyTo(output_image3, m1);

	//imshow("�{�[�_�[", output_image3);

	//debug
	//imshow("�{�[�_�[����", m1);
	
	
	border_labeling(border_mat, m1);

	//63-53
	
	
	//�\�[�g�J�n�B�A�C�R���̊��S�ɏ󋵔c��
	sort(left_labeling_center.begin(), left_labeling_center.end());

	
//printf("L[%d] R[%d]\n", left_labeling_center.size(), right_labeling_center.size());
	sort(right_labeling_center.begin(), right_labeling_center.end());
	
	if(left_labeling_center.size() + right_labeling_center.size() >= 8){
	//	printf("[%d][%d][%d][%d] [%d][%d][%d][%d]\n", left_labeling_center[3].stat,left_labeling_center[2].stat,left_labeling_center[1].stat,left_labeling_center[0].stat,
	//	right_labeling_center[0].stat,right_labeling_center[1].stat,right_labeling_center[2].stat,right_labeling_center[3].stat);
	
		
		//Mat border_mat = screen_mat(cv::Rect(    501, 80, 914, 18));
	mikata_icon[0].img = screen_mat(cv::Rect(501 + left_labeling_center[3].x - 63/2, 45,    63, 53));
	mikata_icon[1].img = screen_mat(cv::Rect(501 + left_labeling_center[2].x - 63/2, 45,    63, 53));
	mikata_icon[2].img = screen_mat(cv::Rect(501 + left_labeling_center[1].x - 63/2, 45,    63, 53));
	mikata_icon[3].img = screen_mat(cv::Rect(501 + left_labeling_center[0].x - 63/2, 45,    63, 53));
		
	teki_icon[0].img =   screen_mat(cv::Rect(501 + right_labeling_center[0].x - 63/2, 45,    63, 53));
	teki_icon[1].img =   screen_mat(cv::Rect(501 + right_labeling_center[1].x - 63/2, 45,    63, 53));
	teki_icon[2].img =   screen_mat(cv::Rect(501 + right_labeling_center[2].x - 63/2, 45,    63, 53));
	teki_icon[3].img =   screen_mat(cv::Rect(501 + right_labeling_center[3].x - 63/2, 45,    63, 53));
		
		/*
		for(int i = 0; i < 4;i++){
			{
				char s[555];
				sprintf_s(s, "����img|%d",i);
				
				cv::imshow(s, mikata_icon[i].img);
				
				sprintf_s(s, "�Gimg|%d", i);
				
				cv::imshow(s, teki_icon[i].img);
			}
			
		}*/
		
		
		
	mikata_icon[0].update(left_labeling_center[3].stat);
	mikata_icon[1].update(left_labeling_center[2].stat);
	mikata_icon[2].update(left_labeling_center[1].stat);
	mikata_icon[3].update(left_labeling_center[0].stat);
	
	teki_icon[0].update(right_labeling_center[0].stat);
	teki_icon[1].update(right_labeling_center[1].stat);
	teki_icon[2].update(right_labeling_center[2].stat);
	teki_icon[3].update(right_labeling_center[3].stat);
		
		
	printf("��[%d][%d][%d][%d] [%d][%d][%d][%d]\n", 
		mikata_icon[0].stat, mikata_icon[1].stat, mikata_icon[2].stat, mikata_icon[3].stat, 
		teki_icon[0].stat, teki_icon[1].stat, teki_icon[2].stat, teki_icon[3].stat);
		
		printf("���f�X�I�I�I[%d][%d][%d][%d] [%d][%d][%d][%d]\n", 
		mikata_icon[0].death_cnt, mikata_icon[1].death_cnt, mikata_icon[2].death_cnt, mikata_icon[3].death_cnt, 
		teki_icon[0].death_cnt, teki_icon[1].death_cnt, teki_icon[2].death_cnt, teki_icon[3].death_cnt);
		
		printf("��stat���ԁI[%f][%f][%f][%f] [%f][%f][%f][%f]\n", 
		mikata_icon[0].getStatTime(), mikata_icon[1].getStatTime(), mikata_icon[2].getStatTime(), mikata_icon[3].getStatTime(), 
		teki_icon[0].getStatTime(), teki_icon[1].getStatTime(), teki_icon[2].getStatTime(), teki_icon[3].getStatTime());
		
		teki_dead_num = teki_icon[0].stat +  teki_icon[1].stat +  teki_icon[2].stat +  teki_icon[3].stat;
			Mat matt = Mat::zeros(cv::Size(1280, 768), CV_8UC3);

			matt = Scalar(255, 255, 255);
		
	int maxv = INT_MIN;
	int minv = INT_MAX;
	int max_idx;
	int min_idx;
	for(int i = 0; i < 4; ++i) {
		if( teki_icon[i].live_time_sum > maxv ){
			maxv = teki_icon[i].live_time_sum;
			max_idx = i;
		}
		if( teki_icon[i].live_time_sum < minv ){
			minv = teki_icon[i].live_time_sum;
			min_idx = i;
		}
	}
		for(int i = 0; i < 4; i++){
			int dx = i*320;
			
			std::stringstream num;
			double du = teki_icon[i].getStatTime() / 1000.0;
			char s[555];
			
			
			double time_rate = teki_icon[i].getStatTime() / 35000 * 768;
			
			if (teki_icon[i].stat == 1) {
				//cv::rectangle(matt, cv::Rect(dx, 0, 320, 768), cv::Scalar(135, 135, 135), CV_FILLED);

				cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(35, 35, 35), CV_FILLED);

				//a
			}
			else {/*
				if(min_idx == i){
					//�������Ԃ��Z���U�R
					cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(100, 255, 100), CV_FILLED);
				}else
				if(max_idx == i){
					//�������Ԃ������G�G�[�X
					cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(0, 00, 255), CV_FILLED);
				}else{
					cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(170, 170, 70), CV_FILLED);
				}*/
				cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(180, 204, 86), CV_FILLED);
				/*
				if (teki_dead_num == 0) {
					cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(175, 175, 175), CV_FILLED);
				}
				else if (teki_dead_num == 1) {
					cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(0, 0, 255), CV_FILLED);
				}
				else if (teki_dead_num == 2) {
					cv::rectangle(matt, cv::Rect(dx,  matt.rows - time_rate, 320, time_rate), cv::Scalar(0, 255, 0), CV_FILLED);
				}
				else if (teki_dead_num == 3) {
					cv::rectangle(matt, cv::Rect(dx,  matt.rows - time_rate, 320, time_rate), cv::Scalar(255, 0,0), CV_FILLED);
				}
				else if (teki_dead_num == 4) {
					
					cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(0, 0, 0), CV_FILLED);
				}*/

			}
			

			if(2){
				sprintf_s(s, "   %.1f", du);
				cv::putText(matt, s, cv::Point(dx, 768 -70), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 0), 3);
				
				sprintf_s(s, " %d", teki_icon[i].death_cnt);
				cv::putText(matt, s, cv::Point(dx, 768 -370), cv::FONT_HERSHEY_SIMPLEX, 5, cv::Scalar(0, 0, 0), 7);
				
				
				sprintf_s(s, "%.2f",( teki_icon[i].live_time_sum/ 1000.0));
				//cv::putText(matt, s, cv::Point(dx, 768 -330), cv::FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 0, 127), 5);
				
			}
			
			
		}
		cv::line(matt, Point(0, 0), Point(0, 768), Scalar(0, 00, 0), 5, 8);
		cv::line(matt, Point(320, 0), Point(320, 768), Scalar(0, 0, 0), 5, 8);
		cv::line(matt, Point(640, 0), Point(640, 768), Scalar(0, 00, 0), 5, 8);
		cv::line(matt, Point(960, 0), Point(960, 768), Scalar(0, 00, 0), 5, 8);
		
		//�u�L�A�C�R�����Q�[�W��ɏd�ˍ��킹
		
		//mikata_icon[0].img.copyTo(matt.rowRange(160, 160+63).
		//			   				 colRange(50, 50+53));
		
		
    //�O�i�摜�̕ό`�s��
		
	int tx = 160 - 107;
	int ty = 768-300;

	for(int i = 0; i < 4; i++){

		resize(teki_icon[i].img, teki_icon[i].img, cv::Size(), 3.4, 3.4);

		cv::Mat mat = (cv::Mat_<double>(2,3)<<1.0, 0.0, tx, 0.0, 1.0, ty);
		cv::warpAffine(teki_icon[i].img, matt, mat, matt.size(), CV_INTER_LINEAR, cv::BORDER_TRANSPARENT);

		tx += 320;
	}


    //imshow("affine", dstImg);
		
		
		  cv::imshow("�틵", matt);
		  cv::moveWindow("�틵", -1280, 170);

		cv::setMouseCallback("�틵", CallBackFunc, (void *)&matt);
		
	}
	//imshow("�����A�C�R��X", mikata_icons_mat);
	//imshow("�����A�C�R��XX", teki_icons_mat);
	
	
	


	teki_dead_num_prev = teki_dead_num;
	mikata_dead_num_prev = mikata_dead_num;
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
			hdc = GetDC(desktop);
			BitBlt(hMemDC, 0, 0, dsk_width, dsk_height, hdc, 0, 0, SRCCOPY);
			ReleaseDC(desktop, hdc);
			
			//�X�N���[���L���v�`���摜��Mat�ɕϊ����Ă����B
			ipl = Convert_DIB_to_IPL(&bmpInfo, lpPixel);
			screen_mat = cvarrToMat(ipl);

			splat_death();
			
			cvReleaseImage(&ipl);
			
			cnt++;
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;
		}
		case WM_DESTROY: {
			DeleteDC(hMemDC);
			DeleteObject(hBitmap);
			PostQuitMessage(0);
			return 0;
		}
			#pragma warning(disable:4996)
		case WM_CREATE: {
			SetTimer(hwnd, 2525, 1, NULL);

			RECT rc;
			desktop = GetDesktopWindow();
			GetWindowRect(desktop, &rc);
			dsk_width = rc.right;
			dsk_height = rc.bottom;
			
			//�e���v���[�g�}�b�`���O�p�̉摜��ǂݍ��ށB
			
			tmp_img = cv::imread("E:\\scan5\\splat_temp.bmp", 1);
			if (!tmp_img.data) dprintf(L"��߂˂�������������������������������");

			BATSU_TMP_IMG = cv::imread("E:\\scan5\\map_batu.bmp", 1);
			if (!BATSU_TMP_IMG.data) dprintf(L"��߂˂�������������������������������");
			
			mika_batsu_mask = cv::imread("E:\\scan5\\mika_batsu_mask.bmp", 1);
			if (!mika_batsu_mask.data) {
				dprintf(L"��߂˂�������������������������������");
				MessageBox(NULL, TEXT("Kitty on your lap"),
					TEXT("���b�Z�[�W�{�b�N�X"), MB_OK);
			}
			
			teki_batsu_mask = cv::imread("E:\\scan5\\teki_batsu_mask.bmp", 1);
			if (!teki_batsu_mask.data) {
				dprintf(L"��߂˂�������������������������������");
				MessageBox(NULL, TEXT("Kitty on your lap"),
					TEXT("���b�Z�[�W�{�b�N�X"), MB_OK);
			}
			
			
			
			
			
			
			cvtColor(mika_batsu_mask, mika_batsu_mask, CV_BGR2GRAY);
			cvtColor(teki_batsu_mask, teki_batsu_mask, CV_BGR2GRAY);
			
			
			tmp_teki_batu = cv::imread("E:\\scan5\\tmp_teki_batu.bmp", 1);
			if (!tmp_teki_batu.data) {
				dprintf(L"��߂˂�������������������������������");
				MessageBox(NULL, TEXT("Kitty on your lap"),
					TEXT("���b�Z�[�W�{�b�N�X"), MB_OK);
			}
			
			printf("�G�o�c�e���v���̃^�C�v�B%d", teki_batsu_mask.type());
			
			tmp_mikata_batu = cv::imread("E:\\scan5\\tmp_mikata_batu.bmp", 1);
			if (!tmp_mikata_batu.data) {
				dprintf(L"��߂˂�������������������������������");
				MessageBox(NULL, TEXT("Kitty on your lap"),
					TEXT("���b�Z�[�W�{�b�N�X"), MB_OK);
			}
			//teki_batsu_mask.convertTo(teki_batsu_mask, CV_8UC1);
			
			TCHAR s[555];
			_stprintf_s(s,L"�^�C�v: %d", teki_batsu_mask.type());
		
			
			SetWindowPos(hwnd, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE);
			
			//DIB�̏���ݒ肷��
			bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmpInfo.bmiHeader.biWidth = dsk_width;
			bmpInfo.bmiHeader.biHeight = dsk_height;
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = 24;
			bmpInfo.bmiHeader.biCompression = BI_RGB;

			//DIBSection�쐬
			hdc = GetDC(hwnd);
			hBitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, (void**)&lpPixel, NULL, 0);
			hMemDC = CreateCompatibleDC(hdc);
			SelectObject(hMemDC, hBitmap);
			ReleaseDC(hwnd, hdc);
			
			HWND hw = FindWindowA( NULL, "HSHS");
			if (!hw) {
				ShellExecute(NULL, L"open", L"C:\\Users\\kento\\Documents\\Visual Studio 2015\\Projects\\opencv_base_GUI\\x64\\Release\\hsptmp.exe", NULL, NULL, SW_SHOWNORMAL);

			}
			Mat m1(Size(1280, 768), CV_8U, Scalar::all(0)); 
			cv::imshow("�틵", m1);
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;                                 // ��`�p�̍\����
	LPTSTR pClassName = TEXT("MWIN");      // �N���X��
	hInstance = hInst;

	wc.cbSize = sizeof(WNDCLASSEX);         // �\���̃T�C�Y
	wc.style = CS_HREDRAW | CS_VREDRAW;    // �N���X�X�^�C��
	wc.lpfnWndProc = (WNDPROC)WndProc;       // �v���V�[�W��
	wc.cbClsExtra = 0;                          // �⑫�������u���b�N
	wc.cbWndExtra = 0;                          //   �̃T�C�Y
	wc.hInstance = hInst;                      // �C���X�^���X
	wc.hIcon = NULL;   // �A�C�R��
	wc.hCursor = LoadCursor(NULL, IDC_ARROW); // �J�[�\��
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);   // �w�i�F
	wc.lpszMenuName = NULL;                   // ���j���[
	wc.lpszClassName = pClassName;                 // �N���X��
	wc.hIconSm = NULL;    // �������A�C�R��
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, TEXT("RegisterClass Error."), TEXT(""), MB_OK | MB_ICONWARNING);
		return -1;
	}

	hwnd = CreateWindow(pClassName, TEXT("���C���E�B���h�E"),
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
