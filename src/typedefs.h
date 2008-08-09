#define OVERVIEW_WIDTH 150
#define OVERVIEW_HEIGHT 20

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

typedef struct _sample sample;

