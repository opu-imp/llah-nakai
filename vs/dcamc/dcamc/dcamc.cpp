#define	GLOBAL_DEFINE	/* extern�ϐ��ɂ����Ŏ��̂�^���� */

#include "def_general.h"	// proctime�p
#include <stdio.h>
#include <winsock2.h>	// socket
#include <cv.h>			// OpneCV
#include <highgui.h>	// OpenCV
#include <windows.h>
#include <mmsystem.h>
#include <dshow.h>
#include <conio.h>
#include <qedit.h>
#include <string.h>
#define _CRT_SECURE_NO_WARNINGS		//for use of string.h wihout error

#include "extern.h"
#include "ws_cl.h"
#include "dirs.h"
#include "proctime.h"
#include "dscap.h"
#include "auto_connect.h"
#include "proj4p.h"
#include "dcamc.h"
#include "nimg.h"
#include "ar.h"
#include "init.h"
#include "fptune.h"
#include "camharris.h"

// �}�E�X�̃R�[���o�b�N�֐��p�̃O���[�o���ϐ�
IplImage *g_thumb = NULL, *g_thumb_draw;
double g_ratio = 0.0;
int g_paste_complete = 0;
CvPoint g_p1, g_p2;

int LoadPointFile( char *fname, CvPoint **ps0, CvSize *size );

int SendPoints( SOCKET sock, CvPoint *ps, int num, CvSize *size, struct sockaddr_in *addr, int ptc )
// �_�f�[�^�𑗐M
{
	int i, num_send, buff_cur;
	char buff[kSendBuffSize];

	// �摜�T�C�Y���o�b�t�@�Ɋi�[
	int j=0;
	for ( i = 0, buff_cur = 0; i < sizeof(CvSize); i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)size)[i];
	}
	// �����_�����o�b�t�@�Ɋi�[
	num_send = (num > kMaxPointNumServer) ? kMaxPointNumServer : num;	// �����_�����T�[�o���̍ő�l�𒴂���ꍇ�͐���
	for ( i = 0; i < sizeof(int); i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)&num_send)[i];
	}
	// �����_�f�[�^���o�b�t�@�Ɋi�[
	for ( i = 0; i < (int)sizeof(CvPoint) * num_send; i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)ps)[i];
	}
	// �o�b�t�@���܂Ƃ߂đ��M�i���̒ʐM�ōςށj
	if ( ptc == kTCP ) {
		send( sock, buff, kSendBuffSize, SEND_FLAG );
	}
	else {
		sendto( sock, buff, kSendBuffSize, SEND_FLAG, (struct sockaddr *)addr, sizeof(*addr) );
	}

	return 1;
}

int SendPointsAreas( SOCKET sock, CvPoint *ps, double *areas, int num, CvSize *size, struct sockaddr_in *addr, int ptc )
// �_�f�[�^�𑗐M
// �ʐς����M
{
	int i, num_send, buff_cur;
	char buff[kSendBuffSizeAreas];
	unsigned short areas_us[kMaxPointNumServer];

	// �摜�T�C�Y���o�b�t�@�Ɋi�[
	int j=0;
	for ( i = 0, buff_cur = 0; i < sizeof(CvSize); i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)size)[i];
	}
	// �����_�����o�b�t�@�Ɋi�[
	num_send = (num > kMaxPointNumServer) ? kMaxPointNumServer : num;	// �����_�����T�[�o���̍ő�l�𒴂���ꍇ�͐���
	for ( i = 0; i < sizeof(int); i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)&num_send)[i];
	}
	// �����_�f�[�^���o�b�t�@�Ɋi�[
	for ( i = 0; i < (int)sizeof(CvPoint) * num_send; i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)ps)[i];
	}
	// �ʐς�unsigned short�ɒ����Ċi�[
	for ( i = 0; i < num_send; i++ ) {
		areas_us[i] = (unsigned short)areas[i];
		// printf("%lf\n", areas[i]);
	}
	buff_cur = sizeof(CvSize) + sizeof(int) + sizeof(CvPoint) * kMaxPointNumServer;
	for ( i = 0; i < (int)sizeof(unsigned short) * num_send; i++, buff_cur++ ) {
		buff[buff_cur] = ((char *)areas_us)[i];
	}
	// �o�b�t�@���܂Ƃ߂đ��M�i���̒ʐM�ōςށj
	if ( ptc == kTCP ) {
		send( sock, buff, kSendBuffSizeAreas, SEND_FLAG );
	}
	else {
		sendto( sock, buff, kSendBuffSizeAreas, SEND_FLAG, (struct sockaddr *)addr, sizeof(*addr) );
	}

	return 1;
}

int SendImage( SOCKET sock, IplImage *img )
// �摜�𑗐M
{
	int ret;
	CvSize size;
	
	size.width = img->width;
	size.height = img->height;
	send( sock, (const char *)&size, sizeof(CvSize), SEND_FLAG );	// �T�C�Y�𑗐M
	send( sock, (const char *)&(img->nChannels), sizeof(int), SEND_FLAG );	// �`���l���𑗐M
	send( sock, (const char *)&(img->depth), sizeof(int), SEND_FLAG );	// �r�b�g���𑗐M
	ret = send( sock, (const char *)(img->imageData), img->imageSize, SEND_FLAG );	// �摜�{�̂𑗐M
	
	return 1;
}

int SendComSetting( SOCKET sock, int ptc, int pt_port, int res_port, char *cl_name )
// �ʐM�ݒ�𑗐M
{
	send( sock, (const char *)&ptc, sizeof(int), SEND_FLAG );
	send( sock, (const char *)&pt_port, sizeof(int), SEND_FLAG );
	send( sock, (const char *)&res_port, sizeof(int), SEND_FLAG );
	send( sock, (const char *)cl_name, kMaxNameLen, SEND_FLAG );

	return 1;
}

int RecvResult( SOCKET sock, char *doc_name, int len )
// ����������M
{
	return recv( sock, doc_name, len, RECV_FLAG );
}

int RecvFileNameList( SOCKET sock, int *doc_num, char ***fname_list0, int len )
{
	int i, ret;
	char **fname_list = NULL;

	ret = recv( sock, (char *)doc_num, sizeof(int), RECV_FLAG );
	fname_list = (char **)calloc( *doc_num, sizeof(char *) );
	for ( i = 0; i < *doc_num; i++ ) {
		fname_list[i] = (char *)calloc( len, sizeof(char) );
		ret = recv( sock, fname_list[i], len, RECV_FLAG );
	}
	*fname_list0 = fname_list;

	return ret;
}

int RecvResultCor( SOCKET sock, char *doc_name, int len, CvPoint corps[][2], int *corpsnum, CvSize *query_size, CvSize *reg_size )
// �������ƑΉ��_����M
{
	int i, ret;
	
	ret = recv( sock, doc_name, len, RECV_FLAG );	// ����������M
	puts(doc_name);
	ret = recv( sock, (char *)query_size, sizeof(CvSize), RECV_FLAG );	// �N�G���摜�̃T�C�Y����M
//	printf("%d,%d\n", query_size->width, query_size->height);
	ret = recv( sock, (char *)reg_size, sizeof(CvSize), RECV_FLAG );	// �o�^�摜�̃T�C�Y����M
	ret = recv( sock, (char *)corpsnum, sizeof(int), RECV_FLAG );	// �Ή��̐�����M
//	printf("%d\n", *corpsnum );
	if ( *corpsnum > 0 )	ret = recv( sock, (char *)corps, sizeof(CvPoint)*2*(*corpsnum), SEND_FLAG );
	for ( i = 0; i < *corpsnum; i++ ) {
//		ret = recv( sock, (char *)&(corps[i][0]), sizeof(CvPoint), RECV_FLAG );
//		ret = recv( sock, (char *)&(corps[i][1]), sizeof(CvPoint), RECV_FLAG );
//		printf("%d : (%d, %d), (%d, %d)\n", i, corps[i][0].x, corps[i][0].y, corps[i][1].x, corps[i][1].y);
	}
	return ret;
}

int RecvResultParam( SOCKET sock, char *doc_name, int len, strProjParam *param, CvSize *img_size )
// �������Ǝˉe�ϊ��p�����[�^����M
{
	int ret;
	char buff[kRecvBuffSize];

	ret = recv( sock, buff, kRecvBuffSize, RECV_FLAG );
	memcpy( doc_name, buff, kMaxDocNameLen );
	memcpy( param, buff + kMaxDocNameLen, sizeof(strProjParam) );
	memcpy( img_size, buff + kMaxDocNameLen + sizeof(strProjParam), sizeof(CvSize) );

	return ret;

#if 0

	ret = recv( sock, doc_name, len, RECV_FLAG );
	ret = recv( sock, (char *)param, sizeof(strProjParam), RECV_FLAG );
	
	return ret;
#endif
}

void Buff2ImageData( unsigned char *buff, IplImage *img )
{
	int i;

	for ( i = 0; i < img->height; i++ ) {
		memcpy( &(img->imageData[img->widthStep*(img->height - i - 1)]), &(buff[img->width*3*i]), img->width*3 );
	}
}

void ReleaseCentres( CvPoint *ps )
{
	free( ps );
}

#define	CENTROID
//#define	SECOND_ORDER_MOMENT
//#define	kMaxNoiseArea	(10)
#define	kMaxNoiseArea	(20)	// ���ʂ�΍�

int CalcCentres(CvPoint **ps0, CvSeq *contours, CvSize *size, double **areas0)
// �d�S���v�Z����
// 06/01/12	�ʐς̌v�Z��ǉ�
{
	int i, num;
	double *areas = NULL;
//	double max_area = 0.0L;	// �A�������̖ʐς̍ő�l�𒲂ׂ�p
	CvSeq *con0;
	CvMoments mom;
	CvPoint *ps;

	// �_�̐����J�E���g�i�b��j
	for ( i = 0, con0 = contours; con0 != 0; con0 = con0->h_next, i++ );
	num = ( i >= kMaxPointNum ) ? kMaxPointNum - 1 : i;	// �S�~���ŘA����������������Ƃ��i�{���͑傫���𒲂ׂ��肷�ׂ��j
	// �_������z����m�ہinum�̒l�͐��m�ł͂Ȃ����C�傫�߂Ƃ������ƂŁj
	ps = (CvPoint *)calloc(num, sizeof(CvPoint));
	*ps0 = ps;
	// �ʐς�����z����m��
	if ( areas0 ) {	// �ʐϔz��̃|�C���^���^�����Ă���
		areas = (double *)calloc(num, sizeof(double));
		*areas0 = areas;
	}
	// �A��������`��E�d�S���v�Z
    for( i = 0, con0 = contours; con0 != 0 && i < num ; con0 = con0->h_next )
    {
		double d00;
		cvMoments(con0, &mom, 1);
		d00 = cvGetSpatialMoment( &mom, 0, 0 );
		if ( d00 < kMaxNoiseArea ) continue;	// ����������A�������͏��O

#ifdef	CENTROID
		ps[i].x = (int)(cvGetSpatialMoment( &mom, 1, 0 ) / d00);
		ps[i].y = (int)(cvGetSpatialMoment( &mom, 0, 1 ) / d00);
#endif
#ifdef	SECOND_ORDER_MOMENT
		// 07/05/30
		// �Ȃ��������ɂ��Ă����̂�������Ȃ��B�����Y�ꂩ�H
		// �ǂ���ɂ��悠�܂�e���͗^���Ă��Ȃ������i2���̍���1�s�N�Z���ɂ������Ȃ��j�B
//		printf( "%f + %f, %f + %f\n", cvGetSpatialMoment( &mom, 1, 0 ) / d00, cvGetNormalizedCentralMoment( &mom, 2, 0 ), cvGetSpatialMoment( &mom, 0, 1 ) / d00, cvGetNormalizedCentralMoment( &mom, 0, 2 ) );
		ps[i].x = (int)(cvGetSpatialMoment( &mom, 1, 0 ) / d00 + cvGetNormalizedCentralMoment( &mom, 2, 0 ) );
		ps[i].y = (int)(cvGetSpatialMoment( &mom, 0, 1 ) / d00 + cvGetNormalizedCentralMoment( &mom, 0, 2 ) );
#endif
		if ( areas ) areas[i] = d00;
//		if ( d00 > max_area ) max_area = d00;
		// 130����f�ŒP��̏d�S�𓾂�ꍇ�ɂ�unsigned short�̍ő�l�i65535�j�𒴂��邱�Ƃ͂Ȃ��������B
		// �A���A�𑜓x���オ������Ώۂ��ς�����肵���ꍇ�ɂ͂��̌���ł͂Ȃ��B
		i++;
    }
	num = i;	// �A���������̍X�V
//	printf("max area: %lf\n", max_area );

	return num;
}

int CalcFloatCentres(CvPoint2D32f **pfs0, CvSeq *contours, CvSize *size, double **areas0)
// �d�S���v�Z����ifloat�Łj
// 06/01/12	�ʐς̌v�Z��ǉ�
{
	int i, num;
	CvSeq *con0;
	CvMoments mom;
	CvPoint2D32f *pfs;

	// �_�̐����J�E���g�i�b��j
	for ( i = 0, con0 = contours; con0 != 0; con0 = con0->h_next, i++ );
	num = ( i >= kMaxPointNum ) ? kMaxPointNum - 1 : i;	// �S�~���ŘA����������������Ƃ��i�{���͑傫���𒲂ׂ��肷�ׂ��j
	// �_������z����m�ہinum�̒l�͐��m�ł͂Ȃ����C�傫�߂Ƃ������ƂŁj
	pfs = (CvPoint2D32f *)calloc(num, sizeof(CvPoint2D32f));
	*pfs0 = pfs;
	// �A��������`��E�d�S���v�Z
    for( i = 0, con0 = contours; con0 != 0 && i < num ; con0 = con0->h_next )
    {
		double d00;
		cvMoments(con0, &mom, 1);
		d00 = cvGetSpatialMoment( &mom, 0, 0 );
		if ( d00 < kMaxNoiseArea ) continue;	// ����������A�������͏��O

#ifdef	CENTROID
		pfs[i].x = (float)(cvGetSpatialMoment( &mom, 1, 0 ) / d00);
		pfs[i].y = (float)(cvGetSpatialMoment( &mom, 0, 1 ) / d00);
#endif
#ifdef	SECOND_ORDER_MOMENT
		pfs[i].x = (float)(cvGetSpatialMoment( &mom, 1, 0 ) / d00 + cvGetNormalizedCentralMoment( &mom, 2, 0 ) );
		pfs[i].y = (float)(cvGetSpatialMoment( &mom, 0, 1 ) / d00 + cvGetNormalizedCentralMoment( &mom, 0, 2 ) );
#endif
		i++;
    }
	num = i;	// �A���������̍X�V

	return num;
}

int MakeCentresFromImage(CvPoint **ps, IplImage *img, CvSize *size, double **areas)
// �摜�̃t�@�C������^���ďd�S���v�Z���C�d�S�̐���Ԃ�
// 06/01/12	�ʐς̌v�Z��ǉ�
{
	int num, ret;
    CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq *contours = 0;

	size->width = img->width;
	size->height = img->height;
	
    ret = cvFindContours( img, storage, &contours, sizeof(CvContour),
		CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );	// �A�������𒊏o����
	if ( ret <= 0 ) {	// �A��������������Ȃ�����
		cvReleaseMemStorage( &storage );
		return 0;
	}
	num = CalcCentres(ps, contours, size, areas);	// �e�A�������̏d�S���v�Z����

	cvClearSeq( contours );
	cvReleaseMemStorage( &storage );

	return num;
}

int MakeFloatCentresFromImage(CvPoint2D32f **pfs, IplImage *img, CvSize *size, double **areas)
// �摜�̃t�@�C������^���ďd�S���v�Z���C�d�S�̐���Ԃ�
// 06/01/12	�ʐς̌v�Z��ǉ�
{
	int num, ret;
    CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq *contours = 0;

	size->width = img->width;
	size->height = img->height;
	
    ret = cvFindContours( img, storage, &contours, sizeof(CvContour),
		CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );	// �A�������𒊏o����
	if ( ret <= 0 ) {	// �A��������������Ȃ�����
		cvReleaseMemStorage( &storage );
		return 0;
	}
	num = CalcFloatCentres(pfs, contours, size, areas);	// �e�A�������̏d�S���v�Z����

	cvClearSeq( contours );
	cvReleaseMemStorage( &storage );

	return num;
}

#define	kDrawPointsHMargin	(100)
#define	kDrawPointsVMargin	(100)
#define	kDrawPointsRectThick	(4)
#define	kDrawPointsPtRad	(4)

void DrawPoints2( CvPoint *ps, int num, CvSize img_size )
// �����_��`�悵�C�ۑ�����
{
	int i;
	IplImage *img;

	img = cvCreateImage( cvSize( kDrawPointsHMargin*2+img_size.width, kDrawPointsVMargin*2+img_size.height ), IPL_DEPTH_8U, 3 );
	cvSet( img, cWhite, NULL );	// ���œh��Ԃ�
	// �g��`��
	cvRectangle( img, cvPoint( kDrawPointsHMargin, kDrawPointsVMargin ), \
		cvPoint( kDrawPointsHMargin + img_size.width, kDrawPointsVMargin + img_size.height ), \
		cBlack, kDrawPointsRectThick, CV_AA, 0 );
	// �����_��`��
	for ( i = 0; i < num; i++ ) {
		cvCircle( img, cvPoint( kDrawPointsHMargin + ps[i].x, kDrawPointsVMargin + ps[i].y ), kDrawPointsPtRad, cBlack, -1, CV_AA, 0 );
	}
	OutPutImage( img );
	cvReleaseImage( &img );
}

IplImage *DrawPoints( CvPoint *ps, int num, CvSize *img_size )
// �����_��`�悷��
{
	int i;
	IplImage *img_pt;
	
	img_pt = cvCreateImage( *img_size, IPL_DEPTH_8U, 1 );
	cvZero( img_pt );
	for ( i = 0; i < num; i++ ) {
		cvCircle( img_pt, ps[i], 3, cWhite, -1, CV_AA, 0 );
	}
	
	return img_pt;
}

IplImage *DrawPointsOverlap( CvPoint *ps, int num, CvSize *img_size, IplImage *img )
// �����_��`�悷��
{
	int i;
	IplImage *img_pt;
	
	img_pt = cvCloneImage( img );
	for ( i = 0; i < num; i++ ) {
		cvCircle( img_pt, ps[i], 3, cRed, -1, CV_AA, 0 );
	}
	
	return img_pt;
}

int OutPutImage(IplImage *img)
// �摜���t�@�C���ɕۑ�����D�t�@�C�����͎����I�ɘA�ԂɂȂ�
{
	static int n = 0;
	char filename[kFileNameLen];

	sprintf(filename, "output%03d.jpg", n++);
	return cvSaveImage(filename, img);
}

void DrawCor( CvPoint corps[][2], int corpsnum, IplImage *img_cap, char *doc_name )
// �Ή��_�̕`��
{
	int i;
	char thumb_fname[kMaxPathLen];
	IplImage *query_small, *reg_small, *cor_img;
	CvSize qs, rs, cor_size;
	CvPoint p1, p2;
	
	sprintf( thumb_fname, "\\\\leo\\nakai\\didb\\thumb\\%s.bmp", doc_name );
	printf("%s\n", thumb_fname );
	if ( doc_name[0] == '\0')	return;
	reg_small = cvLoadImage( thumb_fname, 1 );
	
	qs.width = (int)(img_cap->width / 2);
	qs.height = (int)(img_cap->height / 2);
	rs.width = reg_small->width;
	rs.height = reg_small->height;
	
	cor_size.width = qs.width + rs.width;
	cor_size.height = (qs.height > rs.height) ? qs.height : rs.height;
	
	cor_img = cvCreateImage( cor_size, IPL_DEPTH_8U, 3 );
	for ( i = 0; i < corpsnum; i++ ) {
		p1.x = (int)(corps[i][0].x / 2);
		p1.y = (int)(corps[i][0].y / 2);
		p2.x = (int)(corps[i][1].x / 4) + qs.width;
		p2.y = (int)(corps[i][1].y / 4);
		cvLine( cor_img, p1, p2, cWhite, 1, CV_AA, 0 );
	}
	cvShowImage( "Corres", cor_img );
	cvReleaseImage( &reg_small );
	cvReleaseImage( &cor_img );
}

void DrawSash( IplImage *img, strProjParam *param )
// �g��`��
{
	int i;
	strPoint p1[5], p2[5];


	p1[0].x = 0;
	p1[0].y = 0;
	p1[1].x = 1700;
	p1[1].y = 0;
	p1[2].x = 1700;
	p1[2].y = 2200;
	p1[3].x = 0;
	p1[3].y = 2200;
	p1[4].x = 0;
	p1[4].y = 0;
/*	
	p1[0].x = 1000;
	p1[0].y = 1000;
	p1[1].x = 1200;
	p1[1].y = 1000;
	p1[2].x = 1200;
	p1[2].y = 1200;
	p1[3].x = 1000;
	p1[3].y = 1200;
	p1[4].x = 1000;
	p1[4].y = 1000;
*/
	for ( i = 0; i < 5; i++ )	ProjTrans( &(p1[i]), &(p2[i]), param );
	for ( i = 0; i < 4; i++ ) {
//		printf("(%d, %d)\n", p2[i].x, p2[i].y);
		cvLine( img, cvPoint( p2[i].x, p2[i].y ), cvPoint( p2[i+1].x, p2[i+1].y ), cRed, 5, CV_AA, 0 );
	}
}

int IsEqualParam( strProjParam param1, strProjParam param2 )
{
	if ( param1.a1 == param2.a1 &&
	param1.a1 == param2.a1 &&
	param1.a2 == param2.a2 &&
	param1.a3 == param2.a3 &&
	param1.b1 == param2.b1 &&
	param1.b2 == param2.b2 &&
	param1.b3 == param2.b3 &&
	param1.c1 == param2.c1 &&
	param1.c2 == param2.c2 )	return 1;
	else	return 0;
}
	
int main( int argc, char *argv[] )
{
	SOCKET sock, sockpt, sockres;
	int i, num = 0, num0 = 0, key, ret, corpsnum, ar, draw_fp = -1, draw_fp_overlap = -1, draw_con = -1, draw_con_overlap = -1;
	unsigned char *img_buff;
	char doc_name[kMaxDocNameLen] = "", doc_name_prev[kMaxDocNameLen] = "invalid";
	long buff_size;
	TIME_COUNT start, end, start_cap, end_cap, start_show, end_show, start_connect, end_connect, start_conv, end_conv, start_ret, end_ret, start_fp, end_fp, start_com, end_com, start_loop, end_loop;
	
	CvPoint *ps = NULL, *ps0 = NULL, corps[kMaxPointNum][2], ar_p1, ar_p2, point;
	double *areas = NULL, *areas0 = NULL;
	CvSize img_size, query_size, reg_size, thumb_size, merge_size, small_img_size, small_thumb_size, res_size, resized_size;
	strDirectShowCap dsc;
	IplImage *img_cap = NULL, *img_pt = NULL, *img_cap0 = NULL, *tmp = NULL, *thumb = NULL, *thumb_draw = NULL, *merge = NULL, *small_cap, *small_thumb, *clone = NULL, *cap_file, *cap_resized, *img_con = NULL, *img_con0 = NULL, *img_disp = NULL;
	IplImage *flag;
	// �\���摜�֌W
	IplImage *disp_cap, *disp_con;
	CvPoint *disp_ps;
	int disp_num;
	// �t�H�[�J�X�֌W
	CameraControlProperty prop;
	int focus_available, auto_focus_tb, focus_tb;
	long focus, focus_min, focus_max, focus_step, focus_def, focus_flags;
	// �K�E�V�A���t�B���^�̃g���b�N�o�[�֌W
	int gauss_tb;
#ifdef VIDEO_OUTPUT
	CvSize small_pt_size;
	IplImage *small_pt;
#endif
	strProjParam param, zero_param;;
	struct CvVideoWriter *video_wr;	// �r�f�I�o�͗p
	struct sockaddr_in addr;
	CvCapture *cap_mov;	// �r�f�I���͗p
	// �T���l�C���ۑ��p
	char **fname_list = NULL;
	int doc_num = 0;

//	puts("start");
	InterpretArguments( argc, argv );	// ����������
	switch ( eEntireMode ) {
		case CAP_MOVIE_MODE:
			CaptureMovie( eMovieFileName );
			return 0;
		case DECOMPOSE_MOVIE_MODE:
			DecomposeMovie( eMovieFileName );
			return 0;
		case TUNE_FP_MODE:
			TuneFeaturePointParam( eTuneFpRegFileName );
			return 0;
		case CHK_CAM_MODE:
			CheckDirectShowCap();
			return 0;
		case CONV_MOVIE_MODE:
			ConvMovie();
			return 0;
		case CAM_HARRIS_MODE:
			DetectHarrisCam( eCamHarrisRegFileName, eDetectHarrisCamMode );
			return 0;
		case HARRIS_TEST_MODE:
			DetectHarrisTest( eHarrisTestOrigFileName, eHarrisTestAnnoFileName );
			return 0;
	}

	timeBeginPeriod( 1 );//���x��1ms�ɐݒ�
 
//	if ( ReadIniFile() == 0 ) {	// ini�t�@�C���̓ǂݍ��݂Ɏ��s
//		fprintf( stderr, "Error : ReadIniFile\n" );
//		return 1;
//	}
	ReadIniFile();
	// �f���A���n�b�V�����[�h�͕ʊ֐�
	if ( eDualHash ) {
		RetrieveDualHash();
		return 0;
	}
	if ( eEntireMode == CAM_RET_MODE ) {
		ret = InitDirectShowCap( eCamConfNum, &dsc, &(img_size.width), &(img_size.height) );	// USB�J������������
		if ( ret == 0 )	return 1;
	}
	else if ( eEntireMode == INPUT_MOVIE_MODE ) {
		cap_mov = cvCaptureFromFile( eMovieFileName );	// �t�@�C������̃L���v�`��
		if ( cap_mov == NULL )	return 1;	// �L���v�`�����s
		img_size.width = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_WIDTH );	// �T�C�Y�ǂݍ���
		img_size.height = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_HEIGHT );
	}
	buff_size = img_size.width * img_size.height * 3;
	thumb_size = cvSize( 425, 550 );	// �T���l�C���̃T�C�Y
	// �r�f�I�p�̏k���T�C�Y
	small_img_size = cvSize( (int)(img_size.width * kResizeScaleImg), (int)(img_size.height * kResizeScaleImg) );
	small_thumb_size = cvSize( (int)(thumb_size.width * kResizeScaleThumb), (int)(thumb_size.height *kResizeScaleThumb) );
//	merge_size = cvSize( img_size.width + thumb_size.width, max( img_size.height, thumb_size.height ) );
	merge_size = cvSize( small_img_size.width + small_thumb_size.width, max( small_img_size.height, small_thumb_size.height ) );
	// �A�X�y�N�g��ɍ��킹��
	if ( merge_size.width > (int)((double)merge_size.height * kVideoAspectRatio) )
		merge_size.height = (int)((double)merge_size.width / kVideoAspectRatio);
	else if ( merge_size.width < (int)((double)merge_size.height * kVideoAspectRatio) )
		merge_size.width = (int)((double)merge_size.height * kVideoAspectRatio);
	img_buff = (unsigned char *)malloc( buff_size );	// �摜�̃o�b�t�@���m��
	img_cap = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬
	img_cap0 = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬
	if ( eCaptureWidth != 0 )	{	// �L���v�`���E�B���h�E�̕��w�肪�L��
		resized_size.width = eCaptureWidth;
		resized_size.height = (int)((double)img_size.height * ((double)eCaptureWidth / (double)img_size.width));
		cap_resized = cvCreateImage( resized_size, IPL_DEPTH_8U, 3 );
	}
	img_con = cvCreateImage( img_size, IPL_DEPTH_8U, 1 );
	img_con0 = cvCreateImage( img_size, IPL_DEPTH_8U, 1 );
	img_disp = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬
#ifdef	VIDEO_OUTPUT
	small_pt_size.width = 212;
	small_pt_size.height = 159;
	small_cap = cvCreateImage( small_img_size, IPL_DEPTH_8U, 3 );
	small_thumb = cvCreateImage( small_thumb_size, IPL_DEPTH_8U, 3 );
	merge = cvCreateImage( merge_size, IPL_DEPTH_8U, 3 );
	small_pt = cvCreateImage( small_pt_size, IPL_DEPTH_8U, 3 );
#endif
#ifdef	AR
	ar = 1;
#else
	ar = -1;
#endif
	zero_param.a1 = 0.0; zero_param.a2 = 0.0; zero_param.a3 = 0.0; zero_param.b1 = 0.0; zero_param.b2 = 0.0; zero_param.b3 = 0.0; zero_param.c1 = 0.0; zero_param.c2 = 0.0;
	cvNamedWindow( "Capture", CV_WINDOW_AUTOSIZE );	// �E�B���h�E���쐬
//	cvNamedWindow( "Connected", CV_WINDOW_AUTOSIZE );
//	cvNamedWindow( "Points", CV_WINDOW_AUTOSIZE );
#ifdef	DRAW_COR	// �Ή��_�`��
	cvNamedWindow( "Corres", CV_WINDOW_AUTOSIZE );
#else
	cvNamedWindow( "Thumb", CV_WINDOW_AUTOSIZE );
#endif
#ifdef	DRAW_HIST	// �q�X�g�O�����`��
	cvNamedWindow( "Hist", CV_WINDOW_AUTOSIZE );
#endif
#ifdef	DRAW_FLAG
	cvNamedWindow( "Language", CV_WINDOW_AUTOSIZE );
#endif
	////////////////////////// �t�H�[�J�X�̃g���b�N�o�[�֌W //////////////////////////
	prop = CameraControl_Focus;
	focus_available = GetCameraControlRange( dsc.pCameraControl, prop, &focus_min, &focus_max, &focus_step, &focus_def, &focus_flags );
//	// �y���J�����̂��߂Ɉꎞ�I�Ƀt�H�[�J�X�֌W���E���Ă���
//	focus_available = 0;
	if ( focus_available )	focus = GetCameraControlProperty( dsc.pCameraControl, prop, &focus_flags );
	// �擾�����v���p�e�B����g���b�N�o�[���쐬����
	if ( focus_available ) {
//		auto_focus_tb = ( focus_flags == CameraControl_Flags_Auto ) ? 1 : 0;
		focus_tb = (int)(focus - focus_min);
//		cvCreateTrackbar( "Manu/Auto", "Capture", &auto_focus_tb, 1, NULL );
		cvCreateTrackbar( "Focus", "Capture", &focus_tb, focus_max - focus_min, NULL );
	}
	////////////////////////// �K�E�V�A���}�X�N�T�C�Y�̃g���b�N�o�[�֌W //////////////////////////
#ifdef	GAUSS_VARIABLE
	gauss_tb = (int)(eCamGaussMaskSize / 2) + 1;
//	cvCreateTrackbar( "Gauss", "Capture", &gauss_tb, 10, NULL );
	eCamGaussMaskSize = gauss_tb * 2 + 1;
#endif

	if ( eEntireMode == CAM_RET_MODE )
		StartDirectShowCap( &dsc );	// �L���v�`�����J�n
	// �l�S�V�G�C�g�J�n
	sock = InitWinSockClTCP( eTCPPort, eServerName );
	if ( sock == INVALID_SOCKET ) {
		fprintf( stderr, "error: connection failure\n" );
		return 1;
	}
	ret = SendComSetting( sock, eProtocol, ePointPort, eResultPort, eClientName );	// �ʐM�ݒ�𑗐M
	if ( ret < 0 )	return 1;
//	ret = RecvFileNameList( sock, &doc_num, &fname_list, kMaxDocNameLen );
//	if  ( ret < 0 )	return 1;
//	for ( i = 0; i < doc_num; i++ ) {
//		printf( "%d : %s\n", i, fname_list[i] );
//	}
//	return 0;	// �Ƃ肠�����I��

	if ( eProtocol == kUDP ) {	// UDP
		sockpt = InitWinSockClUDP( ePointPort, eServerName, &addr );
		sockres = InitWinSockSvUDP( eResultPort );
	}

#ifdef	VIDEO_OUTPUT
	video_wr = cvCreateVideoWriter( kVideoFileName, -1, kVideoFps, merge_size, 1 );	// �r�f�I�o��
#endif
	start = GetProcTimeMiliSec();
	for ( ; ; ) {
		start_loop = GetProcTimeMiliSec();
		if ( eEntireMode == CAM_RET_MODE ) {
			// �L���v�`��
			start_cap = GetProcTimeMiliSec();
			CaptureDirectShowCap( &dsc, img_buff, buff_size );
			end_cap = GetProcTimeMiliSec();
			// �o�b�t�@���摜�ɕϊ�
			start_conv = GetProcTimeMiliSec();
			Buff2ImageData( img_buff, img_cap );
			end_conv = GetProcTimeMiliSec();
		}
		else if ( eEntireMode == INPUT_MOVIE_MODE ) {
			start_cap = GetProcTimeMiliSec();
			cap_file = cvQueryFrame( cap_mov );
			end_cap = GetProcTimeMiliSec();
			if ( cap_file == NULL )	goto end_cap;
			CopyImageData( cap_file, img_cap );
		}
		// �����摜���쐬
//		start_fp = GetProcTimeMiliSec();
//		start_connect = GetProcTimeMiliSec();
		if ( eIsJp )	img_con = GetConnectedImageCamJp2( img_cap );
		else			GetConnectedImageCam( img_cap, img_con, eCamGaussMaskSize );
//		end_connect = GetProcTimeMiliSec();
		if ( ps0 != NULL && num0 > 0 )	ReleaseCentres( ps0 );
		if ( areas0 != NULL && num0 > 0 )	free( areas0 );
		ps0 = ps;
		areas0 = areas;
		num0 = num;
		// �t�H�[�J�X�A���K�E�V�A���p�����[�^
//		if ( eFocusCoupledGaussian )	eCamGaussMaskSize = (int)(0.0652 * (double)focus_tb - 2)*2+1;	// �P��̏ꍇ
		if ( eFocusCoupledGaussian )	eCamGaussMaskSize = (int)(0.0326 * (double)focus_tb - 2)*2+1;	// �����̏ꍇ
		if ( eCamGaussMaskSize < 1 )	eCamGaussMaskSize = 1;
		printf("%d, %d\n", focus_tb, eCamGaussMaskSize);
		// �����摜����d�S���v�Z����
		num = MakeCentresFromImage( &ps, img_con, &img_size, &areas );
		end_fp = GetProcTimeMiliSec();
//		printf("cap : %dms\nfp : %dms\n", end_cap - start_cap, end_fp - start_fp);
//		if ( img_pt != NULL )	cvReleaseImage( &img_pt );
		// �\���摜�̍쐬
		if ( draw_fp > 0 || draw_con > 0)	disp_cap = NULL;
		else	disp_cap = img_cap0;
		if ( draw_con > 0 || draw_con_overlap > 0 )	disp_con = img_con0;
		else	disp_con = NULL;
		if ( draw_fp > 0 || draw_fp_overlap > 0 ) {
			disp_ps = ps0;
			disp_num = num0;
		} else {
			disp_ps = NULL;
			disp_num = 0;
		}
		MakeDispImage( img_disp, disp_cap, disp_con, disp_ps, disp_num );
		// �����_��`��
/*		if ( draw_fp > 0 ) {
			if ( img_pt != NULL )	cvReleaseImage( &img_pt );
			img_pt = DrawPoints( ps0, num0, &img_size );
		}
		else if ( draw_fp_overlap > 0 ) {
			if ( img_pt != NULL )	cvReleaseImage( &img_pt );
			img_pt = DrawPointsOverlap( ps0, num0, &img_size, img_cap0 );
		}*/
//		cvShowImage( "Points", img_pt );
//		DrawPoints2( ps, num, img_size );
//		printf("cap : %dms\nconv : %dms\nshow : %dms\nconnect : %dms\nfp : %d\n", end_cap - start_cap, end_conv - start_conv, end_show - start_show, end_connect - start_connect, end_fp - start_fp );
//		cvWaitKey( 1 );	// �Ȃ��ƃE�B���h�E���X�V����Ȃ�
		start_com = GetProcTimeMiliSec();
		// ���ʂ���M�i��M���ɂ��邱�ƂŁA�T�[�o�Ƃ̕��񏈗����\�ɂȂ�B�Ȃ����ڂ̓_�~�[�̃f�[�^��������j
//		RecvResult( sock, doc_name, 20 );
#ifdef	DRAW_COR
		printf("%x\n", ps );
		RecvResultCor( sock, doc_name, 20, corps, &corpsnum, &query_size, &reg_size );
#else
//		puts("recv start");
		if ( eProtocol == kTCP ) {
			RecvResultParam( sock, doc_name, kMaxDocNameLen, &param, &res_size );
		}
		else {
			RecvResultParam( sockres, doc_name, kMaxDocNameLen, &param, &res_size );
		}
#endif
//		printf("recv : %d ms\n", GetProcTimeMiliSec() - start_com );
		// �����_���T�[�o�֑��M
//		start_com = GetProcTimeMiliSec();
//		puts("send start");
		if ( eProtocol == kTCP ) {
			SendPointsAreas( sock, ps, areas, num, &img_size, &addr, eProtocol );
		}
		else {
			SendPointsAreas( sockpt, ps, areas, num, &img_size, &addr, eProtocol );
		}
		end_com = GetProcTimeMiliSec();
//		printf("com : %dms\n", end_com - start_com);
		// ���ʂ�\��
		printf("%s\n", doc_name);
//		printf("size:%d, %d\n", res_size.width, res_size.height );
#ifdef	DRAW_COR
//		if ( corpsnum > 0 )	DrawCor( corps, corpsnum, img_cap, doc_name );
		DrawCor2( corps, corpsnum, img_cap, doc_name );
#else
		// �T���l�C����\��
		printf( "%x\n", doc_name );
#if 1
		DrawThumb( doc_name, doc_name_prev, &thumb, &thumb_draw, param, zero_param, img_size, res_size );
//		if ( doc_name[0] != '\0' && strcmp( doc_name, doc_name_prev ) ) {	// �������ʂ������A���O��̌��ʂƈقȂ�
//			thumb = LoadThumb( doc_name );
//			if ( thumb != NULL ) {
//				cvShowImage( "Thumb", thumb );
//				cvReleaseImage( &thumb );
//			}
//			strcpy( doc_name_prev, doc_name );
//		}
		cvShowImage( "Thumb", thumb_draw );
#ifdef	DRAW_FLAG
		// ����̉摜��\��
		printf( "%x\n", doc_name );
		flag = GetFlagImage( doc_name );
		cvShowImage( "Language", flag );
#endif

#ifdef	THUMB_OUT
		OutPutImage( thumb_draw );
#endif
#endif
//		DrawParam( img_cap0, thumb, param, zero_param );
		if ( ar > 0 ) {	// �g�������̕`��
//			if ( clone != NULL )	cvReleaseImage( &clone );
//			clone = cvCloneImage( img_cap0 );	// �K�v�H�@����Ȃ��Ȃ����
			DrawAR( img_disp, doc_name, param );	// AR��`��
//			if ( draw_fp > 0 || draw_fp_overlap > 0 ) 			DrawAR( img_pt, doc_name, param );	// AR��`��
//			else						DrawAR( img_cap0, doc_name, param );	// AR��`��
//			OutPutImage( img_cap0 );
		}
#endif
		strcpy( doc_name_prev, doc_name );
#ifdef	VIDEO_OUTPUT
		cvResize( img_cap0, small_cap, CV_INTER_NN );
		cvResize( thumb_draw, small_thumb, CV_INTER_NN );
		// �k�������_�摜�̍쐬
		cvZero( small_pt );
		cvNot( small_pt, small_pt );
		for ( i = 0; i < num; i++ ) {
			point.x = (int)((double)ps[i].x * (double)small_pt_size.width / (double)img_size.width);
			point.y = (int)((double)ps[i].y * (double)small_pt_size.height / (double)img_size.height);
			cvCircle( small_pt, point, 1, CV_RGB(0,0,0), -1, CV_AA, 0 );
		}
		MergeImage3( small_cap, small_thumb, small_pt, merge );
		// �L���v�`���摜���r�f�I�ɏo��
		cvWriteFrame( video_wr, merge );
#endif
		// �g��`��
//		if ( !IsEqualParam( param, zero_param ) )	DrawSash( img_cap0, &param );
		// �L���v�`���E�B���h�E�̕`��
		if ( eCaptureWidth == 0 ) {	// ���w��Ȃ�
			cvShowImage( "Capture", img_disp );
		}
		else {	// ���w�肠��
			cvResize( img_disp, cap_resized, CV_INTER_LINEAR );	// ���T�C�Y
			cvShowImage( "Capture", cap_resized );
		}
/*		if ( eCaptureWidth == 0 ) {	// ���w��Ȃ�
			if ( draw_fp > 0 || draw_fp_overlap > 0 )	cvShowImage( "Capture", img_pt );
			else if ( draw_con > 0 && img_con0 != NULL )	cvShowImage( "Capture", img_con0 );
			else				cvShowImage( "Capture", img_cap0 );
		}
		else {
			if ( draw_fp > 0 || draw_fp_overlap > 0 )	cvResize( img_pt, cap_resized, CV_INTER_LINEAR );
			else if ( draw_con > 0 && img_con0 != NULL )	cvResize( img_con0, cap_resized, CV_INTER_LINEAR );
			else				cvResize( img_cap0, cap_resized, CV_INTER_LINEAR );
			cvShowImage( "Capture", cap_resized );
		}*/
		end = GetProcTimeMiliSec();
//		printf("Total : %dms\n", end - start );
		// �t�H�[�J�X�̃g���b�N�o�[�֌W
		if ( focus_available ) {
//			if ( auto_focus_tb == 0 )
				focus_flags = 2;	// �}�j���A��
//			else
//				focus_flags = 3;		// �I�[�g
			SetCameraControlProperty( dsc.pCameraControl, prop, focus_tb + focus_min, focus_flags );
			focus = GetCameraControlProperty( dsc.pCameraControl, prop, &focus_flags );
			// �g���b�N�o�[�ϐ��Ƀt�B�[�h�o�b�N
			if ( focus_flags == 3 )	auto_focus_tb = 1;
			else	auto_focus_tb = 0;
			focus_tb = focus - focus_min;
//			cvSetTrackbarPos( "Manu/Auto", "Capture", auto_focus_tb );
			cvSetTrackbarPos( "Focus", "Capture", focus_tb );
//			printf( "%ld:%ld\n", focus_flags, focus );
		}
		// �K�E�V�A���t�B���^�̃g���b�N�o�[�֌W
#ifdef	GAUSS_VARIABLE
		gauss_tb = (int)((double)focus * 0.0565 - 3.5326);
		if ( gauss_tb < 0 )	gauss_tb = 0;
		eCamGaussMaskSize = gauss_tb * 2 + 1;
//		cvSetTrackbarPos( "Gauss", "Capture", gauss_tb );
#endif
		// �L�[���͊֌W
		start = end;
		key = cvWaitKey( 1 );	// �Ȃ��ƃE�B���h�E���X�V����Ȃ�
		if ( key >= 0 ) {
			switch ( key ) {
				case 'p':	// �ꎞ��~
					if ( eEntireMode == CAM_RET_MODE )
						PauseDirectShowCap( &dsc );
					printf("Hit any key to resume\n");
					for ( ; cvWaitKey(100) < 0; );
					if ( eEntireMode == CAM_RET_MODE )
						ResumeDirectShowCap( &dsc );
					break;
				case 'q':	// �I��
					goto end_cap;
#ifndef	FUNC_LIMIT
				case ' ':	// �L���v�`���摜��ۑ�
					OutPutImage( img_cap );
					printf("Save complete\n");
					break;
				case 's':	// �B�e���[�h
					ret = GotoSnapshotMode( &dsc, img_buff, buff_size, img_cap, thumb );
					if ( ret == 1 ) {	// �L���v�`�����Ė߂���
						ret = GotoPasteMode( img_cap, thumb, &ar_p1, &ar_p2 );
						if ( ret == 1 ) {	// �ꏊ���w�肵�Ė߂���
							SaveAR( doc_name, img_cap, cvSize( (int)((double)thumb->width / eThumbScale), (int)((double)thumb->height / eThumbScale)), &ar_p1, &ar_p2 );
						}
					}
					printf("*** Retrieval mode ***\n");
					printf("Pause(p) Capture(c) Snapshot(s) Quit(q)\n");
					break;
				case 'a':	// AR�̕`��̃I���^�I�t
					ar *= -1;
					break;
#endif
				case 'F':	// �����_���[�h�̃I���^�I�t
					draw_fp *= -1;
					draw_fp_overlap = -1;
					break;
				case 'f':	// �����_�㏑�����[�h�̃I���^�I�t
					draw_fp_overlap *= -1;
					draw_fp = -1;
					break;
				case 'C':	// �A���������[�h�̃I���^�I�t
					draw_con *= -1;
					draw_con_overlap = -1;
					break;
				case 'c':	// �A�������㏑�����[�h�̃I���^�I�t
					draw_con_overlap *= -1;
					draw_con = -1;
					break;
				default:
					break;
			}
		}
		// �L���v�`���摜��1�t���[���O�Ƃ̓���ւ�
		tmp = img_cap0;
		img_cap0 = img_cap;
		img_cap = tmp;
		// �A�������摜��1�t���[���O�Ƃ̓���ւ�
		tmp = img_con0;
		img_con0 = img_con;
		img_con = tmp;
//		Sleep(100);
		end_loop = GetProcTimeMiliSec();
		if ( eExperimentMode )	printf( "%d ms\n", end_loop - start_loop );
	}
end_cap:
	// �I������
	// �I���̃��b�Z�[�W�Ƃ��ĕ��̓����_���𑗂�
	if ( eProtocol == kTCP ) {
		SendPointsAreas( sock, NULL, NULL, -1, &img_size, &addr, eProtocol );
	}
	else {
		SendPointsAreas( sockpt, NULL, NULL, -1, &img_size, &addr, eProtocol );
	}
	CloseWinSock( sock );
	if ( eProtocol == kUDP ) {
		CloseWinSock( sockpt );
		CloseWinSock( sockres );
	}
	cvDestroyWindow( "Capture" );	// �E�B���h�E��j��
//	cvDestroyWindow( "Connected" );
//	cvDestroyWindow( "Points" );
#ifdef	DRAW_COR
	cvDestroyWindow( "Corres" );
#else
	cvDestroyWindow( "Thumb" );
#endif
#ifdef	DRAW_HIST
	cvDestroyWindow( "Hist" );
#endif
	if ( eEntireMode == CAM_RET_MODE ) {
		StopDirectShowCap( &dsc );	// �L���v�`�����I��
		ReleaseDirectShowCap( &dsc );	// DirectShow�̏��X�������[�X
	}
	else if ( eEntireMode == INPUT_MOVIE_MODE ) {
		cvReleaseCapture( &cap_mov );
	}
	cvReleaseImage( &img_cap );
//	cvReleaseImage( &img );
//	cvReleaseImage( &img_pt );
	ReleaseCentres( ps );
	free( areas );
	free( img_buff );
#ifdef	VIDEO_OUTPUT
	cvReleaseVideoWriter( &video_wr );
#endif
	timeEndPeriod( 1 );

	return 0;
}

int GotoSnapshotMode( strDirectShowCap *dsc, unsigned char *img_buff, long buff_size, IplImage *img_cap, IplImage *thumb )
// �X�i�b�v�V���b�g�B�e���[�h
{
	int key;

	printf( "*** Snapshot mode ***\n");
	printf("Capture(c) Quit(q)\n");
	for ( ; ; ) {
		CaptureDirectShowCap( dsc, img_buff, buff_size );
		Buff2ImageData( img_buff, img_cap );
		cvShowImage( "Capture", img_cap );
		cvShowImage( "Thumb", thumb );
		key = cvWaitKey(1);
		if ( key >= 0 ) {
			switch ( key ) {
				case 'c':	// �L���v�`��
					return 1;
				case 'q':	// �I��
					return 0;
				default:
					break;
			}
		}
	}
	return 0;
}

int GotoPasteMode( IplImage *img_cap, IplImage *thumb, CvPoint *p1, CvPoint *p2 )
// �\��t�����[�h
{
	int key;

	g_thumb = cvCloneImage( thumb );
	g_thumb_draw = cvCloneImage( thumb );
	g_ratio = (double)img_cap->height / (double)img_cap->width;
	g_paste_complete = 0;

	printf("*** Paste mode ***\n");
	printf("Drag on \"Thumb\" window to paste the snapshot\n");
	cvSetMouseCallback( "Thumb", OnMouseThumb, 0 );

	cvShowImage( "Thumb", g_thumb_draw );

	for ( ; ; ) {
		key = cvWaitKey(1);
		switch ( key ) {
			case 'q':
				cvShowImage( "Thumb", thumb );

				cvReleaseImage( &g_thumb );
				cvReleaseImage( &g_thumb_draw );
			
				cvSetMouseCallback( "Thumb", NULL, 0 );
				return 0;
			case 's':
				goto paste_end;
			default:
				break;
		}
	}
paste_end:

	cvShowImage( "Thumb", thumb );

	cvReleaseImage( &g_thumb );
	cvReleaseImage( &g_thumb_draw );

	cvSetMouseCallback( "Thumb", NULL, 0 );

	if ( g_paste_complete == 1 ) {
		p1->x = min( (int)((double)g_p1.x/eThumbScale), (int)((double)g_p2.x/eThumbScale) );
		p1->y = min( (int)((double)g_p1.y/eThumbScale), (int)((double)g_p2.y/eThumbScale) );
		p2->x = max( (int)((double)g_p1.x/eThumbScale), (int)((double)g_p2.x/eThumbScale) );
		p2->y = max( (int)((double)g_p2.y/eThumbScale), (int)((double)g_p2.y/eThumbScale) );
		return 1;
	}
	else {
		return 0;
	}
}

void OnMouseThumb( int event, int x, int y, int flags, void *param )
// �}�E�X�̃R�[���o�b�N�֐�
{
//	static CvPoint p1, p2;
	static int status = 0;
	int width, height;
	double ratio;

	width = abs(g_p1.x - x);
	height = abs(g_p1.y - y);
	ratio = (double)height / (double)width;
	if ( ratio > g_ratio ) {	// y���D��
		if ( x > g_p1.x ) {	// �E��
			g_p2.x = g_p1.x + (int)((double)height / g_ratio);
		}
		else {	// ����
			g_p2.x = g_p1.x - (int)((double)height / g_ratio);
		}
		if ( y > g_p1.y ) {	// ����
			g_p2.y = g_p1.y + height;
		}
		else {	// ���
			g_p2.y = g_p1.y - height;
		}
	}
	else {	// x���D��
		if ( x > g_p1.x ) {	// �E��
			g_p2.x = g_p1.x + width;
		}
		else {	// ����
			g_p2.x = g_p1.x - width;
		}
		if ( y > g_p1.y ) {	// ����
			g_p2.y = g_p1.y + (int)((double)width * g_ratio);
		}
		else {	// ���
			g_p2.y = g_p1.y - (int)((double)width * g_ratio);
		}
	}

	switch( event ) {
		case CV_EVENT_LBUTTONDOWN:
			g_p1 = cvPoint( x, y );
			status = 1;	// �}�E�X�������ꂽ
			g_paste_complete = 0;
			break;
		case CV_EVENT_LBUTTONUP:
			if ( status == 1 ) {
				cvReleaseImage( &g_thumb_draw );
				g_thumb_draw = cvCloneImage( g_thumb );
				cvRectangle( g_thumb_draw, g_p1, g_p2, CV_RGB( 255, 0, 0 ), 2, 8, 0 );
				status = 0;
				cvShowImage( "Thumb", g_thumb_draw );
			}
			g_paste_complete = 1;
			break;
		case CV_EVENT_MOUSEMOVE:
			if ( status == 1 ) {
				cvReleaseImage( &g_thumb_draw );
				g_thumb_draw = cvCloneImage( g_thumb );
				cvRectangle( g_thumb_draw, g_p1, g_p2, CV_RGB( 0, 0, 0 ), 1, 8, 0 );
				cvShowImage( "Thumb", g_thumb_draw );
			}
			break;
	}
}

int LoadPointFile( char *fname, CvPoint **ps0, CvSize *size )
// �_�f�[�^��ǂݍ���
{
	int num;
	char line[kMaxLineLen];
	FILE *fp;
	CvPoint *ps;
	
	if ( (fp = fopen(fname, "r")) == NULL )	return 0;
	fgets(line, kMaxLineLen, fp);
	sscanf(line, "%d,%d", &(size->width), &(size->height));
	ps = (CvPoint *)calloc(kMaxPointNum, sizeof(CvPoint));
	*ps0 = ps;
	num = 0;
	while ( fgets(line, kMaxLineLen, fp) != NULL && num < kMaxPointNum ) {
		sscanf(line, "%d,%d", &(ps[num].x), &(ps[num].y));
		num++;
	}
	fclose(fp);
	return num;
}

IplImage *LoadThumb( char *fname )
// fname�Ŏw�肳�ꂽ�T���l�C���摜�����[�h����
{
	char thumb_fname[kMaxPathLen];

	if ( *fname == '\0' )	return NULL;	// fname���󕶎���
//	sprintf( thumb_fname, "%s%s.%s", kThumbDir, fname, kThumbSuffix );
	sprintf( thumb_fname, "%s%s.%s", eThumbDir, fname, eThumbSuffix );
	puts(thumb_fname);
	return cvLoadImage( thumb_fname, 1 );
}

#define	kDrawParamScale	(0.5)
#define	kDrawParamHMargin	(100)
#define	kDrawParamVMargin	(100)
#define	kDrawParamSpace	(400)
#define	kDrawParamRectThick	(4)
#define	kDrawParamLineThick	(4)

void DrawParam( IplImage *img_cap, IplImage *thumb, strProjParam param, strProjParam zero_param )
// �L���v�`���摜�ƃT���l�C���ƃp�����[�^�������g�̑Ή���`�悵�ďo��
{
	int i, width_all, height_all;
	IplImage *img, *thumb_res;
	strPoint p1[5], p2[5];

	// �T���l�C���̊g��ł����
	thumb_res = cvCreateImage( cvSize( (int)(thumb->width / eThumbScale * kDrawParamScale), (int)(thumb->height / eThumbScale * kDrawParamScale) ), IPL_DEPTH_8U, 3 );
	cvResize( thumb, thumb_res, CV_INTER_LINEAR );
	// �S�̉摜�����
	width_all = kDrawParamHMargin * 2 + kDrawParamSpace + img_cap->width + thumb_res->width;
	height_all = kDrawParamVMargin * 2 + max( img_cap->height, thumb_res->height );
	img = cvCreateImage( cvSize( width_all, height_all ), IPL_DEPTH_8U, 3 );
	cvSet( img, cWhite, NULL );	// ���œh��Ԃ�
	// �L���v�`���摜��z�u����
	PutImage( img_cap, img, cvPoint( kDrawParamHMargin, kDrawParamVMargin ) );
	// �g��T���l�C����z�u����
	PutImage( thumb_res, img, cvPoint( kDrawParamHMargin + img_cap->width + kDrawParamSpace, kDrawParamVMargin ) );
	// �T���l�C���̘g��`��
	cvRectangle( img, cvPoint( kDrawParamHMargin + img_cap->width + kDrawParamSpace, kDrawParamVMargin), \
		cvPoint( kDrawParamHMargin + img_cap->width + kDrawParamSpace + thumb_res->width, kDrawParamVMargin + thumb_res->height ), \
		cBlack, kDrawParamRectThick, CV_AA, 0 );
	// �L���v�`���摜�̘g��`��
	cvRectangle( img, cvPoint( kDrawParamHMargin, kDrawParamVMargin ), cvPoint( kDrawParamHMargin + img_cap->width, kDrawParamVMargin + img_cap->height ), \
		cRed, kDrawParamRectThick, CV_AA, 0 );
	// �l���̑Ή�����`��
	if ( !IsEqualParam( param, zero_param ) ) {	// �p�����[�^�v�Z�ɐ���
		// �g��`��
		p1[0].x = 0;
		p1[0].y = 0;
		p1[1].x = img_cap->width;
		p1[1].y = 0;
		p1[2].x = img_cap->width;
		p1[2].y = img_cap->height;
		p1[3].x = 0;
		p1[3].y = img_cap->height;
		p1[4].x = 0;
		p1[4].y = 0;
		for ( i = 0; i < 5; i++ ) {
			ProjTrans( &(p1[i]), &(p2[i]), &param );
			p2[i].x = (int)(p2[i].x * kDrawParamScale);
			p2[i].y = (int)(p2[i].y * kDrawParamScale);
		}
		for ( i = 0; i < 4; i++ )	cvLine( img, cvPoint( kDrawParamHMargin + img_cap->width + kDrawParamSpace + p2[i].x, kDrawParamVMargin + p2[i].y ), cvPoint( kDrawParamHMargin + img_cap->width + kDrawParamSpace + p2[i+1].x, kDrawParamHMargin + p2[i+1].y ), cRed, kDrawParamLineThick, CV_AA, 0 );
		for ( i = 0; i < 4; i++ )	cvLine( img, cvPoint( kDrawParamHMargin + p1[i].x, kDrawParamVMargin + p1[i].y ), cvPoint( kDrawParamHMargin + img_cap->width + kDrawParamSpace + p2[i].x, kDrawParamVMargin + p2[i].y ), cRed, kDrawParamLineThick, CV_AA, 0 );
	}


	OutPutImage( img );
//	OutPutImage( thumb_res );
	cvReleaseImage( &thumb_res );
	cvReleaseImage( &img );
}

void DrawThumb( char *doc_name, char *doc_name_prev, IplImage **thumb0, IplImage **thumb_draw0, strProjParam param, strProjParam zero_param, CvSize cap_size, CvSize res_size )
// �T���l�C����g���ŕ`��
{
	int i;
	char thumb_fname[kMaxPathLen];
	IplImage *thumb, *thumb_orig, *thumb_draw;
	CvSize thumb_size;
	strPoint p1[5], p2[5];

	// �T���l�C���̃T�C�Y
	if ( eThumbWidth != 0 ) {	// ���w�肪�L��
		thumb_size.width = eThumbWidth;
		thumb_size.height = (int)((double)res_size.height * ((double)eThumbWidth / (double)res_size.width));
		eThumbScale = (double)eThumbWidth / (double)res_size.width;
	}
	else {	// �X�P�[���w��
		thumb_size.width = (int)((double)res_size.width * eThumbScale);
		thumb_size.height = (int)((double)res_size.height * eThumbScale);
	}

	thumb = *thumb0;
	if ( strcmp( doc_name, doc_name_prev ) ) {	// �O��ƌ������ʂ��قȂ�
		if ( *thumb0 != NULL )	cvReleaseImage( thumb0 );	// �����[�X�������
		thumb = cvCreateImage( thumb_size, IPL_DEPTH_8U, 3 );	// �k���摜���쐬
		cvSet( thumb, CV_RGB(255,255,255), NULL );	// ���ɃN���A
		sprintf( thumb_fname, "%s%s.%s", eThumbDir, doc_name, eThumbSuffix );
//		puts( thumb_fname );
		if ( ( thumb_orig = cvLoadImage( thumb_fname, 1 ) ) != NULL ) {	// �T���l�C���摜�����[�h���C
			cvResize( thumb_orig, thumb, CV_INTER_NN );	// ���T�C�Y���ĕ\��
			cvReleaseImage( &thumb_orig );
		}
		*thumb0 = thumb;
	}
	if ( *thumb_draw0 != NULL )	cvReleaseImage( thumb_draw0 );
	thumb_draw = cvCloneImage( thumb );	// �N���[�����쐬
	*thumb_draw0 = thumb_draw;
	if ( !IsEqualParam( param, zero_param ) ) {	// �p�����[�^�v�Z�ɐ���
		// �g��`��
//		cvLine( thumb_draw, cvPoint(0,0), cvPoint(100,100), cRed, 4, CV_AA, 0 );
		p1[0].x = 0;
		p1[0].y = 0;
		p1[1].x = cap_size.width;
		p1[1].y = 0;
		p1[2].x = cap_size.width;
		p1[2].y = cap_size.height;
		p1[3].x = 0;
		p1[3].y = cap_size.height;
		p1[4].x = 0;
		p1[4].y = 0;
		for ( i = 0; i < 5; i++ ) {
			ProjTrans( &(p1[i]), &(p2[i]), &param );
			p2[i].x = (int)(p2[i].x * eThumbScale);
			p2[i].y = (int)(p2[i].y * eThumbScale);
		}
		for ( i = 0; i < 4; i++ )	cvLine( thumb_draw, cvPoint( p2[i].x, p2[i].y ), cvPoint( p2[i+1].x, p2[i+1].y ), cRed, 5, CV_AA, 0 );
	}
#if 0
	thumb = *thumb0;
	thumb_draw = *thumb_draw0;
	if ( doc_name[0] == '\0' ) {	// �����Ɏ��s
		strcpy( doc_name, "0" );		// 0.bmp��\��
	}
	if ( strcmp( doc_name, doc_name_prev ) ) {	// �O��ƌ������ʂ��قȂ�
		if ( *thumb0 != NULL )	cvReleaseImage( thumb0 );	// �����[�X�������
//		puts("start load thumb");
		thumb = LoadThumb( doc_name );		// ���[�h����
//		puts("finish load thumb");
		*thumb0 = thumb;
	}
	if ( *thumb_draw0 != NULL )	cvReleaseImage( thumb_draw0 );
	thumb_draw = cvCloneImage( thumb );	// �N���[�����쐬
	*thumb_draw0 = thumb_draw;

	if ( !IsEqualParam( param, zero_param ) ) {	// �p�����[�^�v�Z�ɐ���
		// �g��`��
//		cvLine( thumb_draw, cvPoint(0,0), cvPoint(100,100), cRed, 4, CV_AA, 0 );
		p1[0].x = 0;
		p1[0].y = 0;
		p1[1].x = cap_size.width;
		p1[1].y = 0;
		p1[2].x = cap_size.width;
		p1[2].y = cap_size.height;
		p1[3].x = 0;
		p1[3].y = cap_size.height;
		p1[4].x = 0;
		p1[4].y = 0;
		for ( i = 0; i < 5; i++ ) {
			ProjTrans( &(p1[i]), &(p2[i]), &param );
			p2[i].x = (int)(p2[i].x * kThumbScale);
			p2[i].y = (int)(p2[i].y * kThumbScale);
		}
		for ( i = 0; i < 4; i++ )	cvLine( thumb_draw, cvPoint( p2[i].x, p2[i].y ), cvPoint( p2[i+1].x, p2[i+1].y ), cRed, 5, CV_AA, 0 );
	}
#endif
}

void DrawCor2( CvPoint corps[][2], int corpsnum, IplImage *img_cap, char *doc_name )
// �Ή��_�̕`��Q�i�L���v�`���摜�ƃT���l�C���t���j
{
	int i;
	char thumb_fname[kMaxPathLen];
	IplImage *cor_img, *thumb;
	CvSize qs, rs, cor_size;
	CvPoint p1, p2;
	
	if ( doc_name[0] == '\0' ) {	// �����Ɏ��s
		strcpy( doc_name, "0" );		// 0.bmp��\��
	}
//	puts("start load thumb");
	thumb = LoadThumb( doc_name );		// ���[�h����
//	puts("finish load thumb");

	cor_size = cvSize( img_cap->width + thumb->width, max( img_cap->height, thumb->height ) );
	cor_img = cvCreateImage( cor_size, IPL_DEPTH_8U, 3 );
	MergeImage( img_cap, thumb, MERGE_HOR, cor_img );

	for ( i = 0; i < corpsnum; i++ ) {
		p1.x = (int)(corps[i][0].x);
		p1.y = (int)(corps[i][0].y);
		p2.x = (int)(corps[i][1].x * eThumbScale) + img_cap->width;
		p2.y = (int)(corps[i][1].y * eThumbScale);
		cvLine( cor_img, p1, p2, cBlack, 1, CV_AA, 0 );
//		cvLine( cor_img, p1, p2, cWhite, 1, CV_AA, 0 );
	}
	cvShowImage( "Corres", cor_img );
	OutPutImage( cor_img );
	
	cvReleaseImage( &thumb );
	cvReleaseImage( &cor_img );
}

int SaveAR( char *doc_name, IplImage *img_cap, CvSize size, CvPoint *p1, CvPoint *p2 )
// AR����ۑ�����
{
	char ar_fname[kMaxPathLen], img_fname[kMaxPathLen], img_path[kMaxPathLen];
	int num, ar_exist;
	FILE *fp;

	srand( GetTickCount() );
	num = rand() % 1000;

	sprintf( ar_fname, "%s%s.txt", kARDir, doc_name );
	sprintf( img_fname, "%s%03d.bmp", doc_name, num );
	sprintf( img_path, "%s%s", kARImageDir, img_fname );
//	puts( ar_fname );
//	puts( img_fname );
//	getchar();
	if ( ( fp = fopen( ar_fname, "r" ) ) == NULL ) {
		ar_exist = 0;
	}
	else {
		fclose( fp );
		ar_exist = 1;
	}
	if ( ( fp = fopen( ar_fname, "a" ) ) == NULL ) {
		return 0;
	}
	if ( !ar_exist )	fprintf( fp, "%d,%d\n", size.width, size.height );
	fprintf( fp, "%c\n%d,%d,%d,%d\n%s\n", kARTypeImg, p1->x, p1->y, p2->x, p2->y, img_fname );
	fclose( fp );
	cvSaveImage( img_path, img_cap );

	return 1;
}

int CaptureMovie( char *movfile )
// ����̎B�e
{
	int key, frame_msec;
	long buff_size;
	unsigned char *img_buff;
	CvSize img_size;
	IplImage *img_cap = NULL;
	strDirectShowCap dsc;
	struct CvVideoWriter *video_wr;	// �r�f�I�o�͗p
	TIME_COUNT start, end, start_cap, end_cap, start_show, end_show, start_connect, end_connect, start_conv, end_conv, start_ret, end_ret, start_fp, end_fp, start_com, end_com;

	timeBeginPeriod( 1 );//���x��1ms�ɐݒ�

	if ( ReadIniFile() == 0 ) {	// ini�t�@�C���̓ǂݍ��݂Ɏ��s
		fprintf( stderr, "Error : ReadIniFile\n" );
		return 1;
	}

	printf("%d\n", eCamConfNum);
	InitDirectShowCap( eCamConfNum, &dsc, &(img_size.width), &(img_size.height) );	// USB�J������������
	buff_size = img_size.width * img_size.height * 3;
	img_buff = (unsigned char *)malloc( buff_size );	// �摜�̃o�b�t�@���m��
	img_cap = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬

	StartDirectShowCap( &dsc );	// �L���v�`�����J�n
	cvNamedWindow( "Capture", CV_WINDOW_AUTOSIZE );	// �E�B���h�E���쐬

	video_wr = cvCreateVideoWriter( movfile, -1, kVideoFps, img_size, 1 );	// �r�f�I�o��
	frame_msec = (int)(1000.0L / (double)kVideoFps );

	for ( ; ; ) {
		start = GetProcTimeMiliSec();
		// �L���v�`��
		start_cap = GetProcTimeMiliSec();
		CaptureDirectShowCap( &dsc, img_buff, buff_size );
		end_cap = GetProcTimeMiliSec();
		// �o�b�t�@���摜�ɕϊ�
		start_conv = GetProcTimeMiliSec();
		Buff2ImageData( img_buff, img_cap );
		end_conv = GetProcTimeMiliSec();
		// �L���v�`���摜��\��
		start_show = GetProcTimeMiliSec();
		cvShowImage( "Capture", img_cap );
		end_show = GetProcTimeMiliSec();
//		OutPutImage( img_cap );
		// �L���v�`���摜���r�f�I�ɏo��
		cvWriteFrame( video_wr, img_cap );
		key = cvWaitKey( 1 );	// �Ȃ��ƃE�B���h�E���X�V����Ȃ�
		if ( key >= 0 ) {
			switch ( key ) {
				case 'p':	// �ꎞ��~
					PauseDirectShowCap( &dsc );
					printf("Hit any key to resume\n");
					for ( ; cvWaitKey(100) < 0; );
					ResumeDirectShowCap( &dsc );
					break;
				case 'q':	// �I��
					goto end_cap;
				case 'c':	// �L���v�`���摜��ۑ�
					OutPutImage( img_cap );
					printf("Save complete\n");
					break;
				default:
					break;
			}
		}
//		for ( end = GetProcTimeMiliSec(); end - start < frame_msec; end = GetProcTimeMiliSec() )
//			Sleep( 1 );
//		Sleep(100);
	}
end_cap:
	// �I������
	cvDestroyWindow( "Capture" );	// �E�B���h�E��j��
	StopDirectShowCap( &dsc );	// �L���v�`�����I��
	ReleaseDirectShowCap( &dsc );	// DirectShow�̏��X�������[�X
	cvReleaseImage( &img_cap );
	free( img_buff );
	cvReleaseVideoWriter( &video_wr );
	timeEndPeriod( 1 );

	return 0;
}

int DecomposeMovie( char *mov_file )
// ����t�@�C���𕪉����C�摜���o��
{
	int i;
	IplImage *img_cap;
	CvCapture *cap_mov;
	CvSize img_size;

	cap_mov = cvCaptureFromFile( mov_file );
	img_size.width = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_WIDTH );
	img_size.height = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_HEIGHT );
	printf("%d,%d\n", img_size.width, img_size.height );
	while ( ( img_cap = cvQueryFrame( cap_mov ) ) != NULL ) {
		OutPutImage( img_cap );
	}
	cvReleaseCapture( &cap_mov );
	return 1;
}

void ConvMovie( void )
// ����̕ϊ�
{
	CvCapture *cap_mov;
	CvSize img_size;
	double mov_fps;
	CvVideoWriter *video_wr;
	IplImage *cap_file;

	if ( ( cap_mov = cvCaptureFromFile( eMovieFileName ) ) == NULL )	return;
	img_size.width = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_WIDTH );	// �T�C�Y�ǂݍ���
	img_size.height = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_HEIGHT );
	mov_fps = cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FPS );
	video_wr = cvCreateVideoWriter( eConvMovieFileName, -1, mov_fps, img_size, 1 );	// �r�f�I�o��
	cvNamedWindow( "Movie", CV_WINDOW_AUTOSIZE );
	for ( ; ; ) {
		cap_file = cvQueryFrame( cap_mov );
		if ( cap_file == NULL )	break;
		cvShowImage( "Movie", cap_file );
		cvWriteFrame( video_wr, cap_file );
	}
	cvReleaseCapture( &cap_mov );
	cvReleaseVideoWriter( &video_wr );

	return;
}

IplImage *LoadFlagImage( char *doc_name )
// doc_name���獑�𔻕ʂ��C������ǂݍ���
{
	char path[kMaxPathLen];
	char langs[][20] = { "arabic", "chinese", "english", "french", "hindi", "japanese", "korean", "lhao", "russian", "spanish", "tamil", "thai" };
	int i, num;
	IplImage *img;

	num = sizeof( langs );
	for ( i = 0; i < num; i++ ) {
		if ( strncmp( doc_name, langs[i], strlen( langs[i] ) ) == 0 ) {
			sprintf( path, "%s%s.%s", kFlagDir, langs[i], kFlagSuffix );
			printf("%s\n", path);
			img = cvLoadImage( path, 1 );
			return img;
		}
	}
	return NULL;
}

void MakeDispImage( IplImage *img_disp, IplImage *disp_cap, IplImage *disp_con, CvPoint *disp_ps, int disp_num )
// �\���p�摜���쐬����
{
	int i;

	// �L���v�`���摜
	if ( disp_cap != NULL )	cvCopyImage( disp_cap, img_disp );
	else	cvZero( img_disp );
	// �A������
	if ( disp_con != NULL )	cvSet( img_disp, cGreen, disp_con );
	// �����_
	for ( i = 0; i < disp_num; i++ ) {
		cvCircle( img_disp, disp_ps[i], 3, cRed, -1, CV_AA, 0 );
	}
}

IplImage *GetFlagImage( char *doc_name )
// �t�@�C�������獑���摜�𓾂�
{
	char path[kMaxPathLen];
	char langs[13][20] = { "none", "arabic", "chinese", "english", "french", "hindi", "japanese", "korean", "lhao", "russian", "spanish", "tamil", "thai" };
	int i;
	static int loaded = 0;	// �摜�����[�h�������ǂ����̃t���O
	static int num = 13;
	static IplImage **flag_imgs = NULL;

	if ( loaded == 0 ) {	// �܂��ǂݍ���ł��Ȃ�
		flag_imgs = (IplImage **)calloc( num, sizeof(IplImage *) );
		for ( i = 0; i < num; i++ ) {
			sprintf( path, "%s%s.%s", kFlagDir, langs[i], kFlagSuffix );
			flag_imgs[i] = cvLoadImage( path, 1 );
		}
		loaded = 1;
	}
	if ( doc_name[0] == '\0' )	return flag_imgs[0];	// �������ʂȂ��Ȃ�none��Ԃ�
	for ( i = 1; i < num; i++ ) {	// none�͏Ȃ��̂�1����
		if ( strncmp( doc_name, langs[i], strlen( langs[i] ) ) == 0 ) {
			return flag_imgs[i];
		}
	}
	return flag_imgs[0];	// �Y��������̂��Ȃ����none��Ԃ�
}

int RetrieveDualHash( void )
{
	SOCKET sock, sockpt, sockres;
	int i, num1 = 0, num10 = 0, num2 = 0, num20 = 0, key, ret, corpsnum, ar, draw_fp = -1, draw_fp_overlap = -1, draw_con = -1, draw_con_overlap = -1;
	unsigned char *img_buff;
	char doc_name[kMaxDocNameLen] = "", doc_name_prev[kMaxDocNameLen] = "invalid";
	long buff_size;
	TIME_COUNT start, end, start_cap, end_cap, start_show, end_show, start_connect, end_connect, start_conv, end_conv, start_ret, end_ret, start_fp, end_fp, start_com, end_com, start_loop, end_loop, start_all, end_all;
	
	CvPoint *ps1 = NULL, *ps10 = NULL, *ps2 = NULL, *ps20 = NULL, corps[kMaxPointNum][2], ar_p1, ar_p2, point;
	double *areas1 = NULL, *areas10 = NULL, *areas2 = NULL, *areas20 = NULL;
	CvSize img_size, query_size, reg_size, thumb_size, merge_size, small_img_size, small_thumb_size, res_size, resized_size;
	strDirectShowCap dsc;
	IplImage *img_cap = NULL, *img_pt = NULL, *img_cap0 = NULL, *tmp = NULL, *thumb = NULL, *thumb_draw = NULL, *merge = NULL, *small_cap, *small_thumb, *clone = NULL, *cap_file, *cap_resized, *img_con1 = NULL, *img_con10 = NULL, *img_con2 = NULL, *img_con20 = NULL, *img_disp = NULL;
	IplImage *flag;
	// �\���摜�֌W
	IplImage *disp_cap, *disp_con;
	CvPoint *disp_ps;
	int disp_num;
	// �t�H�[�J�X�֌W
	CameraControlProperty prop;
	int focus_available = 0, auto_focus_tb, focus_tb;
	long focus, focus_min, focus_max, focus_step, focus_def, focus_flags;
	// �K�E�V�A���t�B���^�̃g���b�N�o�[�֌W
	int gauss_tb;
#ifdef VIDEO_OUTPUT
	CvSize small_pt_size;
	IplImage *small_pt1, *small_pt2;;
#endif
	strProjParam param, zero_param;;
	struct CvVideoWriter *video_wr;	// �r�f�I�o�͗p
	struct sockaddr_in addr;
	CvCapture *cap_mov;	// �r�f�I���͗p
	// �T���l�C���ۑ��p
	char **fname_list = NULL;
	int doc_num = 0;

	if ( eEntireMode == CAM_RET_MODE ) {
		ret = InitDirectShowCap( eCamConfNum, &dsc, &(img_size.width), &(img_size.height) );	// USB�J������������
		if ( ret == 0 )	return 1;
	}
	else if ( eEntireMode == INPUT_MOVIE_MODE ) {
		cap_mov = cvCaptureFromFile( eMovieFileName );	// �t�@�C������̃L���v�`��
		if ( cap_mov == NULL )	return 1;	// �L���v�`�����s
		img_size.width = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_WIDTH );	// �T�C�Y�ǂݍ���
		img_size.height = (int)cvGetCaptureProperty( cap_mov, CV_CAP_PROP_FRAME_HEIGHT );
	}
	buff_size = img_size.width * img_size.height * 3;
	thumb_size = cvSize( 425, 550 );	// �T���l�C���̃T�C�Y
	// �r�f�I�p�̏k���T�C�Y
	small_img_size = cvSize( (int)(img_size.width * kResizeScaleImg), (int)(img_size.height * kResizeScaleImg) );
	small_thumb_size = cvSize( (int)(thumb_size.width * kResizeScaleThumb), (int)(thumb_size.height *kResizeScaleThumb) );
	merge_size = cvSize( small_img_size.width + small_thumb_size.width, max( small_img_size.height, small_thumb_size.height ) );
	// �A�X�y�N�g��ɍ��킹��
	if ( merge_size.width > (int)((double)merge_size.height * kVideoAspectRatio) )
		merge_size.height = (int)((double)merge_size.width / kVideoAspectRatio);
	else if ( merge_size.width < (int)((double)merge_size.height * kVideoAspectRatio) )
		merge_size.width = (int)((double)merge_size.height * kVideoAspectRatio);
	img_buff = (unsigned char *)malloc( buff_size );	// �摜�̃o�b�t�@���m��
	img_cap = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬
	img_cap0 = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬
	if ( eCaptureWidth != 0 )	{	// �L���v�`���E�B���h�E�̕��w�肪�L��
		resized_size.width = eCaptureWidth;
		resized_size.height = (int)((double)img_size.height * ((double)eCaptureWidth / (double)img_size.width));
		cap_resized = cvCreateImage( resized_size, IPL_DEPTH_8U, 3 );
	}
	img_con1 = cvCreateImage( img_size, IPL_DEPTH_8U, 1 );
	img_con10 = cvCreateImage( img_size, IPL_DEPTH_8U, 1 );
	img_con2 = cvCreateImage( img_size, IPL_DEPTH_8U, 1 );
	img_con20 = cvCreateImage( img_size, IPL_DEPTH_8U, 1 );
	img_disp = cvCreateImage( img_size, IPL_DEPTH_8U, 3 );	// �摜���쐬
#ifdef	VIDEO_OUTPUT
	small_pt_size.width = 212;
	small_pt_size.height = 159;
	small_cap = cvCreateImage( small_img_size, IPL_DEPTH_8U, 3 );
	small_thumb = cvCreateImage( small_thumb_size, IPL_DEPTH_8U, 3 );
	merge = cvCreateImage( merge_size, IPL_DEPTH_8U, 3 );
	small_pt1 = cvCreateImage( small_pt_size, IPL_DEPTH_8U, 3 );
	small_pt2 = cvCreateImage( small_pt_size, IPL_DEPTH_8U, 3 );
#endif
#ifdef	AR
	ar = 1;
#else
	ar = -1;
#endif
	zero_param.a1 = 0.0; zero_param.a2 = 0.0; zero_param.a3 = 0.0; zero_param.b1 = 0.0; zero_param.b2 = 0.0; zero_param.b3 = 0.0; zero_param.c1 = 0.0; zero_param.c2 = 0.0;
	cvNamedWindow( "Capture", CV_WINDOW_AUTOSIZE );	// �E�B���h�E���쐬
#ifdef	DRAW_COR	// �Ή��_�`��
	cvNamedWindow( "Corres", CV_WINDOW_AUTOSIZE );
#else
	cvNamedWindow( "Thumb", CV_WINDOW_AUTOSIZE );
#endif
#ifdef	DRAW_HIST	// �q�X�g�O�����`��
	cvNamedWindow( "Hist", CV_WINDOW_AUTOSIZE );
#endif
#ifdef	DRAW_FLAG
	cvNamedWindow( "Language", CV_WINDOW_AUTOSIZE );
#endif
	////////////////////////// �t�H�[�J�X�̃g���b�N�o�[�֌W //////////////////////////
	if ( eEntireMode == CAM_RET_MODE ) {
		prop = CameraControl_Focus;
		focus_available = GetCameraControlRange( dsc.pCameraControl, prop, &focus_min, &focus_max, &focus_step, &focus_def, &focus_flags );
//	// �y���J�����̂��߂Ɉꎞ�I�Ƀt�H�[�J�X�֌W���E���Ă���
//	focus_available = 0;
		if ( focus_available )	focus = GetCameraControlProperty( dsc.pCameraControl, prop, &focus_flags );
	// �擾�����v���p�e�B����g���b�N�o�[���쐬����
		if ( focus_available ) {
//		auto_focus_tb = ( focus_flags == CameraControl_Flags_Auto ) ? 1 : 0;
			focus_tb = (int)(focus - focus_min);
//		cvCreateTrackbar( "Manu/Auto", "Capture", &auto_focus_tb, 1, NULL );
			cvCreateTrackbar( "Focus", "Capture", &focus_tb, focus_max - focus_min, NULL );
		}
	}
	////////////////////////// �K�E�V�A���}�X�N�T�C�Y�̃g���b�N�o�[�֌W //////////////////////////
#ifdef	GAUSS_VARIABLE
	gauss_tb = (int)(eCamGaussMaskSize / 2) + 1;
//	cvCreateTrackbar( "Gauss", "Capture", &gauss_tb, 10, NULL );
	eCamGaussMaskSize = gauss_tb * 2 + 1;
#endif

	if ( eEntireMode == CAM_RET_MODE )
		StartDirectShowCap( &dsc );	// �L���v�`�����J�n
	// �l�S�V�G�C�g�J�n
	sock = InitWinSockClTCP( eTCPPort, eServerName );
	if ( sock == INVALID_SOCKET ) {
		fprintf( stderr, "error: connection failure\n" );
		return 1;
	}
	ret = SendComSetting( sock, eProtocol, ePointPort, eResultPort, eClientName );	// �ʐM�ݒ�𑗐M
	if ( ret < 0 )	return 1;

	if ( eProtocol == kUDP ) {	// UDP
		sockpt = InitWinSockClUDP( ePointPort, eServerName, &addr );
		sockres = InitWinSockSvUDP( eResultPort );
	}

#ifdef	VIDEO_OUTPUT
//	video_wr = cvCreateVideoWriter( kVideoFileName, -1, kVideoFps, merge_size, 1 );	// �r�f�I�o��
	video_wr = cvCreateVideoWriter( kVideoFileName, CV_FOURCC( 'X', 'V', 'I', 'D' ), kVideoFps, merge_size, 1 );	// �r�f�I�o��
#endif
#ifdef	VIDEO_RECORD
//	video_wr = cvCreateVideoWriter( "recorded.avi", -1, kVideoFps, img_size, 1 );	// �r�f�I�o��
	video_wr = cvCreateVideoWriter( "recorded.avi", CV_FOURCC( 'X', 'V', 'I', 'D' ), kVideoFps, img_size, 1 );	// �r�f�I�o��
#endif

	start = GetProcTimeMiliSec();
	start_all = GetProcTimeMiliSec();
	for ( ; ; ) {
		start_loop = GetProcTimeMiliSec();
		if ( eEntireMode == CAM_RET_MODE ) {
			// �L���v�`��
			start_cap = GetProcTimeMiliSec();
			CaptureDirectShowCap( &dsc, img_buff, buff_size );
			end_cap = GetProcTimeMiliSec();
			// �o�b�t�@���摜�ɕϊ�
			start_conv = GetProcTimeMiliSec();
			Buff2ImageData( img_buff, img_cap );
			end_conv = GetProcTimeMiliSec();
		}
		else if ( eEntireMode == INPUT_MOVIE_MODE ) {
			start_cap = GetProcTimeMiliSec();
			cap_file = cvQueryFrame( cap_mov );
			end_cap = GetProcTimeMiliSec();
			if ( cap_file == NULL )	goto end_cap;
			CopyImageData( cap_file, img_cap );
		}
#ifdef	VIDEO_RECORD
		cvWriteFrame( video_wr, img_cap );
#endif
		// �����摜���쐬
		// �����P��
		GetConnectedImageCam( img_cap, img_con1, eCamGaussMaskSize1 );
		if ( ps10 != NULL && num10 > 0 )	ReleaseCentres( ps10 );
		if ( areas10 != NULL && num10 > 0 )	free( areas10 );
		ps10 = ps1;
		areas10 = areas1;
		num10 = num1;
		// �P��P��
		GetConnectedImageCam( img_cap, img_con2, eCamGaussMaskSize2 );
		if ( ps20 != NULL && num20 > 0 )	ReleaseCentres( ps20 );
		if ( areas20 != NULL && num20 > 0 )	free( areas20 );
		ps20 = ps2;
		areas20 = areas2;
		num20 = num2;
		// �t�H�[�J�X�A���K�E�V�A���p�����[�^
		if ( eEntireMode == INPUT_MOVIE_MODE )	focus_tb = 112;	// ������͂̎��̓v���Z�b�g��
		if ( eFocusCoupledGaussian )	eCamGaussMaskSize1 = (int)(0.0326 * (double)focus_tb - 2)*2+1;	// �����̏ꍇ
//		if ( eFocusCoupledGaussian )	eCamGaussMaskSize2 = (int)(0.0652 * (double)focus_tb - 2)*2+1;	// �P��̏ꍇ
		if ( eFocusCoupledGaussian )	eCamGaussMaskSize2 = (int)(0.08 * (double)focus_tb - 2)*2+1;	// �P��̏ꍇ
		if ( eCamGaussMaskSize1 < 1 )	eCamGaussMaskSize1 = 1;
		if ( eCamGaussMaskSize2 < 1 )	eCamGaussMaskSize2 = 1;
		// �����摜����d�S���v�Z����
		num1 = MakeCentresFromImage( &ps1, img_con1, &img_size, &areas1 );
		num2 = MakeCentresFromImage( &ps2, img_con2, &img_size, &areas2 );
		printf("%d, %d\n", num1, num2);
		end_fp = GetProcTimeMiliSec();
//		printf("cap : %dms\nfp : %dms\n", end_cap - start_cap, end_fp - start_fp);
//		if ( img_pt != NULL )	cvReleaseImage( &img_pt );
		// �\���摜�̍쐬
		if ( draw_fp > 0 || draw_con > 0)	disp_cap = NULL;
		else	disp_cap = img_cap0;
		if ( draw_con > 0 || draw_con_overlap > 0 )	disp_con = img_con10;
		else	disp_con = NULL;
		if ( draw_fp > 0 || draw_fp_overlap > 0 ) {
			disp_ps = ps10;
			disp_num = num10;
		} else {
			disp_ps = NULL;
			disp_num = 0;
		}
		MakeDispImage( img_disp, disp_cap, disp_con, disp_ps, disp_num );
		start_com = GetProcTimeMiliSec();
		// ���ʂ���M�i��M���ɂ��邱�ƂŁA�T�[�o�Ƃ̕��񏈗����\�ɂȂ�B�Ȃ����ڂ̓_�~�[�̃f�[�^��������j
#ifdef	DRAW_COR
		printf("%x\n", ps );
		RecvResultCor( sock, doc_name, 20, corps, &corpsnum, &query_size, &reg_size );
#else
//		puts("recv start");
		if ( eProtocol == kTCP ) {
			RecvResultParam( sock, doc_name, kMaxDocNameLen, &param, &res_size );
		}
		else {
			RecvResultParam( sockres, doc_name, kMaxDocNameLen, &param, &res_size );
		}
#endif
//		printf("recv : %d ms\n", GetProcTimeMiliSec() - start_com );
		// �����_���T�[�o�֑��M
//		start_com = GetProcTimeMiliSec();
//		puts("send start");
		if ( eProtocol == kTCP ) {
			SendPointsAreas( sock, ps1, areas1, num1, &img_size, &addr, eProtocol );
			SendPointsAreas( sock, ps2, areas2, num2, &img_size, &addr, eProtocol );
		}
		else {
			SendPointsAreas( sockpt, ps1, areas1, num1, &img_size, &addr, eProtocol );
			SendPointsAreas( sockpt, ps2, areas2, num2, &img_size, &addr, eProtocol );
		}
		end_com = GetProcTimeMiliSec();
//		printf("com : %dms\n", end_com - start_com);
		// ���ʂ�\��
		printf("%s\n", doc_name);
//		printf("size:%d, %d\n", res_size.width, res_size.height );
#ifdef	DRAW_COR
//		if ( corpsnum > 0 )	DrawCor( corps, corpsnum, img_cap, doc_name );
		DrawCor2( corps, corpsnum, img_cap, doc_name );
#else
		// �T���l�C����\��
//		printf( "%x\n", doc_name );
#if 1
		DrawThumb( doc_name, doc_name_prev, &thumb, &thumb_draw, param, zero_param, img_size, res_size );
		cvShowImage( "Thumb", thumb_draw );
//		printf("thumb drawn\n");
#ifdef	DRAW_FLAG
		// ����̉摜��\��
		printf( "%x\n", doc_name );
		flag = GetFlagImage( doc_name );
		cvShowImage( "Language", flag );
#endif

#ifdef	THUMB_OUT
		OutPutImage( thumb_draw );
#endif
#endif
		if ( ar > 0 ) {	// �g�������̕`��
			DrawAR( img_disp, doc_name, param );	// AR��`��
		}
#endif
		strcpy( doc_name_prev, doc_name );
#ifdef	VIDEO_OUTPUT
		cvResize( img_cap0, small_cap, CV_INTER_NN );
		cvResize( thumb_draw, small_thumb, CV_INTER_NN );
		// �k�������_�摜�̍쐬
		cvZero( small_pt1 );
		cvNot( small_pt1, small_pt1 );
		for ( i = 0; i < num1; i++ ) {
			point.x = (int)((double)ps1[i].x * (double)small_pt_size.width / (double)img_size.width);
			point.y = (int)((double)ps1[i].y * (double)small_pt_size.height / (double)img_size.height);
			cvCircle( small_pt1, point, 1, CV_RGB(0,0,0), -1, CV_AA, 0 );
		}
		cvZero( small_pt2 );
		cvNot( small_pt2, small_pt2 );
		for ( i = 0; i < num2; i++ ) {
			point.x = (int)((double)ps2[i].x * (double)small_pt_size.width / (double)img_size.width);
			point.y = (int)((double)ps2[i].y * (double)small_pt_size.height / (double)img_size.height);
			cvCircle( small_pt2, point, 1, CV_RGB(0,0,0), -1, CV_AA, 0 );
		}
#ifdef	POINT_OUTPUT
		OutPutImage( small_pt1 );
		OutPutImage( small_pt2 );
#endif
//		MergeImage3( small_cap, small_thumb, small_pt, merge );
		MergeImage4( small_cap, small_thumb, small_pt1, small_pt2, merge );
		// �L���v�`���摜���r�f�I�ɏo��
		cvWriteFrame( video_wr, merge );
#endif
		// �L���v�`���E�B���h�E�̕`��
		if ( eCaptureWidth == 0 ) {	// ���w��Ȃ�
			cvShowImage( "Capture", img_disp );
		}
		else {	// ���w�肠��
			cvResize( img_disp, cap_resized, CV_INTER_LINEAR );	// ���T�C�Y
			cvShowImage( "Capture", cap_resized );
		}
		end = GetProcTimeMiliSec();
//		printf("Total : %dms\n", end - start );
		// �t�H�[�J�X�̃g���b�N�o�[�֌W
		if ( focus_available ) {
//			if ( auto_focus_tb == 0 )
				focus_flags = 2;	// �}�j���A��
//			else
//				focus_flags = 3;		// �I�[�g
			SetCameraControlProperty( dsc.pCameraControl, prop, focus_tb + focus_min, focus_flags );
			focus = GetCameraControlProperty( dsc.pCameraControl, prop, &focus_flags );
			// �g���b�N�o�[�ϐ��Ƀt�B�[�h�o�b�N
			if ( focus_flags == 3 )	auto_focus_tb = 1;
			else	auto_focus_tb = 0;
			focus_tb = focus - focus_min;
//			cvSetTrackbarPos( "Manu/Auto", "Capture", auto_focus_tb );
			cvSetTrackbarPos( "Focus", "Capture", focus_tb );
//			printf( "%ld:%ld\n", focus_flags, focus );
		}
		// �K�E�V�A���t�B���^�̃g���b�N�o�[�֌W
#ifdef	GAUSS_VARIABLE
		gauss_tb = (int)((double)focus * 0.0565 - 3.5326);
		if ( gauss_tb < 0 )	gauss_tb = 0;
		eCamGaussMaskSize = gauss_tb * 2 + 1;
//		cvSetTrackbarPos( "Gauss", "Capture", gauss_tb );
#endif
		// �L�[���͊֌W
		start = end;
		key = cvWaitKey( 1 );	// �Ȃ��ƃE�B���h�E���X�V����Ȃ�
		if ( key >= 0 ) {
			switch ( key ) {
				case 'p':	// �ꎞ��~
					if ( eEntireMode == CAM_RET_MODE )
						PauseDirectShowCap( &dsc );
					printf("Hit any key to resume\n");
					for ( ; cvWaitKey(100) < 0; );
					if ( eEntireMode == CAM_RET_MODE )
						ResumeDirectShowCap( &dsc );
					break;
				case 'q':	// �I��
					goto end_cap;
#ifndef	FUNC_LIMIT
				case ' ':	// �L���v�`���摜��ۑ�
					OutPutImage( img_cap );
					printf("Save complete\n");
					break;
				case 's':	// �B�e���[�h
					ret = GotoSnapshotMode( &dsc, img_buff, buff_size, img_cap, thumb );
					if ( ret == 1 ) {	// �L���v�`�����Ė߂���
						ret = GotoPasteMode( img_cap, thumb, &ar_p1, &ar_p2 );
						if ( ret == 1 ) {	// �ꏊ���w�肵�Ė߂���
							SaveAR( doc_name, img_cap, cvSize( (int)((double)thumb->width / eThumbScale), (int)((double)thumb->height / eThumbScale)), &ar_p1, &ar_p2 );
						}
					}
					printf("*** Retrieval mode ***\n");
					printf("Pause(p) Capture(c) Snapshot(s) Quit(q)\n");
					break;
				case 'a':	// AR�̕`��̃I���^�I�t
					ar *= -1;
					break;
#endif
				case 'F':	// �����_���[�h�̃I���^�I�t
					draw_fp *= -1;
					draw_fp_overlap = -1;
					break;
				case 'f':	// �����_�㏑�����[�h�̃I���^�I�t
					draw_fp_overlap *= -1;
					draw_fp = -1;
					break;
				case 'C':	// �A���������[�h�̃I���^�I�t
					draw_con *= -1;
					draw_con_overlap = -1;
					break;
				case 'c':	// �A�������㏑�����[�h�̃I���^�I�t
					draw_con_overlap *= -1;
					draw_con = -1;
					break;
				default:
					break;
			}
		}
		// �L���v�`���摜��1�t���[���O�Ƃ̓���ւ�
		tmp = img_cap0;
		img_cap0 = img_cap;
		img_cap = tmp;
		// �A�������摜��1�t���[���O�Ƃ̓���ւ�
		tmp = img_con10;
		img_con10 = img_con1;
		img_con1 = tmp;
		tmp = img_con20;
		img_con20 = img_con2;
		img_con2 = tmp;
//		Sleep(100);
		end_loop = GetProcTimeMiliSec();
		if ( eExperimentMode )	printf( "%d ms\n", end_loop - start_loop );
	}
end_cap:
	// �I������
	end_all = GetProcTimeMiliSec();
	printf("Total: %d ms\n", end_all - start_all );
	// �I���̃��b�Z�[�W�Ƃ��ĕ��̓����_���𑗂�
	if ( eProtocol == kTCP ) {
		SendPointsAreas( sock, NULL, NULL, -1, &img_size, &addr, eProtocol );
		SendPointsAreas( sock, NULL, NULL, -1, &img_size, &addr, eProtocol );
	}
	else {
		SendPointsAreas( sockpt, NULL, NULL, -1, &img_size, &addr, eProtocol );
		SendPointsAreas( sockpt, NULL, NULL, -1, &img_size, &addr, eProtocol );
	}
	CloseWinSock( sock );
	if ( eProtocol == kUDP ) {
		CloseWinSock( sockpt );
		CloseWinSock( sockres );
	}
	cvDestroyWindow( "Capture" );	// �E�B���h�E��j��
#ifdef	DRAW_COR
	cvDestroyWindow( "Corres" );
#else
	cvDestroyWindow( "Thumb" );
#endif
#ifdef	DRAW_HIST
	cvDestroyWindow( "Hist" );
#endif
	if ( eEntireMode == CAM_RET_MODE ) {
		StopDirectShowCap( &dsc );	// �L���v�`�����I��
		ReleaseDirectShowCap( &dsc );	// DirectShow�̏��X�������[�X
	}
	else if ( eEntireMode == INPUT_MOVIE_MODE ) {
		cvReleaseCapture( &cap_mov );
	}
	cvReleaseImage( &img_cap );
	ReleaseCentres( ps1 );
	free( areas1 );
	ReleaseCentres( ps2 );
	free( areas2 );
	free( img_buff );
#ifdef	VIDEO_OUTPUT
	cvReleaseVideoWriter( &video_wr );
#endif
#ifdef	VIDEO_RECORD
	cvReleaseVideoWriter( &video_wr );
#endif
	timeEndPeriod( 1 );

	return 0;
}