#include "ctp/c3.h"

#include "ui/interface/EndgameWindow.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_progressbar.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_coloredstatic.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/interface/UIUtils.h"
#include "ui/aui_utils/primitives.h"
#include "sound/soundmanager.h"
#include "ui/ldl/ldl_file.hpp"
#include "ui/ldl/ldl_data.hpp"

#include "gs/database/StrDB.h"
#include "SoundRecord.h"
#include "gs/database/EndGameDB.h"
#include "gs/gameobj/EndGame.h"
#include "gs/utility/TurnCnt.h"                // g_turn

#include <vector>

#include "ui/aui_ctp2/keypress.h"

#define k_C3_BLEND_MAXBLEND					32

#define k_C3_ANIMATION_MODIFIER				20
#define k_C3_ANIMATION_SPEED				"animationSpeed"
#define k_C3_ANIMATION_FRAMES				"FrameTable"

#define k_ENDGAME_BITS_PER_PIXEL			16
#define k_ENDGAME_TOO_MUCH_TIME				500
#define k_ENDGAME_DARKEN_VALUE				64
#define k_LDL_ENDGAME_WINDOW				"EndGameWindow"
#define k_LDL_ENDGAME_EXIT_BUTTON			"ExitButton"
#define k_LDL_ENDGAME_BLEND_SPEED			"blendSpeed"
#define k_ENDGAME_AMBIENT_SOUND				"SOUND_ID_ALIEN_AMBIENT"

#define k_LDL_ENDGAME_BACKGROUND			"BackgroundImage"
#define k_LDL_ENDGAME_BACKANIM_COUNT		"backgroundAnims"
#define k_LDL_ENDGAME_BACKANIM_BASE			"BackgroundAnim"
#define k_LDL_ENDGAME_BORDER				"BorderImage"
#define k_LDL_ENDGAME_TANK_NAME				"embryoTankName"
#define k_LDL_ENDGAME_BROKEN_TANK			"BrokenTank"
#define k_LDL_ENDGAME_EMBRYO_TANK			"EmbryoTank"
#define k_LDL_ENDGAME_EMBRYO_GLOW			"EmbryoGlow"
#define k_LDL_ENDGAME_STAGE_COUNT			"stages"
#define k_LDL_ENDGAME_STAGE_BASE			"EmbryoStage"
#define k_LDL_ENDGAME_CONTAIN_NAME			"containmentFieldGeneratorName"
#define k_LDL_ENDGAME_CONTAIN_COUNT			"containmentFieldGenerators"
#define k_LDL_ENDGAME_CONTAIN_BASE			"ContainmentFieldGenerator"
#define k_LDL_ENDGAME_ECD_NAME				"ecdName"
#define k_LDL_ENDGAME_ECD_COUNT				"ecds"
#define k_LDL_ENDGAME_ECD_BASE				"ECD"
#define k_LDL_ENDGAME_SPLICER_NAME			"geneSplicerName"
#define k_LDL_ENDGAME_SPLICER_COUNT			"geneSplicers"
#define k_LDL_ENDGAME_SPLICER_BASE			"GeneSplicer"
#define k_LDL_ENDGAME_SPLICERANIM_BASE		"GeneSplicerAnim"

#define k_ENDGAME_FB_TURN_RED				(RGB(255,0,0))
#define k_ENDGAME_FB_TURN_GREEN				(RGB(0,255,0))
#define k_ENDGAME_FB_STAGE_GREY				(COLOR_GRAY)
#define k_ENDGAME_FB_STAGE_GREEN			(COLOR_GREEN)
#define k_LDL_ENDGAME_FB_DARKEN_AREA		"DarkenArea"
#define k_LDL_ENDGAME_FB_LABEL_COUNT		"feedbackLabels"
#define k_LDL_ENDGAME_FB_LABEL_BASE			"FeedbackLabel"
#define k_LDL_ENDGAME_FB_LIGHT_COUNT		"stageLights"
#define k_LDL_ENDGAME_FB_LIGHT_BASE			"StageLight"
#define k_LDL_ENDGAME_FB_PROGRESS_BG		"TurnProgressBackground"
#define k_LDL_ENDGAME_FB_TURN_PROGRESS		"TurnProgress"
#define k_LDL_ENDGAME_FB_TURNS_REMAINING	"TurnsRemaining"
#define k_LDL_ENDGAME_FB_ECD_RATIO			"ECDRatio"
#define k_LDL_ENDGAME_FB_CONTAIN_RATIO		"ContainmentFieldRatio"
#define k_LDL_ENDGAME_FB_SPLICER_RATIO		"GeneSplicerRatio"
#define k_LDL_ENDGAME_FB_FAILURE_CHANCE		"ChanceOfFailure"

extern sint32		g_ScreenWidth;
extern sint32		g_ScreenHeight;



static std::unique_ptr<EndGameWindow>	g_endgameWindow;

EndGameWindow * endgamewindow_Get()
{
	return g_endgameWindow.get();
}




void DarkenSurface( aui_Surface *pSurface, RECT *pArea, sint32 percentDarken )
{
	// Stub: DarkenSurface was originally in a missing/lost source file.
	// For now, do nothing. The endgame window will render without darkening.
	(void)pSurface;
	(void)pArea;
	(void)percentDarken;
}


class c3_DarkenArea : public aui_Static {
public:

	c3_DarkenArea(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock) :
		aui_ImageBase( ldlBlock ),
		aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
		aui_Static(retval, id, ldlBlock)
		{ }

	c3_DarkenArea(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height,
		MBCHAR *text, uint32 maxLength) :
		aui_ImageBase( (sint32)0 ),
		aui_TextBase( text, maxLength ),
		aui_Static(retval, id, x, y, width, height)
		{ }

	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr, sint32 x = 0, sint32 y = 0) override;
};

AUI_ERRCODE c3_DarkenArea::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	DarkenSurface(surface, &rect, k_ENDGAME_DARKEN_VALUE);

	return(aui_Static::DrawThis(surface, x, y));
}

class c3_YetAnotherProgressBar : public aui_ProgressBar {
public:

	c3_YetAnotherProgressBar(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock) :
		aui_ImageBase( ldlBlock ),
		aui_TextBase( ldlBlock, (const MBCHAR *)nullptr ),
		aui_ProgressBar( retval, id, ldlBlock )
	{}

	c3_YetAnotherProgressBar(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height) :
		aui_ImageBase( (sint32)0 ),
		aui_TextBase( nullptr ),
		aui_ProgressBar( retval, id, x, y, width, height )
	{}

protected:
	c3_YetAnotherProgressBar() : aui_ProgressBar() {}

	AUI_ERRCODE CalculateIntervals( double *start, double *stop ) override {

		double x = (double)m_curValue / (double)m_maxValue;

		*start = 0.0;
		*stop = x;

		return AUI_ERRCODE_OK;
	}
};

class RemoveEndGameAction : public aui_Action
{
public:
	void Execute(aui_Control *control, uint32 action, uint32 data) override;
};

void RemoveEndGameAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	endgamewindow_Cleanup();
}

void EndGameWindow::kh_Close()
{
	endgamewindow_Cleanup();
}

void endgamewindow_ExitButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{

	if(action == (uint32)AUI_BUTTON_ACTION_EXECUTE) {

		AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow(g_endgameWindow->Id());

		Assert(auiErr == AUI_ERRCODE_OK);
		if(auiErr != AUI_ERRCODE_OK) return;

		RemoveEndGameAction	*actionObj = new RemoveEndGameAction;
		c3ui_Get()->AddAction(actionObj);
	}
}

sint32 endgamewindow_Initialize()
{

	AUI_ERRCODE errcode;

	if(g_endgameWindow) return(0);






	g_endgameWindow.reset(new EndGameWindow(&errcode, aui_UniqueId(), k_LDL_ENDGAME_WINDOW,
		k_ENDGAME_BITS_PER_PIXEL, AUI_WINDOW_TYPE_POPUP));
	TestControl(g_endgameWindow.get());

	keypress_RegisterHandler(g_endgameWindow.get());

	sint32 snd_id = g_theSoundDB->FindTypeIndex(k_ENDGAME_AMBIENT_SOUND);
	if(soundmgr_Get()) soundmgr_Get()->AddLoopingSound(SOUNDTYPE_SFX, (uintptr_t)g_endgameWindow.get(), snd_id);

	Assert(AUI_SUCCESS(errcode));
	if(!AUI_SUCCESS(errcode)) return(-1);

	g_endgameWindow->Move((g_ScreenWidth - g_endgameWindow->Width()) / 2,
		(g_ScreenHeight - g_endgameWindow->Height()) / 2);

	Assert(AUI_SUCCESS(errcode));
	if(!AUI_SUCCESS(errcode)) return(-1);

	return(0);
}

sint32 endgamewindow_Cleanup()
{

	if(!g_endgameWindow) return(0);

	if(soundmgr_Get()) soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX, (uintptr_t)g_endgameWindow.get());

	c3ui_Get()->RemoveWindow(g_endgameWindow->Id());

	keypress_RemoveHandler(g_endgameWindow.get());

	g_endgameWindow.reset();

	return(0);
}







class c3_Blend : public aui_Static {
public:

	c3_Blend(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock) :
		aui_ImageBase( ldlBlock, true ),
		aui_TextBase((const MBCHAR *)nullptr, (uint32)0),
		aui_Static(retval, id, ldlBlock)
		{ m_blendVal = 0; m_soundID = -1; }

	c3_Blend(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height,
		const MBCHAR *text = nullptr, uint32 maxLength = 0 ) :
		aui_ImageBase( (sint32)0, AUI_IMAGEBASE_BLTTYPE_COPY, AUI_IMAGEBASE_BLTFLAG_COPY, true),
		aui_TextBase((const MBCHAR *)nullptr, (uint32)0),
		aui_Static(retval, id, x, y, width, height, text, maxLength)
		{ m_blendVal = 0; m_soundID = -1; }

	AUI_ERRCODE Draw(aui_Surface *surface, sint32 x, sint32 y) override;

	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr, sint32 x = 0, sint32 y = 0) override;

	void SetBlend(sint32 val) { m_blendVal = val; }

	sint32 GetBlend() { return(m_blendVal); }

	sint32 m_soundID;

protected:

	c3_Blend() : aui_Static() { m_blendVal = 0; m_soundID = -1; }

	virtual AUI_ERRCODE DrawBlendImage(aui_Surface *destSurf, RECT *destRect);

private:

	sint32 m_blendVal;
};

AUI_ERRCODE c3_Blend::Draw( aui_Surface *surface, sint32 x, sint32 y )
{

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	if(!IsHidden() || (m_blendVal > 0)) errcode = DrawThis( surface, x, y );




	if(errcode == AUI_ERRCODE_OK) DrawChildren( surface, x, y );

	m_draw = 0;

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE c3_Blend::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{

	if (IsHidden() && (m_blendVal <= 0)) return AUI_ERRCODE_OK;

	if (!surface) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	DrawBlendImage(surface, &rect);





	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE c3_Blend::DrawBlendImage(aui_Surface *destSurf, RECT *destRect)
{

	AUI_ERRCODE errcode;

	if(m_blendVal >= k_C3_BLEND_MAXBLEND) {
		DrawThisStateImage(0, destSurf, destRect);
		return AUI_ERRCODE_OK;
	}


	aui_Image *image = GetImage( 0, AUI_IMAGEBASE_SUBSTATE_STATE );
	if(!image) return AUI_ERRCODE_OK;

	aui_Surface *srcSurf = image->TheSurface();

	RECT srcRect = { 0, 0, srcSurf->Width(), srcSurf->Height() };

	std::unique_ptr<aui_Surface> backSurface(aui_Factory::new_Surface(errcode, srcRect.right, srcRect.bottom));
	Assert(AUI_NEWOK(backSurface, errcode));

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


	aui_Image *highlightImage = GetImage( 1, AUI_IMAGEBASE_SUBSTATE_STATE );
	if(!highlightImage) {
		return AUI_ERRCODE_OK;
	}

	aui_Surface *highlightSurf = highlightImage->TheSurface();

	RECT highlightRect = { 0, 0, highlightSurf->Width(), highlightSurf->Height() };

	c3ui_Get()->TheBlitter()->Blt(backSurface.get(), 0, 0, destSurf, destRect, k_AUI_BLITTER_FLAG_COPY);


	sint32 highlightBlendVal = m_blendVal * 3;
	if(highlightBlendVal > ((3 * (k_C3_BLEND_MAXBLEND)) / 4)) {

		if(highlightBlendVal > ((2 * (k_C3_BLEND_MAXBLEND)) + ((k_C3_BLEND_MAXBLEND) / 4)))
			highlightBlendVal = (3 * (k_C3_BLEND_MAXBLEND)) - highlightBlendVal;
		else
			highlightBlendVal = (3 * (k_C3_BLEND_MAXBLEND)) / 4;
	}


	if(m_imagebltflag == AUI_IMAGEBASE_BLTFLAG_CHROMAKEY) {

		std::unique_ptr<aui_Surface> frontSurface(aui_Factory::new_Surface(errcode, highlightRect.right,
			highlightRect.bottom));
		Assert(AUI_NEWOK(frontSurface, errcode));

		c3ui_Get()->TheBlitter()->Blt(frontSurface.get(), 0, 0, destSurf, destRect, k_AUI_BLITTER_FLAG_COPY);

		c3ui_Get()->TheBlitter()->Blt(frontSurface.get(), 0, 0, highlightSurf,
			&highlightRect, k_AUI_BLITTER_FLAG_CHROMAKEY);

		primitives_BlendSurfaces(frontSurface.get(), backSurface.get(), destSurf, destRect, highlightBlendVal);
	} else {

		primitives_BlendSurfaces(highlightSurf, backSurface.get(), destSurf, destRect, highlightBlendVal);
	}

	return AUI_ERRCODE_OK;
}




class c3_Animation : public c3_Blend {
public:

	c3_Animation(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock) :
		aui_ImageBase( ldlBlock, true ),
		aui_TextBase((const MBCHAR *)nullptr, (uint32)0),
		c3_Blend(retval, id, ldlBlock)
		{

			m_frames = nullptr;
			m_currentFrame = 0;
			m_animationSpeed = 100;
			lastIdle = GetTickCount();
			InitCommonLdl(ldlBlock);
		}

	c3_Animation(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height,
		const MBCHAR *text = nullptr, uint32 maxLength = 0 ) :
		aui_ImageBase( (sint32)0, AUI_IMAGEBASE_BLTTYPE_COPY, AUI_IMAGEBASE_BLTFLAG_COPY, true),
		aui_TextBase((const MBCHAR *)nullptr, (uint32)0),
		c3_Blend(retval, id, x, y, width, height, text, maxLength)
		{

			m_currentFrame = 0;
			m_animationSpeed = 100;
			lastIdle = GetTickCount();
		}


	AUI_ERRCODE Idle() override;

	void InitCommonLdl(MBCHAR *ldlBlock);

	void UpdateAnimation(sint32 deltaTime);

private:

	std::unique_ptr<aui_StringTable> m_frames;

	sint32 m_currentFrame;

	sint32 m_animationSpeed;

	uint32 lastIdle;
};


AUI_ERRCODE c3_Animation::Idle()
{

	int index = 0;

	sint32 deltaTime = GetTickCount() - lastIdle;

	if(deltaTime < m_animationSpeed) return AUI_ERRCODE_OK;


	sint32 animationTime = 0;
	do {

		animationTime++;

		deltaTime -= m_animationSpeed;
	} while((deltaTime - m_animationSpeed) > 0);

	lastIdle = GetTickCount() + (m_animationSpeed - deltaTime);

	UpdateAnimation(animationTime);

	return AUI_ERRCODE_OK;
}

void c3_Animation::InitCommonLdl(MBCHAR *ldlBlock)
{

	AUI_ERRCODE errcode;
	MBCHAR ldlString[k_AUI_LDL_MAXBLOCK + 1];

	aui_Ldl *theLdl = c3ui_Get()->GetLdl();

	BOOL valid = aui_Ldl::IsValid( ldlBlock );
	Assert(valid);
	if(!valid) return;

	ldl_datablock *datablock = aui_Ldl::GetLdl()->FindDataBlock( ldlBlock );
	Assert(datablock != nullptr);
	if(!datablock) return;

	m_animationSpeed			= datablock->GetInt(k_C3_ANIMATION_SPEED);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_C3_ANIMATION_FRAMES);
	m_frames.reset(new aui_StringTable(&errcode, ldlString));
	Assert(m_frames);

	m_currentFrame = 0;
	SetImage(m_frames->GetString(m_currentFrame));
}

void c3_Animation::UpdateAnimation(sint32 deltaTime)
{

	if(!m_frames) return;

	m_currentFrame += deltaTime;

	while(m_currentFrame >= m_frames->GetNumStrings())
		m_currentFrame -= m_frames->GetNumStrings();

	SetImage(m_frames->GetString(m_currentFrame));

	GetParentWindow()->ShouldDraw();
}

EndGameWindow::EndGameWindow(AUI_ERRCODE *retval, sint32 id, MBCHAR *ldlBlock, sint32 bpp,
							 AUI_WINDOW_TYPE type, bool bevel)
	:	C3Window(retval, id, ldlBlock, bpp, type, bevel)
{

	m_blendSpeed = 100;
	lastIdle = GetTickCount();

	InitCommonLdl(ldlBlock);
}

EndGameWindow::EndGameWindow(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y,
							 sint32 width, sint32 height, sint32 bpp, MBCHAR *pattern,
							 AUI_WINDOW_TYPE type, bool bevel)
	:	C3Window(retval, id, x, y, width, height, bpp, pattern, type, bevel)
{

	m_blendSpeed = 100;
	lastIdle = GetTickCount();
}

// Children release in the order the hand-written teardown used (button,
// border, feedback stack, embryo tank, background); every child is owned
// here even though the aui parent list keeps non-owning pointers.
EndGameWindow::~EndGameWindow()
{
	m_exitButton.reset();
	m_border.reset();

	m_stageLights.clear();
	m_chanceOfFailure.reset();
	m_splicerRatio.reset();
	m_containmentFieldRatio.reset();
	m_ecdRatio.reset();
	m_turnsRemaining.reset();
	m_turnProgress.reset();
	m_progressBackground.reset();
	m_labels.clear();
	m_darkenArea.reset();

	m_splicer.clear();

	m_ECD.clear();
	m_containmentField.clear();
	m_embryoStage.clear();
	m_embryoGlow.reset();
	m_embryoTank.reset();
	m_brokenTank.reset();
	m_backgroundAnim.clear();
	m_background.reset();
}

void EndGameWindow::SetStage(sint32 stage, sint32 lastStage)
{
	int index;
	for(index = 0; index < m_numberOfStages; index++) {
		if(index == stage) {
			m_embryoStage[index]->ShowThis();
			if(index == lastStage) m_embryoStage[index]->SetBlend(k_C3_BLEND_MAXBLEND);
		} else {
			m_embryoStage[index]->HideThis();
			if(index == lastStage) m_embryoStage[index]->SetBlend(0);
		}
	}

	for(index = 0; index < m_numberOfStageLights; index++) {
		if(index <= stage) {
			m_stageLights[index]->SetColor(k_ENDGAME_FB_STAGE_GREEN);
		} else {
			m_stageLights[index]->SetColor(k_ENDGAME_FB_STAGE_GREY);
		}
	}

	if(stage == m_numberOfStages) {

		m_background->SetImage(m_background->GetImage(2)->GetFilename());
	} else {

		m_background->SetImage(m_background->GetImage(1)->GetFilename());
	}
}

void EndGameWindow::SetEmbryoTank(bool exists, bool existed, bool isBroken, sint32 soundID)
{

	if(isBroken) {

		m_brokenTank->ShowThis();

		m_embryoTank->HideThis();
		m_embryoGlow->HideThis();

		m_embryoTank->SetBlend(0);
		m_embryoGlow->SetBlend(0);
	} else if(exists) {

		m_brokenTank->HideThis();

		m_embryoTank->ShowThis();
		m_embryoGlow->ShowThis();

		m_embryoTank->m_soundID = soundID;

		if(existed) m_embryoTank->SetBlend(k_C3_BLEND_MAXBLEND);
		if(existed) m_embryoGlow->SetBlend(k_C3_BLEND_MAXBLEND);
	} else {

		m_brokenTank->HideThis();

		m_embryoTank->HideThis();
		m_embryoGlow->HideThis();

		if(!existed) m_embryoTank->SetBlend(0);
		if(!existed) m_embryoGlow->SetBlend(0);
	}
}

void EndGameWindow::SetContainmentFields(sint32 number, sint32 alreadyDisplayed, sint32 soundID)
{

	for(int index = 0; index < m_numberOfContainmentFields; index++) {
		if(index < number) {
			m_containmentField[index]->ShowThis();
			m_containmentField[index]->m_soundID = soundID;
			if(index < alreadyDisplayed)
				m_containmentField[index]->SetBlend(k_C3_BLEND_MAXBLEND);
		} else {
			m_containmentField[index]->HideThis();
			if(index >= alreadyDisplayed)
				m_containmentField[index]->SetBlend(0);
		}
	}
}

void EndGameWindow::SetECDs(sint32 number, sint32 alreadyDisplayed, sint32 soundID)
{

	for(int index = 0; index < m_numberOfECDs; index++) {
		if(index < number) {
			m_ECD[index]->ShowThis();
			m_ECD[index]->m_soundID = soundID;
			if(index < alreadyDisplayed)
				m_ECD[index]->SetBlend(k_C3_BLEND_MAXBLEND);
		} else {
			m_ECD[index]->HideThis();
			if(index >= alreadyDisplayed)
				m_ECD[index]->SetBlend(0);
		}
	}
}

void EndGameWindow::SetSplicers(sint32 number, sint32 alreadyDisplayed, sint32 soundID)
{

	for(int index = 0; index < m_numberOfSplicers; index++) {
		if(index < number) {
			m_splicer[index]->ShowThis();
			m_splicer[index]->m_soundID = soundID;

			if(index < alreadyDisplayed) {
				m_splicer[index]->SetBlend(k_C3_BLEND_MAXBLEND);

			}
		} else {
			m_splicer[index]->HideThis();

			if(index >= alreadyDisplayed) {
				m_splicer[index]->SetBlend(0);

			}
		}
	}
}

void EndGameWindow::Update(EndGame *endGame)
{

	static char requirementName[256];

	static char textBuffer[256];

	for(int i = 0; i < endgamedb_Get()->m_nRec; i++) {

		const EndGameRecord *egrec = endgamedb_Get()->Get(i);

		const char *constName = stringdb_Get()->GetIdStr(egrec->GetName());

		sint32 soundID = egrec->GetSoundID();

		if(!constName) continue;

		strlcpy(requirementName, constName, sizeof(requirementName));


		if(!strcmp(m_embryoTankName, requirementName) && endGame->GetNumberBuilt(i))
			SetEmbryoTank(true, (endGame->GetNumberShown(i) > 0),
			(endGame->GetStage() == m_numberOfStages), soundID);
		if(!strcmp(m_containmentFieldName, requirementName)) {
			SetContainmentFields(endGame->GetNumberBuilt(i), endGame->GetNumberShown(i), soundID);

			sint32 numberRequired = ((endGame->GetStage() < 0) ||
				(endGame->GetStage() >= endgamedb_Get()->GetNumStages())) ? 0 :
				endgamedb_Get()->Get(i)->RequiredToAdvanceFromStage(endGame->GetStage());
			snprintf(textBuffer, sizeof(textBuffer), "%d/%d/%d", endGame->GetNumberBuilt(i),
				numberRequired, endgamedb_Get()->Get(i)->GetMaxAllowed());
			m_containmentFieldRatio->SetText(textBuffer);
		}
		if(!strcmp(m_ECDName, requirementName)) {
			SetECDs(endGame->GetNumberBuilt(i), endGame->GetNumberShown(i), soundID);

			sint32 numberRequired = ((endGame->GetStage() < 0) ||
				(endGame->GetStage() >= endgamedb_Get()->GetNumStages())) ? 0 :
				endgamedb_Get()->Get(i)->RequiredToAdvanceFromStage(endGame->GetStage());
			snprintf(textBuffer, sizeof(textBuffer), "%d/%d/%d", endGame->GetNumberBuilt(i),
				numberRequired, endgamedb_Get()->Get(i)->GetMaxAllowed());
			m_ecdRatio->SetText(textBuffer);
		}
		if(!strcmp(m_splicerName, requirementName)) {
			SetSplicers(endGame->GetNumberBuilt(i), endGame->GetNumberShown(i), soundID);

			snprintf(textBuffer, sizeof(textBuffer), "%d/-/%d", endGame->GetNumberBuilt(i),
				endgamedb_Get()->Get(i)->GetMaxAllowed());
			m_splicerRatio->SetText(textBuffer);
		}
	}

	SetStage(endGame->GetStage(), endGame->GetDisplayedStage());

	sint32 turnsForNextStage = 0;
	if((endGame->GetStage() >= 0) && (endGame->GetStage() < endgamedb_Get()->GetNumStages()))
		turnsForNextStage = endGame->GetTurnsForNextStage();
	sint32 turnsSinceStageBegan = 0;
	if(endGame->GetStage() >= 0) turnsSinceStageBegan = endGame->GetTurnsSinceStageBegan(turn_Get()->GetRound());
	if(turnsSinceStageBegan > turnsForNextStage) turnsSinceStageBegan = turnsForNextStage;

	m_turnProgress->SetMaxValue(turnsForNextStage);
	if(turnsForNextStage) m_turnProgress->SetCurValue(turnsSinceStageBegan);
	else m_turnProgress->SetCurValue(0);

	m_turnProgress->SetBarColor(k_ENDGAME_FB_TURN_RED);
	if((endGame->GetStage() >= 0) && (endGame->MetRequirementsForNextStage()))
		m_turnProgress->SetBarColor(k_ENDGAME_FB_TURN_GREEN);

	if(turnsForNextStage) snprintf(textBuffer, sizeof(textBuffer), "%d", turnsForNextStage - turnsSinceStageBegan + 1);
	else snprintf(textBuffer, sizeof(textBuffer), "");
	m_turnsRemaining->SetText(textBuffer);

	snprintf(textBuffer, sizeof(textBuffer), "%d%%", endGame->GetCataclysmChance());
	m_chanceOfFailure->SetText(textBuffer);

	endGame->UpdateDisplayState();
}

void EndGameWindow::UpdateTurn(EndGame *endGame)
{

	static char textBuffer[256];

	sint32 turnsForNextStage = 0;
	if((endGame->GetStage() >= 0) && (endGame->GetStage() < endgamedb_Get()->GetNumStages()))
		turnsForNextStage = endGame->GetTurnsForNextStage();
	sint32 turnsSinceStageBegan = 0;
	if(endGame->GetStage() >= 0) turnsSinceStageBegan = endGame->GetTurnsSinceStageBegan(turn_Get()->GetRound());
	if(turnsSinceStageBegan > turnsForNextStage) turnsSinceStageBegan = turnsForNextStage;

	m_turnProgress->SetMaxValue(turnsForNextStage);
	if(turnsForNextStage) m_turnProgress->SetCurValue(turnsSinceStageBegan);
	else m_turnProgress->SetCurValue(0);

	m_turnProgress->SetBarColor(k_ENDGAME_FB_TURN_RED);
	if((endGame->GetStage() >= 0) && (endGame->MetRequirementsForNextStage()))
		m_turnProgress->SetBarColor(k_ENDGAME_FB_TURN_GREEN);

	if(turnsForNextStage) snprintf(textBuffer, sizeof(textBuffer), "%d", turnsForNextStage - turnsSinceStageBegan + 1);
	else snprintf(textBuffer, sizeof(textBuffer), "");
	m_turnsRemaining->SetText(textBuffer);

	ShouldDraw(TRUE);
}


AUI_ERRCODE EndGameWindow::Idle()
{

	int index = 0;

	sint32 deltaTime = GetTickCount() - lastIdle;


	if(deltaTime > k_ENDGAME_TOO_MUCH_TIME) {
		lastIdle = GetTickCount();
		deltaTime = m_blendSpeed;
	}

	if(deltaTime < m_blendSpeed) return AUI_ERRCODE_OK;


	sint32 blendTime = 0;
	do {

		blendTime++;

		deltaTime -= m_blendSpeed;
	} while((deltaTime - m_blendSpeed) > 0);

	lastIdle = GetTickCount() + (m_blendSpeed - deltaTime);

	UpdateBlend(blendTime, m_embryoTank.get());
	UpdateBlend(blendTime, m_embryoGlow.get());
	for(index = m_numberOfStages-1; index >= 0; index--) UpdateBlend(blendTime, m_embryoStage[index].get());
	for(index = m_numberOfSplicers-1; index >= 0; index--) UpdateBlend(blendTime, m_splicer[index].get());

	for(index = m_numberOfECDs-1; index >= 0; index--) UpdateBlend(blendTime, m_ECD[index].get());
	for(index = m_numberOfContainmentFields-1; index >= 0; index--)
		UpdateBlend(blendTime, m_containmentField[index].get());

	return AUI_ERRCODE_OK;
}

void EndGameWindow::InitCommonLdl(MBCHAR *ldlBlock)
{

	AUI_ERRCODE errcode;
	MBCHAR ldlString[k_AUI_LDL_MAXBLOCK + 1];
	int index;

	aui_Ldl *theLdl = c3ui_Get()->GetLdl();

	BOOL valid = aui_Ldl::IsValid( ldlBlock );
	Assert(valid);
	if(!valid) return;

	ldl_datablock *datablock = aui_Ldl::GetLdl()->FindDataBlock( ldlBlock );
	Assert(datablock != nullptr);
	if(!datablock) return;

	m_blendSpeed			= datablock->GetInt(k_LDL_ENDGAME_BLEND_SPEED);

	m_embryoTankName		= datablock->GetString(k_LDL_ENDGAME_TANK_NAME);
	m_containmentFieldName	= datablock->GetString(k_LDL_ENDGAME_CONTAIN_NAME);
	m_ECDName				= datablock->GetString(k_LDL_ENDGAME_ECD_NAME);
	m_splicerName			= datablock->GetString(k_LDL_ENDGAME_SPLICER_NAME);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_BACKGROUND);
	m_background.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_background);

	m_numberOfBackgroundAnims = datablock->GetInt(k_LDL_ENDGAME_BACKANIM_COUNT);
	m_backgroundAnim.resize(m_numberOfBackgroundAnims > 0 ? m_numberOfBackgroundAnims : 0);

	for(index = 0; index < m_numberOfBackgroundAnims; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_BACKANIM_BASE, index+1);
		m_backgroundAnim[index].reset(new c3_Animation(&errcode, aui_UniqueId(), ldlString));
		Assert(m_backgroundAnim[index]);
		m_backgroundAnim[index]->SetBlend(k_C3_BLEND_MAXBLEND);
	}

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_EMBRYO_TANK);
	m_embryoTank.reset(new c3_Blend(&errcode, aui_UniqueId(), ldlString));
	Assert(m_embryoTank);

	m_embryoTank->HideThis();

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_BROKEN_TANK);
	m_brokenTank.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_brokenTank);

	m_brokenTank->HideThis();

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_EMBRYO_GLOW);
	m_embryoGlow.reset(new c3_Animation(&errcode, aui_UniqueId(), ldlString));
	Assert(m_embryoGlow);

	m_embryoGlow->HideThis();

	m_numberOfStages = datablock->GetInt(k_LDL_ENDGAME_STAGE_COUNT);
	m_embryoStage.resize(m_numberOfStages > 0 ? m_numberOfStages : 0);

	for(index = 0; index < m_numberOfStages; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_STAGE_BASE, index+1);
		m_embryoStage[index].reset(new c3_Blend(&errcode, aui_UniqueId(), ldlString));
		Assert(m_embryoStage[index]);
		m_embryoStage[index]->HideThis();
	}

	m_numberOfContainmentFields = datablock->GetInt(k_LDL_ENDGAME_CONTAIN_COUNT);
	m_containmentField.resize(m_numberOfContainmentFields > 0 ? m_numberOfContainmentFields : 0);

	for(index = 0; index < m_numberOfContainmentFields; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_CONTAIN_BASE, index+1);
		m_containmentField[index].reset(new c3_Blend(&errcode, aui_UniqueId(), ldlString));
		Assert(m_containmentField[index]);
		m_containmentField[index]->HideThis();
	}

	m_numberOfECDs = datablock->GetInt(k_LDL_ENDGAME_ECD_COUNT);
	m_ECD.resize(m_numberOfECDs > 0 ? m_numberOfECDs : 0);

	for(index = 0; index < m_numberOfECDs; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_ECD_BASE, index+1);
		m_ECD[index].reset(new c3_Blend(&errcode, aui_UniqueId(), ldlString));
		Assert(m_ECD[index]);
		m_ECD[index]->HideThis();
	}

	m_numberOfSplicers = datablock->GetInt(k_LDL_ENDGAME_SPLICER_COUNT);
	m_splicer.resize(m_numberOfSplicers > 0 ? m_numberOfSplicers : 0);


	for(index = 0; index < m_numberOfSplicers; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_SPLICER_BASE, index+1);
		m_splicer[index].reset(new c3_Blend(&errcode, aui_UniqueId(), ldlString));
		Assert(m_splicer[index]);
		m_splicer[index]->HideThis();
	}

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_DARKEN_AREA);
	m_darkenArea.reset(new c3_DarkenArea(&errcode, aui_UniqueId(), ldlString));
	Assert(m_darkenArea);

	m_numberOfLabels = datablock->GetInt(k_LDL_ENDGAME_FB_LABEL_COUNT);
	m_labels.resize(m_numberOfLabels > 0 ? m_numberOfLabels : 0);

	for(index = 0; index < m_numberOfLabels; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_FB_LABEL_BASE, index+1);
		m_labels[index].reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
		Assert(m_labels[index]);
	}

	m_numberOfStageLights = datablock->GetInt(k_LDL_ENDGAME_FB_LIGHT_COUNT);
	m_stageLights.resize(m_numberOfStageLights > 0 ? m_numberOfStageLights : 0);

	for(index = 0; index < m_numberOfStageLights; index++) {
		snprintf(ldlString, sizeof(ldlString), "%s.%s%d", ldlBlock, k_LDL_ENDGAME_FB_LIGHT_BASE, index+1);
		m_stageLights[index].reset(new c3_ColoredStatic(&errcode, aui_UniqueId(), ldlString));
		Assert(m_stageLights[index]);
	}

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_PROGRESS_BG);
	m_progressBackground.reset(new c3_ColoredStatic(&errcode, aui_UniqueId(), ldlString));
	Assert(m_progressBackground);
	m_progressBackground->SetColor(k_ENDGAME_FB_STAGE_GREY);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_TURN_PROGRESS);
	m_turnProgress.reset(new c3_YetAnotherProgressBar(&errcode, aui_UniqueId(), ldlString));
	Assert(m_turnProgress);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_TURNS_REMAINING);
	m_turnsRemaining.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_turnsRemaining);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_ECD_RATIO);
	m_ecdRatio.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_ecdRatio);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_CONTAIN_RATIO);
	m_containmentFieldRatio.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_containmentFieldRatio);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_SPLICER_RATIO);
	m_splicerRatio.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_splicerRatio);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_FB_FAILURE_CHANCE);
	m_chanceOfFailure.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_chanceOfFailure);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_BORDER);
	m_border.reset(new aui_Static(&errcode, aui_UniqueId(), ldlString));
	Assert(m_border);

	snprintf(ldlString, sizeof(ldlString), "%s.%s", ldlBlock, k_LDL_ENDGAME_EXIT_BUTTON);
	m_exitButton.reset(new c3_Button(&errcode, aui_UniqueId(), ldlString, endgamewindow_ExitButtonActionCallback));
	Assert(m_exitButton);

	AddControl(m_exitButton.get());
	AddControl(m_border.get());
	AddControl(m_chanceOfFailure.get());
	AddControl(m_splicerRatio.get());
	AddControl(m_containmentFieldRatio.get());
	AddControl(m_ecdRatio.get());
	AddControl(m_turnsRemaining.get());
	AddControl(m_turnProgress.get());
	AddControl(m_progressBackground.get());
	for(index = m_numberOfStageLights-1; index >= 0; index--)
		AddControl(m_stageLights[index].get());
	for(index = m_numberOfLabels-1; index >= 0; index--)
		AddControl(m_labels[index].get());
	AddControl(m_darkenArea.get());

	for(index = m_numberOfSplicers-1; index >= 0; index--)
		AddControl(m_splicer[index].get());
	for(index = m_numberOfECDs-1; index >= 0; index--)
		AddControl(m_ECD[index].get());
	for(index = m_numberOfContainmentFields-1; index >= 0; index--)
		AddControl(m_containmentField[index].get());
	for(index = m_numberOfStages-1; index >= 0; index--)
		AddControl(m_embryoStage[index].get());
	AddControl(m_embryoGlow.get());
	AddControl(m_embryoTank.get());
	AddControl(m_brokenTank.get());
	for(index = m_numberOfBackgroundAnims-1; index >= 0; index--)
		AddControl(m_backgroundAnim[index].get());
	AddControl(m_background.get());
}

void EndGameWindow::UpdateBlend(sint32 deltaTime, c3_Blend *blendControl)
{

	if(!blendControl) return;

	if(blendControl->IsHidden()) {

		if(blendControl->GetBlend() <= 0) return;

		sint32 newBlendVal = blendControl->GetBlend() - deltaTime;

		if(newBlendVal <= 0) blendControl->SetBlend(0);
		else blendControl->SetBlend(newBlendVal);

		ShouldDraw(TRUE);
	} else {

		if(blendControl->GetBlend() >= k_C3_BLEND_MAXBLEND) return;


		if((blendControl->GetBlend() == 0) && (blendControl->m_soundID != -1)) {
			if(soundmgr_Get()) soundmgr_Get()->AddSound(SOUNDTYPE_SFX, (uint32)0, blendControl->m_soundID);
		}

		sint32 newBlendVal = blendControl->GetBlend() + deltaTime;

		if(newBlendVal >= k_C3_BLEND_MAXBLEND) blendControl->SetBlend(k_C3_BLEND_MAXBLEND);
		else blendControl->SetBlend(newBlendVal);

		ShouldDraw(TRUE);
	}
}
