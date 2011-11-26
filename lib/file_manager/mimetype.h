/*
 * ROX-Filer, filer for the ROX desktop project
 * By Thomas Leonard, <tal197@users.sourceforge.net>.
 */

#ifndef _TYPE_H
#define _TYPE_H

#include <gtk/gtk.h>
#include "utils/mime_type.h"


extern MIME_type *text_plain;		// often used as a default type
extern MIME_type *application_octet_stream;
extern MIME_type *application_x_shellscript;
extern MIME_type *application_executable;
extern MIME_type *application_x_desktop;
extern MIME_type *inode_directory;
/*
extern MIME_type *inode_mountpoint;
extern MIME_type *inode_pipe;
extern MIME_type *inode_socket;
extern MIME_type *inode_block_dev;
extern MIME_type *inode_char_dev;
extern MIME_type *inode_unknown;
extern MIME_type *inode_door;
*/


struct _Option {
	guchar		*value;
	long		int_value;
	gboolean	has_changed;

	guchar		*backup;	/* Copy of value to Revert to */

	GtkWidget	*widget;		/* NULL => No UI yet */
	//void		(*update_widget)(Option *option);
	//guchar *	(*read_widget)(Option *option);
};
typedef struct _Option Option;




/* Prototypes */
void               type_init(void);
//const char*        basetype_name(DirItem *item);
MIME_type*         type_get_type(const guchar *path);

MIME_type*         type_from_path(const char *path);
gboolean           type_open(const char *path, MIME_type *type);
GdkAtom            type_to_atom(MIME_type *type);
MIME_type*         mime_type_from_base_type(int base_type);
int                mode_to_base_type(int st_mode);
void               type_set_handler_dialog(MIME_type *type);
//gboolean           can_set_run_action(DirItem *item);
gchar*             describe_current_command(MIME_type *type);
//GdkColor*          type_get_colour(DirItem *item, GdkColor *normal);
void               reread_mime_files(void);
extern const char* mime_type_comment(MIME_type *type);
extern MIME_type*  mime_type_lookup(const char *type);
extern GList*      mime_type_name_list(void);

void               _set_icon_theme(void);

#define EXECUTABLE_FILE(item) ((item)->mime_type && (item)->mime_type->executable && \
				((item)->flags & ITEM_FLAG_EXEC_FILE))

#endif /* _TYPE_H */


