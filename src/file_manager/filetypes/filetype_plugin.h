
#define FILETYPE_PLUGIN_API_VERSION 1

typedef struct filetypePlugin {
	guint       api_version;
	gchar*      name;
	guint       type;

	void 		(*plugin_init)		();
	void 		(*plugin_deinit) 	();

	//void*       symbols;       // plugin type specific symbol table

	//void*       client_data;
} *FileTypePluginPtr;

#include <gmodule.h>
#define DECLARE_FILETYPE_PLUGIN(plugin) \
	G_MODULE_EXPORT FileTypePluginPtr plugin_get_info() { \
		return &plugin; \
	}

