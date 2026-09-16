#ifndef __AUI_HEADER_H__
#define __AUI_HEADER_H__

#include "ui/aui_common/aui_switchgroup.h"
#include "ui/aui_common/aui_action.h"


#define k_AUI_HEADER_LDL_SWITCH		"switch"


class aui_Header : public aui_SwitchGroup
{
public:

	aui_Header(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock );
	aui_Header(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height );
	~aui_Header() override;

protected:
	aui_Header() : aui_SwitchGroup() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateSwitches( MBCHAR const *ldlBlock = nullptr );

public:
	AUI_ERRCODE	AddChild( aui_Region *child ) override;
	AUI_ERRCODE	RemoveChild( uint32 switchId ) override;

protected:
	AUI_ERRCODE	CalculateDimensions( );
	AUI_ERRCODE	RepositionSwitches( );
};


class aui_HeaderSwitchAction : public aui_Action
{
public:
	aui_HeaderSwitchAction(sint32 column)
    :   aui_Action  (),
        m_column    (column)
    { ; };

	void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	) override;

protected:
	sint32 m_column;
};

#endif
