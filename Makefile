all: obj bin bin/run

bin:
	mkdir -p bin

obj:
	mkdir -p obj

bin/run: obj/main.o obj/listener.o
	gcc -g $^ -o $@ -lpthread

obj/main.o: src/main.c src/listener.h
	gcc -g -c $< -o $@

obj/listener.o: src/listener.c src/listener.h
	gcc -g -c $< -o $@ -DUSE_POLL

clean:
	rm -r ./bin/
	rm -r ./obj/
