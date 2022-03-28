EXEC=3dLoader

LIBS=-lX11 -lXxf86vm -lGL -lGLU -lSOIL

LIBDIRS=-L/usr/X11R6/lib

SRC=main.c window_manage.c graph.c

all:
	gcc -o $(EXEC) $(SRC) $(LIBS) $(LIBDIRS)
	
debug:
	gcc -g -o $(EXEC) $(SRC) $(LIBS) $(LIBDIRS)
