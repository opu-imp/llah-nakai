#define	kIniFileName	"server.ini"
#define	kDefaultTCPPort		(12345)
#define	kDefaultProtocol	(2)	/* 1ÍTCPC2ÍUDP */
#define	kDefaultPointPort	(65431)
#define	kDefaultResultPort	(65432)
#define	kDefaultClientName	"localhost"
#define	kDefaultServerName	"localhost"

// öJÉü¯ÄüÏ
#define	kDfltDirsDir	"./"
#define	kDfltHashSrcDir	"./"
#define	kDfltHashSrcSuffix	"bmp"
#define	kDfltPointDatFileName	"point.dat"
#define	kDfltHashDatFileName	"hash.dat"
#define	kDfltConfigFileName	"config.dat"

#define	kCopyright	"Copyright (C) 2006 Tomohiro Nakai, Intelligent Media Processing Laboratory, Osaka Prefecture University"

// Á¥_o
//#define	kDfltHashGaussMaskSize	(15)
// ÅyÊÌÁ¥_oÌ½ßÉÏX@07/06/07
#define	kDfltHashGaussMaskSize	(5)

// Á¥ÊvZ
#define	kDfltInvType	AFFINE
#define	kDfltGroup1Num	(7)
#define	kDfltGroup2Num	(6)
#define	kDfltDiscNum	(15)
#define	kDfltTerminate	TERM_NON
#define	kDfltIncludeCentre	0

// £U»
#define	kDfltDiscFileName	"disc.txt"
#define	kDfltDocNumForMakeDisc	(10)

// äáè
#define	kDfltPropMakeNum	(10)

// êÊ
#define	kConfigVerStr	"<config.dat 1.0>"

// nbV³k
#define	kCompressHashDiscroptor	"Compress Hash"
#define	kNormalHashDiscroptor	"Normal Hash"

int AnalyzeArgAndSetExtern( int argc, char *argv[], int *p_emode, int *p_rmode, int *p_fmode, int *p_pmode );
int AnalyzeArgAndSetExtern2( int argc, char *argv[] );
int SetExtern( int emode, int n, int m, int inv_type, int disc_num, double prop, int inc_centre, int use_area, int vec_chk, \
 char *hs_path, char *dirs_dir, char *pf_pref, char *pf_suf, char *cor_fn, \
 char *disc_dir, char *dm_path, char *prop_path, char *pn_fn, char *hash_fn, int dbdocs, int start_num );
int SaveSetting( int inv_type, int n, int m, int d, char *point_dir, char *dbcor_fn, char *pn_fn );
int LoadSetting( int *inv_type, int *n, int *m, int *d, char *point_dir, char *dbcor_fn, char *pn_fn, double *p, int *dbdocs );
int ReadIniFile( void );
int SaveConfig( void );
int LoadConfig( void );