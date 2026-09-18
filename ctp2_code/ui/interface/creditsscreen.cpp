//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The credit screen
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Corrected initialisations that were causing ambiguity.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Standardized code. (May 29th 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/creditsscreen.h"

#include <algorithm>	            // std::fill
#include <memory>
#include <vector>
#include "ui/aui_common/aui_action.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "gs/utility/Globals.h"                // allocated::clear
#include "ui/interface/UIUtils.h"
#include "ui/aui_utils/primitives.h"
#include "sound/soundmanager.h"           // soundmgr_Get()
#include "ui/aui_common/aui_bitmapfont.h"
#include "ui/interface/MessageBoxDialog.h"
#include "gfx/gfx_utils/colorset.h"               // colorset_Get()
#include "gs/database/StrDB.h"					// stringdb_Get()

#include "ui/ldl/ldl_data.hpp"

extern sint32		g_ScreenWidth;
extern sint32		g_ScreenHeight;

#define k_C3_ANIMATION_MODIFIER				20
#define k_C3_ANIMATION_SPEED				"animationSpeed"
#define k_C3_ANIMATION_FRAMES				"FrameTable"
#define k_C3_ANIMATION_MAXBLEND				32
#define k_C3_ANIMATION_BLEND_STEP			1
#define k_C3_ANIMATION_BLEND_SPEED			"blendSpeed"

#define k_CREDITS_BITS_PER_PIXEL			16
#define k_LDL_CREDITS_WINDOW				"CreditsScreen"
#define k_LDL_CREDITS_EXIT_BUTTON			"ExitButton"
#define k_LDL_CREDITS_PAUSE_BUTTON			"PauseButton"
#define k_LDL_CREDITS_SECRET_BUTTON			"SecretButton"

#define k_LDL_CREDITS_BACKGROUND			"BackgroundImage"
#define k_LDL_CREDITS_BACKANIM_COUNT		"backgroundAnims"
#define k_LDL_CREDITS_BACKANIM_BASE			"BackgroundAnim"
#define k_LDL_CREDITS_TRIGGERANIM_COUNT		"triggeredAnims"
#define k_LDL_CREDITS_TRIGGERANIM_BASE		"TriggeredAnim"
#define k_LDL_CREDITS_CREDITANIM			"CreditAnim"
#define k_LDL_CREDITS_BORDER				"BorderImage"
#define k_LDL_CREDITS_SECRET_IMAGE			"SecretImage"

#define k_CREDITS_FILENAME					"credits.txt"






size_t const	k_CreditsLineLen		= 80;
size_t const	kCreditsTextNumFonts	= 6;
uint32 const    NUMBER_INVALID          = static_cast<uint32>(-1);

static std::unique_ptr<CreditsWindow>  g_creditsWindow;

CreditsWindow * creditsscreen_GetWindow()
{
    return g_creditsWindow.get();
}

AUI_ACTION_BASIC(RemoveCreditsAction);

void RemoveCreditsAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	creditsscreen_Cleanup();
}

void creditsscreen_ExitButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if (action == (uint32)AUI_BUTTON_ACTION_EXECUTE)
    {
		AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow(g_creditsWindow->Id());
		Assert(auiErr == AUI_ERRCODE_OK);
		if (auiErr == AUI_ERRCODE_OK)
        {
			c3ui_Get()->AddAction(new RemoveCreditsAction());
        }
	}
}

void creditsscreen_PauseButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{

	if(action == (uint32)AUI_BUTTON_ACTION_EXECUTE) {
		g_creditsWindow->ToggleAnimation();
	}
}

void creditsscreen_SecretButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{

	if(action == (uint32)AUI_BUTTON_ACTION_EXECUTE) {
		g_creditsWindow->ShowSecretImage();
	}
}

sint32 creditsscreen_Initialize()
{
	if (!g_creditsWindow)
    {
	    AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	    g_creditsWindow.reset(new CreditsWindow
            (&errcode, aui_UniqueId(), const_cast<MBCHAR *>(k_LDL_CREDITS_WINDOW),
		     k_CREDITS_BITS_PER_PIXEL, AUI_WINDOW_TYPE_FLOATING
            ));
	    Assert(AUI_SUCCESS(errcode));
	    if (!AUI_SUCCESS(errcode)) { g_creditsWindow.reset(); return -1; }

	    TestControl(g_creditsWindow.get());

        g_creditsWindow->Move((g_ScreenWidth - g_creditsWindow->Width()) / 2,
		    (g_ScreenHeight - g_creditsWindow->Height()) / 2);

	    Assert(AUI_SUCCESS(errcode));
	    if (!AUI_SUCCESS(errcode)) return -1;
    }

    return 0;
}

void creditsscreen_Cleanup()
{
	if (g_creditsWindow)
    {
        if (c3ui_Get())
        {
            c3ui_Get()->RemoveWindow(g_creditsWindow->Id());
        }

        g_creditsWindow.reset();
    }
}




class c3_SimpleAnimation : public aui_Static {
public:

	c3_SimpleAnimation(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock)
	:	aui_Static(retval, id, ldlBlock),
		m_currentFrame(0),
		m_animationSpeed(100),
		m_lastIdleTicks(GetTickCount())
	{
		InitCommonLdl(ldlBlock);
	};

	c3_SimpleAnimation(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height,
		const MBCHAR *text = nullptr, uint32 maxLength = 0 )
	:	aui_Static(retval, id, x, y, width, height, text, maxLength),
		m_currentFrame(0),
		m_animationSpeed(100),
		m_lastIdleTicks(GetTickCount())
	{
	};




	AUI_ERRCODE Idle() override;

	void InitCommonLdl(MBCHAR *ldlBlock);

	void UpdateAnimation(sint32 deltaTime);

private:

	std::unique_ptr<aui_StringTable> m_frames;

	sint32 m_currentFrame;

	sint32 m_animationSpeed;

	uint32 m_lastIdleTicks;
};


AUI_ERRCODE c3_SimpleAnimation::Idle()
{
	sint32 deltaTime = GetTickCount() - m_lastIdleTicks;

	if(deltaTime < m_animationSpeed) return AUI_ERRCODE_OK;


	sint32 animationTime = 0;
	do {

		animationTime++;

		deltaTime -= m_animationSpeed;
	} while((deltaTime - m_animationSpeed) > 0);

	m_lastIdleTicks = GetTickCount() + (m_animationSpeed - deltaTime);

	UpdateAnimation(animationTime);

	return AUI_ERRCODE_OK;
}

void c3_SimpleAnimation::InitCommonLdl(MBCHAR *ldlBlock)
{
	ldl_datablock * datablock = aui_Ldl::FindDataBlock(ldlBlock);
	Assert(datablock != nullptr);
	if(!datablock) return;

	m_animationSpeed			= datablock->GetInt(k_C3_ANIMATION_SPEED);

	MBCHAR ldlString[k_AUI_LDL_MAXBLOCK + 1];
	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_C3_ANIMATION_FRAMES);
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	m_frames = std::make_unique<aui_StringTable>(&errcode, ldlString);
	Assert(m_frames);

	m_currentFrame = 0;
	SetImage(m_frames->GetString(m_currentFrame));
}

void c3_SimpleAnimation::UpdateAnimation(sint32 deltaTime)
{

	if(!m_frames) return;

	m_currentFrame += deltaTime;

	while(m_currentFrame >= m_frames->GetNumStrings())
		m_currentFrame -= m_frames->GetNumStrings();

	SetImage(m_frames->GetString(m_currentFrame));

	GetParentWindow()->ShouldDraw();
}





class c3_TriggeredAnimation : public aui_Static {
public:

	c3_TriggeredAnimation(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock)
	:	aui_Static(retval, id, ldlBlock),
		m_currentFrame(0),
		m_blendSpeed(100),
		m_blendVal(k_C3_ANIMATION_MAXBLEND),
		m_lastIdleTicks(GetTickCount())
	{
		InitCommonLdl(ldlBlock);
	};

	c3_TriggeredAnimation(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height,
		const MBCHAR *text = nullptr, uint32 maxLength = 0 )
	:	aui_Static(retval, id, x, y, width, height, text, maxLength),
		m_currentFrame(0),
		m_blendSpeed(100),
		m_blendVal(k_C3_ANIMATION_MAXBLEND),
		m_lastIdleTicks(GetTickCount())
	{
	};

	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr, sint32 x = 0, sint32 y = 0) override;


	AUI_ERRCODE Idle() override;

	void TriggerAnimationStep();


	void TriggerAnimationBlend();



	virtual AUI_ERRCODE DrawBlendImage(aui_Surface *destSurf, RECT *destRect);

	void InitCommonLdl(MBCHAR *ldlBlock);

private:

	std::unique_ptr<aui_StringTable> m_frames;

	sint32 m_currentFrame;

	sint32 m_blendSpeed;

	sint32 m_blendVal;

	uint32 m_lastIdleTicks;
};

AUI_ERRCODE c3_TriggeredAnimation::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{

	if (IsHidden()) return AUI_ERRCODE_OK;

	if (!surface) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	DrawBlendImage(surface, &rect);

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE c3_TriggeredAnimation::DrawBlendImage(aui_Surface *destSurf, RECT *destRect)
{
	if (m_blendVal >= k_C3_ANIMATION_MAXBLEND) {
		DrawThisStateImage(0, destSurf, destRect);
		return AUI_ERRCODE_OK;
	}


	aui_Image *image = GetImage( 0, AUI_IMAGEBASE_SUBSTATE_STATE );
	if(!image) return AUI_ERRCODE_OK;

	aui_Surface *srcSurf = image->TheSurface();

	RECT srcRect = { 0, 0, srcSurf->Width(), srcSurf->Height() };


	aui_Image *lastImage = GetImage( 1, AUI_IMAGEBASE_SUBSTATE_STATE );
	if(!lastImage) return AUI_ERRCODE_OK;

	aui_Surface *lastSurf = lastImage->TheSurface();

	RECT lastRect = { 0, 0, lastSurf->Width(), lastSurf->Height() };

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	std::unique_ptr<aui_Surface> backSurface(aui_Factory::new_Surface(errcode, srcRect.right, srcRect.bottom));
	Assert(AUI_NEWOK(backSurface, errcode));

	c3ui_Get()->TheBlitter()->Blt(backSurface.get(), 0, 0, destSurf, destRect, k_AUI_BLITTER_FLAG_COPY);


	if(m_imagebltflag == AUI_IMAGEBASE_BLTFLAG_CHROMAKEY) {

		std::unique_ptr<aui_Surface> frontSurface(aui_Factory::new_Surface(errcode, lastRect.right, lastRect.bottom));
		Assert(AUI_NEWOK(frontSurface, errcode));

		c3ui_Get()->TheBlitter()->Blt(frontSurface.get(), 0, 0, destSurf, destRect, k_AUI_BLITTER_FLAG_COPY);

		c3ui_Get()->TheBlitter()->Blt(frontSurface.get(), 0, 0, lastSurf, &lastRect, k_AUI_BLITTER_FLAG_CHROMAKEY);


		primitives_BlendSurfaces(frontSurface.get(), backSurface.get(), destSurf, destRect,
			k_C3_ANIMATION_MAXBLEND - m_blendVal);
	} else {

		primitives_BlendSurfaces(lastSurf, backSurface.get(), destSurf, destRect,
			k_C3_ANIMATION_MAXBLEND - m_blendVal);
	}

	c3ui_Get()->TheBlitter()->Blt(backSurface.get(), 0, 0, destSurf, destRect, k_AUI_BLITTER_FLAG_COPY);


	if(m_imagebltflag == AUI_IMAGEBASE_BLTFLAG_CHROMAKEY) {

		std::unique_ptr<aui_Surface> frontSurface(aui_Factory::new_Surface(errcode, srcRect.right, srcRect.bottom));
		Assert(AUI_NEWOK(frontSurface, errcode));

		c3ui_Get()->TheBlitter()->Blt(frontSurface.get(), 0, 0, destSurf, destRect, k_AUI_BLITTER_FLAG_COPY);

		c3ui_Get()->TheBlitter()->Blt(frontSurface.get(), 0, 0, srcSurf, &srcRect, k_AUI_BLITTER_FLAG_CHROMAKEY);

		primitives_BlendSurfaces(frontSurface.get(), backSurface.get(), destSurf, destRect, m_blendVal);
	} else {

		primitives_BlendSurfaces(srcSurf, backSurface.get(), destSurf, destRect, m_blendVal);
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE c3_TriggeredAnimation::Idle()
{

	if(m_blendVal >= k_C3_ANIMATION_MAXBLEND) return AUI_ERRCODE_OK;

	sint32 deltaTime = GetTickCount() - m_lastIdleTicks;

	if(deltaTime < m_blendSpeed) return AUI_ERRCODE_OK;


	sint32 blendTime = 0;
	do {

		blendTime += k_C3_ANIMATION_BLEND_STEP;

		deltaTime -= m_blendSpeed;
	} while((deltaTime - m_blendSpeed) > 0);

	m_lastIdleTicks = GetTickCount() + (m_blendSpeed - deltaTime);


	if(!m_blendVal) blendTime = k_C3_ANIMATION_BLEND_STEP;

	m_blendVal += blendTime;
	if(m_blendVal > k_C3_ANIMATION_MAXBLEND) m_blendVal = k_C3_ANIMATION_MAXBLEND;

	GetParentWindow()->ShouldDraw();

	return AUI_ERRCODE_OK;
}

void c3_TriggeredAnimation::InitCommonLdl(MBCHAR *ldlBlock)
{
    ldl_datablock * datablock = aui_Ldl::FindDataBlock(ldlBlock);
	Assert(datablock != nullptr);
	if(!datablock) return;

	m_blendSpeed			= datablock->GetInt(k_C3_ANIMATION_BLEND_SPEED);


	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR ldlString[k_AUI_LDL_MAXBLOCK + 1];
	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_C3_ANIMATION_FRAMES);
	m_frames = std::make_unique<aui_StringTable>(&errcode, ldlString);
	Assert(m_frames);

	m_currentFrame = 0;
	SetImage(m_frames->GetString(m_currentFrame));
}

void c3_TriggeredAnimation::TriggerAnimationStep()
{

	if(!m_frames) return;

	m_currentFrame++;

	while(m_currentFrame >= m_frames->GetNumStrings())
		m_currentFrame -= m_frames->GetNumStrings();

	SetImage(m_frames->GetString(m_currentFrame));

	GetParentWindow()->ShouldDraw();
}


void c3_TriggeredAnimation::TriggerAnimationBlend()
{

	if(!m_frames) return;

	SetImage(m_frames->GetString(m_currentFrame), 1);

	m_blendVal = 0;

	TriggerAnimationStep();
}









// One rendered line of credits text: a fixed-size string plus the index of
// the font it is drawn with. Lines live in a page-owned std::vector, so no
// manual allocation remains here.
struct sCreditsLine
{
	MBCHAR			 m_text[k_CreditsLineLen];
	uint32			 m_font;

	sCreditsLine(uint32 font, MBCHAR *pText);
};


// One screen of credits: an ordered set of lines drawn top-down.
class cCreditsPage
{
public:
	void			 AddLine(uint32 font, MBCHAR *pText);

	std::vector<sCreditsLine> m_lines;
};

enum eTokenType
{
	kBadToken,
	kFont,
	kText,
	kFontDefinition,
	kPageBreak,
	kEndOfCredits,
	kComment,
	kFontSize
};




class c3_CreditsText : public aui_Static
{
public:
	// Parses the credits text file into pages of lines. Returns false on a
	// malformed file; the object stays usable (empty) and the CALLER decides
	// on disposal — this method no longer deletes `this` internally.
	bool Parse(FILE *textfile);
	void ResetPages();

	c3_CreditsText
	(
		AUI_ERRCODE *	retval,
		uint32			id,
		MBCHAR *		ldlBlock
	)
	:	aui_Static(retval, id, ldlBlock),
		m_lastIdle(GetTickCount()),
		m_currPageIndex(0),
		m_definingFont(false),
		m_currFontNumber(0),
		m_currFontSize(0),
		m_numFonts(0)
	{
		std::fill(m_fonts, m_fonts + kCreditsTextNumFonts, (aui_BitmapFont *) nullptr);

        ldl_datablock * datablock = aui_Ldl::FindDataBlock(ldlBlock);
		Assert(datablock);
		if (!datablock) return;

		m_animationSpeed			= datablock->GetInt(k_C3_ANIMATION_SPEED);
		InitCommonLdl(ldlBlock);
	};

	c3_CreditsText
	(
		AUI_ERRCODE *	retval,
		uint32			id,
		sint32			x,
		sint32			y,
		sint32			width,
		sint32			height,
		const MBCHAR *	text		= nullptr,
		uint32			maxLength	= 0
	)
	:	aui_Static(retval, id, x, y, width, height, text, maxLength),
		m_lastIdle(GetTickCount()),
		m_currPageIndex(0),
		m_definingFont(false),
		m_currFontNumber(0),
		m_currFontSize(0),
		m_animationSpeed(3000),
		m_numFonts(0)
	{
		std::fill(m_fonts, m_fonts + kCreditsTextNumFonts, (aui_BitmapFont *) nullptr);
	};

	~c3_CreditsText() override
	{
		for (auto & m_font : m_fonts)
		{
			if (m_font)
				c3ui_Get()->UnloadBitmapFont(m_font);
		}

	};

	void NewPage();

	AUI_ERRCODE DrawThis(aui_Surface *pSurface = nullptr, sint32 x = 0, sint32 y = 0) override;


	AUI_ERRCODE Idle() override;

	uint32 m_lastIdle;

private:

	bool ParseFontDef(MBCHAR *pToken);
	uint32 GetFontNumber(MBCHAR *pToken);
	uint32 GetFontSize(MBCHAR *pToken);
	uint32 GetNumberFromToken(MBCHAR *pToken);
	eTokenType TokenType(MBCHAR *pToken);
	bool IsDelimiter(char c);
	bool IsComment(char c);
	void ReadToEOL(FILE *textfile);

	std::vector<cCreditsPage>	m_pages;
	size_t			 m_currPageIndex;

	aui_BitmapFont	*m_fonts[kCreditsTextNumFonts];

	bool			 m_definingFont;
	uint32			 m_currFontNumber;
	uint32			 m_currFontSize;
	uint32			 m_currTextfileLine;

	sint32 m_animationSpeed;

	uint32 m_numFonts;

};








CreditsWindow::CreditsWindow(AUI_ERRCODE *retval, sint32 id, MBCHAR *ldlBlock, sint32 bpp,
							 AUI_WINDOW_TYPE type, bool bevel)
	:	C3Window(retval, id, ldlBlock, bpp, type, bevel)
{

	m_animationSpeed = 10000;
	m_animating = true;
	m_lastIdleTicks = GetTickCount();

	InitCommonLdl(ldlBlock);
}

CreditsWindow::CreditsWindow(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y,
							 sint32 width, sint32 height, sint32 bpp, MBCHAR *pattern,
							 AUI_WINDOW_TYPE type, bool bevel)
	:	C3Window(retval, id, x, y, width, height, bpp, pattern, type, bevel)
{

	m_animationSpeed = 10000;
	m_animating = true;
	m_lastIdleTicks = GetTickCount();
}

// Children are released in the same order the hand-written teardown used
// (buttons/text/anims/background); every child is owned here even though the
// aui parent list keeps non-owning pointers for drawing.
CreditsWindow::~CreditsWindow()
{
	m_exitButton.reset();
	m_secretButton.reset();
	m_pauseButton.reset();
	m_border.reset();
	m_secretImage.reset();

	m_creditsText.reset();

	m_backgroundAnims.clear();
	m_triggeredAnims.clear();
	m_background.reset();
}

AUI_ERRCODE CreditsWindow::Idle()
{

	if(!m_animating) return AUI_ERRCODE_OK;

	sint32 deltaTime = GetTickCount() - m_lastIdleTicks;

	if(deltaTime < m_animationSpeed) return AUI_ERRCODE_OK;


	do {

		deltaTime -= m_animationSpeed;
	} while((deltaTime - m_animationSpeed) > 0);

	m_lastIdleTicks = GetTickCount() - deltaTime;


	for(auto animIter = m_triggeredAnims.rbegin(); animIter != m_triggeredAnims.rend(); ++animIter)
		(*animIter)->TriggerAnimationStep();

	return AUI_ERRCODE_OK;
}

void CreditsWindow::InitCommonLdl(MBCHAR *ldlBlock)
{
    ldl_datablock * datablock = aui_Ldl::FindDataBlock(ldlBlock);
	Assert(datablock != nullptr);
	if(!datablock) return;

	MBCHAR ldlString[k_AUI_LDL_MAXBLOCK + 1];
	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_CREDITS_BACKGROUND);
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	m_background = std::make_unique<aui_Static>(&errcode, aui_UniqueId(), ldlString);
	Assert(m_background);
	m_background->IgnoreEvents(TRUE);

	sint32 numberOfBackgroundAnims = datablock->GetInt(k_LDL_CREDITS_BACKANIM_COUNT);
	m_backgroundAnims.resize(numberOfBackgroundAnims > 0 ? numberOfBackgroundAnims : 0);

	for(sint32 index = 0; index < numberOfBackgroundAnims; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_CREDITS_BACKANIM_BASE, index+1);
		m_backgroundAnims[index] = std::make_unique<c3_SimpleAnimation>(&errcode, aui_UniqueId(), ldlString);
		Assert(m_backgroundAnims[index]);
		m_backgroundAnims[index]->IgnoreEvents(TRUE);
	}

	m_animationSpeed			= datablock->GetInt(k_C3_ANIMATION_SPEED);

	sint32 numberOfTriggeredAnims = datablock->GetInt(k_LDL_CREDITS_TRIGGERANIM_COUNT);
	m_triggeredAnims.resize(numberOfTriggeredAnims > 0 ? numberOfTriggeredAnims : 0);

	for(sint32 index = 0; index < numberOfTriggeredAnims; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_CREDITS_TRIGGERANIM_BASE, index+1);
		m_triggeredAnims[index] = std::make_unique<c3_TriggeredAnimation>(&errcode, aui_UniqueId(), ldlString);
		Assert(m_triggeredAnims[index]);
		m_triggeredAnims[index]->IgnoreEvents(TRUE);
	}










	if (m_creditsText == nullptr)
	{
		FILE *fp = c3files_fopen(C3DIR_UIDATA, k_CREDITS_FILENAME, "r");
		Assert(fp);

		std::unique_ptr<c3_CreditsText> creditsText =
			std::make_unique<c3_CreditsText>(&errcode, aui_UniqueId(), ldlBlock);
		Assert(creditsText);

		if (!creditsText->Parse(fp))
		{
			creditsText.reset();
		}

		fclose(fp);

		m_creditsText = std::move(creditsText);

		if (m_creditsText)
		{
			m_creditsText->IgnoreEvents(TRUE);
			m_creditsText->m_lastIdle = GetTickCount();
		}
	}

	if (m_creditsText)
	{
		m_creditsText->ResetPages();
	}
	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_CREDITS_SECRET_IMAGE);
	m_secretImage = std::make_unique<aui_Static>(&errcode, aui_UniqueId(), ldlString);
	Assert(m_secretImage);
	m_secretImage->IgnoreEvents(TRUE);
	m_secretImage->Hide();

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_CREDITS_BORDER);
	m_border = std::make_unique<aui_Static>(&errcode, aui_UniqueId(), ldlString);
	Assert(m_border);
	m_border->IgnoreEvents(TRUE);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_CREDITS_PAUSE_BUTTON);
	m_pauseButton = std::make_unique<aui_Button>(&errcode, aui_UniqueId(), ldlString,
		creditsscreen_PauseButtonActionCallback);
	Assert(m_pauseButton);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_CREDITS_SECRET_BUTTON);
	m_secretButton = std::make_unique<aui_Button>(&errcode, aui_UniqueId(), ldlString,
		creditsscreen_SecretButtonActionCallback);
	Assert(m_secretButton);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_CREDITS_EXIT_BUTTON);
	m_exitButton = std::make_unique<ctp2_Button>(&errcode, aui_UniqueId(), ldlString,
		creditsscreen_ExitButtonActionCallback);
	Assert(m_exitButton);

	AddControl(m_exitButton.get());
	AddControl(m_secretButton.get());
	AddControl(m_pauseButton.get());
	AddControl(m_border.get());
	AddControl(m_secretImage.get());




	if (m_creditsText)
		AddControl(m_creditsText.get());

	for(auto animIter = m_backgroundAnims.rbegin(); animIter != m_backgroundAnims.rend(); ++animIter)
		AddControl(animIter->get());
	for(auto animIter = m_triggeredAnims.rbegin(); animIter != m_triggeredAnims.rend(); ++animIter)
		AddControl(animIter->get());
	AddControl(m_background.get());
}

void CreditsWindow::ToggleAnimation()
{

	m_animating = !m_animating;

	if(!(m_secretImage->IsHidden())) {
		m_secretImage->Hide();
		ShouldDraw();
	}
}

void CreditsWindow::ShowSecretImage()
{

	if(m_secretImage->IsHidden()) {

		m_animating = false;

		m_secretImage->Show();

		ShouldDraw();
	} else {

		ToggleAnimation();
	}
}









sCreditsLine::sCreditsLine(uint32 font, MBCHAR *pText)
{
	strlcpy(m_text, pText, sizeof(m_text));
	m_font = font;
}

void cCreditsPage::AddLine(uint32 font, MBCHAR *pText)
{
	m_lines.emplace_back(font, pText);
}


AUI_ERRCODE c3_CreditsText::DrawThis(aui_Surface *pSurface, sint32 x, sint32 y)
{

	if (IsHidden()) return AUI_ERRCODE_OK;

	if (!pSurface) pSurface = m_window->TheSurface();

	if (m_currPageIndex < m_pages.size())
	{
		RECT rect = {};

		cCreditsPage &currPage = m_pages[m_currPageIndex];

		sint32 initialX = 8;
		sint32 initialY = 8;
		sint32 centerX = (((g_creditsWindow->Width() - 8) - initialX)/2)+initialX;

		sint32 currY = 0;

		for (sCreditsLine &currLine : currPage.m_lines)
		{
			aui_BitmapFont *pCurrFont = m_fonts[currLine.m_font];

			MBCHAR *pText = currLine.m_text;

			sint32 width = pCurrFont->GetStringWidth(pText);
			sint32 height = pCurrFont->GetMaxHeight();
			sint32 lineX = centerX - width/2;


			SetRect(&rect, lineX, initialY+currY, lineX+width, initialY+currY+pCurrFont->GetMaxHeight());

			pCurrFont->DrawString(pSurface, &rect, &rect, pText, 0,
								  colorset_Get()->GetColorRef(COLOR_WHITE), 0);

			currY += height + (height / 8);
		}


		if ( pSurface == m_window->TheSurface() )
			m_window->AddDirtyRect( &rect );

	}


	return AUI_ERRCODE_OK;
}

bool c3_CreditsText::Parse(FILE *textfile)
{
	MBCHAR      currToken[128];
	MBCHAR      errorStr[128];
	char        c;
	eTokenType  tokenType;
	uint32      currFont    = NUMBER_INVALID;
	int         i;
	bool        done        = false;
	bool        gettingText = false;

	NewPage();

	m_currTextfileLine = 0;

	while (done == false)
	{
		if (gettingText == false)
		{
			bool sawOpen = false;

			c = (char) fgetc(textfile);

			for (i = 0; ((i < 127) && (!IsDelimiter(c))); i++, c = (char)fgetc(textfile) )
			{
				currToken[i] = c;
				if (IsComment(c))
				{
					if (sawOpen == false)
					{
						i++;
						break;
					}

				}
				if (c == '<')
					sawOpen = true;
			}
		}
		else
		{

			c = (char)fgetc(textfile);
			for (i = 0; ((i < k_CreditsLineLen) && (c != '\n')); i++, c = (char)fgetc(textfile) )
			{
				currToken[i] = c;
			}

			currToken[i] = '\0';

			m_pages[m_currPageIndex].AddLine(currFont, currToken);
			m_currTextfileLine++;

			gettingText = false;
			continue;
		}


		currToken[i] = '\0';


		tokenType = TokenType(currToken);
		switch (tokenType)
		{
			case kFont:
			{
				currFont = GetFontNumber(currToken);
				if ((currFont == NUMBER_INVALID) || (currFont > m_numFonts) )
				{
					snprintf(errorStr, sizeof(errorStr), "%s line %d: Bad font specifier '%s'", k_CREDITS_FILENAME, m_currTextfileLine, currToken);
					MessageBoxDialog::Information(errorStr, "CreditsError");

					return false;
				}

				gettingText = TRUE;
				break;
			}
			case kText:
			{
				if (m_definingFont)
				{
					if (ParseFontDef(currToken))
					{
						m_definingFont = FALSE;
					}
					else
					{
						snprintf(errorStr, sizeof(errorStr), "%s line %d: Bad font filename '%s'", k_CREDITS_FILENAME, m_currTextfileLine, currToken);
						MessageBoxDialog::Information(errorStr, "CreditsError");

						return false;
					}
					break;
				}
				snprintf(errorStr, sizeof(errorStr), "%s line %d: Illegal text'%s'", k_CREDITS_FILENAME, m_currTextfileLine, currToken);
				MessageBoxDialog::Information(errorStr, "CreditsError");

				return false;
			}
			case kFontDefinition:
			{
				m_definingFont = TRUE;
				m_currFontNumber = GetFontNumber(currToken);
				if (m_currFontNumber == NUMBER_INVALID)
				{
					snprintf(errorStr, sizeof(errorStr), "%s line %d: Bad font definition '%s'", k_CREDITS_FILENAME, m_currTextfileLine, currToken);
					MessageBoxDialog::Information(errorStr, "CreditsError");

					return false;
				}
				break;
			}
			case kFontSize:
			{
				m_currFontSize = GetFontSize(currToken);
				if (m_currFontSize == NUMBER_INVALID)
				{
					snprintf(errorStr, sizeof(errorStr), "%s line %d: Bad font size '%s'", k_CREDITS_FILENAME, m_currTextfileLine, currToken);
					MessageBoxDialog::Information(errorStr, "CreditsError");

					return false;
				}
				break;
			}

			case kPageBreak:
			{
				NewPage();
				ReadToEOL(textfile);
				break;
			}
			case kComment:
			{
				ReadToEOL(textfile);
				break;
			}

			case kEndOfCredits:
			{

				return true;
			}
			case kBadToken:
			{
				snprintf(errorStr, sizeof(errorStr), "%s line %d: Bad token '%s'", k_CREDITS_FILENAME, m_currTextfileLine, currToken);
				MessageBoxDialog::Information(errorStr, "CreditsError");

				return false;
			}
		}


	}

	snprintf(errorStr, sizeof(errorStr), "%s line %d: Unexpected end of file found", k_CREDITS_FILENAME, m_currTextfileLine);
	MessageBoxDialog::Information(errorStr, "CreditsError");

	return false;
}

void c3_CreditsText::NewPage()
{
	m_pages.emplace_back();
	m_currPageIndex = m_pages.size() - 1;
}

void c3_CreditsText::ReadToEOL(FILE *textfile)
{
	char c = (char)fgetc(textfile);
	while( ((c != '\n') && !feof(textfile) ))
	{
		c = (char)fgetc(textfile);
	}

	if (c == '\n')
		m_currTextfileLine++;
}

bool c3_CreditsText::IsComment(char c)
{
	return (c == ';');
}

bool c3_CreditsText::IsDelimiter(char c)
{
	switch (c)
	{
		case '\n':
		{
			m_currTextfileLine++;

			// A newline is a delimiter too — deliberate fallthrough.
			[[fallthrough]];
		}
		case '>':
		case ' ':
		{
			return true;
		}
	}

	return false;
}




eTokenType c3_CreditsText::TokenType(MBCHAR *pToken)
{
	if (!strnicmp(pToken, "<fontDef", 8))
	{
		return kFontDefinition;
	}

	if (!strnicmp(pToken, "<font", 5))
	{
		return kFont;
	}

	if (!strnicmp(pToken, "<size", 5))
	{
		return kFontSize;
	}

	if (!stricmp(pToken, "<page"))
	{
		return kPageBreak;
	}

	if (!stricmp(pToken, "<end"))
	{
		return kEndOfCredits;
	}

	if (IsComment(*pToken))
	{
		return kComment;
	}

	if (*pToken == '>')
	{

		return kBadToken;
	}

	return kText;
}
bool c3_CreditsText::ParseFontDef(MBCHAR *pToken)
{
	MBCHAR errorStr[128];

	if (m_fonts[m_currFontNumber])
	{

		snprintf(errorStr, sizeof(errorStr), "%s line %d: Font already defined '%s'", k_CREDITS_FILENAME, m_currTextfileLine, pToken);
		MessageBoxDialog::Information(errorStr, "CreditsError");

		return FALSE;
	}

	m_fonts[m_currFontNumber] = c3ui_Get()->LoadBitmapFont(pToken, m_currFontSize);
	Assert(m_fonts[m_currFontNumber] != nullptr);
	if (!m_fonts[m_currFontNumber])
	{
		// The font file failed to load; report it instead of dereferencing.
		snprintf(errorStr, sizeof(errorStr), "%s line %d: Could not load font '%s'", k_CREDITS_FILENAME, m_currTextfileLine, pToken);
		MessageBoxDialog::Information(errorStr, "CreditsError");

		return FALSE;
	}

	m_fonts[m_currFontNumber]->SetPointSize(m_currFontSize);

	if ((m_currFontNumber + 1)> m_numFonts)
		m_numFonts = m_currFontNumber + 1;

	return TRUE;
}


uint32 c3_CreditsText::GetFontSize(MBCHAR *pToken)
{

	return GetNumberFromToken(pToken);
}

uint32 c3_CreditsText::GetNumberFromToken(MBCHAR *pToken)
{
	MBCHAR *    digitStart    = pToken;
	size_t      len     = strlen(pToken);

	for (size_t i = 0; i < len; ++i)
	{
		if (isdigit(*digitStart))
		{
			return atoi(digitStart);
		}

		++digitStart;
	}

    return NUMBER_INVALID;
}


uint32 c3_CreditsText::GetFontNumber(MBCHAR *pToken)
{
	uint32 const retval = GetNumberFromToken(pToken);

	// kCreditsTextNumFonts is the size of m_fonts — a number equal to it is
	// already one past the last slot (indices run 0..kCreditsTextNumFonts-1).
    return (retval >= kCreditsTextNumFonts) ? NUMBER_INVALID : retval;
}

void c3_CreditsText::ResetPages()
{
	m_currPageIndex = 0;
}

AUI_ERRCODE c3_CreditsText::Idle()
{
	sint32 deltaTime = GetTickCount() - m_lastIdle;

	if (deltaTime < m_animationSpeed) return AUI_ERRCODE_OK;


	// Consume whole animation steps; the leftover delta keeps the next
	// Idle() call's timing accurate.
	do {
		deltaTime -= m_animationSpeed;
	} while((deltaTime - m_animationSpeed) > 0);

	m_lastIdle = GetTickCount() - deltaTime;




	if (!m_pages.empty())
	{
		if (m_currPageIndex + 1 < m_pages.size())
			m_currPageIndex++;
		else
			ResetPages();
	}

	GetParentWindow()->ShouldDraw();


	return AUI_ERRCODE_OK;
}
