#include "debug/debug.h"
#include "gdl-dock-master.h"

extern int indent;
extern int gdl_debug;

#ifdef DEBUG
#define ENTER {indent++;}
#define LEAVE {indent--; indent = MAX(indent, 0); }
#define cdbg(A, B, ...) \
	{ \
		{ \
			int i; \
			if(A <= gdl_debug) for(i=0;i<indent;i++) printf("  "); \
			fflush(stdout); \
		} \
		gdl_debug_printf(__func__, A, B, ##__VA_ARGS__); \
	}
#else
#define ENTER
#define LEAVE
#define cdbg(A, B, ...)
#endif

void gdl_debug_printf (const char* func, int level, const char* format, ...);
void gdl_dock_print   (GdlDockMaster*);
