#ifndef __AUI_HYPERTEXTBOX_H__
#define __AUI_HYPERTEXTBOX_H__

#include <memory>

#include "ui/aui_common/aui_hypertextbase.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_ranger.h"


#define k_AUI_HYPERTEXTBOX_LDL_RANGERY			"rangery"
#define k_AUI_HYPERTEXTBOX_LDL_ALWAYSRANGER		"alwaysranger"


enum AUI_HYPERTEXTBOX_ACTION
{
	AUI_HYPERTEXTBOX_ACTION_FIRST = 0,
	AUI_HYPERTEXTBOX_ACTION_NULL = 0,
	AUI_HYPERTEXTBOX_ACTION_LAST
};


class aui_HyperTextBox : public aui_Control, public aui_HyperTextBase
{
public:

	aui_HyperTextBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	aui_HyperTextBox(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~aui_HyperTextBox() override;

protected:
	aui_HyperTextBox() : aui_Control(), aui_HyperTextBase() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateRanger( MBCHAR *ldlBlock = nullptr );

public:
	AUI_ERRCODE	Resize( sint32 width, sint32 height ) override;

	AUI_ERRCODE Show( ) override;

	aui_Ranger *GetRanger( ) const { return m_ranger.get(); }

	sint32		GetRangerSize( ) const { return m_rangerSize; }
	AUI_ERRCODE	SetRangerSize( sint32 rangerSize )
		{ m_rangerSize = rangerSize; return RepositionRanger(); }
	void		SetAlwaysRanger( BOOL always ) { m_alwaysRanger = always; }

	AUI_ERRCODE	RangerMoved( );

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	AUI_ERRCODE	SetHyperText(
		const MBCHAR *hyperText,
		uint32 maxlen = 0xffffffff ) override;
	AUI_ERRCODE	AppendHyperText( const MBCHAR *hyperText ) override;

	sint32 GetVirtualHeight( ) const { return m_virtualHeight; }

protected:
	AUI_ERRCODE AddHyperStatics( const MBCHAR *hyperText ) override;

	AUI_ERRCODE	RepositionRanger( );

	std::unique_ptr<aui_Ranger>	m_ranger;
	sint32		m_rangerSize;
	BOOL		m_alwaysRanger;

	sint32		m_virtualHeight;
	POINT		m_curStaticPos;
};


aui_Control::ControlActionCallback HyperTextBoxRangerActionCallback;

#endif
