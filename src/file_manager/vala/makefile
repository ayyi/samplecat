tmp="filemanager.tmp"

all:
	valac -C -H filemanager.h --pkg gtk+-2.0 --pkg gmodule-2.0 --pkg rox filemanager.vala

	cat filemanager.c | sed '/^typedef/d' > filemanager.tmp
	mv ${tmp} filemanager.c
	cat filemanager.c | sed 's/^struct _AyyiLibfilemanager/struct _XAyyiLibfilemanager/' > filemanager.tmp
	mv ${tmp} filemanager.c
	#add extern directive - dont know how to do this in vala
	cat filemanager.c | sed 's/static gpointer ayyi_libfilemanager_parent_class = NULL;/static gpointer ayyi_libfilemanager_parent_class = NULL; \nextern char theme_name[];/' > filemanager.tmp
	mv ${tmp} filemanager.c
	#mv app.c app.h ..
