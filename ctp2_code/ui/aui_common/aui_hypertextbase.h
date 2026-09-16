#ifndef __AUI_HYPERTEXTBASE_H__
#define __AUI_HYPERTEXTBASE_H__

#include <string>

#include "ui/aui_common/tech_wllist.h"

class aui_Static;
class aui_Surface;
class aui_BitmapFont;


#define k_AUI_HYPERTEXTBASE_DEFAULT_MAXLEN		1024
#define k_AUI_HYPERTEXTBOX_LDL_MAXSTATICS		100


#define k_AUI_HYPERTEXTBASE_LDL_TEXT			"hypertext"
#define k_AUI_HYPERTEXTBASE_LDL_MAXLEN			"hypermaxlength"


class aui_HyperTextBase
{
public:

	aui_HyperTextBase(
		AUI_ERRCODE *retval,
		MBCHAR const *ldlBlock );
	aui_HyperTextBase(
		AUI_ERRCODE *retval,
		MBCHAR const *hyperText,
		uint32 hyperMaxLen );
	virtual ~aui_HyperTextBase();

protected:
	aui_HyperTextBase() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon(
		MBCHAR const *hyperText,
		uint32 hyperMaxLen );

public:
	const MBCHAR *GetHyperText( ) const { return m_hyperText.c_str(); }
	virtual AUI_ERRCODE	SetHyperText(
		const MBCHAR *hyperText,
		uint32 maxlen = 0xffffffff );
	virtual AUI_ERRCODE	AppendHyperText( const MBCHAR *hyperText );

protected:
	virtual AUI_ERRCODE AddHyperStatics( const MBCHAR *hyperText );
	void RemoveHyperStatics( );
	static aui_Static *CreateHyperStatic(
		const MBCHAR *string,
		uint32 len,
		MBCHAR ttffile[ MAX_PATH + 1 ],
		sint32 pointSize,
		sint32 bold,
		sint32 italic,
		COLORREF color,
		sint32 underline,
		BOOL shadow,
		COLORREF shadowColor,
		uint32 flags );

	AUI_ERRCODE DrawThisHyperText(
		aui_Surface *destSurf,
		RECT *clipRect,
		sint32 x = 0,
		sint32 y = 0 );

	std::string	m_hyperText;
	uint32	m_hyperMaxLen;
	uint32	m_hyperCurLen;

	tech_WLList<aui_Static *>	*m_hyperStaticList;

	MBCHAR		m_hyperTtffile[ MAX_PATH + 1 ];
	sint32		m_hyperPointSize;
	sint32		m_hyperBold;
	sint32		m_hyperItalic;
	COLORREF	m_hyperColor;
	COLORREF	m_hyperColorOld;
	sint32		m_hyperUnderline;
	BOOL		m_hyperShadow;
	COLORREF	m_hyperShadowColor;
	uint32		m_hyperFlags;
};

#endif
