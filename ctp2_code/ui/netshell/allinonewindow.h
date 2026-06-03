#ifndef __ALLINONEWINDOW_H__
#define __ALLINONEWINDOW_H__

#include "ui/netshell/ns_aiplayersetup.h"
#include "ui/netshell/ns_window.h"
#include "ui/aui_common/aui_action.h"
#include "ui/netshell/ns_string.h"
#include "ui/netshell/ns_gamesetup.h"
#include "ui/netshell/ns_units.h"
#include "ui/netshell/ns_improvements.h"
#include "ui/netshell/ns_wonders.h"
#include "gs/fileio/gamefile.h"

class aui_Switch;
class aui_StringTable;
class ns_HPlayerItem;

class DialogBoxWindow;

class AllinoneWindow;
// g_allinoneWindow demoted to file-scope `static` in allinonewindow.cpp.
// External consumers go through allinonewindow_Get().  Returns NULL
// when no allinone window has been instantiated.
AllinoneWindow * allinonewindow_Get();

#define k_PPT_PUBLIC	0
#define k_PPT_PRIVATE	1


#include "gs/fileio/StartingPosition.h"

struct ns_ScenarioInfo {
	uint8 isScenario;
	MBCHAR m_fileName[_MAX_PATH];
	MBCHAR m_gameName[_MAX_PATH];
	uint8 m_civs[k_MAX_START_POINTS];
	uint8 m_numStartPositions;
	uint8 m_startInfoType;
	uint8 m_haveSavedGame;
	sint32 m_legalCivs[k_MAX_PLAYERS];
	MBCHAR m_scenarioName[_MAX_PATH];

};

class AllinoneWindow : public ns_Window
{
public:

	AllinoneWindow( AUI_ERRCODE *retval );
	~AllinoneWindow() override;

protected:
	AllinoneWindow() : ns_Window() {}
	AUI_ERRCODE	InitCommon( ) override;
	AUI_ERRCODE CreateControls( );

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	sint32 CurNumHumanPlayers( );
	sint32 CurNumAiPlayers( );
	sint32 CurNumPlayers( );

	MBCHAR m_lname[ 101 ];


	tech_WLList<nf_AIPlayer *> *m_aiplayerList;

	bool m_receivedGuids;
	CivGuid m_civGuids[k_MAX_PLAYERS];




	BOOL WhoHasTribe( sint32 index, uint16 *curKey, BOOL *curIsAI, BOOL *curIsFemale );




	sint32 FindTribe( uint16 key, BOOL isAI, BOOL *isFemale = nullptr );


	BOOL AssignTribe(
		sint32 index,
		uint16 key,
		BOOL isAI,
		BOOL isFemale,
		BOOL unassign );





	void RequestTribe( sint32 index );

	BOOL IsMine( NETFunc::Player *player );
	ns_HPlayerItem *GetHPlayerFromId( dpid_t id );
	NETFunc::Player *GetPlayerFromKey( uint16 key );
	nf_AIPlayer *GetAIPlayerFromKey( uint16 key );

	enum Mode {
		CREATE,
		JOIN,
		CONTINUE_CREATE,
		CONTINUE_JOIN
	};
	void SetMode(Mode m);
	Mode GetMode() { return m_mode; }

	BOOL IsScenarioGame( ) const { return m_isScenarioGame; }
	void SetScenarioGame( BOOL s ) { m_isScenarioGame = s; }
	void SetupNewScenario();

	void	Update( );

	void	UpdateGameSetup( bool b = false );

	void	ReallyUpdateGameSetup( );

	void	UpdatePlayerSetup( );
	void	UpdateAIPlayerSetup( nf_AIPlayer *aiplayer );
	void	DeleteAIPlayer( nf_AIPlayer *aiplayer );
	void	ReallyUpdateAIPlayerSetup( );

	void	AddAIPlayer( sint32 curCount = 0 );

	sint32	OKToAddPlayers( );

	BOOL    LoadGUIDs(SaveInfo *info);
	BOOL    SetScenarioInfo(SaveInfo *info);

	ns_ScenarioInfo *GetScenarioInfo() { return &m_scenInfo; }

	AUI_ERRCODE Idle( ) override;
	AUI_ERRCODE SetParent( aui_Region *region ) override;

	void EnableButtonsForUnlaunch();


	enum CONTROL
	{
		CONTROL_FIRST = 0,

		CONTROL_GAMENAMESTATICTEXT = CONTROL_FIRST,
		CONTROL_GAMENAMETEXTFIELD,
		CONTROL_PLAYSTYLESTATICTEXT,
		CONTROL_PLAYSTYLEDROPDOWN,
		CONTROL_PLAYSTYLEVALUESTATICTEXT,
		CONTROL_PLAYSTYLEVALUESPINNER,

		CONTROL_CHATBOX,
		CONTROL_LOCKSWITCH,
		CONTROL_INFOBUTTON,
		CONTROL_KICKBUTTON,
		CONTROL_PPTSWITCH,
		CONTROL_REVIEWBUTTON,








		CONTROL_PLAYERSTITLESTATICTEXT,
		CONTROL_PLAYERSSHEET,

		CONTROL_HPLAYERSLISTBOX,
		CONTROL_GPLAYERSLISTBOX,
		CONTROL_AIPLAYERSLISTBOX,

		CONTROL_ADDAIBUTTON,

		CONTROL_RULESBUTTON,
		CONTROL_EXCLUSIONSBUTTON,





		CONTROL_AGESBUTTON,
		CONTROL_MAPSIZEBUTTON,
		CONTROL_WORLDTYPEBUTTON,
		CONTROL_WORLDSHAPEBUTTON,
		CONTROL_DIFFICULTYBUTTON,




		CONTROL_RULESTITLESTATICTEXT,
		CONTROL_RULESTITLESTATICTEXT2,
		CONTROL_RULESSHEET,

		CONTROL_BLOODLUSTSWITCH,


		CONTROL_POLLUTIONSWITCH,

		CONTROL_DYNAMICJOINSWITCH,

		CONTROL_HANDICAPPINGSWITCH,
		CONTROL_GOLDSTATICTEXT,
		CONTROL_PWSTATICTEXT,


		CONTROL_CIVPOINTSBUTTON,
		CONTROL_PWPOINTSBUTTON,

		CONTROL_RULESOKBUTTON,

		CONTROL_EXCLUSIONSTITLESTATICTEXT,
		CONTROL_EXCLUSIONSSHEET,

		CONTROL_SMALLNASTYTABGROUP,
		CONTROL_UNITSTAB,
		CONTROL_UNITSLISTBOX,
		CONTROL_IMPROVEMENTSTAB,
		CONTROL_IMPROVEMENTSLISTBOX,
		CONTROL_WONDERSTAB,
		CONTROL_WONDERSLISTBOX,

		CONTROL_EXCLUSIONSOKBUTTON,








		CONTROL_OKBUTTON,
		CONTROL_CANCELBUTTON,
		CONTROL_LAST,
		CONTROL_MAX = CONTROL_LAST - CONTROL_FIRST
	};

	AUI_ERRCODE CreateExclusions( );

	void UpdatePlayerButtons( );
	void SpitOutGameSetup( );

	void	UpdateTribeSwitches( );


protected:
	void	UpdateConfig( );
	void	UpdateDisplay( );

	bool m_createdExclusions;

	Mode m_mode;
	BOOL m_isScenarioGame;







	uint32 m_tickGame;
	uint32 m_tickPlayer;
	uint32 m_tickAIPlayer;

	ns_String	*m_messageRequestDenied;
	ns_String	*m_messageKicked;
	ns_String	*m_messageGameSetup;
	ns_String	*m_messageGameEnter;
	ns_String	*m_messageGameHost;
	ns_String	*m_messageGameCreate;
	ns_String	*m_messageLaunched;

	aui_StringTable *m_playStyleValueStrings;
	aui_StringTable *m_PPTStrings;

	bool m_shouldUpdateGame;
	bool m_shouldUpdatePlayer;
	bool m_shouldUpdateAIPlayer;
	sint32 m_shouldAddAIPlayer;

	bool m_joinedGame;

	aui_Action *m_dbActionArray[ 1 ];

	sint32 m_numAvailUnits;
	aui_Switch *m_units[ k_UNITS_MAX ];
	sint32 m_numAvailImprovements;
	aui_Switch *m_improvements[ k_IMPROVEMENTS_MAX ];
	sint32 m_numAvailWonders;
	aui_Switch *m_wonders[ k_WONDERS_MAX ];
	friend nf_GameSetup;

	ns_ScenarioInfo m_scenInfo;

    AUI_ACTION_BASIC(GameNameTextFieldAction);
    AUI_ACTION_BASIC(PPTSwitchAction);
	AUI_ACTION_BASIC(KickButtonAction);
    AUI_ACTION_BASIC(InfoButtonAction);
    AUI_ACTION_BASIC(OKButtonAction);
	AUI_ACTION_BASIC(CancelButtonAction);
    AUI_ACTION_BASIC(ReviewButtonAction);
	AUI_ACTION_BASIC(LockSwitchAction);
    AUI_ACTION_BASIC(AddAIButtonAction);
    AUI_ACTION_BASIC(DialogBoxPopDownAction);
    AUI_ACTION_BASIC(PlayersListBoxAction);
	AUI_ACTION_BASIC(PlayStyleDropDownAction);
	AUI_ACTION_BASIC(RulesButtonAction);
    AUI_ACTION_BASIC(ExclusionsButtonAction);
	AUI_ACTION_BASIC(RulesOKButtonAction);
    AUI_ACTION_BASIC(ExclusionsOKButtonAction);
    AUI_ACTION_BASIC(DynamicJoinSwitchAction);
    AUI_ACTION_BASIC(HandicappingSwitchAction);
	AUI_ACTION_BASIC(BloodlustSwitchAction);
    AUI_ACTION_BASIC(PollutionSwitchAction);
    AUI_ACTION_BASIC(CivPointsButtonAction);
    AUI_ACTION_BASIC(PwPointsButtonAction);
	AUI_ACTION_BASIC(AgesButtonAction);
    AUI_ACTION_BASIC(MapSizeButtonAction);
    AUI_ACTION_BASIC(WorldTypeButtonAction);
    AUI_ACTION_BASIC(WorldShapeButtonAction);
    AUI_ACTION_BASIC(DifficultyButtonAction);

	friend class CivPointsButtonAction;
	friend class HandicappingSwitchAction;
	friend class PlayStyleDropDownAction;
	friend class PPTSwitchAction;
	friend class PwPointsButtonAction;




	static void PlayStyleValueSpinnerCallback(aui_Control *control, uint32 action, uint32 data, void *cookie);




	class UnitExclusionAction : public aui_Action
	{
	public:
		UnitExclusionAction(sint32 index)
        :   aui_Action  (),
            m_index     (index)
        { ; };

	    void	Execute
	    (
		    aui_Control	*	control,
		    uint32			action,
		    uint32			data
	    ) override;

	private:
		sint32 m_index;
	};

	class ImprovementExclusionAction : public aui_Action
	{
	public:
		ImprovementExclusionAction(sint32 index)
        :   aui_Action  (),
            m_index     (index)
        { ; };

	    void	Execute
	    (
		    aui_Control	*	control,
		    uint32			action,
		    uint32			data
	    ) override;

	private:
		sint32 m_index;
	};

	class WonderExclusionAction : public aui_Action
	{
	public:
		WonderExclusionAction(sint32 index)
        :   aui_Action  (),
            m_index     (index)
        { ; };

	    void	Execute
	    (
		    aui_Control	*	control,
		    uint32			action,
		    uint32			data
	    ) override;

	private:
		sint32 m_index;
	};
};

void AllinoneAgesCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );
void AllinoneMapSizeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );
void AllinoneWorldShapeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );
void AllinoneTribeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );










void AllinoneDifficultyCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );
void AllinoneWorldTypeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie );

#endif
