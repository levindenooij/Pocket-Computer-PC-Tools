#/bin/sh
#
#  Make /dev/fd0casio device for access to Casio MD-100 floppy disks
#  Needs fdutils-5.5
#
DEV=/dev/fd0casio
BIN=/user/local/bin

if [ ! -b $DEV ] then
	mknod $DEV b 2 80
	echo Device $DEV created
fi

$BIN/setfdprm $DEV SS DD sect=16 ssize=256
