#include "defines.h"

XSceneGraph scene_graph;

Display *display;    /* Указатель на структуру Display */
int screen;    /* Номер экрана */
Window window;
GLXContext context;
XSetWindowAttributes winAttr;
int fullscreen = True;
int doubleBuffered;

XF86VidModeModeInfo desktopMode;
int x, y;
unsigned int width, height, lastWidth = 0, lastHeight = 0;
unsigned int depth;

char path_mtl[256];
char object_filename[256];

void main(int argc, char *argv[]) {

    if (argc > 2) {
        strcpy(path_mtl, argv[1]);
        strcpy(object_filename, argv[2]);
    }
    else {
        fprintf(stderr, "Set path to materials and object filename!\n");
        exit(1);
    }
    
    char object_path[512];
    strcpy(object_path, path_mtl);
    strcat(object_path, object_filename);

    /* загрузка графа */
    FILE *file;
    if ((file = fopen(object_path, "r")) == NULL) {
        fprintf(stderr, "Can not open model file!\n");
        exit(1);
    }
    scene_graph.base_scale = 1;
    XLoadSceneGraph(file, &scene_graph, path_mtl);
    fclose(file);

    width = WIDTH;
    height = HEIGHT;

    /* Создадим цикл получения и обработки ошибок */
    do {
        createWindow();
        XLoadTextures(&scene_graph, path_mtl);
    } while (dispatch(display, window));

    /* Очистка ресурсов */
    destroyWindow();
    XFreeSceneGraph(&scene_graph);

    exit(0);
}

