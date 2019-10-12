objects = server.o report.o
initport = 10020
server: $(objects)
server.o: server.c server.h
report.o: report.c server.h
clean:
	rm server $(objects)
start $(initport):
	make
	./server -port $(initport)