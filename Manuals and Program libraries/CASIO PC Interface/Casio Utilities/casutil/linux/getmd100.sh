#/bin/sh
#
# Get the md100 disk image from a floppy under Linux with fdutils-5.5 installed
#
# To setup the drive: setfdprm /dev/fd0casio SS DD sect=80 ssize=256
#
rm md100
dd ibs=256 obs=1024 count=1280 if=/dev/fd0casio of=md100
