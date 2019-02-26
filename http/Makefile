bin=HttpServer
cc=g++ -std=c++11
LDFLAGS=-lpthread

.PHONY:all
all:$(bin) cal
$(bin):HttpServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) #-D_DEBUG_ #makefile中的注释# 

cal:cal.c
	gcc -o $@ $^

.PHONY:clean	
clean:
		rm -f $(bin) cal
