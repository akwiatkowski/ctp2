#include "ctp/c3.h"
#include <memory>

#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"

#include "ui/aui_ctp2/c3_button.h"

#include "ui/netshell/allinonewindow.h"
#include "ui/interface/spnewgametribescreen.h"

#include "ui/netshell/ns_tribes.h"

#include "ui/netshell/ns_item.h"


ns_ListItem::ns_ListItem(
	AUI_ERRCODE *retval,
	const MBCHAR *name,
	MBCHAR *ldlBlock )
	:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(name, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE ns_ListItem::InitCommonLdl(
	const MBCHAR *name,
	MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Name");
	auto subItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block);
	AddChild(subItem.get());

	subItem->SetText( name );

	Update();
	subItem.release();
	return AUI_ERRCODE_OK;
}


ns_HPlayerItem::ns_HPlayerItem(
	AUI_ERRCODE *retval,
	void *player,
	BOOL isAI,
	MBCHAR *ldlBlock)
	:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(player, isAI, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

ns_HPlayerItem::~ns_HPlayerItem()
{
}

AUI_ERRCODE ns_HPlayerItem::InitCommonLdl(
	void *player,
	BOOL isAI,
	MBCHAR *ldlBlock)
{
	m_player = player;
	m_isAI = isAI;

	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;









	SetBlindness( TRUE );
	SetImageBltFlag( AUI_IMAGEBASE_BLTFLAG_CHROMAKEY );

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "launched");
	m_launchedItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block);
	m_launchedItem->SetImageBltFlag( AUI_IMAGEBASE_BLTFLAG_CHROMAKEY );
	m_launchedItem->SetBlindness( TRUE );
	AddChild(m_launchedItem.get());

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "name");
	m_nameItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block);
	m_nameItem->SetBlindness( TRUE );
	AddChild(m_nameItem.get());

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "ping");
	m_pingItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block);
	m_pingItem->SetBlindness( TRUE );
	AddChild(m_pingItem.get());


	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "tribe");
	m_tribeItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block);

	aui_Control::ControlActionCallback TribesButtonCallback;
	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "tribe.button");
	m_tribeButton = std::make_unique<c3_Button>(
		&retval,
		aui_UniqueId(),
		block,
		TribesButtonCallback,
		this );

	m_tribeItem->AddChild(m_tribeButton.get());

	AddChild(m_tribeItem.get());




















	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "civpoints");
	m_civpointsItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(),block);

	aui_Control::ControlActionCallback CivPointsButtonCallback;
	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "civpoints.button");
	m_civpointsButton = std::make_unique<c3_EditButton>(
		&retval,
		aui_UniqueId(),
		block,
		CivPointsButtonCallback,
		this );

	m_civpointsItem->AddChild(m_civpointsButton.get());

	AddChild(m_civpointsItem.get());

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "pwpoints");
	m_pwpointsItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(),block);

	aui_Control::ControlActionCallback PwPointsButtonCallback;
	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "pwpoints.button");
	m_pwpointsButton = std::make_unique<c3_EditButton>(
		&retval,
		aui_UniqueId(),
		block,
		PwPointsButtonCallback,
		this );

	m_pwpointsItem->AddChild(m_pwpointsButton.get());

	AddChild(m_pwpointsItem.get());

	Update();

	return AUI_ERRCODE_OK;
}


void ns_HPlayerItem::SetTribe( sint32 tribe )
{
	m_tribeButton->SetText( nstribes_Get()->GetStrings()->GetString( tribe ) );

	AllinoneWindow *w = allinonewindow_Get();
	if ( !IsAI() && w->IsMine( GetPlayer() ) )
		spnewgametribescreen_setTribeIndex(
			tribe - 1,
			strlen( w->m_lname ) ? w->m_lname : nullptr );
}
















void ns_HPlayerItem::SetCivpoints( sint32 civpoints )
{
	aui_Control::ControlActionCallback *actionFunc =
		m_civpointsButton->GetActionFunc();
	void *cookie = m_civpointsButton->GetCookie();

	m_civpointsButton->SetActionFuncAndCookie( nullptr, nullptr );
	m_civpointsButton->SetValue( civpoints );
	m_civpointsButton->SetActionFuncAndCookie( actionFunc, cookie );
}


void ns_HPlayerItem::SetPwpoints( sint32 pwpoints )
{
	aui_Control::ControlActionCallback *actionFunc =
		m_pwpointsButton->GetActionFunc();
	void *cookie = m_pwpointsButton->GetCookie();

	m_pwpointsButton->SetActionFuncAndCookie( nullptr, nullptr );
	m_pwpointsButton->SetValue( pwpoints );
	m_pwpointsButton->SetActionFuncAndCookie( actionFunc, cookie );
}
