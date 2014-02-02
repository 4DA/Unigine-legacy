CC = g++

MAYA_LOCATION = /usr/aw/maya6.0

CFLAGS = -D_BOOL -DLINUX -DUNIX -pipe -Wno-deprecated -fno-gnu-keywords -I$(MAYA_LOCATION)/include
LIBS = -L$(MAYA_LOCATION)/lib -lOpenMaya

TARGET = MeshExport.so
OBJS = main.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) -shared -o $@ $^ $(LIBS)

clean:
	rm -f *.o
