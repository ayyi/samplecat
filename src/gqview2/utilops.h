/* these avoid the location entry dialog, list must be files only and
 * dest_path must be a valid directory path
*/
void file_util_move_simple(GList *list, const gchar *dest_path);
void file_util_copy_simple(GList *list, const gchar *dest_path);
