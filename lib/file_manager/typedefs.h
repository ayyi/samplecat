#ifndef __file_manager_typedefs_h__
#define __file_manager_typedefs_h__

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
} FmSortType;

typedef struct _GimpActionGroup    GimpActionGroup;

#endif //__file_manager_typedefs_h__
