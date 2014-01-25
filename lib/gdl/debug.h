
extern int indent;
extern int gdl_debug;
#define F1 (indent++)
#define F2 {indent--; indent = MAX(indent, 0); }
#define cdbg(A, B, ...) \
	{ \
		{ \
			int i; \
			if(A <= gdl_debug) for(i=0;i<indent;i++) printf("  "); \
			fflush(stdout); \
		} \
		gdl_debug_printf(__func__, A, B, ##__VA_ARGS__); \
	}
void gdl_debug_printf(const char* func, int level, const char* format, ...);

