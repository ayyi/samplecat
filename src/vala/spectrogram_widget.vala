/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://samplecat.orford.org          |
* | copyright (C) 2007-2013 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/

//
// Johan Dahlin 2008
//
// A quite simple Gtk.Widget subclass which demonstrates how to subclass
// and do realizing, sizing and drawing.
// 
// from http://live.gnome.org/Vala/GTKSample?highlight=%28widget%29%7C%28vala%29

using GLib;
using Gtk;
using Cairo;

[CCode(
   cheader_filename = "stdint.h",
   cheader_filename = "spectrogram.h",
   lower_case_cprefix = "", cprefix = "")]

public delegate void RenderDoneFunc(char* filename, Gdk.Pixbuf* a, void* user_data_);

public extern void get_spectrogram_with_target(char* path, RenderDoneFunc on_ready, void* user_data);
//[CCode (has_target = false)]
//public extern void get_spectrogram(char* path, RenderDoneFunc callback, void* user_data);
public extern void cancel_spectrogram          (char* path);
 
public class SpectrogramWidget : Gtk.Widget
{
	private string _filename;
	private Gdk.Pixbuf* pixbuf = null;

	construct {
	}

	public void image_ready(char* filename, Gdk.Pixbuf* _pixbuf, void* user_data)
	{
		if((bool)pixbuf) pixbuf->unref();
		pixbuf = _pixbuf;
		this.queue_draw();
	}

	public void set_file(char* filename)
	{
		_filename = ((string*)filename)->dup();

		if ((bool)pixbuf) pixbuf->fill(0x0);
		cancel_spectrogram(null);

		//TODO currently not using this as self is not set properly when called?
		RenderDoneFunc p1 = (filename, a, b) => {
			pixbuf = a;
			print("%s: got callback!\n", Log.METHOD);
			this.queue_draw();
		};

		get_spectrogram_with_target(filename, image_ready, null);
		//stdout.printf("set_file: pixbuf=%p\n", pixbuf);

		//_filename = new string(filename);
		//string* result = GLib.malloc0 (((string*)filename)->size());

		/*
		if(!(bool)cr){
			if((flags & Gtk.WidgetFlags.REALIZED) != 0){
				stdout.printf("set_file: creating cr...\n");
				cr = Gdk.cairo_create (this.window);
			}
			else stdout.printf("set_file: not realized\n");
		}

		if((bool)cr){
		*/
			/*
			Cairo.Surface* s = cr.get_target();
			SurfaceType st = s->get_type();
			if(st == Cairo.SurfaceType.XLIB){
				stdout.printf("set_file: type=XLIB\n");
				int width = ((Cairo.ImageSurface*)s)->get_width();
				stdout.printf("set_file: width=%i\n", width);
			}else if(st == Cairo.SurfaceType.IMAGE){
				stdout.printf("set_file: type=IMAGE\n");
			}else{
				stdout.printf("set_file: type=%i\n", st);
			}
			*/

			/*
			Cairo.Status status = s->status();
			if(status == Cairo.Status.SUCCESS){
				stdout.printf("set_file: status=SUCCESS\n");
			}else{
				stdout.printf("set_file: **** status=%i\n", status);
			}
			*/
		/*
		}
		*/
	}

	public override void realize ()
	{
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

	/*
	private uchar* image_32_to_24(uchar* image, int width, int height)
	{
		int rowstride24 = width * 3;
		int rowstride32 = width * 4;

		uchar* img2 = GLib.malloc0 (width * height * 3); //TODO free
		int x;
		for(x=0;x<width;x++){
			int y;
			for(y=0;y<height;y++){
				img2[y * rowstride24 + x * 3    ] = image[y * rowstride32 + x * 4    ];
				img2[y * rowstride24 + x * 3 + 1] = image[y * rowstride32 + x * 4 + 1];
				img2[y * rowstride24 + x * 3 + 2] = image[y * rowstride32 + x * 4 + 2];
			}
		}
		return img2;
	}
	*/

	public override bool expose_event (Gdk.EventExpose event)
	{
		if(pixbuf == null) return true;
		//copy the bitmap onto the window:

		//uchar* image = image_32_to_24((uchar*)data, 640, 640);
		//uchar* image = pixbuf->get_pixels();
		//stdout.printf("expose: pixbuf: %ix%i %i\n", pixbuf->get_width(), pixbuf->get_height(), pixbuf->get_rowstride());

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


		/*
		// draw a rectangle in the foreground color
		if(!(bool)cr){ stdout.printf("expose_event: cr null\n"); return true; }

		var cr = Gdk.cairo_create (this.window);
		Cairo.Surface* s = cr.get_target();
		render_spectrogram(_filename, s);

		Gdk.cairo_set_source_color (cr, this.style.fg[this.state]);
		cr.rectangle (this._BORDER_WIDTH,
		              this._BORDER_WIDTH,
		              this.allocation.width - 2*this._BORDER_WIDTH,
		              this.allocation.height - 2*this._BORDER_WIDTH);
		cr.set_line_width (5.0);
		cr.set_line_join (Cairo.LineJoin.ROUND);
		cr.stroke ();

		// And draw the text in the middle of the allocated space
		int fontw, fonth;
		this._layout.get_pixel_size (out fontw, out fonth);
		cr.move_to ((this.allocation.width - fontw)/2, (this.allocation.height - fonth)/2);
		Pango.cairo_update_layout (cr, this._layout);
		Pango.cairo_show_layout (cr, this._layout);
		*/
		return true;
	}
}

