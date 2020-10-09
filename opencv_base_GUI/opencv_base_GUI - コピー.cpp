// opencv_base_GUI.cpp : �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include "opencv_base_GUI.h"

#include <Shellapi.h>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector> //for vector
#include <algorithm> //for sort
#include <chrono>


using namespace std;
using namespace cv;

#define dprintf( str, ... ) \
	{ \
		TCHAR c[2560]; \
		_stprintf_s( c, str, __VA_ARGS__ ); \
		OutputDebugString( c ); \
	}
HWND hwnd;
HINSTANCE hInstance;

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

//#define BIT_BMP_WIDTH 1280
//#define BIT_BMP_HEIGHT 368
#define BORDER_CENTER 456



Mat screen_mat;
cv::Mat tmp_img, BATSU_TMP_IMG;
cv::Mat ki_ika, ki_tako, ao_ika, ao_tako;

cv::Mat ki_batsu, ao_batsu, siro_batsu;

cv::Mat mika_batsu_mask, teki_batsu_mask;

cv::Mat wapons_tmp[8];


class WAPON_ICON {
public:
	int lr;
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
	
	void reset() {
		live_time = dead_time = stat_time = 0;
		death_cnt = 0;
		start = chrono::system_clock::now();
		
		live_time_sum = 0;
	}
	void stop(int st){
		
	}

	double getStatTime() {
		if (stat == 0) {
			return live_time;
		}
		else {
			return dead_time;
		}
	}
	void update(int st){
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
					
					//live_time_sum += live_time;
					
					HWND hw = FindWindowA(NULL, "HSHS");
					if (hw) {
						
						if(lr == 0){
							SendMessage(hw, YARARE_MYMESSAGE, lr, num);
						} else {
							SendMessage(hw, YARI_MYMESSAGE,   lr, num);
						}
					}
					
				}
				
			}
			
		}
		
		
		
		prev_stat = stat_buf;
	}
	
	
	/*
	
	
	void update(int st){
		
		stat_buf = st;
		
		if(is_count == true){
			double dt = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now()-stat_clock).count();
			//stat�ω����Ă������ms�o�߂�����K�p�B
			if(dt < 1500){
				//stat�ω����Ă����莞�ԗ����Ă��Ȃ�
				
			} else {
				//��ԕω����Ă���A�K�莞�Ԃ��o��
				
				//�R�R�ŏ��߂�stat���f�I�I
				stat = stat_buf;
				if(stat == 0){
					//�������Ԍv���J�n�I
					live_time = dt;
				} else if(stat == 1){
					//�f�X���Ԍv���J�n�I
					dead_time = dt;
					
					death_cnt++;
				}
				
				is_count = false;
			}
		}
		if(stat_buf != prev_stat){
			
			is_count = true;
			stat_clock = chrono::system_clock::now();
		}
		
		double dt2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now()-start).count();
		start = chrono::system_clock::now();
		dead_time += dt2;
		
		
		
		prev_stat = stat_buf;
	}*/

};
WAPON_ICON mikata_icon[4] = { WAPON_ICON(0, 0, 0),WAPON_ICON(0, 1, 0),WAPON_ICON(0, 2, 0),WAPON_ICON(0, 3, 0) };
WAPON_ICON teki_icon[4]   = { WAPON_ICON(1, 0, 0),WAPON_ICON(1, 1, 0),WAPON_ICON(1, 2, 0),WAPON_ICON(1, 3, 0) };


/*

#include <iostream>
using namespace std;

class samp{

int a, b;

public:
samp(int n, int m){ a = n; b = m; }   // �������R���X�g���N�^

int get_a(){ return a; }
int get_b(){ return b; }
};

int main(){

samp ob[4]={ samp(1,2), samp(3,4),  samp(5,6),  samp(7,8)}; // (��)

*/


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



BYTE * GetBits(BITMAPINFO *st, BYTE *pix, int x, int y) {
	//32bit
	//byte�z��s�N�Z���񂩂���W�w��Ń|�C���^�Ԃ��֐��B
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

	//lp += (st->bmiHeader.biHeight - y - 1) * st->bmiHeader.biWidth * 4;
	//lp += 4 * x;
	return lp;
}
BYTE * GetBits_8bit(BITMAPINFO *st, BYTE *pix, int x, int y) {
	//32bit
	//byte�z��s�N�Z���񂩂���W�w��Ń|�C���^�Ԃ��֐��B
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
	lp += (st->bmiHeader.biHeight - y - 1) * st->bmiHeader.biWidth;
	lp += x;
	return lp;
}
int adjust_lining_limit(int m_width, int m_height, int *p_start, int *p_end, int sx, int sy, int ex, int ey, int *p_cx, int *p_cy) {
	int img_lim, img_lim_other, base_lim, other_lim;
	int *p_res=0, base_co, other_co, tmp, cx, cy;
	cx = *p_cx = ex - sx;
	cy = *p_cy = ey - sy;
	int base_axis = (abs(cx)>abs(cy)) ? 'x' : 'y';
	*p_start = *p_end = 0;

	//
	if (!cx && !cy) return -1;
	// x,y�̒����ق��̍��W�����x�[�X�ɂ���
	if (base_axis == 'x') {    // x ���W�x�[�X
		img_lim = m_width;
		img_lim_other = m_height;
		base_lim = cx;
		other_lim = cy;
	}
	else {   // y ���W�x�[�X
		img_lim = m_height;
		img_lim_other = m_width;
		base_lim = cy;
		other_lim = cx;
	}

	for (int i = 0; i<2; i++) {
		if (i == 0) {
			p_res = p_start;
			base_co = (base_axis == 'x') ? sx : sy;
			other_co = (base_axis == 'x') ? sy : sx;
		}
		else if (i == 1) {
			p_res = p_end;
			base_co = (base_axis == 'x') ? ex : ey;
			other_co = (base_axis == 'x') ? ey : ex;
			other_lim = -other_lim;
		}

		// x,y�̒����ق��̍��W���Ŕ���
		if (base_co<0)     *p_res = -base_co;
		if (base_co >= img_lim) *p_res = base_co - img_lim + 1;

		// x,y�̒Z���ق��̍��W���Ŕ���
		if (!other_lim) continue;
		tmp = other_co + ((*p_res) * (2 * other_lim) + abs(base_lim)) / (2 * base_lim);

		int aim_co;
		if (tmp<0) { aim_co = -other_co; }
		else if (tmp >= img_lim_other) { aim_co = other_co - img_lim_other + 1; }
		else continue;  // �͈͓�

		tmp = aim_co * 2 * abs(base_lim) - abs(base_lim);
		*p_res = tmp / abs(2 * other_lim);
		if (i == 0 && tmp%abs(2 * other_lim)) (*p_res)++;
		if (i == 1) (*p_res)++;
	}
	*p_end = abs(base_lim) - *p_end;
	return 0;
}
void lineTo_blend(BITMAPINFO *st, BYTE *pix, int sx, int sy, int ex, int ey, int diameter, BYTE r, BYTE g, BYTE b) {
	int cx, cy, pos_x, pos_y, i, lim, start, end;


	int col_index;
	double e;
	int *p_co1, *p_co2, ddis1, ddis2, dn1, dn2;
	if (adjust_lining_limit(st->bmiHeader.biWidth, st->bmiHeader.biHeight
		, &start, &end, sx, sy, ex, ey, &cx, &cy)<0) return;

	int base_axis = (abs(cx)>abs(cy)) ? 'x' : 'y';
	p_co1 = (base_axis == 'x') ? &pos_x : &pos_y;
	p_co2 = (base_axis == 'x') ? &pos_y : &pos_x;
	ddis1 = (base_axis == 'x') ? 2 * cx : 2 * cy;
	ddis2 = (base_axis == 'x') ? 2 * cy : 2 * cx;
	dn1 = dn2 = 1;
	if (ddis1<0) { dn1 = -1; ddis1 = -ddis1; }
	if (ddis2<0) { dn2 = -1; ddis2 = -ddis2; }
	pos_x = sx;
	pos_y = sy;
	*p_co1 += dn1* start;
	*p_co2 += dn2* (start*ddis2 + ddis1 / 2) / ddis1;
	e = (ddis1 / 2 + start*ddis2) % ddis1;

	for (i = start; i <= end; i++) {
			BYTE *p;
			
			if ((p = GetBits_8bit(st, pix, pos_x, pos_y)))
			{
				*(p) = r;
			}
		(*p_co1) += dn1;
		e += ddis2;
		if (e >= ddis1) {
			e -= ddis1;
			(*p_co2) += dn2;
		}
	}
	return;
};
IplImage* Convert_DIB_to_IPL(BITMAPINFO *bi, BYTE *pix) {
	/*
	24bit�r�b�g�}�b�v��OpenCV�̉摜�t�H�[�}�b�g�ɕϊ�
	*/
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
int saveBMP(LPCTSTR lpszFn, LPBITMAPINFO lpInfo, LPBYTE lpPixel) {
	LPVOID lpBufl;
	LPBITMAPFILEHEADER lpHead;
	LPBITMAPINFOHEADER lpInfol;
	LPBYTE lpPixell;
	DWORD dwWidth, dwHeight, dwLength, dwSize;
	HANDLE fh;

	/* �r�b�g�}�b�v�̑傫���ݒ� */
	dwWidth = lpInfo->bmiHeader.biWidth;
	dwHeight = lpInfo->bmiHeader.biHeight;

	if ((dwWidth * 3) % 4 == 0) /* �o�b�t�@�̂P���C���̒������v�Z */
		dwLength = dwWidth * 3;
	else
		dwLength = dwWidth * 3 + (4 - (dwWidth * 3) % 4);

	lpBufl = GlobalAlloc(GPTR, /* �������m�� */
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwLength*dwHeight);

	/* �������C���[�W�p�|�C���^�ݒ� */
	lpHead = (LPBITMAPFILEHEADER)lpBufl;
	lpInfol = (LPBITMAPINFOHEADER)((LPBYTE)lpBufl + sizeof(BITMAPFILEHEADER));
	lpPixell = (LPBYTE)lpBufl + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	/* �r�b�g�}�b�v�̃w�b�_�쐬 */
	lpHead->bfType = 'M' * 256 + 'B';
	lpHead->bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwLength*dwHeight;
	lpHead->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	lpInfol->biSize = sizeof(BITMAPINFOHEADER);
	lpInfol->biWidth = dwWidth;
	lpInfol->biHeight = dwHeight;
	lpInfol->biPlanes = 1;
	lpInfol->biBitCount = 24;

	/* �s�N�Z������R�s�[ */
	CopyMemory(lpPixell, lpPixel, dwLength*dwHeight);

	fh = CreateFile(lpszFn, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	WriteFile(fh, lpBufl, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + dwLength*dwHeight, &dwSize, NULL);

	CloseHandle(fh);

	if (dwSize<sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + dwLength*dwHeight) {
		return -1;
		MessageBox(hwnd,L"�t�@�C���쐬�Ɏ��s���܂����B�󂫗e�ʂ��m�F���Ă�������",L"�������݃G���[",MB_OK);

	}
	return 0;
}
int trimming_DIB(BITMAPINFO *src_bi, BYTE **src_pix, BITMAPINFO *dst_bi, BYTE **dst_pix, RECT rc) {

	memcpy(dst_bi, src_bi, sizeof(BITMAPINFO));

	dst_bi->bmiHeader.biWidth = rc.right - rc.left + 1;
	dst_bi->bmiHeader.biHeight = rc.bottom - rc.top + 1;

	*dst_pix = (BYTE *)malloc(dst_bi->bmiHeader.biWidth * dst_bi->bmiHeader.biHeight * 4);


	BYTE *p, *p2;
	int x, y;

	int xx, yy;

	yy = 0;
	for (y = rc.top; y <= rc.bottom; y++) {
		xx = 0;
		for (x = rc.left; x <= rc.right; ++x) {
			if ((p = GetBits(src_bi, *src_pix, x, y)) && (p2 = GetBits(dst_bi, *dst_pix, xx, yy))) {
				*(p2 + 2) = *(p + 2);
				*(p2 + 1) = *(p + 1);
				*(p2 + 0) = *(p + 0);
			}
			xx++;
		}
		yy++;
	}
	return 0;
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


int MatMatching(char *s, Mat output_image ,Mat  tmp_img, double rate, int cnt){
	cv::Mat result_img;
	
	
	cv::Mat search_img0 = output_image.clone();
	cv::Mat search_img;
	search_img0.copyTo( search_img );
//	cv::Mat result_img;
	// �ő�̃X�R�A�̏ꏊ��T��
	for(int i = 0; i < cnt; i ++){
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
			
			  	  
			  }
	}
	//cv::imshow(s, search_img0 );
	return 0;
}

Mat wapon_SP_Mask(Mat src){
	
	//�@47-100
	//���@58-100
	int v_max = (int)( (double)100.0 * 255.0 / 100.0 );
	int v_min = (int)( (double)47.0 *  255.0 /  100.0 );
	
	int s_max = (int)( (double)50.0 * 255.0 / 100.0 );
	int s_min = (int)( (double)0.0 *  255.0 /  100.0 );
	
	
	Mat hsv, mask;
	cvtColor(src, hsv, COLOR_BGR2HSV);
	cv::inRange(hsv, Scalar(0, s_min, v_min), Scalar(180, s_max, v_max), mask);
	
	dilate(mask, mask, cv::Mat());
	
	return mask;
}
Mat wapons_labeling(Mat mat){
	//�A�C�R���̐F�}�X�N
	Mat mask_color_b1, mask_color_b2, mask_color_y1, mask_color_y2;
	Scalar s_min, s_max;

	//���F���C��
	s_min = Scalar(143 - 10, 0, 43 - 10);
	s_max = Scalar(149 + 10, 3 + 10, 47 + 10);
	cv::inRange( mat, s_min, s_max, mask_color_b1);
	//���F���F
	s_min = Scalar(173 - 10, 18 - 10, 66 - 10);
	s_max = Scalar(176 + 10, 23 + 10, 71 + 10);
	cv::inRange( mat, s_min, s_max, mask_color_b2);
	
	//���F���C��
	s_min = Scalar(0, 145-10, 192 - 10);
	s_max = Scalar(3 + 10, 150 + 10, 198 + 10);
	cv::inRange( mat, s_min, s_max, mask_color_y1);
	//���F���F
	s_min = Scalar( 0, 174 - 10, 216 - 10);
	s_max = Scalar(15 + 10, 178 + 10, 222 + 10);
	cv::inRange( mat, s_min, s_max, mask_color_y2);
	

	Mat m1, m2, m3;
	//�A�C�R���̃}�X�N�쐬
	bitwise_or(mask_color_b1, mask_color_b2, m1);
	//���A�C�R���̃}�X�N�쐬
	bitwise_or(mask_color_y1, mask_color_y2, m2);
	
	//�����}�X�N�^��
	//bitwise_or(mask_color_y1, mask_color_b1, cv::Mat());
	bitwise_or(m1, m2, m3);
	dilate(m3, m3, cv::Mat());
	//�������烉�x�����O
	return m3;
	
}

int Is_SP(Mat src){
	//���F�@���ʁ@�G���FV 39-58%
	int v_max = (int)( (double)39.0 * 100.0 / 255.0 );
	int v_min = (int)( (double)58.0 * 100.0 / 255.0 );
	
	Mat hsv, mask;
	cvtColor(src, hsv, COLOR_BGR2HSV);
	//cv::inRange(hsv, cv::Scalar(hueMin, saturation, brightness, 0), cv::Scalar(hueMax, 255, 255, 0), result);
	//hsv = cvtColor(src, COLOR_BGR2HSV);
	cv::inRange(hsv, Scalar(0, 0, v_min), Scalar(180, 100, v_max), mask);
	
	Vec3b *pv, bgr;
	for(int y = 0; y < mask.rows; ++y){
		for(int x = 0; x < mask.cols; ++x){
			pv = mask.ptr<Vec3b>(y);
			pv[x] = Vec3b(255, 0, 0);
			bgr = pv[x];
			dprintf(L"%d,%d,%d\n", bgr[0], bgr[1], bgr[2]);
		}
	}
}

		
//�}�b�v���[�h���ǂ������聕����
int splat_map() {
	Mat upper_left_mat(screen_mat, cv::Rect(51, 0, 111, 180));
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
	cv::Rect rc_teki   (1632, 50,  208, 58);
	cv::Rect rc_teki_sp(1620, 61,  17 , 34);
	
	Mat teki_batsu_mat[4];
	Mat teki_sp_mat[4];

	for(int i = 0; i < 4; i++){
		teki_batsu_mat[i] = screen_mat(rc_teki);
		teki_sp_mat[i]    = screen_mat(rc_teki_sp);

		//teki_batsu_mat[i] = screen_mat(rc_teki);
		
		//teki_batsu_mat[i].copyTo(output_image, teki_batsu_mask);
		output_image = teki_batsu_mat[i];
		char s[555];
		sprintf_s(s, "�Ă��΂� %d ch[%d]", i, teki_batsu_mat[i].type());
		//imshow(s, output_image);

		sprintf_s(s, "SP�̂� %d ch[%d]", i, teki_batsu_mat[i].type());
		//imshow(s, teki_sp_mat[i]);


		rc_teki.y += 67;
		rc_teki_sp.y += 67;
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

		//output_image = mikata_batsu_mat[i];
		char s[555];
		sprintf_s(s, "�����΂� %d ch[%d]", i, 44);
		//imshow(s, output_image);

		sprintf_s(s, "SP���� %d ch[%d]", i, 44);
		//imshow(s, mikata_sp_mat[i]);

	}
	return ret;
}

//2�l�����ꂽMAT�������ɃX�ׂ�
int splat_labeling(Mat src_img, Mat mask_img) {
	// �O���[�X�P�[���ŉ摜�ǂݍ���
	 Mat src = mask_img;


	//���׃����O����
	cv::Mat LabelImg;
	cv::Mat stats;
	cv::Mat centroids;
	int nLab = cv::connectedComponentsWithStats(src, LabelImg, stats, centroids);

	//dprintf(L"labe�z���F%d\n", nLab);
	//if (nLab <= 1)return -1;
	// ���x�����O���ʂ̕`��F������
	std::vector<cv::Vec3b> colors(nLab);
	colors[0] = cv::Vec3b(0, 0, 0);
	for (int i = 1; i < nLab; ++i) {
		colors[i] = cv::Vec3b((rand() & 255), (rand() & 255), (rand() & 255));
	}

	

	cv::Mat Dst(src.size(), CV_8UC3);
	Dst =  Scalar(0, 0, 0);


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
		
		/*
		void cut_img(Mat src_img, int x, int y) {
			int xx, yy;
			xx = x; yy = y;
			x -= rc.x;
			y -= rc.y;
			Vec3b *pv, *pv2;
			pv =   mat.ptr<Vec3b>(y);
			pv2 =  src_img.ptr<Vec3b>(yy);
			
			
			//�}�X�N�ł͂Ȃ��\�[�X�摜���烉�x�����O������؂蔲���B
			Vec3b bgr = pv2[xx];
			pv[x] = cv::Vec3b(bgr[0], bgr[1], bgr[2]);
		}*/
	};
	LABELINGED_IMG labeling[500];
	/*
	//�ʐϒl�Ńt�B���^�����O�B臒l�ȉ��̖ʐ�(�s�N�Z���ʂȂ�)
	for (int i = 1; i < nLab; ++i) {
		int *param = stats.ptr<int>(i);
		labeling[i - 1].area = param[cv::ConnectedComponentsTypes::CC_STAT_AREA];
	}*/

	int val[500] = {0};
	int cnt = 0;
	//ROI�̐ݒ�
	for (int i = 1; i < nLab; ++i) {
		int *param = stats.ptr<int>(i);

		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
		int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];

		//�̈���W���o�̂��łɁA
		//���x�����O��f�؂蔲���pMat�z����`�B
		//0���߁B�^����Mat(width, height, src.type(), 0)
		//labeling[i - 1].mat = Mat(width, height, src.type(), 0);
		//(cv::Size(640, 480), CV_8UC3, cv::Scalar(0,0,255))
		labeling[i - 1].mat = Mat(cv::Size(width, height),  CV_8UC3);
		labeling[i - 1].mat =  Scalar(0, 0, 0);
		labeling[i - 1].rc = cv::Rect(x, y, width, height);

		
		//�ʒu�A�T�C�Y�ɂ��t�B���^�����O�������B
		//if(y <= 110 && width >= )
		if (width >= 29 && width <= 85) {
		//if(1){
			val[i - 1] = 1;
			cnt++;
		}
		else {
			val[i - 1] =0;
		}
		//dprintf(L"���x��[%d %d %d %d][%d]\n", x, y, width, height, val[i - 1]);
		//cv::rectangle(Dst, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);
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
			if(idx != 0)labeling[idx-1].cut_img(src_img, j, i);
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

	//�d�S�̏o��
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		double *param = centroids.ptr<double>(i);
		int x = static_cast<int>(param[0]);
		int y = static_cast<int>(param[1]);

		labeling[i - 1].centroids = Point(x, y);
		cv::circle(Dst, cv::Point(x, y), 3, cv::Scalar(0, 0, 255), -1);
	}

	//�ʐϒl�̏o��
	for (int i = 1; i < nLab; ++i) {
		if (val[i - 1] == 0)continue;
		int *param = stats.ptr<int>(i);
		//std::cout << "area " << i << " = " << param[cv::ConnectedComponentsTypes::CC_STAT_AREA] << std::endl;
		labeling[i - 1].area = param[cv::ConnectedComponentsTypes::CC_STAT_AREA];

		//ROI�̍���ɔԍ�����������
		int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
		int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
		std::stringstream num;
		num << i;
		cv::putText(Dst, num.str(), cv::Point(x + 5, y + 20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

		char s[256];
		sprintf_s(s, "Src %d", i);
		
		//resize(labeling[i - 1].mat, labeling[i - 1].mat, cv::Size(), 5, 5);
		//cv::imshow(s, labeling[i - 1].mat );
	}

	//cv::imshow("Src", src);
	
	//���x�����O�؂���摜����e���v��Matching�Ő����u�L�A�C�R�����o
	for (int i = 1; i < nLab; ++i) {
		if (val[i-1] == 0)continue;
		//�^�R,�C�J�A�A��
		//int = 
		int ret;
		
		//cv::imshow("������", labeling[i - 1].mat );
		
		//���C�J
		//ret = MatMatching(labeling[i - 1].mat, ki_ika, 0.4);
		//���^�R
		//ret = MatMatching(labeling[i - 1].mat, ki_ika, 0.4);
		
		
		//ao�C�J
		//ret = MatMatching(labeling[i - 1].mat, ao_ika, 0.4);

	

		Mat result_img, ao_ika2;

		//resize(labeling[i - 1].mat, labeling[i - 1].mat, cv::Size(), 5, 5);
		//resize(ao_ika2, ao_ika, cv::Size(), 5, 5);


		//�\�[�X�摜�@< �e���v���摜�@�T�C�Y�̏ꍇ�}�b�`���O���Ȃ��B
		if (labeling[i - 1].mat.cols < ao_ika.cols
			|| labeling[i - 1].mat.rows < ao_ika.rows) {
			continue;
		}
		try
		{
			cv::matchTemplate(labeling[i - 1].mat, ao_ika, result_img, CV_TM_CCOEFF_NORMED);
			//dprintf(L"%d %p Xaaa\n", 26, tmp_img);
			cv::Rect roi_rect(0, 0, ao_ika.cols, ao_ika.rows);
			cv::Point max_pt;
			double maxVal;
			cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
			// ���X�R�A�ȉ��̏ꍇ�͏����I��
			if (maxVal < 0.6) {
				//return -1;
			}
			else {
				roi_rect.x = max_pt.x;
				roi_rect.y = max_pt.y;
				//dprintf(L"idx: %d [%d/%d]\n", i, max_pt.x, max_pt.y);
				//std::cout << "(" << max_pt.x << ", " << max_pt.y << "), score=" << maxVal << std::endl;
				// �T�����ʂ̏ꏊ�ɋ�`��`��
				cv::rectangle(labeling[i - 1].mat, roi_rect, cv::Scalar(0, 255, 255), 1);

				char s[555];
				sprintf_s(s, "�}�b�`���O %i", i);
				//cv::imshow(s, labeling[i - 1].mat);
				
				
			}

		}
		catch (cv::Exception& e)
		{
			// ��O���L���b�`������G���[���b�Z�[�W��\��


			//std::cout << e.what() << std::endl;

			printf("�\�[�X[%d/%d]�e���v��[%d/%d]", labeling[i - 1].mat.cols, labeling[i - 1].mat.rows, ao_ika.cols, ao_ika.rows);
		//cv::imshow("��O����1", labeling[i - 1].mat);
		//cv::imshow("��O����2", ao_ika);
			
			continue;
		}

		//cv::imshow("�n�����˃N�Y", labeling[i - 1].mat);
		//cv::imshow("������", ao_ika);

		//���^�R
		//ret = MatMatching(labeling[i - 1].mat, ao_tako, 0.4);


		//wap.x = 
		//wap.stat = 0;//�����ASP����

	}
	
	char s[256];
		sprintf_s(s, "AAAAAASrc ");
	//cv::imshow("gagaga", Dst);
	//cv::waitKey();
	// cv::waitKey(0);
	return 0;
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
	cv::imshow("�{�[�_�[���x�����O", Dst);
	return 0;
}

//�L���v�`����ʂ��炢�낢���͂���֐�
//���ׂĂ̎n�܂�
void splat_capture_analysis(){
	IplImage *ipl;
	Mat mask_image;
	
	
	
	//�}�b�v��ʍ���̃o�E���h�{�b�N�X
	//RECT rc = { 1034, 20, 1426, 132 };
	
	
	
	//�}�b�v���[�h���ǂ����m�F�B
	if (splat_map() == 0) {
		SetWindowText(hwnd, L"�}�b�v���[�h�ł��I�I�I�u�L�A�C�R�����ׂȂ���I");
		
		return;
	}
	SetWindowText(hwnd, L"map����Ȃ���B�u�L�A�C�R����������");
		
	
	
	

	Mat teki_icons_mat;
	
	//�G�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
	teki_icons_mat = screen_mat(cv::Rect(1034, 20, 392, 112));
	
	//�u�L�A�C�R���F���o�A���x�����O����
	//�A���F�̃}�X�N�����
	mask_image = wapons_labeling(teki_icons_mat);
	//mask_image =wapon_SP_Mask(mat_enemy);
	
	Mat output_image;
	teki_icons_mat.copyTo(output_image, mask_image);
	//imshow("Window", output_image);

	//�y�����z�u�L�A�C�R����ʂ̃o�E���h�{�b�N�X
	Mat mikata_icons_mat = screen_mat(cv::Rect(493, 20, 392, 112));
	mask_image = wapons_labeling(mikata_icons_mat);
	

	dilate(mask_image, mask_image, cv::Mat());
	bitwise_not(mask_image, mask_image);
	
	Mat output_image2;
	mikata_icons_mat.copyTo(output_image2, mask_image);
	
	//imshow("�����A�C�R��", output_image2);
	char s[555];
	sprintf_s(s, "��1����");
	//cv::imshow(s, mikata_icons_mat);
	
	if(wapons_tmp[0].data){
		
		try
		{
			Mat result_img;
			cv::matchTemplate(mikata_icons_mat, wapons_tmp[0], result_img, CV_TM_CCOEFF_NORMED);
			//dprintf(L"%d %p Xaaa\n", 26, tmp_img);
			cv::Rect roi_rect(0, 0, wapons_tmp[0].cols, wapons_tmp[0].rows);
			cv::Point max_pt;
			double maxVal;
			cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
			// ���X�R�A�ȉ��̏ꍇ�͏����I��
			if (maxVal < 0.5) {
				//return -1;
			} else {
				roi_rect.x = max_pt.x;
				roi_rect.y = max_pt.y;
				//dprintf(L"idx: %d [%d/%d]\n", i, max_pt.x, max_pt.y);
				//std::cout << "(" << max_pt.x << ", " << max_pt.y << "), score=" << maxVal << std::endl;
				// �T�����ʂ̏ꏊ�ɋ�`��`��
				cv::rectangle(mikata_icons_mat, roi_rect, cv::Scalar(0, 255, 255), 3);

				char s[555];
				sprintf_s(s, "��1����");
				//cv::imshow(s, mikata_icons_mat);
			}

		}
		catch (cv::Exception& e)
		{
			
		}
	}
	//�}�X�Nmat���烉�x�����O�@
	//splat_labeling(teki_icons_mat, mask_image);
	
	/*
	
	WAPON_ICON 
	
	wapon_icon;
	
	mat.copyTo(output_image, mask_image);
	//dprintf(L"OUT1 size:%d %d\n", output_image.cols, output_image.rows);
	resize(output_image, output_image, cv::Size(), BIT_BMP_WIDTH / mat.cols, BIT_BMP_HEIGHT / mat.rows);
	//imshow("Window", output_image);
	
	
	//�u�L�A�C�R���e���v���[�g�}�b�`���O
	Live_();
	
	
	//�f�X�A�C�R���e���v���[�g�}�b�`���O�B
	int deat_num = Death_(enemy_icons, );
	
	//������X���W�l�Ń\�[�g����A�R���Ŋe�A�C�R���������牽�ڂ����킩��c
	sort(enemy_icons.begin(), enemy_icons.end());
	*/
	
	
	
}

int teki_dead_num_prev;
int mikata_dead_num_prev;
//�G�A�����̃f�X�����Ƃ��̌��m�݂̂̊ȈՏ����̊֐��B
void splat_death(){
	IplImage *ipl;
	Mat mask_image;
	
	right_labeling_center.clear();
	left_labeling_center.clear();
	//�}�b�v���[�h���ǂ����m�F�B
	if (splat_map() == 0) {
		SetWindowText(hwnd, L"�}�b�v���[�h�ł��I�I�I�u�L�A�C�R�����ׂȂ���I");
		
		/*
	mikata_icon[0].batu_;
	mikata_icon[1].update(left_labeling_center[2].stat);
	mikata_icon[2].update(left_labeling_center[1].stat);
	mikata_icon[3].update(left_labeling_center[0].stat);
	
	teki_icon[0].update(right_labeling_center[0].stat);
	teki_icon[1].update(right_labeling_center[1].stat);
	teki_icon[2].update(right_labeling_center[2].stat);
	teki_icon[3].update(right_labeling_center[3].stat);
		*/
		
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
	
	cv::Rect teki_wap[4];
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
			  }
			  else {
				  teki_dead_num++;
			  }
              roi_rect.x = max_pt.x;
              roi_rect.y = max_pt.y;
          	teki_wap[teki_dead_num-1] =  roi_rect;
          		dprintf(L"idx: %d [%d/%d]\n", i, max_pt.x, max_pt.y);
              // �T�����ʂ̏ꏊ�ɋ�`��`��
              cv::rectangle(output_image, roi_rect, cv::Scalar(0,255,255), 3);


				 
			 // roi_rect.x = max_pt.x;
			 // roi_rect.y = max_pt.y;

			 // roi_rect.x = roi_rect.y = 0;

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
	


	//bitwise_not(m1, m1);
	//
		//bitwise_or(mask_image, m1, m2);

	

	//border_mat, m1

	//resize(border_mat, border_mat, cv::Size(), teki_icons_mat.cols/BIT_BMP_WIDTH  , teki_icons_mat.rows/BIT_BMP_HEIGHT);
	//resize(m1, border_mat, cv::Size(), teki_icons_mat.cols / BIT_BMP_WIDTH, teki_icons_mat.rows / BIT_BMP_HEIGHT);

	Mat output_image3;
	border_mat.copyTo(output_image3, m1);

	imshow("�{�[�_�[", output_image3);

	border_labeling(border_mat, m1);

	
	
	//�\�[�g�J�n�B�A�C�R���̊��S�ɏ󋵔c��
	sort(left_labeling_center.begin(), left_labeling_center.end());

	
//printf("L[%d] R[%d]\n", left_labeling_center.size(), right_labeling_center.size());
	sort(right_labeling_center.begin(), right_labeling_center.end());
	
	if(left_labeling_center.size() + right_labeling_center.size() >= 8){
	//	printf("[%d][%d][%d][%d] [%d][%d][%d][%d]\n", left_labeling_center[3].stat,left_labeling_center[2].stat,left_labeling_center[1].stat,left_labeling_center[0].stat,
	//	right_labeling_center[0].stat,right_labeling_center[1].stat,right_labeling_center[2].stat,right_labeling_center[3].stat);
	
	
	
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
			/*
		  if (teki_dead_num == 0) {
			  matt =Scalar(255,255,255);
		  }else if (teki_dead_num == 1) {
				matt =Scalar(00,0,255);
		  }else if (teki_dead_num == 2) {
				matt =Scalar(0,255,0);
		  }else if (teki_dead_num == 3) {
				matt =Scalar(255,0,0);
		}else if (teki_dead_num == 4) {
				matt =Scalar(0,0,0);
		}*/
		
		
		
		
		
		
		//cv::rectangle(matt, cv::Rect(0, 0, 320, 768), cv::Scalar(0, 255, 0), 2);
		

			Vec3b bgr;
			bgr = cv::Vec3b(255, 0, 0);


			if (teki_dead_num == 0) {
				bgr = cv::Vec3b(255, 255, 255);
			}
			else if (teki_dead_num == 1) {
				bgr = cv::Vec3b(0, 0,255);
			}
			else if (teki_dead_num == 2) {
				bgr = cv::Vec3b(0,255,  0);
			}
			else if (teki_dead_num == 3) {
				bgr = cv::Vec3b(255, 0, 0);
			}
			else if (teki_dead_num == 4) {
				bgr = cv::Vec3b(0, 0, 0);
				
			}
		
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
			
			sprintf_s(s, "%.2f", du);
			
			
			
			
			double time_rate = teki_icon[i].getStatTime() / 35000 * 768;
			
			//time_rate = 768  �� 0-768, 768
			//            384     �� 384-768, 384
							
			
			
			//time_rate = 20000 - time_rate;
			//cv::rectangle(matt, cv::Rect(dx, 0, 320, time_rate), cv::Scalar(0, 255, 0), 2, CV_FILLED);
		  
			if (teki_icon[i].stat == 1) {
				cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(45, 45, 45), CV_FILLED);
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
				cv::rectangle(matt, cv::Rect(dx, matt.rows - time_rate, 320, time_rate), cv::Scalar(170, 170, 70), CV_FILLED);
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
				cv::putText(matt, s, cv::Point(dx, 768 -100), cv::FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 0, 0), 5);
				
				sprintf_s(s, "%d", teki_icon[i].death_cnt);
				cv::putText(matt, s, cv::Point(dx, 768 -200), cv::FONT_HERSHEY_SIMPLEX, 5, cv::Scalar(0, 0, 0), 5);
				
				
				sprintf_s(s, "%.2f",( teki_icon[i].live_time_sum/ 1000.0));
				cv::putText(matt, s, cv::Point(dx, 768 -330), cv::FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(0, 0, 127), 5);
				
			}
			
			
		}
		cv::line(matt, Point(0, 0), Point(0, 768), Scalar(0, 00, 0), 5, 8);
		cv::line(matt, Point(320, 0), Point(320, 768), Scalar(0, 0, 0), 5, 8);
		cv::line(matt, Point(640, 0), Point(640, 768), Scalar(0, 00, 0), 5, 8);
		cv::line(matt, Point(960, 0), Point(960, 768), Scalar(0, 00, 0), 5, 8);
		
		
		
		  cv::imshow("�틵", matt);
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
		//splat_capture_analysis();
		//splat_trim_cap();
		//splat_template_maching();
		//debug
		//splat_recode_icons();
		
		cvReleaseImage(&ipl);
		
		cnt++;

		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
	}
	case WM_KEYDOWN: {
		switch (wp) {
		case 'A': {
			mikata_icon[0].reset();
			mikata_icon[1].reset();
			mikata_icon[2].reset();
			mikata_icon[3].reset();

			teki_icon[0].reset();
			teki_icon[1].reset();
			teki_icon[2].reset();

			teki_icon[3].reset();
			//cnt = 0;
			/*
			Mat mtt = screen_mat(cv::Rect(555, 44,  38, 51) );
			
			printf("P[%p", mtt);
			try
			{
				cv::imshow("1wap",mtt);

			}
			catch (cv::Exception& e)
			{

				std::cout << e.what() << std::endl;
			}*/
			break;
		}
		}
		return 0;
	}
	case WM_LBUTTONDOWN: {
		int x = (lp & 0xFFFF);
		int y = ((lp >> 16) & 0xFFFF);

		return 0;
	}
	case WM_MOUSEMOVE: {
		int x = (lp & 0xFFFF);
		int y = ((lp >> 16) & 0xFFFF);

		return 0;
	}
	case WM_LBUTTONUP: {
		int x = (lp & 0xFFFF);
		int y = ((lp >> 16) & 0xFFFF);

		return 0;
	}
	case WM_PAINT: {
		hdc = BeginPaint(hwnd, &ps);

		/*
		SetDIBitsToDevice(
			hdc, 0, 0,
			bmpInfo.bmiHeader.biWidth, bmpInfo.bmiHeader.biHeight,
			0, 0, 0, bmpInfo.bmiHeader.biHeight,
			lpPixel, &bmpInfo, DIB_PAL_COLORS);
*/
		SetDIBitsToDevice(
			hdc, 0, 0,
			trim_bi.bmiHeader.biWidth, trim_bi.bmiHeader.biHeight,
			0, 0, 0, trim_bi.bmiHeader.biHeight,
			trim_pix, &trim_bi, DIB_PAL_COLORS);

		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_DESTROY: {

		//����lpPixel���������ׂ��炸
		DeleteDC(hMemDC);
		DeleteObject(hBitmap);  //BMP���폜�������AlpPixel�������I�ɉ�������
		PostQuitMessage(0);
		return 0;
	}
#pragma warning(disable:4996)
	case WM_CREATE: {
		SetTimer(hwnd, 2525, 1, NULL);
		if (!::AttachConsole(ATTACH_PARENT_PROCESS)) {
			::AllocConsole();           // �R�}���h�v�����v�g����̋N������Ȃ����͐V���ɊJ��
		}
		freopen("CON", "r", stdin);     // �W�����͂̊��蓖��
		freopen("CON", "w", stdout);    // �W���o�͂̊��蓖��


		printf("input no ... \n");
		//int num;
		//scanf("%d", &num);
		//std::cout << num << std::endl;
		std::cerr << "Hahaha!" << std::endl;
		//labeling();

		RECT rc;
		desktop = GetDesktopWindow();
		GetWindowRect(desktop, &rc);
		dsk_width = rc.right;
		dsk_height = rc.bottom;
tmp_img = cv::imread("E:\\scan5\\splat_temp.bmp", 1);
if (!tmp_img.data) dprintf(L"��߂˂�������������������������������");

BATSU_TMP_IMG = cv::imread("E:\\scan5\\map_batu.bmp", 1);
if (!BATSU_TMP_IMG.data) dprintf(L"��߂˂�������������������������������");
		

		ki_ika = cv::imread("E:\\scan5\\ki_ika_l.bmp", 1);
		if (!ki_ika.data) dprintf(L"��߂˂�������������������������������");

		ki_tako = cv::imread("E:\\scan5\\ki_tako_l.bmp", 1);
		if (!ki_tako.data) dprintf(L"��߂˂�������������������������������");

		ao_ika = cv::imread("E:\\scan5\\ao_ika_l.bmp", 1);
		if (!ao_ika.data) dprintf(L"��߂˂�������������������������������");

		ao_tako = cv::imread("E:\\scan5\\ao_tako_l.bmp", 1);
		if (!ao_tako.data) dprintf(L"��߂˂�������������������������������");

		
		
		
		
		
		
		
		
		ki_batsu = cv::imread("E:\\scan5\\ki_batsu.bmp", 1);
		if (!ki_batsu.data) dprintf(L"��߂˂�������������������������������");

		ao_batsu = cv::imread("E:\\scan5\\ao_batsu.bmp", 1);
		if (!ao_batsu.data) dprintf(L"��߂˂�������������������������������");
		
		siro_batsu = cv::imread("E:\\scan5\\siro_batsu.png", 1);
		if (!siro_batsu.data) dprintf(L"��߂˂�������������������������������");

		
		
		
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

		//cvtColor(teki_batsu_mask, teki_batsu_mask, CV_GRAY2BGR);
		//cvtColor(mika_batsu_mask, mika_batsu_mask, CV_GRAY2BGR);
		
		
		cvtColor(mika_batsu_mask, mika_batsu_mask, CV_BGR2GRAY);
		cvtColor(teki_batsu_mask, teki_batsu_mask, CV_BGR2GRAY);
		//teki_batsu_mask.convertTo(teki_batsu_mask, CV_8UC1);
		
		TCHAR s[555];
		_stprintf_s(s,L"�^�C�v: %d", teki_batsu_mask.type());
		
		
		
		
		
		//MessageBox(NULL, s,TEXT("���b�Z�[�W�{�b�N�X"), MB_OK);
		
		
		
		
		SetWindowPos(
			hwnd, HWND_TOPMOST,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE
		);
		
		
		
		
		//DIB�̏���ݒ肷��
		
		bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth = dsk_width;
		bmpInfo.bmiHeader.biHeight = dsk_height;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 24;
		bmpInfo.bmiHeader.biCompression = BI_RGB;

		//�n�t�ϊ���ɒ������m�̌�_���m�F���邽�߂�8�r�b�g�r�b�g�}�b�v������
		//DIB�̏���ݒ肷��
	

		//DIBSection�쐬
		hdc = GetDC(hwnd);
		hBitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, (void**)&lpPixel, NULL, 0);
		hMemDC = CreateCompatibleDC(hdc);
		SelectObject(hMemDC, hBitmap);
		ReleaseDC(hwnd, hdc);
		
		//for(int i = 0 ; i < 8; i++)
		//	base_wapon[i].n = i;

		//memcpy(&srcBit_Info, &bmpInfo, sizeof(BITMAPINFO));
		//pix = (BYTE *)malloc(srcBit_Info.bmiHeader.biWidth * srcBit_Info.bmiHeader.biHeight * 4);

		//BitBlt(hdc, 0, 0, dsk_width, dsk_height, hMemDC, 0, 0, SRCCOPY);
		ShellExecute(NULL, L"open", L"C:\\Users\\kento\\Documents\\Visual Studio 2015\\Projects\\opencv_base_GUI\\x64\\Release\\hsptmp.exe", NULL, NULL, SW_SHOWNORMAL);

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


	hwnd = CreateWindow(pClassName, TEXT("�^�C�g��"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
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
/*

*/
