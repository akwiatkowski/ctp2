#include "ctp/c3.h"

#include "gs/utility/RandGen.h"
extern HWND gHwnd;

sint32 g_is_rand_test=FALSE;
sint32 g_ai_rand_test_wait = 2;
sint32 g_ai_rand_test_max = 1000;
sint32 g_ai_rand_test_total = 0;

void ai_rand_test()

{

    #if 0 // CTP1?
    static count=0;
    static total=0;

    if (0 <g_ai_rand_test_wait) {
       count = (count + 1) %g_ai_rand_test_wait;
       if (0 < count) {
            return;
        }
    }

    g_ai_rand_test_total++;

    if (g_ai_rand_test_max < g_ai_rand_test_total ) {
        g_is_rand_test = FALSE;
    }






   if (rand_ptr()->Next(5) == 0) {
   	    PostMessage(gHwnd, WM_CHAR, 'b', 0);
   }

   if (rand_ptr()->Next(20) == 0) {
       	PostMessage(gHwnd, WM_CHAR, 's', 0);
   }
   PostMessage(gHwnd, WM_CHAR, '1' + rand_ptr()->Next(9), 0);
#endif
}
