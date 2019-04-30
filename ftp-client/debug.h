#define DEBUG 1
// COMMENT ABOVE LINE TO TURN OFF DEBUG MODE
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

/*
USAGE:

DEBUG_PRINT(("var1: %d; var2: %d; str: %s\n", var1, var2, str));

Double Parenthesis are needed. see: https://stackoverflow.com/questions/1941307/debug-print-macro-in-c
*/
