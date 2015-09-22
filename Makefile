all: bin/run

bin/run: obj/main.o obj/listener.o
	gcc $^ -o $@

obj/main.o: src/main.c src/listener.h
	gcc -c $< -o $@

obj/listener.o: src/listener.c src/listener.h
	gcc -c $< -o $@
