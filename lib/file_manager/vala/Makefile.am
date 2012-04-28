
all:
	valac -C -H filemanager.h --pkg gtk+-2.0 --pkg gmodule-2.0 --pkg fm --vapidir . filemanager.vala

	@sed -i -e '/^typedef/d' filemanager.c
	@sed -i -e 's/^struct _AyyiLibfilemanager/struct _XAyyiLibfilemanager/' filemanager.c
	@#add extern directive - dont know how to do this in vala
	@sed -i -e 's/static gpointer ayyi_libfilemanager_parent_class = NULL;/static gpointer ayyi_libfilemanager_parent_class = NULL; \nextern char theme_name[];/' filemanager.c
	@#mv app.c app.h ..

EXTRA_DIST = \
	fm.vapi \
	filemanager.vala
