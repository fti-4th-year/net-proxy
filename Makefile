all: obj bin bin/run

bin:
	mkdir -p bin

obj:
	mkdir -p obj

bin/run: obj/main.o
	gcc $^ -o $@

obj/main.o: src/main.c
	gcc -c $< -o $@
