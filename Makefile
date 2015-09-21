all: bin/run

bin/run: obj/main.o
	gcc $^ -o $@

obj/main.o: src/main.c
	gcc -c $< -o $@
