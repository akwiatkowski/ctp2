//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Screen utilities
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _DEBUG
// - Generate debug version when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added close_AllScreensAndUpdateInfoScreen so that on a new turn the
//   information window can stay open. (Aug. 7th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SCREENUTILS_H__
#define __SCREENUTILS_H__

#ifdef _DEBUG

#define SET_TIME				g_screenTime = GetTickCount();

#define GET_ELAPSED_TIME(x)		g_screenTime = (GetTickCount() - g_screenTime) * 0.001;	\
								MBCHAR str[50];	\
								snprintf( str, sizeof(str), "%4.2f secs - %s", g_screenTime, x );	\
								if ( g_debugWindow) g_debugWindow->AddText( str );
#endif

sint32  open_CreditsScreen( );
void    close_CreditsScreen( );

sint32  open_WorkView( );
void    close_WorkView( );

sint32  open_CityView( );
void    close_CityView( );

sint32  open_CityStatus( );
void    close_CityStatus( );

sint32  open_CivStatus( );
void    close_CivStatus( );

sint32  open_ScienceStatus( );
void    close_ScienceStatus();

sint32  open_ScienceVictory( );
void    close_ScienceVictory( );

sint32  open_UnitStatus( );
void    close_UnitStatus( );

sint32  open_TradeStatus( );
void    close_TradeStatus( );

sint32  open_VictoryWindow( );
void    close_VictoryWindow( );

sint32  open_Diplomacy( );
void    close_Diplomacy( );

sint32  open_InfoScreen( );
void    close_InfoScreen( );

bool    open_GreatLibrary( sint32 index, bool sci = false );
bool    open_GreatLibrary( );
void    close_GreatLibrary( );

sint32  open_OptionsScreen( sint32 fromWhichScreen );
void    close_OptionsScreen( );

sint32  open_KeyMappingScreen( );
void    close_KeyMappingScreen( );

sint32  open_EndGame( );
void    close_EndGame( );

sint32  open_TutorialWin( );
void    close_TutorialWin( );

void    close_AllScreens( );
void    close_AllScreensAndUpdateInfoScreen( );

sint32  open_ScenarioEditor();
void    close_ScenarioEditor();

#endif
