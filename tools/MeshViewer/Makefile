CC = g++

CFLAGS = -DGL_GLEXT_LEGACY -DGL_GLEXT_PROTOTYPES
LIBS = -L/usr/X11R6/lib -lX11 -lXxf86vm -lGL -lGLU -ljpeg -lpng -lz

CFLAGS += -DHAVE_GTK `gtk-config --cflags`
LDFLAGS += `gtk-config --libs`

TARGET = MeshViewer
OBJS = main.o glapp.o font.o mesh.o meshvbo.o texture.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o
