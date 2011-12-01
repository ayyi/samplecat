#ifndef __SAMPLECAT_TYPEDEFS_H_
#define __SAMPLECAT_TYPEDEFS_H_

#define OVERVIEW_WIDTH (200)
#define OVERVIEW_HEIGHT (20)

#ifndef __PRI64_PREFIX
#if (defined __X86_64__ || defined __LP64__)
# define __PRI64_PREFIX  "l"
#else
# define __PRI64_PREFIX  "ll"
#endif
#endif

#ifndef PRIu64
# define PRIu64   __PRI64_PREFIX "u"
#endif
#ifndef PRIi64
# define PRIi64   __PRI64_PREFIX "i"
#endif

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

typedef enum {
	BACKEND_NONE = 0,
	BACKEND_MYSQL,
	BACKEND_SQLITE,
	BACKEND_TRACKER,
} BackendType;

typedef struct _menu_def          MenuDef;
typedef struct _sample            Sample;
typedef struct _inspector         Inspector;
typedef struct _playctrl          PlayCtrl;
typedef struct _auditioner        Auditioner;
typedef struct _view_option       ViewOption;

#ifndef __file_manager_h__
typedef struct _GimpActionGroup    GimpActionGroup;
#endif

#ifndef true
  #define true TRUE
#endif

#ifndef false
  #define false FALSE
#endif

#ifndef bool
  #define bool gboolean
#endif

#endif
