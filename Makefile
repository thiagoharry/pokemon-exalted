all:
	$(CC) -DW_RNG_CHACHA20 src/fight.c src/random.c -o pokemon-exalted

