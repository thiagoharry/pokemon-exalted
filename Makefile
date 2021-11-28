all:
	$(CC) -DW_RNG_CHACHA20 fight.c random.c -o pokemon-exalted

