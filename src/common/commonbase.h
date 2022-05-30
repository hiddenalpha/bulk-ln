
/*
 * common config for project. Here goes stuff like feature-test-macros etc.
 *
 * Every header file MUST include this file AS THE 1ST include.
 */


#define _POSIX_C_SOURCE 200809L 


#define STR_QUOT_IAHGEWIH(s) #s
#define STR_QUOT(s) STR_QUOT_IAHGEWIH(s)

#define STR_CAT(a, b) a ## b

#ifndef likely
#   define likely(a) (a)
#endif
#ifndef unlikely
#   define unlikely(a) (a)
#endif

