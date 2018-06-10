/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2012-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define XLIB_ILLEGAL_ACCESS // needed to access Display internals
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include "debug/debug.h"
#include "agl/ext.h"
#include "agl/actor.h"
#include "samplecat/typedefs.h"
#include "keys.h"
#include "gl-app/glx.h"

static AGlRootActor* scene = NULL;
extern Key keys[];
extern GHashTable* key_handlers;

GLboolean need_draw = GL_TRUE;

#ifndef GLX_MESA_swap_control
typedef GLint (*PFNGLXSWAPINTERVALMESAPROC)    (unsigned interval);
typedef GLint (*PFNGLXGETSWAPINTERVALMESAPROC) (void);
#endif

#if !defined( GLX_OML_sync_control ) && defined( _STDINT_H )
#define GLX_OML_sync_control 1
typedef Bool (*PFNGLXGETMSCRATEOMLPROC) (Display*, GLXDrawable, int32_t* numerator, int32_t* denominator);
#endif

#ifndef GLX_MESA_swap_frame_usage
#define GLX_MESA_swap_frame_usage 1
typedef int (*PFNGLXGETFRAMEUSAGEMESAPROC) (Display*, GLXDrawable, float* usage);
#endif

#ifndef USE_GLIB_LOOP
static int current_time();
#endif

#define BENCHMARK
#define NUL '\0'

PFNGLXGETFRAMEUSAGEMESAPROC get_frame_usage = NULL;

static GLboolean has_OML_sync_control = GL_FALSE;
static GLboolean has_SGI_swap_control = GL_FALSE;
static GLboolean has_MESA_swap_control = GL_FALSE;
static GLboolean has_MESA_swap_frame_usage = GL_FALSE;


static char** extension_table = NULL;
static unsigned num_extensions;


void
glx_init(Display* dpy)
{
	PFNGLXSWAPINTERVALMESAPROC set_swap_interval = NULL;
	PFNGLXGETSWAPINTERVALMESAPROC get_swap_interval = NULL;
	GLboolean do_swap_interval = GL_FALSE;
	GLboolean force_get_rate = GL_FALSE;
	int swap_interval = 1;

	// TODO is make_extension_table needed as well as agl_get_extensions ?
	make_extension_table((char*)glXQueryExtensionsString(dpy,DefaultScreen(dpy)));
	has_OML_sync_control = is_extension_supported("GLX_OML_sync_control");
	has_SGI_swap_control = is_extension_supported("GLX_SGI_swap_control");
	has_MESA_swap_control = is_extension_supported("GLX_MESA_swap_control");
	has_MESA_swap_frame_usage = is_extension_supported("GLX_MESA_swap_frame_usage");

	if (has_MESA_swap_control) {
		set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddressARB((const GLubyte*) "glXSwapIntervalMESA");
		get_swap_interval = (PFNGLXGETSWAPINTERVALMESAPROC) glXGetProcAddressARB((const GLubyte*) "glXGetSwapIntervalMESA");
	}
	else if (has_SGI_swap_control) {
		set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddressARB((const GLubyte*) "glXSwapIntervalSGI");
	}
	if (has_MESA_swap_frame_usage) {
		get_frame_usage = (PFNGLXGETFRAMEUSAGEMESAPROC)  glXGetProcAddressARB((const GLubyte*) "glXGetFrameUsageMESA");
	}

	if(_debug_){
		printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
		if(has_OML_sync_control || force_get_rate){
			show_refresh_rate(dpy);
		}
		if(get_swap_interval){
			printf("Default swap interval = %d\n", (*get_swap_interval)());
		}
	}

	if(do_swap_interval){
		if(set_swap_interval != NULL){
			if(((swap_interval == 0) && !has_MESA_swap_control) || (swap_interval < 0)){
				printf( "Swap interval must be non-negative or greater than zero "
					"if GLX_MESA_swap_control is not supported.\n" );
			}
			else{
				(*set_swap_interval)(swap_interval);
			}

			if(_debug_ && (get_swap_interval != NULL)){
				printf("Current swap interval = %d\n", (*get_swap_interval)());
			}
		}
		else {
			printf("Unable to set swap-interval. Neither GLX_SGI_swap_control "
				"nor GLX_MESA_swap_control are supported.\n" );
		}
	}
}


static void
draw(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	agl_actor__paint((AGlActor*)scene);
}


/* new window size or exposure */
void
on_window_resize(int width, int height)
{
	glViewport(0, 0, width, height);
	dbg (2, "viewport: %i %i %i %i", 0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	double left   = 0;
	double right  = width;
	double bottom = height;
	double top    = 0;
	glOrtho (left, right, bottom, top, 1.0, -1.0);

	((AGlActor*)scene)->region = (AGliRegion){
		.x2 = width,
		.y2 = height,
	};
	agl_actor__set_size((AGlActor*)scene);
}


/**
 * Remove window border/decorations.
 */
static void
no_border(Display* dpy, Window w)
{
   static const unsigned MWM_HINTS_DECORATIONS = (1 << 1);
   static const int PROP_MOTIF_WM_HINTS_ELEMENTS = 5;

   typedef struct
   {
      unsigned long       flags;
      unsigned long       functions;
      unsigned long       decorations;
      long                inputMode;
      unsigned long       status;
   } PropMotifWmHints;

   PropMotifWmHints motif_hints;
   Atom prop, proptype;
   unsigned long flags = 0;

   /* setup the property */
   motif_hints.flags = MWM_HINTS_DECORATIONS;
   motif_hints.decorations = flags;

   /* get the atom for the property */
   prop = XInternAtom( dpy, "_MOTIF_WM_HINTS", True );
   if (!prop) {
      /* something went wrong! */
      return;
   }

   /* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
   proptype = prop;

   XChangeProperty(dpy, w,                         /* display, window */
                   prop, proptype,                 /* property, type */
                   32,                             /* format: 32-bit datums */
                   PropModeReplace,                /* mode */
                   (unsigned char *) &motif_hints, /* data */
                   PROP_MOTIF_WM_HINTS_ELEMENTS    /* nelements */
                  );
}


/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
void
make_window(Display* dpy, const char* name, int x, int y, int width, int height, bool fullscreen, AGlRootActor* _scene, Window* winRet, GLXContext* ctxRet)
{
	scene = _scene;

	void scene_needs_redraw(AGlScene* scene, gpointer _){ need_draw = true; }
	scene->draw = scene_needs_redraw;

	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 8,
		None
	};
	XSetWindowAttributes attr;
	unsigned long mask;

	int scrnum = DefaultScreen(dpy);
	Window root = RootWindow(dpy, scrnum);

	if (fullscreen) {
		x = y = 0;
		width = DisplayWidth(dpy, scrnum);
		height = DisplayHeight(dpy, scrnum);
	}

	XVisualInfo* visinfo = glXChooseVisual(dpy, scrnum, attrib);
	if (!visinfo) {
		printf("Error: couldn't get an RGB, Double-buffered visual\n");
		exit(1);
	}

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	Window win = XCreateWindow(dpy, root, 0, 0, width, height, 0, visinfo->depth, InputOutput, visinfo->visual, mask, &attr);

	/* set hints and properties */
	{
		XSizeHints sizehints = {
			.x = x,
			.y = y,
			.width  = width,
			.height = height,
			.flags = USSize | USPosition
		};
		XSetNormalHints(dpy, win, &sizehints);
		XSetStandardProperties(dpy, win, name, name, None, (char **)NULL, 0, &sizehints);
	}

	XMoveWindow(dpy, win, x, y);

	if (fullscreen)
		no_border(dpy, win);

	GLXContext ctx = glXCreateContext(dpy, visinfo, NULL, True);
	if (!ctx) {
		printf("Error: glXCreateContext failed\n");
		exit(1);
	}

	XFree(visinfo);

	*winRet = win;
	*ctxRet = ctx;

	XMapWindow(dpy, win);
	glXMakeCurrent(dpy, win, ctx);
}


#ifndef USE_GLIB_LOOP
/*
 *  This main loop uses a select() on X events so will respond
 *  quickly to X events, but gives lower priority to glib events
 *  such as timeouts and idles. This may prevent animations being smooth.
 *
 *  Adding a source to the glib loop for the X events is better
 *  but is not currently working.
 */
void
event_loop(Display* dpy, Window win)
{
	float frame_usage = 0.0;

	fd_set rfds;

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(dpy->fd, &rfds);
#if 0
		int retval =
#endif
		select(dpy->fd + 1, &rfds, NULL, NULL, &(struct timeval){.tv_usec = 50000});
#if 0
		if(retval > 0) dbg(0, "ok %i", retval);
		if(retval == 0) dbg(0, "timeout");
		if(retval < 0) dbg(0, "error");
#endif

		while (XPending(dpy) > 0) {
			XEvent event;
			XNextEvent(dpy, &event);
			switch (event.type) {
				case Expose:
					need_draw = TRUE;
					break;
				case ConfigureNotify:
					on_window_resize(event.xconfigure.width, event.xconfigure.height);
					need_draw = TRUE;
					break;
				case KeyPress: {
					char buffer[10];
					int code = XLookupKeysym(&event.xkey, 0);

					KeyHandler* handler = g_hash_table_lookup(key_handlers, &code);
					if(handler) handler();

					XLookupString(&event.xkey, buffer, sizeof(buffer), NULL, NULL);
					if (buffer[0] == 27 /* ESC */|| buffer[0] == 'q') {
						return;
					}
					else agl_actor__xevent(scene, &event);

					} break;
				case ButtonPress:
				case ButtonRelease:
					if(event.xbutton.button == 1){
						int x = event.xbutton.x;
						int y = event.xbutton.y;
						dbg(1, "button: %i %i\n", x, y);
					}
					agl_actor__xevent(scene, &event);
					break;
				case MotionNotify:
					agl_actor__xevent(scene, &event);
					break;
			}
		}

		if(need_draw){
			// TODO synchronise to frame clock
			draw();
			glXSwapBuffers(dpy, win);
			need_draw = false;
		}

		if (get_frame_usage) {
			GLfloat temp;

			(*get_frame_usage)(dpy, win, & temp);
			frame_usage += temp;
		}

		if(_debug_ > 1){
			// calc framerate
			static int t0 = -1;
			static int frames = 0;
			int t = current_time();

			if (t0 < 0) t0 = t;

			frames++;

			if (t - t0 >= 5.0) {
				GLfloat seconds = t - t0;
				GLfloat fps = frames / seconds;
				if (get_frame_usage) {
					printf("%d frames in %3.1f seconds = %6.3f FPS (%3.1f%% usage)\n", frames, seconds, fps, (frame_usage * 100.0) / (float) frames );
				}
				else {
					printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
				}
				fflush(stdout);

				t0 = t;
				frames = 0;
				frame_usage = 0.0;
			}
		}

		g_main_context_iteration(NULL, false); // update animations
	}
}
#endif


/**
 * Display the refresh rate of the display using the GLX_OML_sync_control
 * extension.
 */
void
show_refresh_rate(Display* dpy)
{
#if defined(GLX_OML_sync_control) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	int32_t  n;
	int32_t  d;

	PFNGLXGETMSCRATEOMLPROC get_msc_rate = (PFNGLXGETMSCRATEOMLPROC)glXGetProcAddressARB((const GLubyte*) "glXGetMscRateOML");
	if (get_msc_rate != NULL) {
		(*get_msc_rate)(dpy, glXGetCurrentDrawable(), &n, &d);
		printf( "refresh rate: %.1fHz\n", (float) n / d);
		return;
	}
#endif
	printf("glXGetMscRateOML not supported.\n");
}


/**
 * Fill in the table of extension strings from a supplied extensions string
 * (as returned by glXQueryExtensionsString).
 *
 * \param string   String of GLX extensions.
 * \sa is_extension_supported
 */
void
make_extension_table(const char* string)
{
	char** string_tab;
	unsigned  num_strings;
	unsigned  base;
	unsigned  idx;
	unsigned  i;

	/* Count the number of spaces in the string.  That gives a base-line
	 * figure for the number of extension in the string.
	 */

	num_strings = 1;
	for (i = 0 ; string[i] != NUL ; i++) {
		if (string[i] == ' ') {
			num_strings++;
		}
	}

	string_tab = (char**) malloc(sizeof( char * ) * num_strings);
	if (string_tab == NULL) {
		return;
	}

	base = 0;
	idx = 0;

	while (string[ base ] != NUL) {
	// Determine the length of the next extension string.

		for ( i = 0 
		; (string[base + i] != NUL) && (string[base + i] != ' ')
		; i++ ) {
			/* empty */ ;
		}

		if(i > 0){
			/* If the string was non-zero length, add it to the table.  We
			 * can get zero length strings if there is a space at the end of
			 * the string or if there are two (or more) spaces next to each
			 * other in the string.
			 */

			string_tab[ idx ] = malloc(sizeof(char) * (i + 1));
			if(string_tab[idx] == NULL){
				unsigned j = 0;

				for(j = 0; j < idx; j++){
					free(string_tab[j]);
				}

				free(string_tab);

				return;
			}

			(void) memcpy(string_tab[idx], & string[base], i);
			string_tab[idx][i] = NUL;
			idx++;
		}


		// Skip to the start of the next extension string.
		for (base += i; (string[ base ] == ' ') && (string[ base ] != NUL); base++ ) {
			/* empty */;
		}
	}

	extension_table = string_tab;
	num_extensions = idx;
}


/**
 * Determine if an extension is supported. The extension string table
 * must have already be initialized by calling \c make_extension_table.
 * 
 * \param ext  Extension to be tested.
 * \return GL_TRUE of the extension is supported, GL_FALSE otherwise.
 * \sa make_extension_table
 */
GLboolean
is_extension_supported( const char * ext )
{
	unsigned   i;

	for(i=0;i<num_extensions;i++) {
		if(strcmp(ext, extension_table[i]) == 0){
			return GL_TRUE;
		}
	}

	return GL_FALSE;
}


#ifndef USE_GLIB_LOOP
#ifdef BENCHMARK
#include <sys/time.h>
#include <unistd.h>

/*
 * return current time (in seconds)
 */
static int
current_time (void)
{
	struct timeval tv;
#ifdef __VMS
	(void) gettimeofday(&tv, NULL );
#else
	struct timezone tz;
	(void) gettimeofday(&tv, &tz);
#endif
	return (int) tv.tv_sec;
}

#else /*BENCHMARK*/

/* dummy */
static int
current_time (void)
{
	return 0;
}

#endif /*BENCHMARK*/
#endif

//------------------------------------------------------------------------------

#ifdef USE_GLIB_LOOP
// TODO why are we getting very long delays?
// TODO look at the source added by GDK to check for x events
// TODO look at Enlightenment:
//     "The Ecore library uses the select system call by default in its implementation of the main loop. This select function can be replaced with a custom select function when desired. This is how the GLib main loop is integrated. A custom select function is installed, which calls the relevant phases of the GLib main loop (prepare, query, check and dispatch as described above) and performs the polling phase by calling select for the file descriptors monitored by Ecore as well as the file descriptors that need to be monitored for the event sources installed in the GLib main loop. So, in this case, the GLib main loop is a secondary main loop and is integrated with the Ecore main loop."

typedef struct
{
    GSource  source;
    Display* dpy;
    Window   w;
} X11Source;


/*
 *  "For file descriptor sources, the prepare function typically returns FALSE,
 *  since it must wait until poll() has been called before it knows whether any
 *  events need to be processed. It sets the returned timeout to -1 to indicate
 *  that it doesn't mind how long the poll() call blocks. In the check function, it
 *  tests the results of the poll() call to see if the required condition has been
 *  met, and returns TRUE if so."
 */

static bool
x11_fd_prepare (GSource* source, gint* timeout)
{
	*timeout = -1;
	return false;
}


static bool
x11_fd_check (GSource* source)
{
	// this is run after the poll has finished

	return XPending(((X11Source*)source)->dpy); // this doesnt make sense - if we have to check this, what is the point of polling? timeouts?
}


/*
 *  Called to dispatch the event source, after it has returned TRUE in either its prepare or its check function.
 */
static bool
x11_fd_dispatch (GSource* source, GSourceFunc callback, gpointer user_data)
{
	Display* dpy = ((X11Source*)source)->dpy;
	Window window = ((X11Source*)source)->w;

	g_return_if_fail(XPending(dpy) > 0, G_SOURCE_CONTINUE);

	while (XPending(dpy) > 0) {
		XEvent event;
		XNextEvent(dpy, &event);
		switch (event.type) {
			case Expose:
				need_draw = true;
				break;
			case MotionNotify:
				agl_actor__xevent(scene, &event);
				break;
		}
	}
	if(need_draw){
		draw();
		glXSwapBuffers(dpy, window);
		need_draw = false;
	}

	return G_SOURCE_CONTINUE;
}


GMainLoop*
main_loop_new (Display* dpy, Window w)
{
	g_return_val_if_fail(dpy, NULL);

	XSelectInput(dpy, w, StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|PropertyChangeMask/*|KeyPressMask|KeyReleaseMask*/);

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);

	GPollFD dpy_pollfd = {dpy->fd, G_IO_IN | G_IO_HUP | G_IO_ERR, 0};

	static GSourceFuncs x11_source_funcs = {
		x11_fd_prepare,
		x11_fd_check,
		x11_fd_dispatch,
		NULL
	};

	GSource* x11_source = g_source_new(&x11_source_funcs, sizeof(X11Source));
	((X11Source*)x11_source)->dpy = dpy;
	((X11Source*)x11_source)->w = w;
	g_source_add_poll(x11_source, &dpy_pollfd);
	g_source_attach(x11_source, NULL);

	return mainloop;
}
#endif
