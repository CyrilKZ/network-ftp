objects = server.o report.o command.o session.o
server: $(objects)
server.o: server.c server.h
report.o: report.c server.h
command.o: command.c server.h
session.o: session.c server.h
clean:
	rm server $(objects)