#include "defines.h"

extern XSceneGraph scene_graph;

extern Display *display;    /* Указатель на структуру Display */
extern int screen;    /* Номер экрана */
extern Window window;
extern GLXContext context;
extern XSetWindowAttributes winAttr;
extern int fullscreen;
extern int doubleBuffered;

GLfloat rotQuad = 0.0f;
extern XF86VidModeModeInfo desktopMode;
extern int x, y;
extern unsigned int width, height, lastWidth, lastHeight;
extern unsigned int depth;

extern char path_mtl[256];
extern char object_filename[256];

static int attrListSgl[] = {
    GLX_RGBA, GLX_RED_SIZE, 4, 
    GLX_GREEN_SIZE, 4, 
    GLX_BLUE_SIZE, 4, 
    GLX_DEPTH_SIZE, 16,
    None
}; 

static int attrListDbl[] = {
    GLX_RGBA, GLX_DOUBLEBUFFER, 
    GLX_RED_SIZE, 4, 
    GLX_GREEN_SIZE, 4, 
    GLX_BLUE_SIZE, 4, 
    GLX_DEPTH_SIZE, 16,
    None
};

void createWindow() {
    XVisualInfo *vi;
    Colormap cmap;
    int i, dpyWidth, dpyHeight;
    int glxMajor, glxMinor, vmMajor, vmMinor;
    XF86VidModeModeInfo **modes;
    int modeNum, bestMode;
    Atom wmDelete;
    Window winDummy;
    unsigned int borderDummy;
   
    /* set best mode to current */
    bestMode = 0;
    /* get a connection */
    display = XOpenDisplay(0);
    screen = DefaultScreen(display);
    XF86VidModeQueryVersion(display, &vmMajor, &vmMinor);
    printf("XF86 VideoMode extension version %d.%d\n", vmMajor, vmMinor);
    XF86VidModeGetAllModeLines(display, screen, &modeNum, &modes);
    desktopMode = *modes[0];
    /* look for mode with requested resolution */
    for (i = 0; i < modeNum; i++) {
        if ((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height))
            bestMode = i;
    }
    /* get an appropriate visual */
    vi = glXChooseVisual(display, screen, attrListDbl);
    if (vi == NULL)
    {
        vi = glXChooseVisual(display, screen, attrListSgl);
        doubleBuffered = False;
        printf("singlebuffered rendering will be used, no doublebuffering available\n");
    }
    else
    {
        doubleBuffered = True;
        printf("doublebuffered rendering available\n");
    }
    glXQueryVersion(display, &glxMajor, &glxMinor);
    printf("GLX-Version %d.%d\n", glxMajor, glxMinor);
    /* create a GLX context */
    context = glXCreateContext(display, vi, 0, GL_TRUE);
    /* create a color map */
    cmap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
    winAttr.colormap = cmap;
    winAttr.border_pixel = 0;

    if (fullscreen) {
        /* switch to fullscreen */
        XF86VidModeSwitchToMode(display, screen, modes[bestMode]);
        XF86VidModeSetViewPort(display, screen, 0, 0);
        width = dpyWidth = modes[bestMode]->hdisplay;
        height = dpyHeight = modes[bestMode]->vdisplay;
        printf("resolution %dx%d\n", dpyWidth, dpyHeight);
        XFree(modes);
   
        /* set window attributes */
        winAttr.override_redirect = True;
        winAttr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
        window = XCreateWindow(display, RootWindow(display, vi->screen),
               0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
               CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
               &winAttr);
        XWarpPointer(display, None, window, 0, 0, 0, 0, 0, 0);

        XMapRaised(display, window);

        XGrabKeyboard(display, window, True, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(display, window, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
    }
    else {
        /* create a window in window mode*/
        winAttr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
        window = XCreateWindow(display, RootWindow(display, vi->screen),
               0, 0, lastWidth ? lastWidth : width, lastHeight ? lastHeight : height, 0, vi->depth, InputOutput, vi->visual,
               CWBorderPixel | CWColormap | CWEventMask, &winAttr);
        /* only set window title and handle wm_delete_events if in windowed mode */
        wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(display, window, &wmDelete, 1);
        XSetStandardProperties(display, window, TITLE, TITLE, None, NULL, 0, NULL);
        XMapRaised(display, window);
    }
    /* connect the glx-context to the window */
    glXMakeCurrent(display, window, context);
    if (glXIsDirect(display, context)) 
        printf("DRI enabled\n");
    else
        printf("no DRI available\n");
    initGL();
}


void destroyWindow() {
    if ( context ) {
        if( !glXMakeCurrent(display, None, NULL)) {
            printf("Could not release drawing context.\n");
        }
        /* destroy the context */
        glXDestroyContext(display, context);
        context = NULL;
    }
    /* switch back to original desktop resolution if we were in fullscreen */
    if ( fullscreen ) {
        XF86VidModeSwitchToMode(display, screen, &desktopMode);
        XF86VidModeSetViewPort(display, screen, 0, 0);
    }
    else {
    	lastWidth = width;
    	lastHeight = height;
    }
    XCloseDisplay(display);
}


void resizeGL(unsigned int width, unsigned int height) {
    /* prevent divide-by-zero */
    if (height == 0)
        height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}


void initGL() {
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    /* we use resizeGL once to set up our initial perspective */
     resizeGL(width, height);
    /* Reset the rotation angle of our object */
    rotQuad = 0;
    glFlush();
}


void renderGL() {
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();
   glTranslatef(0.0f, 0.0f, -5.0f);
   glRotatef(rotQuad, 0.0f, -1.0f, 0.0f);
   
   XDrawSceneGraph(&scene_graph);
   
   rotQuad += 0.1f;
   /* swap the buffers if we have doublebuffered */
   if (doubleBuffered)
   {
       glXSwapBuffers(display, window);
   }
}


GLuint loadGlTextureFromFile(char *filename, char *path_mtl) {
    GLuint texture;
    
    #ifndef TEXTURE_DEBUG
        int img_width, img_height;
        char rel_path[256];
        strcpy(rel_path, path_mtl);
        strcat(rel_path, filename);
        
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &texture);
	    glBindTexture(GL_TEXTURE_2D, texture);
	    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	    unsigned char* image = SOIL_load_image(rel_path, &img_width, &img_height, 0, SOIL_LOAD_RGB);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	    SOIL_free_image_data(image);
	    glBindTexture(GL_TEXTURE_2D, 0);
	
	#else
	
	    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        Pixmap pixmap;
        int pixmap_width = 128, pixmap_height = 128;
        GC gc;
        XImage *xim;
	
	    pixmap	= XCreatePixmap(display, window, pixmap_width, pixmap_height, DefaultDepth(display, screen));
        gc = DefaultGC(display, 0);

        XSetForeground(display, gc, 0x00c0c0);
        XFillRectangle(display, pixmap, gc, 0, 0, pixmap_width, pixmap_height);

        XSetForeground(display, gc, 0x000000);
        XFillArc(display, pixmap, gc, 15, 25, 50, 50, 0, 360*64);

        XSetForeground(display, gc, 0x0000ff);
        XDrawString(display, pixmap, gc, 10, 15, "PIXMAP TO TEXTURE", strlen("PIXMAP TO TEXTURE"));

        XSetForeground(display, gc, 0xff0000);
        XFillRectangle(display, pixmap, gc, 75, 75, 45, 35);

        XFlush(display);
        xim = XGetImage(display, pixmap, 0, 0, pixmap_width, pixmap_height, AllPlanes, ZPixmap);

        if(xim == NULL) {
            printf("\n\tximage could not be created.\n\n");
        }

        /*	CREATE TEXTURE FROM PIXMAP */

        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixmap_height, pixmap_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)(&(xim->data[0])));
        glBindTexture(GL_TEXTURE_2D, 0);

        XDestroyImage(xim);
        
    #endif
	
	return texture;
}

int dispatch(Display *display, Window window) {
    XEvent report;
    KeySym sym;
    
    int done = 0;
    Atom delwin = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delwin, 1);

    while ( !done ) {
        while (XPending(display) > 0) {
            XNextEvent ( display, &report );

            switch ( report.type ) {
                case Expose :
                    renderGL();
                    break;

                case ButtonPress:
                    break;

                case ConfigureNotify: /* call resizeGL only if our window-size changed */
                    width = report.xconfigure.width;
                    height = report.xconfigure.height;
                    resizeGL(width, height);
                    break;

                case KeyPress:
                    sym = XKeycodeToKeysym(display, report.xkey.keycode, 0);
                    if(sym == XK_Escape) {                    /* Press F10 for programm exit */
                        done = 1;
                    }
                    if(sym == XK_F1) {                    /* Press F10 for programm exit */
                        destroyWindow();
                        fullscreen = !fullscreen;
                        return 1;
                    }
                    break;
                    
                case ClientMessage:
                   done = *report.xclient.data.l == delwin;
                   break;
            }
        }
        renderGL();
    }
    
    return 0;
}
