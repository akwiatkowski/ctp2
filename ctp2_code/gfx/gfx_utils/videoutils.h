#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __VIDEOUTILS_H__
#define __VIDEOUTILS_H__

void videoutils_Initialize();
sint32 videoutils_PlayVideoInWindow(MBCHAR *name, MBCHAR *pattern);
void videoutils_Cleanup();

#endif
