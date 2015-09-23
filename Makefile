all: obj bin bin/run

bin:
	mkdir -p bin

obj:
	mkdir -p obj

bin/run: obj/main.o obj/listener.o
	gcc $^ -o $@ -lpthread

obj/main.o: src/main.c src/listener.h
	gcc -c $< -o $@

obj/listener.o: src/listener.c src/listener.h
	gcc -c $< -o $@
