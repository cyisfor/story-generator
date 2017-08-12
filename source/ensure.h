#include "note.h" // error(...)

#define ensure0(test) { int res = (test);				\
		if(res != 0) {															\
			error(#test " %d not zero",res);					\
			abort();																	\
		}																						\
	}

#define ensure_eq(less,more) { int res1 = (less);		\
		int res2 = (more);															\
		if(res1 != res2) {															\
			error(#less " %d != " #more " %d",res1,res2); \
			abort();																			\
		}																								\
}


#define ensure_gt(less,more) { int res1 = (less);		\
		int res2 = (more);															\
		if(res1 <= res2) {															\
			error(#less " %d <= " #more " %d",res1,res2); \
			abort();																			\
		}																								\
}
