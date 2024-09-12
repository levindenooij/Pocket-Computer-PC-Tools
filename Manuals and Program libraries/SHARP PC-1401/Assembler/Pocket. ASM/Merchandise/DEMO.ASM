ORG	8192	# Sets the start address of the program

mark:
DB	&20	# Fills this place with a single byte in memory
DB	&20
LIDP	mark	' Sets the DP pointer to marker
JRNZM	mark	# Makes a relative jump to marker when zero flag is not set
RTN		; Returns to system
