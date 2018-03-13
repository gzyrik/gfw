#ifndef __X_ALLOCA_H__
#define __X_ALLOCA_H__
#include "xtype.h"

#ifdef X_HAVE_ALLOCA_H
/* a native and working alloca.h is there */ 
# include <alloca.h>
#elif defined(__GNUC__)
/* GCC does the right thing */
# undef alloca
# define alloca(size)   __builtin_alloca (size)
#else /* !__GNUC__ && !X_HAVE_ALLOCA_H */
# if defined(_MSC_VER) || defined(__DMC__)
#  include <malloc.h>
#  define alloca _alloca
# else /* !_MSC_VER && !__DMC__ */
#  ifdef _AIX
#   pragma alloca
#  else /* !_AIX */
#   ifndef alloca /* predefined by HP cc +Olibcalls */
X_BEGIN_DECLS
char* alloca ();
X_END_DECLS
#   endif /* !alloca */
#  endif /* !_AIX */
# endif /* !_MSC_VER && !__DMC__ */
#endif /* !__GNUC__ && !X_HAVE_ALLOCA_H */


/**
 * Allocat memory at the current stack.
 */
#define x_alloca(size)  alloca (size)
#define x_newa(T, n)    ((T*) x_alloca (sizeof (T) * (xsize) (n)))

#endif /* __X_ALLOCA_H__ */
