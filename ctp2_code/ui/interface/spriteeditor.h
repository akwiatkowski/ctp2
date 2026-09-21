#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SPRITEEDITOR_H__
#define __SPRITEEDITOR_H__

class SpriteEditWindow;

#include <memory>

#include "ui/aui_ctp2/c3window.h"       // C3Window
#include "os/include/ctp2_inttypes.h"  // sintNN, uintNN
#include "gs/world/MapPoint.h"       // MapPoint
// AUI_ERRCODE, AUI_WINDOW_TYPE
// BOOL, MBCHAR, RECT

class Action;
class Anim;
class aui_Surface;
class c3_Static;
class C3TextField;
class ctp2_Button;
class Sprite;
class UnitSpriteGroup;

class SpriteEditWindow : public C3Window
{
public:


	SpriteEditWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_BACKGROUND );




	SpriteEditWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 bpp,
		MBCHAR *pattern,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_BACKGROUND );




	~SpriteEditWindow() override;




	virtual AUI_ERRCODE InitCommon();
	virtual AUI_ERRCODE InitCommonLdl(MBCHAR *ldlBlock);




	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;




	AUI_ERRCODE	Idle( ) override;




	void	InitializeControls(AUI_ERRCODE *errcode,MBCHAR const *windowsBlock);
	void	TopLevel();

	void			LoadSprite(char *name=nullptr);

	void			SaveSprite(char *name=nullptr);

	void			DrawSprite();
	void			ReDrawLargeSprite();

	void	AddFrame	(sint32 add);
	void	SetFrame	(sint32 frame);
	void	AddFacing	(sint32 add);
	void	SetFacing	(sint32 facing);
	void	SetAnimation(sint32 anim);
	void	Animate();
	void	BeginAnimation();

	bool	FileExists(char *name);

	bool			m_loopInProgress;
	bool			m_stopAfterLoop;

	sint32			m_drawFlag;
	sint32			m_facing;
	sint32			m_frame;
	sint32			m_animation;

	sint32			m_lastTime;
private:

	sint32	m_dest;
	sint32	m_current;
	BOOL	m_scroll;
	sint32	m_mouseChangeY;
	sint32	m_drawX,m_drawY;




	std::unique_ptr<UnitSpriteGroup>	m_currentSprite;
	Anim				*m_currentAnim;
	Sprite				*m_spriteData;
	std::unique_ptr<aui_Surface>		m_spriteSurface;
	RECT				m_spriteRect;

	std::unique_ptr<Action>	m_actionObj;






	std::unique_ptr<ctp2_Button>	m_Load;
	std::unique_ptr<ctp2_Button>	m_Save;

	std::unique_ptr<C3TextField>	m_fileName;

	std::unique_ptr<ctp2_Button>	m_MOVEAnim;
	std::unique_ptr<ctp2_Button>	m_ATTACKAnim;
	std::unique_ptr<ctp2_Button>	m_IDLEAnim;
	std::unique_ptr<ctp2_Button>	m_VICTORYAnim;
	std::unique_ptr<ctp2_Button>	m_WORKAnim;

	std::unique_ptr<ctp2_Button>	m_stepPlus;
	std::unique_ptr<ctp2_Button>	m_stepMinus;
	std::unique_ptr<ctp2_Button>	m_playOnce;
	std::unique_ptr<ctp2_Button>	m_playLoop;
	std::unique_ptr<ctp2_Button>	m_facingPlus;
	std::unique_ptr<ctp2_Button>	m_facingMinus;

	std::unique_ptr<C3Window>		m_largeImage;
	aui_Surface		*m_largeSurface;
	RECT			m_largeRect;
	RECT			m_largeRectAbs;

	std::unique_ptr<c3_Static>		m_hotCoordsCurrent;
	std::unique_ptr<c3_Static>		m_hotCoordsMouse;
	std::unique_ptr<c3_Static>		m_hotCoordsHerald;

	float			m_widthRatio;
	float			m_heightRatio;
	float			m_oneOverWidthRatio;
	float			m_oneOverHeightRatio;

	MapPoint        m_drawPoint;

};




int SpriteEditWindow_Initialize( );
void SpriteEditWindow_Cleanup();
#endif
