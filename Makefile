clean:
	rm src/*.out src/*.o src/*.h.gch client server *.o

cleancs:
	rm *o client server

build: main2.o
	g++ -std=c++11 src/dataframe.h src/key.h src/kvstore.h main2.o

main2.o:
	g++ -std=c++11 -c tests/m2/main2.cpp -o main2.o

run: build
	./a.out

valgrind: 
	valgrind --leak-check=full -v ./a.out

buildcs: client.o server.o src/object.h
	g++ -std=c++11 src/network/network.h client.o -o client
	g++ -std=c++11 src/network/network.h server.o -o server

client.o:
	g++ -std=c++11 -c src/network/client.cpp -o client.o

server.o:
	g++ -std=c++11 -c src/network/server.cpp -o server.o
