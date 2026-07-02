#include <windows.h>

#include "sound/ear_util.h"









#define EU_SUE_TELL_USER_KILL	0
#define EU_SUE_TELL_USER_ASK	1
#define EU_SUE_EAR_POST_QUIT	1





BOOL gbAUDIO_OFF = FALSE;




extern BOOL gbEarLoadedPDS;









VOID EU_Debug(char *format, ...)
 {
#ifdef _DEBUG
	#define EAR_DEBUG_MESSAGE_LENGTH 256

	va_list  list;
	char messageString[EAR_DEBUG_MESSAGE_LENGTH];

	va_start(list, format);
	_vsnprintf(messageString, EAR_DEBUG_MESSAGE_LENGTH, format, list);
	va_end(list);

	OutputDebugString(messageString);

#endif

 }



































































BOOL EU_StartUpEar(VOID)
 {

	BOOL result;
	char message[256] = "Unknown Error.\n\n";
	char title[64] = "Fatal Audio Error:";




	DWORD EAR_STARTUP_FLAGS = EAR_INIT_MUSTBEEAR | EAR_INIT_QUERYUSER;








	BOOL load_earpds = FALSE;






registration_start:

	result = EP_RegisterEarServiceTable(load_earpds);













	if (result == FALSE)
	 {
		if (EAR_DLL == 0)
		 {
			EU_Debug("FATAL AUDIO ERROR: No EAR DLL found!\n");

		strlcpy(message,
			   "You need to install the EAR audio engine onto your system\n"\
			   "(check your manual for instructions).", sizeof(message));

		 }

		else
		 {









			if (EAR_AAA_Validate == 0)
			 {	EU_Debug("FATAL AUDIO ERROR: EAR DLL found, but can't find EAR code!\n");

			strlcpy(message,
				   "The EAR audio engine components on your system"\
				   " are corrupt;\n try re-installing the audio package"\
				   " that came with your hardware.", sizeof(message));

			 }

			else
			 {	EU_Debug("FATAL AUDIO ERROR: EAR DLL found, but PROC handles in ear.h/ear_proc.cpp "\
				         "not match those in the dll!\n");

			strlcpy(message,
				   "There is a version mis-match between this software"\
				   " and the audio engine on your system. Check your"\
				   " manual for the right version numbers.", sizeof(message));

			 }

		 }

		goto startup_failed;

	 }












	result = EAR_AAA_Validate(EAR_VALIDATION_NUMBER);

	if (result == 0)
	 {






#ifdef _DEBUG
		EAR_InitializeEar(EAR_INIT_DEBUGNORMAL);
		EAR_AAA_Validate(EAR_VALIDATION_NUMBER);
#endif

		strlcpy(message,
			   "This program is unable to validate the audio engine"\
			   " on your machine.", sizeof(message));

		goto startup_failed;

	 }








	result = EAR_AssignHwnd((DWORD)AfxGetApp()->GetMainWnd()->GetSafeHwnd());

	if (result == FALSE)
	 {






		EU_Debug("FATAL AUDIO: Cannot find main window... make sure it has been created!\n");

		strlcpy(message, "The audio engine cannot find the program's main window.\n"\
			   "Try closing other applications and restarting.", sizeof(message));

		goto startup_failed;

	 }










	result = EAR_InitializeEar(EAR_STARTUP_FLAGS);

	if (result == FALSE)
	 {




























		if (gbEarLoadedPDS)
			goto pass_two;




		if (!load_earpds)
		 {	result = EAR_GetLastError();

			if (result == EAR_ERR_USERWANTSSTEREO)
			 {	goto pass_two;
			 }

			else if (result == EAR_ERR_USERCANCELLED)
			 {				strlcpy(message, "Audio engine did not load.", sizeof(message));
				goto startup_failed;
			 }

			else
			 {	goto pass_two;
			 }

		 }




		else
		 {				strlcpy(message, "Some unknown error occurred while loading the audio engine.", sizeof(message));
			goto startup_failed;

		 }





pass_two:

		EAR_STARTUP_FLAGS &= ~EAR_INIT_MUSTBEEAR;







		load_earpds = TRUE;

		EP_ClearEarServiceTable();

		goto registration_start;

	 }

	goto startup_succeeded;






startup_failed:

		gbAUDIO_OFF = TRUE;

#if EU_SUE_TELL_USER_KILL

	strlcat(message, "\n\nProgram will terminate.", sizeof(message));
	MessageBox(NULL, message, title, MB_OK | MB_ICONSTOP);

#elif EU_SUE_TELL_USER_ASK

	strlcat(message, "\n\nDo you want to run the program without audio?", sizeof(message));

	if (MessageBox(NULL, message, title, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
	 {
		MessageBox(NULL, "Audio turned off.", title, MB_OK | MB_ICONEXCLAMATION);

		return FALSE;

	 }

	else
	 {
		MessageBox(NULL, "Program will terminate.", title, MB_OK | MB_ICONSTOP);

	 }

#endif

#if EU_SUE_EAR_POST_QUIT

	PostQuitMessage(0xFFFF);

#endif

	return FALSE;





startup_succeeded:

	return TRUE;

 }












VOID EU_TearDownEar(VOID)
 {
	EAR_ShutDownEar();

	EP_ClearEarServiceTable();

 }
