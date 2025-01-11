#include "debug/debug.h"
#include "gdl-dock-master.h"

extern int indent;
extern int gdl_debug;

#ifdef DEBUG
#define ENTER \
	g_auto(Enter) enter; \
	{ indent++; }
#define cdbg(A, B, ...) \
	{ \
		{ \
			int i; \
			if(A <= gdl_debug) for(i=0;i<indent;i++) printf("  "); \
			fflush(stdout); \
		} \
		gdl_debug_printf(__func__, A, B, ##__VA_ARGS__); \
	}

typedef int Enter;
void leave(int*);
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(Enter, leave)

#else
# define ENTER
# define cdbg(A, B, ...)
#endif

#ifdef DEBUG
void gdl_debug_printf (const char* func, int level, const char* format, ...);
void gdl_dock_print   (GdlDockMaster*);
const char* gdl_dock_object_id (GdlDockObject*);
#endif
