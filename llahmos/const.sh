#!/bin/sh

# �o�^����摜�̓_�t�@�C���̃f�B���N�g��
pdir="/home/nakai/dirs/point/icpr06_10000/"
# �_�t�@�C���̑Ή��t�@�C��
dbcor="/home/nakai/dirs/dbcor/icpr06_10000/dbcor.dat"
# �����_���t�@�C��
pn="/home/nakai/dirs/pnum/icpr06_10000/pnum.txt"
# �o�^�摜�̌����p�X
rpath="/home/nakai/dirs/point/icpr06_10000/*.dat"
# ���U���֐�����ѕ␳�萔�̍쐬�p�f�[�^�̌����p�X
mpath="/home/nakai/dirs/point/icpr06_10/*.dat"
# ��������̒T���p�X
# qpath="/home/nakai/image/3deg100/60/*.dat"
# �e��ݒ�t�@�C���̊i�[�f�B���N�g��
ddir="/home/nakai/dirs/icpr06/10000/"
# ���ʏo�̓f�B���N�g��
odir="out/"

inv=af
ch=a
n=7
m=6
#disc=28
disc=15
deg=60

#opath="$odir""$deg""/""$inv""$n""$m""_""$disc""_""$num"".txt"
#apath="$odir""$deg""/avg_""$inv""$n""$m""_""$num"".txt"
#qpath="/home/nakai/image/3deg100/""$deg""/*.dat"

./dcams -"$ch"cf $pdir $dbcor $pn -n"$n" -m"$m" -d"$disc" -D "$mpath" -I $ddir -P "$mpath"