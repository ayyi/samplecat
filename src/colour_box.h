struct _colour_box
{
	GtkWidget* menu;
};

GtkWidget* colour_box_new (GtkWidget* parent);
gboolean   colour_box_add (GdkColor*);

