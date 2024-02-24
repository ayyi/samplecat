/*
 +----------------------------------------------------------------------+
 | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
 | copyright (C) 2007-2023 Tim Orford <tim@orford.org>                  |
 +----------------------------------------------------------------------+
 | This program is free software; you can redistribute it and/or modify |
 | it under the terms of the GNU General Public License version 3       |
 | as published by the Free Software Foundation.                        |
 +----------------------------------------------------------------------+
 |
 */


static GtkWidget*
filters_new ()
{
	static GHashTable* buttons; buttons = g_hash_table_new(NULL, NULL);

	GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	char* label_text (Observable* filter)
	{
		int len = 22 - strlen(((NamedObservable*)filter)->name);
		char value[20] = {0,};
		g_strlcpy(value, filter->value.c ? filter->value.c : "", len);
		return g_strdup_printf("%s: %s%s", ((NamedObservable*)filter)->name, value, filter->value.c && strlen(filter->value.c) > len ? "..." : "");
	}

	for (int i = 0; i < N_FILTERS; i++) {
		Observable* filter = samplecat.model->filters3[i];
		dbg(2, "  %s", filter->value.c);

		g_autofree char* text = label_text(filter);
		GtkWidget* button = gtk_button_new();
		gtk_box_append(GTK_BOX(hbox), button);
		{
			GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
			gtk_button_set_child(GTK_BUTTON(button), box);
			gtk_box_append(GTK_BOX(box), gtk_image_new_from_icon_name("window-close-symbolic"));
			gtk_box_append(GTK_BOX(box), gtk_label_new(text));
		}
		gtk_widget_set_visible(button, filter->value.c && strlen(filter->value.c));

		g_hash_table_insert(buttons, filter, button);

		void on_filter_button_clicked (GtkButton* button, gpointer _filter)
		{
			observable_string_set((Observable*)_filter, g_strdup(""));
		}

		g_signal_connect(button, "clicked", G_CALLBACK(on_filter_button_clicked), filter);

		void set_label (Observable* filter, GtkWidget* button)
		{
			if (button) {
				char* text = label_text(filter);
				gtk_button_set_label((GtkButton*)button, text);
				g_free(text);
				gtk_widget_set_visible(button, filter->value.c && strlen(filter->value.c));
			}
		}

		void on_filter_changed (Observable* filter, AGlVal value, gpointer user_data)
		{
			dbg(1, "value=%s", value.c);
			set_label(filter, g_hash_table_lookup(buttons, filter));
		}

		agl_observable_subscribe (filter, on_filter_changed, NULL);
	}
	return hbox;
}


