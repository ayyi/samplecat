/* gdl-switcher.c
 *
 * Copyright (C) 2003  Ettore Perazzoli,
 *               2007  Naba Kumar
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Copied and adapted from ESidebar.[ch] from evolution
 *
 * Authors: Ettore Perazzoli <ettore@ximian.com>
 *          Naba Kumar  <naba@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "il8n.h"
#include "utils.h"
#include "debug.h"
#include "switcher.h"
#include "libgdlmarshal.h"
#include "libgdltypebuiltins.h"

/**
 * SECTION:gdl-switcher
 * @title: GdlSwitcher
 * @short_description: An improved notebook widget.
 * @stability: Unstable
 * @see_also: #GdlDockNotebook
 *
 * A #GdlSwitcher widget is an improved version of the #GtkNotebook. The
 * tab labels could have different style and could be layouted on several lines.
 *
 * It is used by the #GdlDockNotebook widget to dock other widgets.
 */


static void gdl_switcher_set_property  (GObject            *object,
                                        guint               prop_id,
                                        const GValue       *value,
                                        GParamSpec         *pspec);
static void gdl_switcher_get_property  (GObject            *object,
                                        guint               prop_id,
                                        GValue             *value,
                                        GParamSpec         *pspec);

static void gdl_switcher_add_button  (GdlSwitcher *switcher,
                                      const gchar *label,
                                      const gchar *tooltips,
                                      const gchar *stock_id,
                                      GdkPixbuf *pixbuf_icon,
                                      gint switcher_id,
                                      GtkWidget *page,
                                      int position);
/* static void gdl_switcher_remove_button    (GdlSwitcher *switcher, gint switcher_id); */
static void gdl_switcher_select_page         (GdlSwitcher *switcher, gint switcher_id);
static void gdl_switcher_select_button       (GdlSwitcher *switcher, gint switcher_id);
static void gdl_switcher_set_show_buttons    (GdlSwitcher *switcher, gboolean show);
static void gdl_switcher_set_style           (GdlSwitcher *switcher, GdlSwitcherStyle switcher_style);
static GdlSwitcherStyle
            gdl_switcher_get_style           (GdlSwitcher *switcher);
static void gdl_switcher_set_tab_pos         (GdlSwitcher *switcher, GtkPositionType pos);
static void gdl_switcher_set_tab_reorderable (GdlSwitcher *switcher, gboolean reorderable);
static void gdl_switcher_update_lone_button_visibility (GdlSwitcher *switcher);

enum {
    PROP_0,
    PROP_SWITCHER_STYLE,
    PROP_TAB_POS,
    PROP_TAB_REORDERABLE
};

typedef struct {
    GtkWidget *button_widget;
    GtkWidget *label;
    GtkWidget *icon;
    GtkWidget *arrow;
    GtkWidget *hbox;
    GtkWidget *page;
    int id;
} Button;

struct _GdlSwitcherPrivate {
    GdlSwitcherStyle switcher_style;
    GdlSwitcherStyle toolbar_style;
    GtkPositionType tab_pos;
    gboolean tab_reorderable;

    gboolean show;
    GSList *buttons;

    guint style_changed_id;
    gint buttons_height_request;
    gboolean in_toggle;
};

struct _GdlSwitcherClassPrivate {
    GtkCssProvider *css;
};

G_DEFINE_TYPE_WITH_CODE (GdlSwitcher, gdl_switcher, GTK_TYPE_WIDGET,
	G_ADD_PRIVATE (GdlSwitcher)
	g_type_add_class_private (g_define_type_id, sizeof (GdlSwitcherClassPrivate))
)

#define INTERNAL_MODE(switcher)  (switcher->priv->switcher_style == \
            GDL_SWITCHER_STYLE_TOOLBAR ? switcher->priv->toolbar_style : \
            switcher->priv->switcher_style)

#define H_PADDING 0
#define V_PADDING 0
#define V_SPACING 2

/* Utility functions.  */

static void
gdl_switcher_long_name_changed (GObject* object, GParamSpec* spec, gpointer user_data)
{
    Button* button = user_data;
    gchar* label;

    g_object_get (object, "long-name", &label, NULL);
    gtk_label_set_text (GTK_LABEL (button->label), label);
    g_free (label);
}

static void
gdl_switcher_stock_id_changed (GObject* object, GParamSpec* spec, gpointer user_data)
{
    Button* button = user_data;
    gchar* id;

    g_object_get (object, "stock-id", &id, NULL);
    gtk_image_set_from_icon_name (GTK_IMAGE(button->icon), id);
    g_free (id);
}

static void
gdl_switcher_visible_changed (GObject* object, GParamSpec* spec, gpointer user_data)
{
    Button* button = user_data;

    if (gtk_widget_get_visible (button->page)) {
        gtk_widget_set_visible (button->button_widget, true);
    } else {
        gtk_widget_set_visible (button->button_widget, false);
    }
    GdlSwitcher* switcher = GDL_SWITCHER (gtk_widget_get_parent (button->button_widget));
    gdl_switcher_update_lone_button_visibility (switcher);
}


static Button *
#ifdef GTK4_TODO
button_new (GtkWidget *button_widget, GtkWidget *label, GtkWidget *icon, GtkWidget *arrow, GtkWidget *hbox, int id, GtkWidget *page)
#else
button_new (GtkWidget *button_widget, GtkWidget *label, GtkWidget *icon, GtkWidget *hbox, int id, GtkWidget *page)
#endif
{
    Button *button = g_new (Button, 1);

    button->button_widget = button_widget;
    button->label = label;
    button->icon = icon;
#ifdef GTK4_TODO
    button->arrow = arrow;
#endif
    button->hbox = hbox;
    button->id = id;
    button->page = page;

    g_signal_connect (page, "notify::long-name", G_CALLBACK (gdl_switcher_long_name_changed), button);
    g_signal_connect (page, "notify::stock-id", G_CALLBACK (gdl_switcher_stock_id_changed), button);
    g_signal_connect (page, "notify::visible", G_CALLBACK (gdl_switcher_visible_changed), button);

    g_object_ref (button_widget);
    g_object_ref (label);
    g_object_ref (icon);
#ifdef GTK4_TODO
    g_object_ref (arrow);
#endif
    g_object_ref (hbox);

    return button;
}

static void
button_free (Button *button)
{
    g_signal_handlers_disconnect_by_func (button->page, gdl_switcher_long_name_changed, button);
    g_signal_handlers_disconnect_by_func (button->page, gdl_switcher_stock_id_changed, button);
    g_signal_handlers_disconnect_by_func (button->page, gdl_switcher_visible_changed, button);

    g_object_unref (button->button_widget);
    g_object_unref (button->label);
    g_object_unref (button->icon);
    g_object_unref (button->hbox);
    g_free (button);
}

static gint
gdl_switcher_get_page_id (GtkWidget *widget)
{
    static gint switcher_id_count = 0;

    gint switcher_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "__switcher_id"));
    if (switcher_id <= 0) {
        switcher_id = ++switcher_id_count;
        g_object_set_data (G_OBJECT (widget), "__switcher_id", GINT_TO_POINTER (switcher_id));
    }

    return switcher_id;
}

/* Hide switcher button if they are not needed (only one visible page)
 * or show switcher button if there are two visible pages */
static void
gdl_switcher_update_lone_button_visibility (GdlSwitcher *switcher)
{
    GtkWidget *alone = NULL;

    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *button = p->data;

        if (gtk_widget_get_visible (button->page)) {
            if (alone == NULL) {
                alone = button->button_widget;
            } else {
                gtk_widget_set_visible (alone, true);
                gtk_widget_set_visible (button->button_widget, true);
                alone = NULL;
                break;
            }
        }
    }

    if (alone) gtk_widget_set_visible (alone, false);
}

static void
update_buttons (GdlSwitcher *switcher, int new_selected_id)
{
    switcher->priv->in_toggle = TRUE;

    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *button = p->data;

        if (button->id == new_selected_id) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->button_widget), TRUE);
#ifdef GTK4_TODO
            gtk_widget_set_sensitive (button->arrow, TRUE);
#endif
        } else {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->button_widget), FALSE);
#ifdef GTK4_TODO
            gtk_widget_set_sensitive (button->arrow, FALSE);
#endif
        }
    }

    switcher->priv->in_toggle = FALSE;
}

/* Callbacks.  */

static void
button_toggled_callback (GtkToggleButton *toggle_button, GdlSwitcher *switcher)
{
    int id = 0;

    if (switcher->priv->in_toggle)
        return;

    switcher->priv->in_toggle = TRUE;

    gboolean is_active = gtk_toggle_button_get_active (toggle_button);

    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *button = p->data;

        if (button->button_widget != GTK_WIDGET (toggle_button)) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->button_widget), FALSE);
#ifdef GTK4_TODO
            gtk_widget_set_sensitive (button->arrow, FALSE);
#endif
        } else {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->button_widget), TRUE);
#ifdef GTK4_TODO
            gtk_widget_set_sensitive (button->arrow, TRUE);
#endif
            id = button->id;
        }
    }

    switcher->priv->in_toggle = FALSE;

    if (is_active) {
        gdl_switcher_select_page (switcher, id);
    }
}

static gboolean
delayed_resize_switcher (gpointer data)
{
    gtk_widget_queue_resize (GTK_WIDGET (data));
    return FALSE;
}

/* Returns -1 if layout didn't happen because a resize request was queued */
static int
layout_buttons (GdlSwitcher *switcher, GtkAllocation* allocation)
{
    gint min_height, nat_height;
    int row_number;
    int max_btn_width = 0, max_btn_height = 0;
    int optimal_layout_width = 0;
    int row_last;
    int i;
    int rows_count;
    int last_buttons_height;

    last_buttons_height = switcher->priv->buttons_height_request;

    GTK_WIDGET_CLASS (gdl_switcher_parent_class)->measure (GTK_WIDGET (switcher), GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);

    int y = allocation->y + allocation->height - V_PADDING - 1;

    /* Return bottom of the area if there isn't any visible button */
	GSList* p;
    for (p = switcher->priv->buttons; p != NULL; p = p->next)
		if (gtk_widget_get_visible (((Button *)p->data)->button_widget))
			break;
    if (p == NULL)
        return y;

    GdlSwitcherStyle switcher_style = INTERNAL_MODE (switcher);
    gboolean icons_only = (switcher_style == GDL_SWITCHER_STYLE_ICON);

    /* Figure out the max width and height taking into account visible buttons */
    optimal_layout_width = H_PADDING;
    int num_btns = 0;
    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        GtkRequisition requisition;

        Button* button = p->data;
        if (gtk_widget_get_visible (button->button_widget)) {
            gtk_widget_get_preferred_size (GTK_WIDGET (button->button_widget), &requisition, NULL);
            optimal_layout_width += requisition.width + H_PADDING;
            max_btn_height = MAX (max_btn_height, requisition.height);
            max_btn_width = MAX (max_btn_width, requisition.width);
            num_btns++;
        }
    }

    /* Figure out how many rows and columns we'll use. */
    int btns_per_row = allocation->width / (max_btn_width + H_PADDING);
    /* Use at least one column */
    if (btns_per_row == 0) btns_per_row = 1;

    /* If all the buttons could fit in the single row, have it so */
    if (allocation->width >= optimal_layout_width) {
        btns_per_row = num_btns;
    }
    if (!icons_only) {
        /* If using text buttons, we want to try to have a
         * completely filled-in grid, but if we can't, we want
         * the odd row to have just a single button.
         */
        while (num_btns % btns_per_row > 1)
            btns_per_row--;
    }

    rows_count = num_btns / btns_per_row;
    if (num_btns % btns_per_row != 0)
		rows_count++;

    /* Assign visible buttons to rows */
    GSList **rows = g_new0 (GSList *, rows_count);

    if (!icons_only && num_btns % btns_per_row != 0) {
        p = switcher->priv->buttons;
        for (; p != NULL; p = p->next) if (gtk_widget_get_visible (((Button *)p->data)->button_widget)) break;
        Button* button = p->data;
        rows [0] = g_slist_append (rows [0], button->button_widget);

        p = p->next;
        row_number = p ? 1 : 0;
    } else {
        p = switcher->priv->buttons;
        row_number = 0;
    }

    for (; p != NULL; p = p->next) {
        Button* button = p->data;

        if (gtk_widget_get_visible (button->button_widget)) {
            if (g_slist_length (rows [row_number]) == btns_per_row)
                row_number ++;

            rows [row_number] = g_slist_append (rows [row_number], button->button_widget);
        }
    }

    row_last = row_number;

    /* If there are more than 1 row of buttons, save the current height
     * requirement for subsequent size requests.
     */
    if (row_last > 0) {
        switcher->priv->buttons_height_request = (row_last + 1) * (max_btn_height + V_PADDING) + 1;
    } else { /* Otherwize clear it */
        if (last_buttons_height >= 0) {

            switcher->priv->buttons_height_request = -1;
        }
    }

    /* If it turns out that we now require smaller height for the buttons
     * than it was last time, make a resize request to ensure our
     * size requisition is properly communicated to the parent (otherwise
     * parent tend to keep assuming the older size).
     */
    if (last_buttons_height > switcher->priv->buttons_height_request) {
        g_idle_add (delayed_resize_switcher, switcher);
        return -1;
    }

    /* Layout the buttons. */
    for (i = row_last; i >= 0; i --) {
        int extra_width;

        y -= max_btn_height;

        /* Check for possible size over flow (taking into account client
         * requisition
         */
        if (y < (allocation->y + min_height)) {
            /* We have an overflow: Insufficient allocation */
            if (last_buttons_height < switcher->priv->buttons_height_request) {
                /* Request for a new resize */
                g_idle_add (delayed_resize_switcher, switcher);

                return -1;
            }
        }
        int x = H_PADDING + allocation->x;
        int len = g_slist_length (rows[i]);
        if (switcher_style == GDL_SWITCHER_STYLE_TEXT ||
            switcher_style == GDL_SWITCHER_STYLE_BOTH)
            extra_width = (allocation->width - (len * max_btn_width )
                           - (len * H_PADDING)) / len;
        else
            extra_width = 0;
        for (p = rows [i]; p != NULL; p = p->next) {
            GtkAllocation child_allocation = {
            	.x = x,
            	.y = y,
			};
            if (rows_count == 1 && row_number == 0) {
                GtkRequisition child_requisition;
                gtk_widget_get_preferred_size (GTK_WIDGET (p->data), &child_requisition, NULL);
                child_allocation.width = child_requisition.width;
            } else {
                child_allocation.width = max_btn_width + extra_width;
            }
            child_allocation.height = max_btn_height;

#ifdef GTK4_TODO
            GtkStyleContext* style_context = gtk_widget_get_style_context (GTK_WIDGET (p->data));

            GtkJunctionSides junction = 0;
            if (row_last) {
                if (i) {
                    junction |= GTK_JUNCTION_TOP;
                }
                if (i != row_last) {
                    junction |= GTK_JUNCTION_BOTTOM;
                }
            }

            if (p->next) {
                junction |= GTK_JUNCTION_RIGHT;
            }

            if (p != rows [i]) {
                junction |= GTK_JUNCTION_LEFT;
            }

            gtk_style_context_set_junction_sides (style_context, junction);
#endif

            gtk_widget_size_allocate (GTK_WIDGET (p->data), &child_allocation, -1);

            x += child_allocation.width + H_PADDING;
        }
    }

    y -= V_SPACING;

    for (i = 0; i <= row_last; i ++)
        g_slist_free (rows [i]);
    g_free (rows);

    return y;
}

static void
do_layout (GdlSwitcher *switcher)
{
	GtkAllocation allocation;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	gtk_widget_get_allocation (GTK_WIDGET(switcher), &allocation);
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

   	for (GSList* p = switcher->priv->buttons; p; p = p->next) {
        Button *button = p->data;
		gtk_widget_set_visible(button->button_widget, allocation.height >= 40);
	}

    int y = 0;
	if (allocation.height >= 40) {
		if (switcher->priv->show) {
			y = layout_buttons (switcher, &allocation);
			if (y < 0) /* Layout did not happen and a resize was requested */
				return;
		}
		else
			y = allocation.y + allocation.height;
	}
	

    /* Place the parent widget.  */
    GtkAllocation child_allocation = allocation;
    child_allocation.height = y - allocation.y;

	gtk_widget_size_allocate(GTK_WIDGET(switcher->notebook), &(GtkAllocation){allocation.x, allocation.y, allocation.width, MAX(0, allocation.height - 40)}, -1);

    GTK_WIDGET_CLASS (gdl_switcher_parent_class)->size_allocate (GTK_WIDGET (switcher), child_allocation.width, child_allocation.height, -1);
}

/* GtkContainer methods.  */

void
gdl_switcher_remove (GdlSwitcher *switcher, GtkWidget *widget)
{
    gint switcher_id = gdl_switcher_get_page_id (widget);
    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *b = (Button *) p->data;

        if (b->id == switcher_id) {
            gtk_widget_unparent (b->button_widget);
            switcher->priv->buttons = g_slist_remove_link (switcher->priv->buttons, p);
            button_free (b);
            gtk_widget_queue_resize (GTK_WIDGET (switcher));
            break;
        }
    }
    gdl_switcher_update_lone_button_visibility (switcher);
}

/* GtkWidget methods.  */

static void
gdl_switcher_get_preferred_width (GtkWidget *widget, int for_size, gint *minimum, gint *natural, int *min_baseline, int *natural_baseline)
{
    GdlSwitcher *switcher = GDL_SWITCHER (widget);
    GSList *p;

    GTK_WIDGET_CLASS (gdl_switcher_parent_class)->measure (GTK_WIDGET (switcher), GTK_ORIENTATION_HORIZONTAL, for_size, minimum, natural, min_baseline, natural_baseline);

    if (!switcher->priv->show)
        return;

    for (p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *button = p->data;

        if (gtk_widget_get_visible (button->button_widget)) {
            gint min, nat;

            gtk_widget_measure (button->button_widget, GTK_ORIENTATION_HORIZONTAL, -1, &min, &nat, NULL, NULL);
            *minimum = MAX (*minimum, min + 2 * H_PADDING);
            *natural = MAX (*natural, nat + 2 * H_PADDING);
		}
    }
}

static void
gdl_switcher_get_preferred_height (GtkWidget *widget, int for_size, gint *minimum, gint *natural, int *min_baseline, int *natural_baseline)
{
    GdlSwitcher *switcher = GDL_SWITCHER (widget);
    gint button_min = 0;
    gint button_nat = 0;

    GTK_WIDGET_CLASS (gdl_switcher_parent_class)->measure (GTK_WIDGET (switcher), GTK_ORIENTATION_VERTICAL, -1, minimum, natural, min_baseline, natural_baseline);

    if (!switcher->priv->show)
        return;

    for (GSList* p = switcher->priv->buttons; p; p = p->next) {
        Button *button = p->data;

        if (gtk_widget_get_visible (button->button_widget)) {
            gint min, nat;

            gtk_widget_measure (button->button_widget, GTK_ORIENTATION_VERTICAL, -1, &min, &nat, NULL, NULL);
            button_min = MAX (button_min, min + 2 * V_PADDING);
            button_nat = MAX (button_nat, nat + 2 * V_PADDING);
        }
    }

    if (switcher->priv->buttons_height_request > 0) {
        *minimum += switcher->priv->buttons_height_request;
        *natural += switcher->priv->buttons_height_request;
    } else {
        *minimum += button_min + V_PADDING;
        *natural += button_nat + V_PADDING;
    }
}

static void
gdl_switcher_measure (GtkWidget *widget, GtkOrientation orientation, int for_size, int *minimum, int *natural, int *min_baseline, int *natural_baseline)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		gdl_switcher_get_preferred_width (widget, for_size, minimum, natural, min_baseline, natural_baseline);
	} else {
		gdl_switcher_get_preferred_height (widget, for_size, minimum, natural, min_baseline, natural_baseline);
	}
}

static void
gdl_switcher_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
    do_layout (GDL_SWITCHER (widget));
}

static void
gdl_switcher_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
#ifdef GTK4_TODO
    GdlSwitcher *switcher = GDL_SWITCHER (widget);

    if (switcher->priv->show) {
        for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
            GtkWidget *button = ((Button *) p->data)->button_widget;
            gtk_container_propagate_draw (GTK_CONTAINER (widget), button, cr);
        }
    }
#endif

    GTK_WIDGET_CLASS (gdl_switcher_parent_class)->snapshot (widget, snapshot);
}

static void
gdl_switcher_map (GtkWidget *widget)
{
    GdlSwitcher *switcher = GDL_SWITCHER (widget);

    if (switcher->priv->show) {
        for (GSList* p = switcher->priv->buttons; p; p = p->next) {
            GtkWidget *button = ((Button *) p->data)->button_widget;
            if (gtk_widget_get_visible (button) && !gtk_widget_get_mapped (button))
                gtk_widget_map (button);
        }
    }
    GTK_WIDGET_CLASS (gdl_switcher_parent_class)->map (widget);
}

/* GObject methods.  */

static void
gdl_switcher_set_property  (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GdlSwitcher *switcher = GDL_SWITCHER (object);

    switch (prop_id) {
        case PROP_SWITCHER_STYLE:
            gdl_switcher_set_style (switcher, g_value_get_enum (value));
            break;
        case PROP_TAB_POS:
            gdl_switcher_set_tab_pos (switcher, g_value_get_enum (value));
            break;
        case PROP_TAB_REORDERABLE:
            gdl_switcher_set_tab_reorderable (switcher, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_switcher_get_property  (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GdlSwitcher *switcher = GDL_SWITCHER (object);

    switch (prop_id) {
        case PROP_SWITCHER_STYLE:
            g_value_set_enum (value, gdl_switcher_get_style (switcher));
            break;
        case PROP_TAB_POS:
            g_value_set_enum (value, switcher->priv->tab_pos);
            break;
        case PROP_TAB_REORDERABLE:
            g_value_set_enum (value, switcher->priv->tab_reorderable);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gdl_switcher_dispose (GObject *object)
{
    GdlSwitcherPrivate *priv = GDL_SWITCHER (object)->priv;

#if HAVE_GNOME
    GConfClient *gconf_client = gconf_client_get_default ();

    if (priv->style_changed_id) {
        gconf_client_notify_remove (gconf_client, priv->style_changed_id);
        priv->style_changed_id = 0;
    }
    g_object_unref (gconf_client);
#endif

    g_slist_free_full (priv->buttons, (GDestroyNotify)button_free);
    priv->buttons = NULL;

    G_OBJECT_CLASS (gdl_switcher_parent_class)->dispose (object);
}

static void
gdl_switcher_finalize (GObject *object)
{
    G_OBJECT_CLASS (gdl_switcher_parent_class)->finalize (object);
}

/* Signal handlers */

static void
gdl_switcher_notify_cb (GObject *g_object, GParamSpec *pspec, GdlSwitcher *switcher)
{
}

static void
gdl_switcher_switch_page_cb (GtkNotebook *nb, GtkWidget *page_widget, gint page_num, GdlSwitcher *switcher)
{
#ifndef GTK4_TODO // why is signal blocker not working?
	static int inside;
	if (inside) return;
	inside = true;
#endif

    /* Change switcher button */
    gint switcher_id = gdl_switcher_get_page_id (page_widget);
    gdl_switcher_select_button (GDL_SWITCHER (switcher), switcher_id);

#ifndef GTK4_TODO
	inside = false;
#endif
}

static void
gdl_switcher_page_added_cb (GtkNotebook *nb, GtkWidget *page, gint page_num, GdlSwitcher *switcher)
{
    gint switcher_id = gdl_switcher_get_page_id (page);

    gdl_switcher_add_button (GDL_SWITCHER (switcher), NULL, NULL, NULL, NULL, switcher_id, page, page_num);
    gdl_switcher_select_button (GDL_SWITCHER (switcher), switcher_id);
}

static void
gdl_switcher_select_page (GdlSwitcher *switcher, gint id)
{
	GtkWidget* first = gtk_widget_get_first_child(GTK_WIDGET (switcher->notebook));
	if (GTK_IS_STACK(first)) {
		for (GtkWidget* c = gtk_widget_get_first_child(first); c; c = gtk_widget_get_next_sibling (c)) {
			if (gdl_switcher_get_page_id (c) == id) {
				int page_num = gtk_notebook_page_num (switcher->notebook, c);
				g_signal_handlers_block_by_func (switcher, gdl_switcher_switch_page_cb, switcher);
				gtk_notebook_set_current_page (switcher->notebook, page_num);
				g_signal_handlers_unblock_by_func (switcher, gdl_switcher_switch_page_cb, switcher);
				break;
			}
		}
	}
}

/* Initialization.  */

static void
gdl_switcher_class_init (GdlSwitcherClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

	widget_class->measure = gdl_switcher_measure;
    widget_class->size_allocate = gdl_switcher_size_allocate;
    widget_class->snapshot = gdl_switcher_snapshot;
    widget_class->map = gdl_switcher_map;

    object_class->dispose = gdl_switcher_dispose;
    object_class->finalize = gdl_switcher_finalize;
    object_class->set_property = gdl_switcher_set_property;
    object_class->get_property = gdl_switcher_get_property;

    g_object_class_install_property (
        object_class, PROP_SWITCHER_STYLE,
        g_param_spec_enum ("switcher-style", _("Switcher Style"),
                           _("Switcher buttons style"),
                           GDL_TYPE_SWITCHER_STYLE,
                           GDL_SWITCHER_STYLE_BOTH,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_TAB_POS,
        g_param_spec_enum ("tab-pos", _("Tab Position"),
                           _("Which side of the notebook holds the tabs"),
                           GTK_TYPE_POSITION_TYPE,
                           GTK_POS_BOTTOM,
                           G_PARAM_READWRITE));

    g_object_class_install_property (
        object_class, PROP_TAB_REORDERABLE,
        g_param_spec_boolean ("tab-reorderable", _("Tab reorderable"),
                              _("Whether the tab is reorderable by user action"),
                              FALSE,
                              G_PARAM_READWRITE));

    klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GDL_TYPE_SWITCHER, GdlSwitcherClassPrivate);

    static const gchar button_style[] =
       "* {\n"
           "outline-width : 1px;\n"
           "padding: 0;\n"
       "}";

    klass->priv->css = gtk_css_provider_new ();
	gtk_css_provider_load_from_string(klass->priv->css, button_style);

	gtk_widget_class_set_css_name (widget_class, "switcher");
}

static void
gdl_switcher_init (GdlSwitcher *switcher)
{
	switcher->priv = gdl_switcher_get_instance_private (switcher);
    GdlSwitcherPrivate* priv = switcher->priv;

	priv->buttons = NULL;
    priv->show = TRUE;
    priv->buttons_height_request = -1;
    priv->tab_pos = GTK_POS_BOTTOM;
    priv->tab_reorderable = FALSE;

	switcher->notebook = GTK_NOTEBOOK (gtk_notebook_new());
	gtk_widget_set_parent(GTK_WIDGET (switcher->notebook), GTK_WIDGET (switcher));

    gtk_notebook_set_tab_pos (switcher->notebook, GTK_POS_BOTTOM);
    gtk_notebook_set_show_tabs (switcher->notebook, FALSE);
    gtk_notebook_set_show_border (switcher->notebook, FALSE);
    gdl_switcher_set_style (switcher, GDL_SWITCHER_STYLE_BOTH);

    /* notebook signals */
    g_signal_connect (switcher->notebook, "switch-page", G_CALLBACK (gdl_switcher_switch_page_cb), switcher);
    g_signal_connect (switcher->notebook, "page-added", G_CALLBACK (gdl_switcher_page_added_cb), switcher);
    g_signal_connect (switcher, "notify::show-tabs", G_CALLBACK (gdl_switcher_notify_cb), switcher);
}

/**
 * gdl_switcher_new:
 *
 * Creates a new notebook widget with no pages.
 *
 * Returns: The newly created #GdlSwitcher
 */
GtkWidget *
gdl_switcher_new (void)
{
    return g_object_new (gdl_switcher_get_type (), NULL);
}

/**
 * gdl_switcher_add_button:
 * @switcher: The #GdlSwitcher to which a button will be added
 * @label: The label for the button
 * @tooltips: The tooltip for the button
 * @stock_id: The stock ID for the button
 * @pixbuf_icon: The pixbuf to use for the button icon
 * @switcher_id: The ID of the switcher
 * @page: The notebook page
 *
 * Adds a button to a #GdlSwitcher. The button icon is taken preferentially
 * from the @stock_id parameter. If this is %NULL, then the @pixbuf_icon
 * parameter is used. Failing that, the %GTK_STOCK_NEW stock icon is used.
 * The text label for the button is specified using the @label parameter. If
 * it is %NULL then a default incrementally numbered label is used instead.
 */
static void
gdl_switcher_add_button (GdlSwitcher *switcher, const gchar *label, const gchar *tooltips, const gchar *stock_id, GdkPixbuf *pixbuf_icon, gint switcher_id, GtkWidget* page, int position)
{
	ENTER;

	GtkWidget* button_widget = gtk_toggle_button_new ();
	if (switcher->priv->show && gtk_widget_get_visible (page))
		gtk_widget_set_visible (button_widget, true);
	g_signal_connect (button_widget, "toggled", G_CALLBACK (button_toggled_callback), switcher);
	GtkWidget* hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_button_set_child (GTK_BUTTON (button_widget), hbox);

	GtkWidget *icon_widget;
	if (stock_id) {
		icon_widget = gtk_image_new_from_icon_name (stock_id);
	} else if (pixbuf_icon) {
		icon_widget = gtk_image_new_from_pixbuf (pixbuf_icon);
	} else {
		icon_widget = gtk_image_new_from_icon_name ("go-next-symbolic");
	}

	GtkWidget *label_widget;
	if (!label) {
		gchar *text = g_strdup_printf ("Item %d", switcher_id);
		label_widget = gtk_label_new (text);
		g_free (text);
	} else {
		label_widget = gtk_label_new (label);
	}
#ifdef GTK4_TODO
	gtk_misc_set_alignment (GTK_MISC (label_widget), 0.0, 0.5);
#endif

	gtk_widget_set_tooltip_text (button_widget, tooltips);

	switch (INTERNAL_MODE (switcher)) {
		case GDL_SWITCHER_STYLE_TEXT:
			gtk_box_append (GTK_BOX (hbox), label_widget);
			break;
		case GDL_SWITCHER_STYLE_ICON:
			gtk_box_append (GTK_BOX (hbox), icon_widget);
			break;
		case GDL_SWITCHER_STYLE_BOTH:
		default:
			gtk_box_append (GTK_BOX (hbox), icon_widget);
			gtk_box_append (GTK_BOX (hbox), label_widget);
			break;
	}
#ifdef GTK4_TODO
	GtkWidget* arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
	gtk_box_append (GTK_BOX (hbox), arrow);
#endif

#ifdef GTK4_TODO
	switcher->priv->buttons = g_slist_append (switcher->priv->buttons, button_new (button_widget, label_widget, icon_widget, arrow, hbox, switcher_id, page));
#else
	if (position == 0) {
		switcher->priv->buttons = g_slist_insert (switcher->priv->buttons, button_new (button_widget, label_widget, icon_widget, hbox, switcher_id, page), 0);
	} else {
		switcher->priv->buttons = g_slist_append (switcher->priv->buttons, button_new (button_widget, label_widget, icon_widget, hbox, switcher_id, page));
	}
#endif

	gtk_widget_set_parent (button_widget, GTK_WIDGET (switcher));
	gdl_switcher_update_lone_button_visibility (switcher);
	gtk_widget_queue_resize (GTK_WIDGET (switcher));
}

#if 0
static void
gdl_switcher_remove_button (GdlSwitcher *switcher, gint switcher_id)
{
    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *button = p->data;

        if (button->id == switcher_id) {
            gtk_container_remove (GTK_CONTAINER (switcher), button->button_widget);
            break;
        }
    }
    gtk_widget_queue_resize (GTK_WIDGET (switcher));
}
#endif

static void
gdl_switcher_select_button (GdlSwitcher *switcher, gint switcher_id)
{
    update_buttons (switcher, switcher_id);

    /* Select the notebook page associated with this button */
    gdl_switcher_select_page (switcher, switcher_id);
}


/**
 * gdl_switcher_insert_page:
 * @switcher: The switcher to which a page will be added
 * @page: The page to add to the switcher
 * @tab_widget: The  to add to the switcher
 * @label: The label text for the button
 * @tooltips: The tooltip for the button
 * @stock_id: The stock ID for the button icon
 * @pixbuf_icon: The pixbuf to use for the button icon
 * @position: The position at which to create the page
 *
 * Adds a page to a #GdlSwitcher.  A button is created in the switcher, with its
 * icon taken preferentially from the @stock_id parameter.  If this parameter is
 * %NULL, then the @pixbuf_icon parameter is used.  Failing that, the
 * %GTK_STOCK_NEW stock icon is used.  The text label for the button is specified
 * using the @label parameter.  If it is %NULL then a default incrementally
 * numbered label is used instead.
 *
 * Returns: The index (starting from 0) of the appended page in the notebook, or -1 if function fails
 */
gint
gdl_switcher_insert_page (GdlSwitcher *switcher, GtkWidget *page, GtkWidget *tab_widget, const gchar *label, const gchar *tooltips, const gchar *stock_id, GdkPixbuf *pixbuf_icon, gint position)
{
	ENTER;

    GtkNotebook *notebook = switcher->notebook;

    g_signal_handlers_block_by_func (notebook, gdl_switcher_page_added_cb, switcher);

    if (!tab_widget) {
        tab_widget = gtk_label_new (label);
        if (gtk_widget_get_visible (page)) gtk_widget_set_visible (tab_widget, true);
    }
    gint switcher_id = gdl_switcher_get_page_id (page);
    gdl_switcher_add_button (switcher, label, tooltips, stock_id, pixbuf_icon, switcher_id, page, position);

    gint ret_position = gtk_notebook_insert_page (notebook, page, tab_widget, position);
    gtk_notebook_set_tab_reorderable (notebook, page, switcher->priv->tab_reorderable);
    g_signal_handlers_unblock_by_func (notebook, gdl_switcher_page_added_cb, switcher);

    return ret_position;
}

static void
set_switcher_style_toolbar (GdlSwitcher *switcher, GdlSwitcherStyle switcher_style)
{
	if (switcher_style == GDL_SWITCHER_STYLE_NONE
		|| switcher_style == GDL_SWITCHER_STYLE_TABS)
		return;

	if (switcher_style == GDL_SWITCHER_STYLE_TOOLBAR)
		switcher_style = GDL_SWITCHER_STYLE_BOTH;

	if (switcher_style == INTERNAL_MODE (switcher))
		return;

	gtk_notebook_set_show_tabs (switcher->notebook, FALSE);

	for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
		Button *button = p->data;

#ifdef GTK4_TODO
		gtk_box_remove (GTK_BOX (button->hbox), button->arrow);
#endif

		if (gtk_widget_get_parent (button->icon))
			gtk_box_remove (GTK_BOX (button->hbox), button->icon);
		if (gtk_widget_get_parent (button->label))
			gtk_box_remove (GTK_BOX (button->hbox), button->label);

		switch (switcher_style) {
			case GDL_SWITCHER_STYLE_TEXT:
				gtk_box_append (GTK_BOX (button->hbox), button->label);
				gtk_widget_set_visible (button->label, true);
				break;

			case GDL_SWITCHER_STYLE_ICON:
				gtk_box_append (GTK_BOX (button->hbox), button->icon);
				gtk_widget_set_visible (button->icon, true);
				break;

			case GDL_SWITCHER_STYLE_BOTH:
				gtk_box_append (GTK_BOX (button->hbox), button->icon);
				gtk_box_append (GTK_BOX (button->hbox), button->label);
				gtk_widget_set_visible (button->icon, true);
				gtk_widget_set_visible (button->label, true);
				break;

			default:
				break;
		}

#ifdef GTK4_TODO
		gtk_box_append (GTK_BOX (button->hbox), button->arrow);
#endif
	}

	gdl_switcher_set_show_buttons (switcher, TRUE);
}

static void
gdl_switcher_set_style (GdlSwitcher *switcher, GdlSwitcherStyle switcher_style)
{
    if (switcher->priv->switcher_style == switcher_style)
        return;

    if (switcher_style == GDL_SWITCHER_STYLE_NONE) {
        gdl_switcher_set_show_buttons (switcher, FALSE);
        gtk_notebook_set_show_tabs (switcher->notebook, FALSE);
    }
    else if (switcher_style == GDL_SWITCHER_STYLE_TABS) {
        gdl_switcher_set_show_buttons (switcher, FALSE);
        gtk_notebook_set_show_tabs (switcher->notebook, TRUE);
    }
    else
        set_switcher_style_toolbar (switcher, switcher_style);

#ifdef GTK4_TODO
    gtk_widget_queue_resize (GTK_WIDGET (switcher));
#endif
    switcher->priv->switcher_style = switcher_style;
}

static void
gdl_switcher_set_show_buttons (GdlSwitcher *switcher, gboolean show)
{
    if (switcher->priv->show == show)
        return;

    for (GSList* p = switcher->priv->buttons; p != NULL; p = p->next) {
        Button *button = p->data;

        if (show && gtk_widget_get_visible (button->page))
            gtk_widget_set_visible (button->button_widget, true);
        else
            gtk_widget_set_visible (button->button_widget, false);
    }
    gdl_switcher_update_lone_button_visibility (switcher);
    switcher->priv->show = show;

    gtk_widget_queue_resize (GTK_WIDGET (switcher));
}

static GdlSwitcherStyle
gdl_switcher_get_style (GdlSwitcher *switcher)
{
    if (!switcher->priv->show)
        return GDL_SWITCHER_STYLE_TABS;
    return switcher->priv->switcher_style;
}

static void
gdl_switcher_set_tab_pos (GdlSwitcher *switcher, GtkPositionType pos)
{
    if (switcher->priv->tab_pos == pos)
        return;

    gtk_notebook_set_tab_pos (switcher->notebook, pos);

    switcher->priv->tab_pos = pos;
}

static void
gdl_switcher_set_tab_reorderable (GdlSwitcher *switcher, gboolean reorderable)
{
	if (switcher->priv->tab_reorderable == reorderable)
		return;

	int n_pages = gtk_notebook_get_n_pages(switcher->notebook);
	for (int i = 0; i < n_pages; i++) {
		GtkWidget* page = gtk_notebook_get_nth_page (switcher->notebook, i);
		gtk_notebook_set_tab_reorderable (switcher->notebook, page, reorderable);
	}

	switcher->priv->tab_reorderable = reorderable;
}
