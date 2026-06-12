//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Scenario selection window
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
// - Removed refferences to the civilisation database. (Aug 21st 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_imagebase.h"
#include "ui/aui_common/aui_textbase.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "ui/aui_ctp2/ctp2_listitem.h"
#include "ui/aui_ctp2/ctp2_dropdown.h"
#include "ui/aui_ctp2/ctp2_textfield.h"

#include "gs/database/StrDB.h"

#include "gs/fileio/civscenarios.h"

#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/scenariowindow.h"

#include "gs/database/profileDB.h"

#include "ui/interface/loadsavewindow.h"
#include "ui/interface/scenarioeditor.h"

#include "ctp/civapp.h"



ScenarioWindow                      *s_ScenarioWindow = nullptr;









ScenarioWindow::ScenarioWindow(AUI_ERRCODE *retval, MBCHAR *ldlBlock)
{
	Assert(AUI_SUCCESS(*retval));

	m_scenario = nullptr;
	m_scenarioPack = nullptr;

	m_window = (ctp2_Window *)aui_Ldl::BuildHierarchyFromRoot(ldlBlock);





	m_available		= (ctp2_ListBox *)aui_Ldl::GetObject(ldlBlock, "AvailableListBox");
	m_available->SetActionFuncAndCookie(ScenarioSelect, nullptr);

	m_window->SetType(AUI_WINDOW_TYPE_FLOATING);
	m_window->SetStronglyModal(TRUE);

	m_scenInstructions = (ctp2_Static *)aui_Ldl::GetObject(ldlBlock, "ScenInstructions");
	m_packInstructions = (ctp2_Static *)aui_Ldl::GetObject(ldlBlock, "PackInstructions");

	m_LoadButton = (ctp2_Button *)aui_Ldl::GetObject(ldlBlock, "LoadButton");
	m_SaveButton = (ctp2_Button *)aui_Ldl::GetObject(ldlBlock, "SaveButton");
	m_NewButton = (ctp2_Button *)aui_Ldl::GetObject(ldlBlock, "NewButton");
	m_BackButton = (ctp2_Button *)aui_Ldl::GetObject(ldlBlock, "CancelButton");

	m_LoadButton->SetActionFuncAndCookie(OkPress, nullptr);
	m_SaveButton->SetActionFuncAndCookie(SavePress, nullptr);
	m_NewButton->SetActionFuncAndCookie(NewPress, nullptr);
	m_BackButton->SetActionFuncAndCookie(BackPress, nullptr);












	FillListWithScenarioPacks(m_available);
	m_mode = SCENARIO_WINDOW_MODE_LOAD_PACK;
	m_scenInstructions->Hide();
	m_packInstructions->Show();

	m_exitCallback = nullptr;

	m_newPackWindow = (ctp2_Window *)aui_Ldl::BuildHierarchyFromRoot("NewPackWindow");
	aui_Ldl::SetActionFuncAndCookie("NewPackWindow.OkButton", NewPackOk, nullptr);
	aui_Ldl::SetActionFuncAndCookie("NewPackWindow.CancelButton", NewPackCancel, nullptr);
	m_newPackWindow->SetType(AUI_WINDOW_TYPE_POPUP);

	m_newScenWindow = (ctp2_Window *)aui_Ldl::BuildHierarchyFromRoot("NewScenWindow");
	aui_Ldl::SetActionFuncAndCookie("NewScenWindow.OkButton", NewScenOk, nullptr);
	aui_Ldl::SetActionFuncAndCookie("NewScenWindow.CancelButton", NewScenCancel, nullptr);
	m_newScenWindow->SetType(AUI_WINDOW_TYPE_POPUP);
}

void ScenarioWindow::FillListWithScenarios(ctp2_ListBox *available)
{
	int					i=0;
	ScenarioPack		*scenPack;
	MBCHAR				*ldlBlock = "ScenarioListItem";

	scenPack = m_scenarioPack;

	if ( scenPack ) {
		for (i=0; i<static_cast<sint32>(scenPack->m_scenarios.size()); i++) {
			Scenario *scen = &scenPack->m_scenarios[i];

			ctp2_ListItem	*item=nullptr;
			item = (ctp2_ListItem *) aui_Ldl::BuildHierarchyFromRoot(ldlBlock);
			Assert(item);
			if (item) {
				ctp2_Static *box = (ctp2_Static *)item->GetChildByIndex(0);
				Assert(box);
				if(box) {
					ctp2_Static *name = (ctp2_Static *)box->GetChildByIndex(0);
					if(name) {
						name->SetText(scen->m_name);
					}

					ctp2_Static *description = (ctp2_Static *)box->GetChildByIndex(1);
					if(description) {
						description->SetText(scen->m_description);
					}

					ctp2_Static *image = (ctp2_Static *)box->GetChildByIndex(2);
					if(image) {
						MBCHAR imPath[_MAX_PATH];
						snprintf(imPath, sizeof(imPath), "%s\\%s", scen->m_path, "scenicon.tga");
						if(c3files_PathIsValid(imPath)) {
							image->SetImage(imPath);
						}
					}
				}
				item->SetUserData((void *)scen);

				available->AddItem(item );
			}
		}
	}
}

void ScenarioWindow::FillListWithScenarioPacks(ctp2_ListBox *available,bool hideOriginalScenarios)
{
	int					i=0;
	ScenarioPack		*scenPack;
//	MBCHAR				*ldlBlock = "ScenarioPackListItem";
	MBCHAR checkFile[_MAX_PATH];
	struct stat fileStatus;

	CivScenarios *cs = civscenarios_Get();
	for (i=0; i<cs->GetNumScenarioPacks(); i++) {
		scenPack = cs->GetScenarioPack(i);

		snprintf(checkFile, sizeof(checkFile),"%s\\%s",scenPack->m_path,"Activision.txt");
		if(!(hideOriginalScenarios && !stat(checkFile,&fileStatus)))
		{
			ctp2_ListItem	*item=nullptr;
			item = (ctp2_ListItem*) aui_Ldl::BuildHierarchyFromRoot("ScenarioPackListItem");
			Assert(item);
			if (item) {
				ctp2_Static *box = (ctp2_Static *)item->GetChildByIndex(0);
				if(box) {
					ctp2_Static *name = (ctp2_Static *)box->GetChildByIndex(0);
					if(name) {
						name->SetText(scenPack->m_name);
					}

					ctp2_Static *description = (ctp2_Static *)box->GetChildByIndex(1);
					if(description) {
						description->SetText(scenPack->m_description);
					}

					ctp2_Static *image = (ctp2_Static *)box->GetChildByIndex(2);
					if(image) {
						MBCHAR imPath[_MAX_PATH];
						snprintf(imPath, sizeof(imPath), "%s\\%s", scenPack->m_path, "packicon.tga");
						if(c3files_PathIsValid(imPath)) {
							image->SetImage(imPath);
						}
					}
				}
				item->SetUserData(scenPack);
				available->AddItem(item );
			}
		}
	}
}


void ScenarioWindow::SetScenario(Scenario *scenario)
{
	Assert(scenario != nullptr);

	m_scenario = scenario;


}



void ScenarioWindow::SetMode(SCENARIO_WINDOW_MODE mode)
{
	m_available->Clear();
	m_LoadButton->Enable(FALSE);
	m_SaveButton->Enable(FALSE);

	switch(mode) {
		case SCENARIO_WINDOW_MODE_LOAD_SCEN:


			FillListWithScenarios(m_available);
			m_packInstructions->Hide();
			m_scenInstructions->Show();
			m_LoadButton->Show();
			m_SaveButton->Hide();
			m_NewButton->Hide();
			break;
		case SCENARIO_WINDOW_MODE_LOAD_PACK:


			FillListWithScenarioPacks(m_available);
			m_packInstructions->Show();
			m_scenInstructions->Hide();
			m_LoadButton->Show();
			m_SaveButton->Hide();
			m_NewButton->Hide();
			break;
		case SCENARIO_WINDOW_MODE_SAVE_SCEN:
			FillListWithScenarios(m_available);
			m_packInstructions->Hide();
			m_scenInstructions->Show();
			m_LoadButton->Hide();
			m_SaveButton->Show();
			m_NewButton->Show();
			break;
		case SCENARIO_WINDOW_MODE_SAVE_PACK:
			FillListWithScenarioPacks(m_available,true);
			m_packInstructions->Show();
			m_scenInstructions->Hide();
			m_LoadButton->Show();
			m_SaveButton->Hide();
			m_NewButton->Show();
			break;

	}
	m_mode = mode;
	s_ScenarioWindow->m_window->ShouldDraw(TRUE);
}


ScenarioWindow::~ScenarioWindow()
{
	if(m_window) {
		aui_Ldl::DeleteHierarchyFromRoot("ScenarioWindow");
		m_window = nullptr;
	}

	if(m_newPackWindow) {
		aui_Ldl::DeleteHierarchyFromRoot("NewPackWindow");
		m_newPackWindow = nullptr;
	}

	if(m_newScenWindow) {
		aui_Ldl::DeleteHierarchyFromRoot("NewScenWindow");
		m_newScenWindow = nullptr;
	}
}

void ScenarioWindow::Initialize()
{
	if(s_ScenarioWindow) return;

	AUI_ERRCODE retval = AUI_ERRCODE_OK;

	s_ScenarioWindow = new ScenarioWindow(&retval, "ScenarioWindow");
	Assert(retval == AUI_ERRCODE_OK);
}

void ScenarioWindow::Display(bool load)
{
	if(!s_ScenarioWindow) {
		Initialize();
	}

	Assert(s_ScenarioWindow);
	if(s_ScenarioWindow) {
		Assert(s_ScenarioWindow->m_window);
		if(s_ScenarioWindow->m_window) {
			c3ui_Get()->AddWindow(s_ScenarioWindow->m_window);
			s_ScenarioWindow->m_window->Show();

			if(load) {
				s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_LOAD_PACK);
			} else {
				s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_PACK);
			}
		}
	}
}

void ScenarioWindow::Hide()
{
	if(!s_ScenarioWindow) return;

	Assert(s_ScenarioWindow->m_window);
	if(s_ScenarioWindow->m_window) {
		c3ui_Get()->RemoveWindow(s_ScenarioWindow->m_window->Id());
	}
}

void ScenarioWindow::Cleanup()
{
	if(!s_ScenarioWindow) return;

	Hide();

	delete s_ScenarioWindow;
	s_ScenarioWindow = nullptr;
}

void ScenarioWindow::SetExitCallback(aui_Control::ControlActionCallback *callback)
{
	if(s_ScenarioWindow) {
		s_ScenarioWindow->m_exitCallback = callback;
	}
}

void ScenarioWindow::ScenarioSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if(s_ScenarioWindow) {
		ctp2_ListBox *mylistbox = (ctp2_ListBox*)control;




		if (s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_PACK ||
			s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_PACK) {
			ctp2_ListItem *myitem = (ctp2_ListItem *)mylistbox->GetSelectedItem();


			if (action == (uint32)AUI_LISTBOX_ACTION_SELECT) {
				if (myitem) {
					s_ScenarioWindow->SetScenarioPack((ScenarioPack *)myitem->GetUserData());
				}
			} else


			if (action == (uint32)AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {

				if (myitem) {
					s_ScenarioWindow->SetScenarioPack((ScenarioPack *)myitem->GetUserData());
				}

				if(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_PACK) {
					s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_LOAD_SCEN);
				} else {
					s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_SCEN);
				}
			}
			if(mylistbox->GetSelectedItem()) {
				if(s_ScenarioWindow->m_LoadButton) {
					s_ScenarioWindow->m_LoadButton->Enable(TRUE);
					s_ScenarioWindow->m_SaveButton->Enable(TRUE);
				}
			} else {
				s_ScenarioWindow->m_LoadButton->Enable(FALSE);
				s_ScenarioWindow->m_SaveButton->Enable(FALSE);
			}
		} else


		if (s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_SCEN ||
			s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_SCEN) {
			ctp2_ListItem *myitem = (ctp2_ListItem *)mylistbox->GetSelectedItem();




			if (action == (uint32)AUI_LISTBOX_ACTION_SELECT) {
				if( myitem ) {

					s_ScenarioWindow->SetScenario((Scenario *)myitem->GetUserData());
				}
			} else


			if (action == (uint32)AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {
				if( myitem ) {

					s_ScenarioWindow->SetScenario((Scenario *)myitem->GetUserData());
				}

				if(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_SCEN) {


					SetProfileFromScenario();

					if (s_ScenarioWindow->GetExitCallback()) {
						s_ScenarioWindow->GetExitCallback()(control, action, data, cookie);
					} else {
						c3ui_Get()->AddAction(new CloseScenarioScreenAction);
					}

				} else {

				}
			}

			if(mylistbox->GetSelectedItem()) {
				if(s_ScenarioWindow->m_LoadButton) {
					s_ScenarioWindow->m_LoadButton->Enable(TRUE);
					s_ScenarioWindow->m_SaveButton->Enable(TRUE);
				}
			} else {
				s_ScenarioWindow->m_LoadButton->Enable(FALSE);
				s_ScenarioWindow->m_SaveButton->Enable(FALSE);
			}

		}
	}
}

void ScenarioWindow::CancelPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if (action != uint32(AUI_BUTTON_ACTION_EXECUTE)) return;

	if (s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_SCEN) {
		s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_LOAD_PACK);
	} else
		if (s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_PACK) {


			memset(scenario_name_buf(), '\0', k_SCENARIO_NAME_MAX);

			civpaths_Get()->ClearCurScenarioPath();
			civpaths_Get()->ClearCurScenarioPackPath();

			c3ui_Get()->AddAction(new CloseScenarioScreenAction);
		}
}

void ScenarioWindow::OkPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if (action != uint32(AUI_BUTTON_ACTION_EXECUTE)) return;

	if (s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_SCEN) {
		if (!scenarioscreen_removeMyWindow(action)) return;

		SetProfileFromScenario();


		if (s_ScenarioWindow->GetExitCallback()) {
			s_ScenarioWindow->GetExitCallback()(control, action, data, cookie);
		} else {
			c3ui_Get()->AddAction(new CloseScenarioScreenAction);
		}
	} else if (s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_LOAD_PACK) {
		if(s_ScenarioWindow->m_scenarioPack) {
			s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_LOAD_SCEN);
		}
	} else if(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_PACK) {
		s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_SCEN);
	} else {

		Assert(FALSE);
	}

}

void ScenarioWindow::SavePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if (action != uint32(AUI_BUTTON_ACTION_EXECUTE)) return;

	Assert(s_ScenarioWindow);
	if(!s_ScenarioWindow)
		return;

	Assert(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_SCEN);
	if(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_SCEN) {
		if(!s_ScenarioWindow->m_scenario) return;





		scenario_name_buf()[0] = 0;

		is_scenario_Set(TRUE);

		switch(ScenarioEditor::GetStartLocMode()) {
			case SCEN_START_LOC_MODE_NONE:
				start_info_type_Set(STARTINFOTYPE_NOLOCS);
				break;
			case SCEN_START_LOC_MODE_PLAYER_WITH_CIV:
				start_info_type_Set(STARTINFOTYPE_CIVSFIXED);
				break;
			case SCEN_START_LOC_MODE_PLAYER:
				start_info_type_Set(STARTINFOTYPE_POSITIONSFIXED);
				break;
			case SCEN_START_LOC_MODE_CIV:
				start_info_type_Set(STARTINFOTYPE_CIVS);
				break;
			default:
				Assert(FALSE);
				start_info_type_Set(STARTINFOTYPE_NOLOCS);
				break;
		}

		loadsavescreen_Initialize();
		loadsavewindow_Get()->SetType(LSS_SAVE_GAME);

		loadsavescreen_SaveGame(s_ScenarioWindow->m_scenario->m_path,
								k_SCENARIO_DEFAULT_SAVED_GAME_NAME);
	}
}

void ScenarioWindow::NewPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if (action != uint32(AUI_BUTTON_ACTION_EXECUTE)) return;

	Assert(s_ScenarioWindow);
	if(!s_ScenarioWindow)
		return;

	if(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_PACK) {
		Assert(s_ScenarioWindow->m_newPackWindow);
		if(s_ScenarioWindow->m_newPackWindow) {
			c3ui_Get()->AddWindow(s_ScenarioWindow->m_newPackWindow);
		}
	} else if(s_ScenarioWindow->GetMode() == SCENARIO_WINDOW_MODE_SAVE_SCEN) {
		Assert(s_ScenarioWindow->m_newScenWindow);
		if(s_ScenarioWindow->m_newScenWindow) {
			c3ui_Get()->AddWindow(s_ScenarioWindow->m_newScenWindow);
		}
	}
}

void ScenarioWindow::BackPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	Assert(s_ScenarioWindow);
	if(!s_ScenarioWindow)
		return;

	switch(s_ScenarioWindow->GetMode()) {
		case SCENARIO_WINDOW_MODE_SAVE_PACK:
			Hide();
			break;
		case SCENARIO_WINDOW_MODE_SAVE_SCEN:
			s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_PACK);
			break;
		case SCENARIO_WINDOW_MODE_LOAD_PACK:
			Hide();
			break;
		case SCENARIO_WINDOW_MODE_LOAD_SCEN:
			s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_LOAD_PACK);
			break;
	}
}

void ScenarioWindow::SetProfileFromScenario( )
{
	if (s_ScenarioWindow) {
		if (s_ScenarioWindow->GetScenario() != nullptr) {

			civpaths_Get()->SetCurScenarioPath(s_ScenarioWindow->GetScenario()->m_path);


			civpaths_Get()->SetCurScenarioPackPath(s_ScenarioWindow->GetScenarioPack()->m_path);

			profiledb_Get()->SetIsScenario(TRUE);

			strlcpy(scenario_name_buf(), s_ScenarioWindow->GetScenario()->m_name, k_SCENARIO_NAME_MAX);

			civapp_Get()->CleanupAppDB();
			civapp_Get()->InitializeAppDB();


















		}
	}
}

void ScenarioWindow::LoadScenarioGame( )
{
	if (s_ScenarioWindow) {
		if (s_ScenarioWindow->GetScenario() != nullptr) {

		}
	}
}

void ScenarioWindow::NewPackOk(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	MBCHAR dir[_MAX_PATH];
	MBCHAR name[_MAX_PATH];
	MBCHAR desc[_MAX_PATH];

	ctp2_TextField *field = (ctp2_TextField *)aui_Ldl::GetObject("NewPackWindow", "DirField");
	if(field) {
		field->GetFieldText(dir, _MAX_PATH - 1);
	} else {
		dir[0] = 0;
	}

	field = (ctp2_TextField *)aui_Ldl::GetObject("NewPackWindow", "NameField");
	if(field) {
		field->GetFieldText(name, _MAX_PATH - 1);
	} else {
		name[0] = 0;
	}

	field = (ctp2_TextField *)aui_Ldl::GetObject("NewPackWindow", "DescField");
	if(field) {
		field->GetFieldText(desc, _MAX_PATH - 1);
	} else {
		desc[0] = 0;
	}

	MBCHAR *d;

	for(d = desc; *d != 0; d++) {
		if((*d) == '\n' || (*d) == '\r')
			*d = ' ';
	}

	if((strlen(dir) < 1) || (strlen(name) < 1) || (strlen(desc) < 1)) {

		return;
	}

	CIV_SCEN_ERR err = civscenarios_Get()->MakeNewPack(dir, name, desc);
	Assert(err == CIV_SCEN_OK);

	Assert(s_ScenarioWindow);
	if(s_ScenarioWindow) {

		s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_PACK);

		if(s_ScenarioWindow->m_newPackWindow) {
			c3ui_Get()->RemoveWindow(s_ScenarioWindow->m_newPackWindow->Id());
		}
	}
}

void ScenarioWindow::NewPackCancel(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	if(s_ScenarioWindow) {
		if(s_ScenarioWindow->m_newPackWindow) {
			c3ui_Get()->RemoveWindow(s_ScenarioWindow->m_newPackWindow->Id());
		}
	}
}

void ScenarioWindow::NewScenOk(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	MBCHAR name[_MAX_PATH];
	MBCHAR desc[_MAX_PATH];

	ctp2_TextField *field = (ctp2_TextField *)aui_Ldl::GetObject("NewScenWindow", "NameField");
	if(field) {
		field->GetFieldText(name, _MAX_PATH - 1);
	} else {
		name[0] = 0;
	}

	field = (ctp2_TextField *)aui_Ldl::GetObject("NewScenWindow", "DescField");
	if(field) {
		field->GetFieldText(desc, _MAX_PATH - 1);
	} else {
		desc[0] = 0;
	}

	MBCHAR *d;

	for(d = desc; *d != 0; d++) {
		if((*d) == '\n' || (*d) == '\r')
			*d = ' ';
	}

	if((strlen(name) < 1) || (strlen(desc) < 1)) {

		return;
	}

	Assert(s_ScenarioWindow);
	if(s_ScenarioWindow) {
		MBCHAR scenPackDir[_MAX_PATH];
		strlcpy(scenPackDir, s_ScenarioWindow->m_scenarioPack->m_path, sizeof(scenPackDir));

		CivScenarios *cs = civscenarios_Get();
		CIV_SCEN_ERR err = cs->MakeNewScenario(s_ScenarioWindow->m_scenarioPack, name, desc);
		Assert(err == CIV_SCEN_OK);

		s_ScenarioWindow->m_scenarioPack = cs->GetScenarioPackByPath(scenPackDir);
		Assert(s_ScenarioWindow->m_scenarioPack);
		if(s_ScenarioWindow->m_scenarioPack) {

			s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_SCEN);
		} else {
			s_ScenarioWindow->SetMode(SCENARIO_WINDOW_MODE_SAVE_PACK);
		}

		if(s_ScenarioWindow->m_newScenWindow) {
			c3ui_Get()->RemoveWindow(s_ScenarioWindow->m_newScenWindow->Id());
		}
	}
}

void ScenarioWindow::NewScenCancel(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	if(s_ScenarioWindow) {
		if(s_ScenarioWindow->m_newScenWindow) {
			c3ui_Get()->RemoveWindow(s_ScenarioWindow->m_newScenWindow->Id());
		}
	}
}
