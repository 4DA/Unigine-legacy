CC = g++

CFLAGS = -DGL_GLEXT_LEGACY -DGL_GLEXT_PROTOTYPES
LIBS = -lGL -lGLU -ljpeg -lpng

TARGET = main
OBJS = main.o texture.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET) *.o
