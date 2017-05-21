#if 0
#ifndef __mimetype_h__
#define __mimetype_h__

#include <gtk/gtk.h>
#include "utils/pixmaps.h"

struct _MIME_type
{
	char			*media_type;
	char			*subtype;
	MaskedPixmap 	*image;		/* NULL => not loaded yet */
	time_t			image_time;	/* When we loaded the image */

	/* Private: use mime_type_comment() instead */
	char			*comment;	/* Name in local language */
	gboolean		executable;	/* Subclass of application/x-executable */
};
#ifndef MIME_type
typedef struct _MIME_type MIME_type; 
#endif

extern MIME_type *text_plain;		// often used as a default type
extern MIME_type *application_octet_stream;
extern MIME_type *application_x_shellscript;
extern MIME_type *application_executable;
extern MIME_type *application_x_desktop;
extern MIME_type *inode_directory;


GdkPixbuf*         mime_type_get_pixbuf (MIME_type*);
MaskedPixmap*      type_to_icon         (MIME_type*);

#endif //__mime_type_h__
#endif
