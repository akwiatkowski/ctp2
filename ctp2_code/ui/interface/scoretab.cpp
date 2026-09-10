#include "ctp/c3.h"
#include "ui/interface/scoretab.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "ui/aui_ctp2/ctp2_listitem.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "gs/gameobj/Player.h"
#include "gs/database/profileDB.h"
#include "gs/gameobj/Score.h"
#include "ui/aui_ctp2/SelItem.h"

#include "gs/gameobj/GameSettings.h"

extern void cpw_NumberToCommas( uint64 number, MBCHAR *s, size_t size );






ScoreTab::ScoreTab()
:
	m_difficulty        (static_cast<ctp2_Static *>
                            (aui_Ldl::GetObject
                                ("InfoDialog",
                                 "TabGroup.Tab1.TabPanel.Difficulty.Value"
                                )
                            )
                        ),
	m_rank              (static_cast<ctp2_Static *>
                            (aui_Ldl::GetObject
                                ("InfoDialog",
                                 "TabGroup.Tab1.TabPanel.Rank.Value"
                                )
                             )
                        ),
	m_total             (static_cast<ctp2_Static *>
                            (aui_Ldl::GetObject
                                ("InfoDialog",
                                 "TabGroup.Tab1.TabPanel.Total.Value"
                                )
                            )
                        ),
	m_scoreList         (static_cast<ctp2_ListBox *>
                            (aui_Ldl::GetObject
                                ("InfoDialog",
                                 "TabGroup.Tab1.TabPanel.List"
                                )
                            )
                        ),
    m_difficultyStrings (nullptr)

{
    AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	m_difficultyStrings = std::make_unique<aui_StringTable>(&errcode, "strings.difficulty1strings");

    m_scoreList->Clear();
	for (auto & i : m_scoreElem)
	{
		i = (ctp2_ListItem *)aui_Ldl::BuildHierarchyFromRoot("ScoreElement");
	}
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_TYPE_OF_VICTORY]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_OPPONENTS_CONQUERED]);
	m_scoreList->AddItem((ctp2_ListItem *)aui_Ldl::BuildHierarchyFromRoot("ScoreElement"));
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_POPULATION]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_RANK]);
	m_scoreList->AddItem((ctp2_ListItem *)aui_Ldl::BuildHierarchyFromRoot("ScoreElement"));
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_CITIES0TO30]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_CITIES30TO100]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_CITIES100TO500]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_CITIES500PLUS]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_CITIES_RECAPTURED]);
	m_scoreList->AddItem((ctp2_ListItem *)aui_Ldl::BuildHierarchyFromRoot("ScoreElement"));
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_FEATS]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_WONDERS]);
	m_scoreList->AddItem(m_scoreElem[SCORE_CAT_ADVANCES]);
}

ScoreTab::~ScoreTab()
{
}

void ScoreTab::Update()
{
	sint32      curPlayer   = selitem_Get()->GetVisiblePlayer();
	Player *    pl          = player_Get(curPlayer);
	if (!pl)
    {
		pl = Player::GetDeadPlayer(curPlayer);
	}
	if (!pl)
		return;

	Score *     score       = pl->m_score.get();
	MBCHAR      commaNumber[80];

	m_difficulty->SetText(m_difficultyStrings->GetString(gamesettings_Get()->GetDifficulty()));

	for (int i = 0; i < SCORE_CAT_MAX; i++)
	{
		SCORE_CATEGORY cat = (SCORE_CATEGORY )i;
		((ctp2_Static *)m_scoreElem[i]->GetChildByIndex(1))->SetText(score->GetScoreString(cat));

		((ctp2_Static *)m_scoreElem[i]->GetChildByIndex(2))->SetText(score->GetPartialScoreItemized(cat));

		cpw_NumberToCommas(score->GetPartialScore(cat), commaNumber, sizeof(commaNumber));
		((ctp2_Static *)m_scoreElem[i]->GetChildByIndex(3))->SetText(commaNumber);
	}
	cpw_NumberToCommas(score->GetTotalScore(), commaNumber, sizeof(commaNumber));
	m_total->SetText(commaNumber);

	sint32 rank = 1;
	for (int p = 1; p < k_MAX_PLAYERS; p++)
    {
		if ((p != curPlayer) && player_Get(p) &&
            (player_Get(p)->m_score->GetTotalScore() > score->GetTotalScore())
           )
        {
			rank++;
		}
	}

	char buf[40];
	snprintf(buf, sizeof(buf), "%d", rank);
	m_rank->SetText(buf);
}
