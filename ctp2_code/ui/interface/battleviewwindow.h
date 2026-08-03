#ifndef __BATTLEVIEWWINDOW_H__
#define __BATTLEVIEWWINDOW_H__

#include <memory>

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_common/aui_action.h"

class Battle;
class BattleView;
class ctp2_Static;
class ctp2_Button;
class c3_Icon;
class BattleViewActor;
class Sequence;

class BattleViewWindow : public C3Window {
public:
	static void Initialize(std::weak_ptr<Sequence> seq);
	static void Cleanup();

	BattleViewWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD );

	~BattleViewWindow() override;

	void SetupBattle(Battle *battle);
	void UpdateBattle(Battle *battle);
	void EndBattle();

	void RemoveActor(BattleViewActor *actor);

	void Refresh();

	void GetAttackerPos(sint32 column, sint32 row, sint32 *x, sint32 *y);
	void GetDefenderPos(sint32 column, sint32 row, sint32 *x, sint32 *y);

	void SetSequence(std::weak_ptr<Sequence> seq) { m_sequence = seq; }
	std::weak_ptr<Sequence> GetSequence() { return m_sequence; }


	const BattleView *GetBattleView() const { return m_battleView.get(); }

protected:
	AUI_ERRCODE InitCommonLdl(MBCHAR *ldlBlock);

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
	AUI_ERRCODE Idle() override;

private:

	std::unique_ptr<BattleView>	m_battleView;

	RECT					m_battleViewRect;


	// The window owns every control it builds from LDL.
	std::unique_ptr<ctp2_Static>	m_topBorder;
	std::unique_ptr<ctp2_Static>	m_leftBorder;
	std::unique_ptr<ctp2_Static>	m_rightBorder;
	std::unique_ptr<ctp2_Static>	m_bottomBorder;

	std::unique_ptr<ctp2_Button>	m_exitButton;
	std::unique_ptr<ctp2_Button>	m_retreatButton;
	std::unique_ptr<ctp2_Static>	m_titleText;

	std::unique_ptr<ctp2_Static>	m_attackersText;
	std::unique_ptr<ctp2_Static>	m_attackersName;
	std::unique_ptr<c3_Icon>	m_attackersFlag;

	std::unique_ptr<ctp2_Static>	m_defendersText;
	std::unique_ptr<ctp2_Static>	m_defendersName;
	std::unique_ptr<c3_Icon>	m_defendersFlag;

	std::unique_ptr<ctp2_Static>	m_terrainBonusText;
	std::unique_ptr<ctp2_Static>	m_terrainBonusValue;

	std::unique_ptr<ctp2_Static>	m_cityBonusText;
	std::unique_ptr<ctp2_Static>	m_cityBonusValue;

	std::unique_ptr<ctp2_Static>	m_citylandattackBonusText;
	std::unique_ptr<ctp2_Static>	m_citylandattackBonusValue;
	std::unique_ptr<ctp2_Static>	m_cityairattackBonusText;
	std::unique_ptr<ctp2_Static>	m_cityairattackBonusValue;
	std::unique_ptr<ctp2_Static>	m_cityseaattackBonusText;
	std::unique_ptr<ctp2_Static>	m_cityseaattackBonusValue;

	std::unique_ptr<ctp2_Static>	m_cityName;

	std::unique_ptr<ctp2_Static>	m_fortBonusText;
	std::unique_ptr<ctp2_Static>	m_fortBonusValue;
	std::unique_ptr<ctp2_Static>	m_fortBonusImage;

	std::unique_ptr<ctp2_Static>	m_fortifiedBonusText;
	std::unique_ptr<ctp2_Static>	m_fortifiedBonusValue;

	std::weak_ptr<Sequence>  m_sequence;
};


class RemoveBattleViewAction : public aui_Action
{
public:
	RemoveBattleViewAction(bool kill)
    :   aui_Action      (),
        m_killBattle    (kill)
    { ; };

	void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	) override;

private:
	bool m_killBattle;
};

// g_battleViewWindow demoted to file-scope `static` in battleviewwindow.cpp.
// External callers go through battleviewwindow_Get() (returns NULL when
// no battle is in progress).
BattleViewWindow * battleviewwindow_Get();

#endif
