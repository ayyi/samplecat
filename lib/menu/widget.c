#include "widgetprivate.h"

GtkActionMuxer *
_gtk_widget_get_action_muxer (GtkWidget* widget, gboolean create)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_GET_CLASS (widget);
  GtkWidgetPrivate *priv = gtk_widget_get_instance_private (widget);

  if (priv->muxer)
    return priv->muxer;

  if (create || widget_class->priv->actions) {
      priv->muxer = gtk_action_muxer_new (widget);
      _gtk_widget_update_parent_muxer (widget);

      return priv->muxer;
    }
  else
    return gtk_widget_get_parent_muxer (widget, FALSE);
}

GtkActionMuxer*
gtk_widget_get_parent_muxer (GtkWidget *widget, gboolean create)
{
  GtkWidget *parent;

#if 0
  if (GTK_IS_WINDOW (widget))
    return gtk_application_get_parent_muxer_for_window ((GtkWindow *)widget);
#endif

  parent = _gtk_widget_get_parent (widget);

  if (parent)
    return _gtk_widget_get_action_muxer (parent, create);

  return NULL;
}

void
_gtk_widget_update_parent_muxer (GtkWidget *widget)
{
  GtkWidgetPrivate *priv = gtk_widget_get_instance_private (widget);
  GtkWidget *child;

  if (priv->muxer == NULL)
    return;

  gtk_action_muxer_set_parent (priv->muxer,
                               gtk_widget_get_parent_muxer (widget, FALSE));
  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    _gtk_widget_update_parent_muxer (child);
}

