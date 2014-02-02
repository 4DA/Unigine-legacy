CC = g++

CFLAGS = -DGL_GLEXT_LEGACY -DGL_GLEXT_PROTOTYPES
LDFLAGS = -L/usr/X11R6/lib

LIBS = -lX11 -lXxf86vm -lGL -lGLU -ljpeg -lpng -lz
LIBS += -ldl -lopenal -logg -lvorbis -lvorbisfile -lmad -lpthread

#CFLAGS += -DHAVE_GTK `pkg-config gtk+-2.0 --cflags`
#LDFLAGS += `pkg-config gtk+-2.0 --libs`

#CFLAGS += -pg -finstrument-functions
#LDFLAGS += -pg

TARGET = main
OBJS = main.o glapp.o alapp.o engine.o parser.o font.o console.o frustum.o bsp.o position.o \
	pbuffer.o texture.o shader.o material.o light.o flare.o fog.o mirror.o object.o \
	mesh.o meshvbo.o objectmesh.o \
	skinnedmesh.o objectskinnedmesh.o \
	particles.o objectparticles.o \
	physic.o rigidbody.o collide.o joint.o ragdoll.o \
	map.o

#CFLAGS += -DGRAB
#LIBS += -lavcodec
#OBJS += video.o

.cpp.o:
	$(CC) $(CFLAGS) -Wall -O3 -c -o $@ $<

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

prof:
	gprof $(TARGET) > g.out

clean:
	rm -f $(TARGET) *.o gmon.out g.out
