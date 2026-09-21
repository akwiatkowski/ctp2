//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Battle order box.
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
// - Added unit display name.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Changed occurances of UnitRecord::GetMaxHP to
//   UnitData::CalculateTotalHP. (Aug 3rd 2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gs/world/cellunitlist.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/UnitPool.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"

#include "ui/aui_ctp2/c3_static.h"

#include "gfx/spritesys/SpriteState.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/gfx_utils/colorset.h"               // colorset_Get()
#include "ui/aui_ctp2/c3_coloredstatic.h"
#include "ui/aui_ctp2/controlsheet.h"
#include "ui/aui_ctp2/textbutton.h"
#include <memory>
#include "ui/aui_ctp2/c3_coloriconbutton.h"
#include "ui/aui_ctp2/unittabbutton.h"

#include "ui/aui_ctp2/thermometer.h"
#include "ui/interface/UIUtils.h"

#include "ui/aui_ctp2/battleorderbox.h"
#include "ui/aui_ctp2/battleorderboxactions.h"

#include "gs/database/StrDB.h"

#include "ui/aui_utils/primitives.h"
#include "UnitRecord.h"
#include "IconRecord.h"
#include "gs/gameobj/UnitData.h"

#define k_UNIT_FRAME_THICKNESS	2

BattleOrderBox::BattleOrderBox(AUI_ERRCODE *retval,
					   uint32 id,
					   MBCHAR const *ldlBlock,
					   ControlActionCallback *ActionFunc,
					   void *cookie)
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
	ControlSheet(retval, id, ldlBlock, ActionFunc, cookie)
{
	InitCommon(ldlBlock);
}

BattleOrderBox::BattleOrderBox(AUI_ERRCODE *retval,
					   uint32 id,
					   sint32 x,
					   sint32 y,
					   sint32 width,
					   sint32 height,
					   MBCHAR const *pattern,
					   ControlActionCallback *ActionFunc,
					   void *cookie)
	:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( nullptr ),
	ControlSheet(retval, id, x, y, width, height, pattern, ActionFunc, cookie)
{
	InitCommon(nullptr);
}

AUI_ERRCODE BattleOrderBox::InitCommon( MBCHAR const *ldlBlock)
{
	sint32			buttonBorder = 2;
	sint32			cellWidth = (Width()-buttonBorder*2)/3;
	sint32			cellHeight = (Height()-buttonBorder*2)/3;
	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			coloredBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			fortifyBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	RECT			iconRect;
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	sint32	 i;
	sint32	 j;

	for ( i = 0;i < k_CARGO_CAPACITY;i++ ) {
		m_cargo[i] = nullptr;
	}

	for (i=0; i<3; i++) {
		SetRect(&iconRect, 0, 0, cellWidth, cellHeight);

		OffsetRect(&iconRect, buttonBorder, buttonBorder + i*cellHeight);

		InflateRect(&iconRect, -4, -3);

		for (j=0; j<3; j++)
        {
			sint32 const index = i*3+j;

			m_unitRect[index] = iconRect;


			UnitTabButton * button = std::make_unique<UnitTabButton>
                (&errcode, aui_UniqueId(), iconRect.left, iconRect.top,
				 (iconRect.right-iconRect.left), (iconRect.bottom-iconRect.top),
                 "upba0119.tga"
                ).release();

			Assert(button);
			if (!button) return AUI_ERRCODE_MEMALLOCFAILED;

			button->IconButton()->SetImageBltType(AUI_IMAGEBASE_BLTTYPE_STRETCH);




			BobButtonAction	*   buttonAction = std::make_unique<BobButtonAction>(this).release();
			Assert(buttonAction);
			if (!buttonAction) continue;

			button->IconButton()->SetAction((aui_Action *)buttonAction);

			errcode = AddSubControl(button);
			Assert(errcode == AUI_ERRCODE_OK);







			m_unitControls[index] = button;

			m_unitRectColors[index] = COLOR_BLACK;

			OffsetRect(&iconRect, cellWidth, 0);
		}
	}




	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitImageButton");
	m_unitImage = std::make_unique<c3_ColorIconButton>(&errcode, aui_UniqueId(), controlBlock);

	Assert( AUI_NEWOK(m_unitImage, errcode) );
	if ( !AUI_NEWOK(m_unitImage, errcode) ) return errcode;

	m_unitImage->ShrinkToFit( TRUE );

	BobButtonAction *buttonAction = std::make_unique<BobButtonAction>(this).release();
	Assert(buttonAction);
	if (!buttonAction) return AUI_ERRCODE(-1);

	m_unitImage->SetAction((aui_Action *)buttonAction);


	snprintf(fortifyBlock, sizeof(fortifyBlock), "%s.%s", controlBlock, "UnitF" );
	m_unitFortify = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), fortifyBlock );
	Assert( AUI_NEWOK( m_unitFortify, errcode) );
	if ( !AUI_NEWOK( m_unitFortify, errcode ) ) return errcode;

	m_unitFortify->SetBlindness( TRUE );

	snprintf(fortifyBlock, sizeof(fortifyBlock), "%s.%s", controlBlock, "UnitV" );
	m_unitVeteran = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), fortifyBlock );
	Assert( AUI_NEWOK( m_unitVeteran, errcode) );
	if ( !AUI_NEWOK( m_unitVeteran, errcode ) ) return errcode;

	m_unitVeteran->SetBlindness( TRUE );


	snprintf(coloredBlock, sizeof(coloredBlock), "%s.%s", controlBlock, "Cargo" );

	for ( i = 0;i < k_CARGO_CAPACITY;i++ ) {
		m_cargo[i] = std::make_unique<c3_ColoredStatic>( &errcode, aui_UniqueId(), coloredBlock ).release();
		Assert( AUI_NEWOK( m_cargo[i], errcode) );
		if ( !AUI_NEWOK(m_cargo[i], errcode) ) return errcode;

		m_cargo[i]->SetBlindness( TRUE );




		m_cargo[i]->Move( (m_cargo[i]->Width() + k_BIG_CARGO_OFFSET) * i + 1,
			m_unitImage->Height() - m_cargo[i]->Height() - 1);
		m_cargo[i]->SetColor( COLOR_GREEN );
	}


	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitARDText");
	m_unitARDText = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_unitARDText, errcode) );
	if ( !AUI_NEWOK(m_unitARDText, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitARD");
	m_unitARD = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_unitARD, errcode) );
	if ( !AUI_NEWOK(m_unitARD, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitMText");
	m_unitMText = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_unitMText, errcode) );
	if ( !AUI_NEWOK(m_unitMText, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitMovement");
	m_unitMovement = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_unitMovement, errcode) );
	if ( !AUI_NEWOK(m_unitMovement, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "ActiveDefenseIcon");
	m_activeDefenseIcon = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_activeDefenseIcon, errcode) );
	if ( !AUI_NEWOK(m_activeDefenseIcon, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "VeteranIcon");
	m_veteranIcon = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert( AUI_NEWOK(m_veteranIcon, errcode) );
	if ( !AUI_NEWOK(m_veteranIcon, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitHealthBar" );
	m_unitHealthBar = std::make_unique<Thermometer>( &errcode, aui_UniqueId(), controlBlock ).release();
	Assert( AUI_NEWOK(m_unitHealthBar, errcode) );
	if ( !AUI_NEWOK(m_unitHealthBar, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "FuelLabel" );
	m_fuelLabel = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(m_fuelLabel, errcode) );
	if ( !AUI_NEWOK(m_fuelLabel, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "FuelBox" );
	m_fuelBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(m_fuelBox, errcode) );
	if ( !AUI_NEWOK(m_fuelBox, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "UnitName" );
	m_unitName = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(m_unitName, errcode) );
	if ( !AUI_NEWOK(m_unitName, errcode) ) return errcode;

	errcode = AddSubControl(m_unitImage.get());
	Assert(errcode == AUI_ERRCODE_OK);
	errcode = AddSubControl(m_unitARDText.get());
	Assert(errcode == AUI_ERRCODE_OK);
	errcode = AddSubControl(m_unitMText.get());
	Assert(errcode == AUI_ERRCODE_OK);
	errcode = AddSubControl(m_unitMovement.get());
	Assert(errcode == AUI_ERRCODE_OK);
	errcode = AddSubControl(m_activeDefenseIcon.get());
	Assert(errcode == AUI_ERRCODE_OK);
	errcode = AddSubControl(m_veteranIcon.get());
	Assert(errcode == AUI_ERRCODE_OK);
	errcode = AddSubControl(m_unitName.get());
	Assert(errcode == AUI_ERRCODE_OK);

	return AUI_ERRCODE_OK;
}

BattleOrderBox::~BattleOrderBox()
{
	sint32				i;
	BobButtonAction		*action;

	for (i=0; i<k_MAX_BOB_UNITS; i++) {
		if (m_unitControls[i] != nullptr) {
			action = (BobButtonAction *)m_unitControls[i]->IconButton()->GetAction();
			Assert(action);
			std::unique_ptr<BobButtonAction>{action};
			std::unique_ptr<UnitTabButton>{m_unitControls[i]};
			m_unitControls[i] = nullptr;

		}
	}

	action = (BobButtonAction *)m_unitImage->GetAction();

	Assert(action);
	std::unique_ptr<BobButtonAction>{action};










	DeleteControl( m_unitHealthBar );
	m_fuelLabel.reset();
	m_fuelBox.reset();
	m_unitName.reset();

	for ( i = 0;i < k_CARGO_CAPACITY;i++ ) {
		DeleteControl( m_cargo[i] );
	}
}

AUI_ERRCODE BattleOrderBox::Show( )
{
	ShowThis();


	SetStackMode(m_stackDisplay);

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE BattleOrderBox::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{

	ControlSheet::DrawThis(surface, x, y);

	RECT bounds = { 0, 0, m_width, m_height };
	OffsetRect( &bounds, m_x + x, m_y + y );
	ToWindow( &bounds );

	if ( !surface ) surface = m_window->TheSurface();

	if (surface) {
		sint32		i;

		if (m_stackDisplay) {
			for (i=0; i<k_MAX_BOB_UNITS; i++) {
				if ( m_unitRectColors[i] == COLOR_RED ) {
					RECT tempRect = m_unitRect[i];
					InflateRect( &tempRect, 2, 2);
					OffsetRect(&tempRect, bounds.left, bounds.top);

					primitives_FrameThickRect16(surface, &tempRect, colorset_Get()->GetColor(m_unitRectColors[i]),k_UNIT_FRAME_THICKNESS);
					m_window->AddDirtyRect( &m_unitRect[i] );
				}
				else {
					RECT tempRect = m_unitRect[i];
					InflateRect( &tempRect, 1, 1);
					OffsetRect(&tempRect, bounds.left, bounds.top);

					primitives_FrameRect16(surface, &tempRect, colorset_Get()->GetColor(m_unitRectColors[i]));
					m_window->AddDirtyRect( &m_unitRect[i] );
				}
			}
		}
	}

	return AUI_ERRCODE_OK;
}

void BattleOrderBox::SetSingleUnit(Unit theUnit)
{
	MBCHAR		s[_MAX_PATH];

	strlcpy(s, theUnit.GetDBRec()->GetDefaultIcon()->GetIcon(), sizeof(s));
	m_unitImage->SetIcon( s );

	BobButtonAction *action = (BobButtonAction *)m_unitImage->GetAction();

	Assert(action);
	if (action)
		action->SetUnit(theUnit);






	if ( theUnit.IsVeteran() ) {
		m_unitVeteran->SetText( stringdb_Get()->GetNameStr("str_ldl_V") );
		m_unitVeteran->SetTextColor( colorset_Get()->GetColorRef(COLOR_WHITE) );
		m_unitImage->AddSubControl(m_unitVeteran.get());
	}
	else {
		m_unitImage->RemoveSubControl( m_unitVeteran->Id() );
	}

	if ( theUnit.IsEntrenched() ) {
		m_unitFortify->SetText( stringdb_Get()->GetNameStr("str_ldl_F") );
		m_unitFortify->SetTextColor( colorset_Get()->GetColorRef(COLOR_WHITE) );
		m_unitImage->AddSubControl(m_unitFortify.get());
	}
	else if ( theUnit.IsAsleep() ) {
		m_unitFortify->SetText( stringdb_Get()->GetNameStr("str_tbl_ldl_S") );
		m_unitFortify->SetTextColor( colorset_Get()->GetColorRef(COLOR_WHITE) );
		m_unitImage->AddSubControl(m_unitFortify.get());
	}
	else if ( theUnit.IsEntrenching() ) {
		m_unitFortify->SetText( stringdb_Get()->GetNameStr("str_ldl_F") );
		m_unitFortify->SetTextColor( colorset_Get()->GetColorRef(COLOR_GRAY) );
		m_unitImage->AddSubControl(m_unitFortify.get());
	}
	else {
		m_unitImage->RemoveSubControl( m_unitFortify->Id() );
	}

	snprintf(s, sizeof(s), "%d/%d/%d", (sint32)(theUnit.GetAttack() / 10.0),
		(sint32)(theUnit.GetZBRange() / 10.0), (sint32)(theUnit.GetDefense() / 10.0) );
	m_unitARD->SetText(s);

	snprintf(s, sizeof(s), "%d/%d", (sint32)(theUnit.GetMovementPoints() / 100.0), (sint32)(theUnit.GetMaxMovePoints() / 100.0));
	m_unitMovement->SetText(s);

	sint32 healthPercent  = (sint32)( theUnit.GetHP() * 100  / theUnit->CalculateTotalHP());//.GetDBRec()->GetMaxHP() );
	m_unitHealthBar->SetPercentFilled( healthPercent );

	m_unitName->SetText(theUnit.GetDisplayName().c_str());

	if ( theUnit.GetMovementTypeAir() ) {
		double fuel = theUnit.GetFuel() / 100.0;
		snprintf(s, sizeof(s), "%.1f", fuel );
		m_fuelBox->SetText( s );
		AddSubControl(m_fuelLabel.get());
		AddSubControl(m_fuelBox.get());
	}
	else {
		RemoveSubControl( m_fuelLabel->Id() );
		RemoveSubControl( m_fuelBox->Id() );
	}

	const UnitRecord *rec = theUnit.GetDBRec();
	sint32 maxCargo;
	if(rec->GetCargoDataPtr()) {
		maxCargo = rec->GetCargoDataPtr()->GetMaxCargo();
	} else {
		maxCargo = 0;
	}
	sint32 currentCargo = theUnit.GetNumCarried();

	for (auto & j : m_cargo) {
		m_unitImage->RemoveSubControl( j->Id() );
	}

	if ( rec->GetCanCarry() ) {
		for ( sint32 i = 0;i < k_CARGO_CAPACITY;i++ ) {

			if ( i < maxCargo ) {
				m_unitImage->AddSubControl( m_cargo[i] );

				if ( i < currentCargo ) {
					m_cargo[i]->SetColor( COLOR_GREEN );
				}
				else {
					m_cargo[i]->SetColor( COLOR_WHITE );
				}
			}
			else {
				m_unitImage->RemoveSubControl( m_cargo[i]->Id() );
			}
		}
	}


















}

void BattleOrderBox::SetStackMode(BOOL stackDisplay)
{

	stackDisplay = FALSE;

	sint32				i;
	UnitTabButton		*button;

	for (i=0; i<k_MAX_BOB_UNITS; i++) {
		button = m_unitControls[i];
		Assert(button);
		if (!button) return;

		if (stackDisplay) {
			button->Show();
		}
		else {
			button->Hide();
		}
	}


	stackDisplay = TRUE;

	if (stackDisplay) {
		m_unitImage->Hide();
		m_unitARDText->Hide();
		m_unitARD->Hide();
		m_unitMovement->Hide();
		m_unitMText->Hide();
		m_unitHealthBar->Hide();
		m_fuelLabel->Hide();
		m_fuelBox->Hide();
		m_unitName->Hide();
	} else {
		m_unitImage->Show();
		m_unitARDText->Show();
		m_unitARD->Show();
		m_unitMovement->Show();
		m_unitMText->Show();
		if ( !m_veteranIcon->IsHidden() ) {
			m_veteranIcon->Show();
		}
		if ( !m_activeDefenseIcon->IsHidden() ) {
			m_activeDefenseIcon->Show();
		}
		m_unitHealthBar->Show();

		m_fuelLabel->Show();
		m_fuelBox->Show();
		m_unitName->Show();
	}

	m_stackDisplay = stackDisplay;
	ShouldDraw();
}

void BattleOrderBox::SetStack(Army &selectedArmy, CellUnitList *fullArmy, Unit singleUnit)
{
	sint32			i;
	Unit			unit;
	UnitTabButton	*button;

	if (selectedArmy.m_id == (0) && fullArmy == nullptr) {

		for (i=0; i<k_MAX_BOB_UNITS; i++) {
			button = m_unitControls[i];

			Assert(button);
			if (!button) return;

			button->IconButton()->SetIcon("");

			button->UpdateData( nullptr );

			BobButtonAction *action = (BobButtonAction *)button->IconButton()->GetAction();
			Assert(action);
			if (!action) continue;

			action->SetUnit(Unit());
			action->SetArmy(Army());
		}
	} else {
		sint32		count = fullArmy->Num();
		MBCHAR		iconName[_MAX_PATH];

		for (i=0; i<k_MAX_BOB_UNITS; i++) {
			button = m_unitControls[i];
			Assert(button);
			if (!button) return;

			button->IconButton()->SetIcon("");

			m_unitRectColors[i] = COLOR_BLACK;

			button->UpdateData( nullptr );

			BobButtonAction *action = (BobButtonAction *)button->IconButton()->GetAction();
			Assert(action);
			if (!action) continue;

			action->SetUnit(Unit());
			action->SetArmy(Army());

		}

		if (!m_stackDisplay) {
			if(count > 0) {
				if ( singleUnit.m_id == (0) ) {
					SetSingleUnit(fullArmy->Access(0));

				}
				else {
					SetSingleUnit( singleUnit );
				}
			}
		} else {
			for (i=0; i<count; i++) {
				unit = fullArmy->Access(i);

				if (!unitpool_Get()->IsValid(unit)) {
					Assert(FALSE);
					continue;
				}

				button = m_unitControls[i];
				Assert(button);
				if (!button) continue;

				BobButtonAction *action = (BobButtonAction *)button->IconButton()->GetAction();
				Assert(action);
				if (!action) continue;

				action->SetUnit(unit);
				action->SetArmy( selectedArmy );

				if (selectedArmy.m_id != (0)) {
					if (selectedArmy.IsPresent(unit)) {

						m_unitRectColors[i] = COLOR_RED;
					}
				}

				strlcpy(iconName, unit.GetDBRec()->GetDefaultIcon()->GetIcon(), sizeof(iconName));

				button->IconButton()->SetIcon(iconName);

				button->UpdateData( &unit );

			}
			SetStackMode(TRUE);
		}




	}

}

sint32 BattleOrderBox::ToggleStackDisplay( )
{
	m_stackDisplay = !m_stackDisplay;
	SetStackMode( m_stackDisplay );
	return 0;
}
