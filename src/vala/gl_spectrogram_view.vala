using GLib;
using Gtk;
using Gdk;
using GL;
using GLU;
using Cairo;

[CCode(
   cheader_filename = "stdint.h",
   cheader_filename = "spectrogram.h",
   lower_case_cprefix = "", cprefix = "")]

public delegate void SpectrogramReady(char* filename, Gdk.Pixbuf* a, void* user_data_);

public extern void get_spectrogram_with_target (char* path, SpectrogramReady on_ready, void* user_data);
public extern void cancel_spectrogram          (char* path);
 
public class GlSpectrogram : Gtk.DrawingArea {
	private string _filename;
	private Gdk.Pixbuf* pixbuf = null;
	private bool gl_init_done = false;
	private GL.GLuint Textures[2];
	public static GlSpectrogram instance;

	public GlSpectrogram ()
	{
		//stdout.printf("GlSpectrogram\n");
		add_events (Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK | Gdk.EventMask.POINTER_MOTION_MASK);

		set_size_request (200, 100);

		var glconfig = new GLConfig.by_mode (GLConfigMode.RGB | GLConfigMode.DOUBLE);
		WidgetGL.set_gl_capability (this, glconfig, null, true, GLRenderType.RGBA_TYPE);

		instance = this;
	}

	private void load_texture()
	{
		//stdout.printf("load_texture...\n");
		GLContext glcontext = WidgetGL.get_gl_context(this);
		GLDrawable gldrawable = WidgetGL.get_gl_drawable(this);

		if (!gldrawable.gl_begin(glcontext)) stdout.printf("gl context error!\n");
		if (!gldrawable.gl_begin(glcontext)) return;

		glBindTexture(GL_TEXTURE_2D, Textures[0]);

		Gdk.Pixbuf* scaled = pixbuf->scale_simple(256, 256, Gdk.InterpType.BILINEAR);
		//stdout.printf("load_texture: %ix%ix%i\n", scaled->get_width(), scaled->get_height(), scaled->get_n_channels());

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GL.GLint n_colour_components = (GL.GLint)scaled->get_n_channels();
		GL.GLenum format = scaled->get_n_channels() == 4 ? GL_RGBA : GL_RGB;
		if((bool)gluBuild2DMipmaps(GL_TEXTURE_2D, n_colour_components, (GL.GLsizei)scaled->get_width(), (GL.GLsizei)scaled->get_height(), format, GL_UNSIGNED_BYTE, scaled->get_pixels()))
			stdout.printf("mipmap generation failed!\n");

		scaled->unref();

		gldrawable.gl_end();
	}

	public override bool configure_event (EventConfigure event)
	{
		GLContext glcontext = WidgetGL.get_gl_context(this);
		GLDrawable gldrawable = WidgetGL.get_gl_drawable(this);

		if (!gldrawable.gl_begin(glcontext)) return false;

		glViewport(0, 0, (GLsizei)allocation.width, (GLsizei)allocation.height);

		if(!gl_init_done){
			stdout.printf("GlSpectrogram: texture init...\n");
			glGenTextures(1, Textures);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, Textures[0]);

			gl_init_done = true;
		}

		gldrawable.gl_end();
		return true;
	}

	public override bool expose_event (Gdk.EventExpose event)
	{
		//stdout.printf("expose: x=%i width=%i height=%i\n", event.area.x, event.area.width, event.area.height);
		/*
		var cr = Gdk.cairo_create (this.window);

		cr.rectangle (event.area.x, event.area.y, event.area.width, event.area.height);
		cr.clip ();

		draw (cr);
		*/

		GLContext glcontext = WidgetGL.get_gl_context (this);
		GLDrawable gldrawable = WidgetGL.get_gl_drawable (this);

		if (!gldrawable.gl_begin (glcontext)) return false;

		glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT);

		int x = -1;
		int top = 1;
		int botm = -1;
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0); glVertex2d(x+0.0, top);
		glTexCoord2d(1.0, 0.0); glVertex2d(x+2.0, top);
		glTexCoord2d(1.0, 1.0); glVertex2d(x+2.0, botm);
		glTexCoord2d(0.0, 1.0); glVertex2d(x+0.0, botm);
		glEnd();

		if (gldrawable.is_double_buffered ())
			gldrawable.swap_buffers ();
		else
			glFlush ();

		gldrawable.gl_end ();
		return true;
	}

	public override bool button_press_event (Gdk.EventButton event) {
		return false;
	}

	public override bool button_release_event (Gdk.EventButton event) {
		return false;
	}

	public override bool motion_notify_event (Gdk.EventMotion event) {
		return false;
	}

	/*
	private bool on_configure_event (Widget widget, EventConfigure event)
	{
		stdout.printf("on_configure_event");
		GLContext glcontext = WidgetGL.get_gl_context (widget);
		GLDrawable gldrawable = WidgetGL.get_gl_drawable (widget);

		if (!gldrawable.gl_begin (glcontext)) return false;

		glViewport (0, 0, (GLsizei) widget.allocation.width, (GLsizei) widget.allocation.height);

		gldrawable.gl_end ();
		return true;
	}
	*/

	public void set_file(char* filename)
	{
		_filename = ((string*)filename)->dup();

		cancel_spectrogram(null);

		get_spectrogram_with_target(filename, (filename, _pixbuf, b) => {
			//stdout.printf("set_file: got callback!\n");
			if((bool)_pixbuf){
				if((bool)pixbuf) pixbuf->unref();
				pixbuf = _pixbuf;
				load_texture();
				this.queue_draw();
			}
		}, null);
	}

	/*
	public override void realize ()
	{
		stdout.printf("realize");
		this.set_flags (Gtk.WidgetFlags.REALIZED);

		var attrs = Gdk.WindowAttr ();
		attrs.window_type = Gdk.WindowType.CHILD;
		attrs.width = this.allocation.width;
		attrs.wclass = Gdk.WindowClass.INPUT_OUTPUT;
		attrs.event_mask = this.get_events() | Gdk.EventMask.EXPOSURE_MASK;
		this.window = new Gdk.Window (this.get_parent_window (), attrs, 0);

		this.window.set_user_data (this);

		this.style = this.style.attach (this.window);

		this.style.set_background (this.window, Gtk.StateType.NORMAL);
		this.window.move_resize (this.allocation.x, this.allocation.y, this.allocation.width, this.allocation.height);
	}

	public override void unrealize ()
	{
		this.window.set_user_data (null);
	}

	public override void size_request (out Gtk.Requisition requisition)
	{
		requisition.width = 10;
		requisition.height = 80;

		Gtk.WidgetFlags flags = this.get_flags();
		if((flags & Gtk.WidgetFlags.REALIZED) != 0){
			//cr = Gdk.cairo_create (this.window);
		}
	}

	public override void size_allocate (Gdk.Rectangle allocation)
	{
		this.allocation = (Gtk.Allocation)allocation;

		if ((this.get_flags () & Gtk.WidgetFlags.REALIZED) == 0) return;
		this.window.move_resize (this.allocation.x, this.allocation.y, this.allocation.width, this.allocation.height);
	}
	*/

	/*
	public override bool expose_event (Gdk.EventExpose event)
	{
		if(pixbuf == null) return true;
		//copy the bitmap onto the window:

		//int depth = ((Gdk.Drawable)this.window).get_depth();
		int width = this.allocation.width;
		//int width = 640;
		int height = this.allocation.height;
		//int height = 640;
		//stdout.printf("depth=%i stride=%i\n", depth, width * 3);

		Gdk.Pixbuf* scaled = pixbuf->scale_simple(this.allocation.width, this.allocation.height, Gdk.InterpType.BILINEAR);
		uchar* image = scaled->get_pixels();
		//stdout.printf("expose: pixbuf: %ix%i\n", scaled->get_width(), scaled->get_height());

		//TODO use gdk_draw_pixbuf?
		Gdk.draw_rgb_image(this.window, this.style.fg_gc[Gtk.StateType.NORMAL],
		                   0, 0, width, height,
		                   Gdk.RgbDither.MAX, (uchar[])image, scaled->get_rowstride());

		return true;
	}
	*/
}

