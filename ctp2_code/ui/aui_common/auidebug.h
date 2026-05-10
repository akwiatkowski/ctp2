#ifndef __AUIDEBUG_H__
#define __AUIDEBUG_H__

#ifdef _DEBUG

#ifndef Assert
#ifdef _WIN32
#include <crtdbg.h>
#define Assert(x) _ASSERTE(x)
#else
#define Assert(x) (void)0
#endif
#endif

#else

#ifndef Assert
#define Assert(x) (void)0
#endif

#endif

#endif
