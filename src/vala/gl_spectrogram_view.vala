/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
using GLib;
using Gtk;
using Gdk;
using GL;

[CCode(
   cheader_filename = "stdint.h",
   cheader_filename = "spectrogram.h",
   lower_case_cprefix = "", cprefix = "")]

public delegate void SpectrogramReady(char* filename, Gdk.Pixbuf* a, void* user_data_);

public extern void get_spectrogram_with_target (char* path, SpectrogramReady on_ready, void* user_data);
public extern void cancel_spectrogram          (char* path);
 
public class GlSpectrogram : Gtk.DrawingArea
{
	public static GlSpectrogram instance;
	private static GLContext glcontext;
	private AGl.Instance* agl;

	private string _filename;
	private Gdk.Pixbuf* pixbuf = null;
	private bool gl_init_done = false;
	private GL.GLuint textures[2];
	private const double near = 10.0;
	private const double far = -100.0;

	public GlSpectrogram ()
	{
		add_events (Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK | Gdk.EventMask.POINTER_MOTION_MASK);

		set_size_request (200, 100);

		var glconfig = new GLConfig.by_mode (GLConfigMode.RGB | GLConfigMode.DOUBLE);
		WidgetGL.set_gl_capability (this, glconfig, glcontext, true, GLRenderType.RGBA_TYPE);

		instance = this;
	}

	~GlSpectrogram ()
	{
		cancel_spectrogram(null);
	}

	public static void set_gl_context(GLContext _glcontext)
	{
		glcontext = _glcontext;
	}

	private void load_texture()
	{
		GLDrawable gldrawable = WidgetGL.get_gl_drawable(this);

		if (!gldrawable.gl_begin(WidgetGL.get_gl_context(this))) print("gl context error!\n");
		if (!gldrawable.gl_begin(WidgetGL.get_gl_context(this))) return;

		glBindTexture(GL_TEXTURE_2D, textures[0]);

		Gdk.Pixbuf* scaled = pixbuf->scale_simple(256, 256, Gdk.InterpType.BILINEAR);
		//print("GlSpectrogram.load_texture: %ix%ix%i\n", scaled->get_width(), scaled->get_height(), scaled->get_n_channels());

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GL.GLint n_colour_components = (GL.GLint)scaled->get_n_channels();
		GL.GLenum format = scaled->get_n_channels() == 4 ? GL_RGBA : GL_RGB;
		if((bool)gluBuild2DMipmaps(GL_TEXTURE_2D, n_colour_components, (GL.GLsizei)scaled->get_width(), (GL.GLsizei)scaled->get_height(), format, GL_UNSIGNED_BYTE, scaled->get_pixels()))
			print("mipmap generation failed!\n");

		scaled->unref();

		gldrawable.gl_end();
	}

	public override bool configure_event (EventConfigure event)
	{
		GLDrawable gldrawable = WidgetGL.get_gl_drawable(this);

		if (!gldrawable.gl_begin(WidgetGL.get_gl_context(this))) return false;

		glViewport(0, 0, (GLsizei)allocation.width, (GLsizei)allocation.height);

		if(!gl_init_done){
			glGenTextures(1, textures);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			gl_init_done = true;
		}

		gldrawable.gl_end();

		agl = AGl.get_instance();

		return true;
	}

	public override bool expose_event (Gdk.EventExpose event)
	{
		GLDrawable gldrawable = WidgetGL.get_gl_drawable (this);

		if (!gldrawable.gl_begin (WidgetGL.get_gl_context(this))) return false;

		set_projection(); // has to be done on each expose when using shared context.

		if(agl->use_shaders){
			agl->shaders.texture.uniform.fg_colour = (uint32)0xffffffff;
			AGl.use_program((AGl.Shader*)agl->shaders.texture);
		}

		glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textures[0]);

		double x = 0.0;
		double w = allocation.width;
		double top = allocation.height;
		double botm = 0.0;
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0); glVertex2d(x,     top);
		glTexCoord2d(1.0, 0.0); glVertex2d(x + w, top);
		glTexCoord2d(1.0, 1.0); glVertex2d(x + w, botm);
		glTexCoord2d(0.0, 1.0); glVertex2d(x,     botm);
		glEnd();

		gldrawable.swap_buffers ();
		gldrawable.gl_end ();

		return true;
	}

	private void set_projection()
	{
		glViewport(0, 0, (GLsizei)allocation.width, (GLsizei)allocation.height);
		glMatrixMode((GL.GLenum)GL_PROJECTION);
		glLoadIdentity();

		double left = 0.0;
		double right = allocation.width;
		double top   = allocation.height;
		double bottom = 0.0;
		glOrtho (left, right, bottom, top, near, far);
	}

	/*
	public override bool button_press_event (Gdk.EventButton event) {
		return false;
	}

	public override bool button_release_event (Gdk.EventButton event) {
		return false;
	}

	public override bool motion_notify_event (Gdk.EventMotion event) {
		return false;
	}
	*/

	public void set_file(char* filename)
	{
		_filename = ((string*)filename)->dup();

		cancel_spectrogram(null);

		get_spectrogram_with_target(filename, (filename, _pixbuf, b) => {
			if((bool)_pixbuf){
				if((bool)pixbuf) pixbuf->unref();
				pixbuf = _pixbuf;
				load_texture();
				this.queue_draw();
			}
		}, null);
	}

#if 0
	public override void realize ()
	{
		print("GlSpectrogram.realize\n");

		/*
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
		*/
		base.realize();
	}
#endif

	public override void unrealize ()
	{
		cancel_spectrogram(null);
		base.unrealize();
	}
}

