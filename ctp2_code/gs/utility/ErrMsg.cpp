#include "ctp/c3.h"
#include "ctp/c3types.h"

#include "gs/utility/ErrMsg.h"

#ifdef LPRNT
FILE *lprint_fout;
#endif

ErrorMsg::ErrorMsg()
{

   Assert(0);
   str = "Generic Error";
}

ErrGSParse::ErrGSParse (char *str1)

{
    strncpy(msg, str1, sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = '\0';
}

ErrGSParse::ErrGSParse (char *str1, char *str2)

{
   strncpy(msg, str1, sizeof(msg) - 1);
   msg[sizeof(msg) - 1] = '\0';
   strncat(msg, str2, sizeof(msg) - strlen(msg) - 1);
}

ErrGSParse::ErrGSParse (char *str1, char *str2, sint32 x)
{
   char tmp [2 * _MAX_PATH];

   strncpy(msg, str1, sizeof(msg) - 1);
   msg[sizeof(msg) - 1] = '\0';
   snprintf (tmp, sizeof(tmp), str2, x);
   strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
}

ErrGSParse::ErrGSParse (char *str1, char *str2, char *val1)

{
   char tmp [2 * _MAX_PATH];

   strncpy(msg, str1, sizeof(msg) - 1);
   msg[sizeof(msg) - 1] = '\0';
   snprintf (tmp, sizeof(tmp), str2, val1);
   strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
}

ErrGSParse::ErrGSParse (char *str1, char *str2, char *val1, char *val2)

{
   char tmp [2 * _MAX_PATH];

   strncpy(msg, str1, sizeof(msg) - 1);
   msg[sizeof(msg) - 1] = '\0';
   snprintf (tmp, sizeof(tmp), str2, val1, val2);
   strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
}









void ErrorMsg::display ()

{
   exit(0);
}
