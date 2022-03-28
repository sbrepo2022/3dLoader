#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <X11/extensions/xf86vmode.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "SOIL.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WIDTH 1920
#define HEIGHT 1080
#define WIDTH_MIN 50
#define HEIGHT_MIN 50
#define BORDER_WIDTH 5
#define TITLE "Sergey Borisov's 3dLoader"
#define ICON_TITLE "Sergey Borisov's 3dLoader"
#define PRG_CLASS "3dLoader"

#define DEBUG

void createWindow();
void destroyWindow();
void resizeGL(unsigned int, unsigned int);
void initGL();
void renderGL();
GLuint loadGlTextureFromFile(char *filename, char *path_mtl);

int dispatch(Display *display, Window window);

/* graph */

typedef struct XVertex {
    float x, y, z;
} XVertex;

typedef struct XUVCoordinate {
    float u, v;
} XUVCoordinate;

typedef struct XFace {
    XVertex **vertices;
    XUVCoordinate **uvs;
    int num_verts;
} XFace;

typedef struct XPolyGraph {
    char *name;
    XVertex **vertices;
    XUVCoordinate **uvs;
    XFace **faces;
    int material_index;
} XPolyGraph;

typedef struct XMaterial {
    char *name;
    char *diffuse_tex_filename;
    GLuint diffuse_tex_id;
} XMaterial;

typedef struct XSceneGraph {
    XPolyGraph **poly_graphs;
    XMaterial **materials;
    int base_scale;
} XSceneGraph;


int XLoadSceneGraph(FILE *file, XSceneGraph *scene_graph, char *path_mtl);
int XLoadMTLLib(FILE *file, XSceneGraph *scene_graph);
int XLoadTextures(XSceneGraph *scene_graph, char *path_mtl);
void XFreeSceneGraph(XSceneGraph *scene_graph);
void XDrawSceneGraph(XSceneGraph *scene_graph);

