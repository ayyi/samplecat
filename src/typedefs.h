#define OVERVIEW_WIDTH 150
#define OVERVIEW_HEIGHT 20

typedef enum {
	BACKEND_NONE = 0,
	BACKEND_MYSQL,
	BACKEND_SQLITE,
	BACKEND_TRACKER,
} BackendType;

typedef struct _sample sample;
typedef struct _samplecat_result   SamplecatResult;

#ifndef __file_manager_h__
typedef enum {
	SORT_NONE = 0,
	SORT_NAME,
	SORT_TYPE,
	SORT_DATE,
	SORT_SIZE,
	SORT_OWNER,
	SORT_GROUP,
	SORT_NUMBER,
	SORT_PATH
} SortType;

typedef struct _MaskedPixmap       MaskedPixmap;
typedef struct _MIME_type          MIME_type;
typedef struct _GimpActionGroup    GimpActionGroup;
typedef struct _GFSCache           GFSCache;
#endif

#ifndef __gqview2_typedefs_h__
typedef struct _ViewDirTree ViewDirTree;
#endif

#ifndef true
  #define true TRUE
#endif
