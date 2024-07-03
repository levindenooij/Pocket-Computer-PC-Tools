#/bin/sh
#
# Put the md100 disk image to a floppy under Linux with fdutils installed
#
# to setup the drive: setfdprm /dev/fd0casio SS DD sect=80 ssize=256
#
dd ibs=256 obs=256 count=1280 of=/dev/fd0casio if=md100
