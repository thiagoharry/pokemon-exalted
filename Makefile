prog:
	$(CC) -DW_RNG_CHACHA20 src/fight.c src/random.c -o pokemon-exalted
tabela: prog
	./generate_table.sh 1> docs/tabela.htm
web:
	emcc -DW_RNG_CHACHA20 src/fight.c src/random.c -o docs/pokemon-exalted.html
