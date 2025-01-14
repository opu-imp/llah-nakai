//#define	WIN
#define	LINUX
//#define	RESO_LIMIT
// DLL
#ifdef	WIN	// 現在はWIN版のみ
#define	LLAHDOC_EXPORTS
#endif

// バージョン
#define	kLlahDocVersion	(1.1)

#ifdef	WIN
#define	WIN_TIME
#else	
#define LINUX_TIME
#endif

#ifndef	WIN
#define	SOCKET	int
#endif

#ifdef	WIN
#define	kMkdirNoMesOpt	""
#else
#define	kMkdirNoMesOpt	"-p"
#endif

#ifdef	WIN
#pragma	warning(disable:4996)
#pragma	warning(disable:4819)
#endif

#define	kFileNameLen	(64)
#define	kMaxPathLen	(128)	/* パスの文字列の最大長 */
#define	kMaxLineLen	(1024)	/* fgetsで読み込む場合のバッファサイズ */

#ifndef M_PI /* 円周率 (pi) */
#define M_PI (3.14159265358979323846L)
#endif
