valac=valac-0.56

all:
	$(valac) -C -H filemanager.h --pkg gtk+-2.0 --pkg gmodule-2.0 --pkg fm --vapidir . filemanager.vala

	@sed -i -e '/^typedef/d' filemanager.c
	@sed -i -e 's/^struct _AyyiLibfilemanager/struct _XAyyiLibfilemanager/' filemanager.c
	@#add extern directive
	@sed -i -e 's/static gpointer ayyi_libfilemanager_parent_class = NULL;/static gpointer ayyi_libfilemanager_parent_class = NULL; \nextern char theme_name[];/' filemanager.c

EXTRA_DIST = \
	fm.vapi \
	filemanager.vala
