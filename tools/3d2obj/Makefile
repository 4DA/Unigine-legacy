CC = g++

TARGET = 3d2obj
OBJS = main.o mesh.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o
