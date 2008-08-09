#include <gtk/gtk.h>
#include "../typedefs.h"
#include "support.h"
#include "gqview.h"
#include "ui_fileops.h"
#include "observer.h"
#include "utilops.h"

typedef struct _FileDataMult FileDataMult;
struct _FileDataMult
{
    gint confirm_all;
    gint confirmed;
    gint skip;
    GList *source_list;
    GList *source_next;
    gchar *dest_base;
    gchar *source;
    gchar *dest;
    gint copy;

    gint rename;
    gint rename_auto;
    gint rename_all;

    GtkWidget *rename_box;
    GtkWidget *rename_entry;
    GtkWidget *rename_auto_box;

    GtkWidget *yes_all_button;
};

typedef struct _FileDataSingle FileDataSingle;
struct _FileDataSingle
{
    gint confirmed;
    gchar *source;
    gchar *dest;
    gint copy;

    gint rename;
    gint rename_auto;

    GtkWidget *rename_box;
    GtkWidget *rename_entry;
    GtkWidget *rename_auto_box;
};

/*
 * Single file move
 */

static FileDataSingle *file_data_single_new(const gchar *source, const gchar *dest, gint copy)
{
    FileDataSingle *fds = g_new0(FileDataSingle, 1);
    fds->confirmed = FALSE;
    fds->source = g_strdup(source);
    fds->dest = g_strdup(dest);
    fds->copy = copy;
    return fds;
}

static void file_data_single_free(FileDataSingle *fds)
{
    g_free(fds->source);
    g_free(fds->dest);
    g_free(fds);
}

/*
 * Multi file move
 */

static FileDataMult *file_data_multiple_new(GList *source_list, const gchar *dest, gint copy)
{
    FileDataMult *fdm = g_new0(FileDataMult, 1);
    fdm->confirm_all = FALSE;
    fdm->confirmed = FALSE;
    fdm->skip = FALSE;
    fdm->source_list = source_list;
    fdm->source_next = fdm->source_list;
    fdm->dest_base = g_strdup(dest);
    fdm->source = NULL;
    fdm->dest = NULL;
    fdm->copy = copy;
    return fdm;
}

static void file_data_multiple_free(FileDataMult *fdm)
{
    path_list_free(fdm->source_list);
    g_free(fdm->dest_base);
    g_free(fdm->dest);
    g_free(fdm);
}

static void file_util_move_multiple(FileDataMult *fdm)
{
	PF;
    while (fdm->dest || fdm->source_next)
        {
        gint success = FALSE;
        gint skip_file = FALSE;

        if (!fdm->dest)
            {
            GList *work = fdm->source_next;
            fdm->source = work->data;
            fdm->dest = concat_dir_and_file(fdm->dest_base, filename_from_path(fdm->source));
            fdm->source_next = work->next;
            fdm->confirmed = FALSE;
            }

        if (fdm->dest && fdm->source && strcmp(fdm->dest, fdm->source) == 0)
            {
            if (!fdm->confirmed)
                {
                //GenericDialog *gd;
                const gchar *title;
                gchar *text;

                if (fdm->copy)
                    {
                    title = "Source to copy matches destination";
                    text = g_strdup_printf("Unable to copy file:\n%s\nto itself.", fdm->dest);
                    }
                else
                    {
                    title = "Source to move matches destination";
                    text = g_strdup_printf("Unable to move file:\n%s\nto itself.", fdm->dest);
                    }

				/*
                gd = file_util_gen_dlg(title, "GQview", "dlg_confirm",
                            NULL, TRUE,
                            file_util_move_multiple_cancel_cb, fdm);
                generic_dialog_add_message(gd, GTK_STOCK_DIALOG_WARNING, title, text);
                g_free(text);
                generic_dialog_add_button(gd, GTK_STOCK_GO_FORWARD, "Co_ntinue",
                             file_util_move_multiple_continue_cb, TRUE);

                gtk_widget_show(gd->dialog);
				*/
				dbg(0, text);
				statusbar_print(1, text);
                return;
                }
            skip_file = TRUE;
            }
        else if (isfile(fdm->dest))
            {
            if (!fdm->confirmed && !fdm->confirm_all)
                {
				dbg(0, "FIXME confirm dialogue");
				/*
                GenericDialog *gd;

                gd = file_util_move_multiple_confirm_dialog(fdm);
                gtk_widget_show(gd->dialog);
				*/
                return;
                }
            if (fdm->skip) skip_file = TRUE;
            }

        if (skip_file)
            {
            success = TRUE;
            if (!fdm->confirm_all) fdm->skip = FALSE;
            }
        else
            {
            gint try = TRUE;

            if (fdm->confirm_all && fdm->rename_all && isfile(fdm->dest))
                {
                gchar *buf;
                buf = unique_filename_simple(fdm->dest);
                if (buf)
                    {
                    g_free(fdm->dest);
                    fdm->dest = buf;
                    }
                else
                    {
                    try = FALSE;
                    }
                }
            if (try)
                {
                if (fdm->copy)
                    {
                    if (copy_file(fdm->source, fdm->dest))
                        {
                        success = TRUE;
                        //file_maint_copied(fdm->source, fdm->dest);
                        }
                    }
                else
                    {
                    if (move_file(fdm->source, fdm->dest))
                        {
                        success = TRUE;
                        //file_maint_moved(fdm->source, fdm->dest, fdm->source_list);
                        }
                    }
                }
            }

        if (!success)
            {
            //GenericDialog *gd;
            const gchar *title;
            gchar *text;

            if (fdm->copy)
                {
                title = "Error copying file";

                text = g_strdup_printf("Unable to copy file:\n%s\nto:\n%s\nduring multiple file copy.", fdm->source, fdm->dest);
                }
            else
                {
                title = "Error moving file";
                text = g_strdup_printf("Unable to move file:"/*\n*/"%s"/*\n*/"to:"/*\n*/"%s"/*\n*/"during multiple file move.", fdm->source, fdm->dest);
                }
			/*
            gd = file_util_gen_dlg(title, "GQview", "dlg_confirm",
                        NULL, TRUE,
                        file_util_move_multiple_cancel_cb, fdm);
            generic_dialog_add_message(gd, GTK_STOCK_DIALOG_WARNING, title, text);
            g_free(text);

            generic_dialog_add_button(gd, GTK_STOCK_GO_FORWARD, "Co_ntinue",
                          file_util_move_multiple_continue_cb, TRUE);
            gtk_widget_show(gd->dialog);
			*/
			dbg(0, text);
			statusbar_print(1, text);
		}

		g_free(fdm->dest);
		fdm->dest = NULL;

		if (!success) return;
	}

	observer__files_moved(fdm->source_list, fdm->dest);

	file_data_multiple_free(fdm);
}

static void file_util_move_single(FileDataSingle *fds)
{
	PF;
    if (fds->dest && fds->source && strcmp(fds->dest, fds->source) == 0)
        {
		dbg(0, "Source matches destination. Source and destination are the same, operation cancelled.");
		statusbar_print(1, "Source and destination are the same, operation cancelled.");
		/*
        file_util_warning_dialog("Source matches destination",
                     "Source and destination are the same, operation cancelled.",
                     GTK_STOCK_DIALOG_INFO, NULL);
		*/
        }
    else if (isfile(fds->dest) && !fds->confirmed)
        {
		dbg(0, "FIXME overwrite file?");
#ifdef LATER
        GenericDialog *gd;
        GtkWidget *hbox;

        gd = file_util_gen_dlg(_("Overwrite file"), "GQview", "dlg_confirm",
                    NULL, TRUE,
                    file_util_move_single_cancel_cb, fds);

        generic_dialog_add_message(gd, GTK_STOCK_DIALOG_QUESTION,
                       _("Overwrite file?"),
                       _("Replace existing file with new file."));
        pref_spacer(gd->vbox, 0);

        generic_dialog_add_button(gd, GTK_STOCK_OK, _("_Overwrite"), file_util_move_single_ok_cb, TRUE);
        generic_dialog_add_image(gd, fds->dest, _("Existing file"), fds->source, _("New file"), TRUE);

        /* rename option */
        fds->rename = FALSE;
        fds->rename_auto = FALSE;

        hbox = pref_box_new(gd->vbox, FALSE, GTK_ORIENTATION_HORIZONTAL, PREF_PAD_GAP);

        fds->rename_auto_box = gtk_check_button_new_with_label(_("Auto rename"));
        g_signal_connect(G_OBJECT(fds->rename_auto_box), "clicked",
                 G_CALLBACK(file_util_move_single_rename_auto_cb), gd);
        gtk_box_pack_start(GTK_BOX(hbox), fds->rename_auto_box, FALSE, FALSE, 0);
        gtk_widget_show(fds->rename_auto_box);

        hbox = pref_box_new(gd->vbox, FALSE, GTK_ORIENTATION_HORIZONTAL, PREF_PAD_GAP);

        fds->rename_box = gtk_check_button_new_with_label(_("Rename"));
        g_signal_connect(G_OBJECT(fds->rename_box), "clicked",
                 G_CALLBACK(file_util_move_single_rename_cb), gd);
        gtk_box_pack_start(GTK_BOX(hbox), fds->rename_box, FALSE, FALSE, 0);
        gtk_widget_show(fds->rename_box);

        fds->rename_entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(fds->rename_entry), filename_from_path(fds->dest));
        gtk_widget_set_sensitive(fds->rename_entry, FALSE);
        gtk_box_pack_start(GTK_BOX(hbox), fds->rename_entry, TRUE, TRUE, 0);
        gtk_widget_show(fds->rename_entry);

        gtk_widget_show(gd->dialog);
#endif
        return;
        }
    else
        {
        gint success = FALSE;
        if (fds->copy)
            {
            if (copy_file(fds->source, fds->dest))
                {
                success = TRUE;
                //file_maint_copied(fds->source, fds->dest); //copy metadata
                }
            }
        else
            {
            if (move_file(fds->source, fds->dest))
                {
                success = TRUE;
                //file_maint_moved(fds->source, fds->dest, NULL); //move metadata
                }
            }
        if (success)
			{
			GList* l = g_list_append(NULL, fds->source);
			observer__files_moved(l, fds->dest);
			g_list_free(l);
			}
		else
            {
            gchar *title;
            gchar *text;
            if (fds->copy)
                {
                title = "Error copying file";
                text = g_strdup_printf("Unable to copy file:\n%s\nto:\n%s", fds->source, fds->dest);
                }
            else
                {
                title = "Error moving file";
                text = g_strdup_printf("Unable to move file: "/*\n*/"%s"/*\n*/" to: "/*\n*/"%s", fds->source, fds->dest);
                }
            //file_util_warning_dialog(title, text, GTK_STOCK_DIALOG_ERROR, NULL);
			dbg(0, text);
			statusbar_print(1, text); //TODO add warning icon to statusbar
            g_free(text);
            }
        }

    file_data_single_free(fds);
}

void file_util_move_simple(GList *list, const gchar *dest_path)
{
    if (!list) return;
    if (!dest_path)
        {
        path_list_free(list);
        return;
        }

    if (!list->next)
        {
        const gchar *source;
        gchar *dest;

        source = list->data;
        dest = concat_dir_and_file(dest_path, filename_from_path(source));

        file_util_move_single(file_data_single_new(source, dest, FALSE));
        g_free(dest);
        path_list_free(list);
        return;
        }

    file_util_move_multiple(file_data_multiple_new(list, dest_path, FALSE));
}
