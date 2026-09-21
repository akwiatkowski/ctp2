#ifndef __NS_ITEM_H__
#define __NS_ITEM_H__

#include "ui/aui_common/aui_item.h"
#include <memory>
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ui.h"

#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_listitem.h"

#include "ui/netshell/netfunc.h"
#include "ui/netshell/ns_aiplayersetup.h"

#define k_NS_ITEM_MAXTEXT			100
#define k_NS_ITEM_DEFAULTWIDTH		60
#define k_NS_ITEM_DEFAULTHEIGHT		25

#include "ui/aui_ctp2/c3_button.h"




class ns_ListItem : public c3_ListItem
{
public:
	ns_ListItem(
		AUI_ERRCODE *retval,
		const MBCHAR *name,
		MBCHAR *ldlBlock);
	~ns_ListItem() override = default;

	void Update() override {}
	sint32 Compare(c3_ListItem *item2, uint32 column) override { return 0; }

protected:
	ns_ListItem() : c3_ListItem() {}

	AUI_ERRCODE InitCommonLdl(
		const MBCHAR *name,
		MBCHAR *ldlBlock);
};


class ns_HPlayerItem : public c3_ListItem
{
public:
	ns_HPlayerItem(
		AUI_ERRCODE *retval,
		void *player,
		BOOL isAI,
		MBCHAR *ldlBlock);

	~ns_HPlayerItem() override;

	void Update() override {}
	sint32 Compare(c3_ListItem *item2, uint32 column) override { return 0; }

	NETFunc::Player *GetPlayer( ) const
	{ return IsAI() ? nullptr : (NETFunc::Player *)m_player; }
	nf_AIPlayer *GetAIPlayer( ) const
	{ return IsAI() ? (nf_AIPlayer *)m_player : nullptr; }

	BOOL IsAI( ) const { return m_isAI; }

	aui_Control *GetHostItem( ) { return this; }
	aui_Control *GetLaunchedItem( ) const { return m_launchedItem.get(); }
	aui_Control *GetNameItem( ) const { return m_nameItem.get(); }
	aui_Control *GetPingItem( ) const { return m_pingItem.get(); }

	aui_Control *GetTribeItem( ) const { return m_tribeItem.get(); }
	c3_Button *GetTribeButton( ) const
	{ return m_tribeButton.get(); }
	void SetTribe( sint32 tribe );





	aui_Control *GetCivpointsItem( ) const { return m_civpointsItem.get(); }
	c3_EditButton *GetCivpointsButton( ) const { return m_civpointsButton.get();}
	void SetCivpoints( sint32 civpoints );

	aui_Control *GetPwpointsItem( ) const { return m_pwpointsItem.get(); }
	c3_EditButton *GetPwpointsButton( ) const { return m_pwpointsButton.get();}
	void SetPwpoints( sint32 pwpoints );

protected:
	ns_HPlayerItem() : c3_ListItem() {}

	AUI_ERRCODE InitCommonLdl(
		void *player,
		BOOL isAI,
		MBCHAR *ldlBlock);

	void *m_player;
	BOOL m_isAI;

	std::unique_ptr<c3_Static>	m_launchedItem;
	std::unique_ptr<c3_Static>	m_nameItem;
	std::unique_ptr<c3_Static>	m_pingItem;

	std::unique_ptr<c3_Static>	m_tribeItem;
	std::unique_ptr<c3_Button>	m_tribeButton;




	std::unique_ptr<c3_Static>	m_civpointsItem;
	std::unique_ptr<c3_EditButton>	m_civpointsButton;

	std::unique_ptr<c3_Static>	m_pwpointsItem;
	std::unique_ptr<c3_EditButton>	m_pwpointsButton;
};




template<class T,class NetShellT>
class ns_Item : public aui_Item
{
public:

	ns_Item(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		T *object = NULL );
	ns_Item(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		T *object = NULL );
	~ns_Item() override;

protected:
	ns_Item() : aui_Item() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateNetShellObject( T *object );

public:
	NetShellT	*GetNetShellObject( ) const { return m_netShellT.get(); }

	AUI_ERRCODE	SetIcon( MBCHAR *icon );
	aui_Image	*GetIcon( ) const { return m_icon; }

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	std::unique_ptr<NetShellT>	m_netShellT;
	aui_Image	*m_icon;
};





template<class T,class NetShellT>
ns_Item<T,NetShellT>::ns_Item(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	T *object )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
	aui_Item( retval, id, ldlBlock )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl( ldlBlock );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateNetShellObject( object );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


template<class T,class NetShellT>
ns_Item<T,NetShellT>::ns_Item(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	T *object )
	:
	aui_Item( retval, id, x, y, width, height ),
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( NULL, k_NS_ITEM_MAXTEXT )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateNetShellObject( object );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


template<class T,class NetShellT>
AUI_ERRCODE ns_Item<T,NetShellT>::InitCommonLdl( MBCHAR *ldlBlock )
{

	return InitCommon();
}


template<class T,class NetShellT>
AUI_ERRCODE ns_Item<T,NetShellT>::InitCommon( )
{
	m_netShellT = nullptr;
	m_icon = nullptr;

	m_textflags = k_AUI_BITMAPFONT_DRAWFLAG_JUSTLEFT |
		k_AUI_BITMAPFONT_DRAWFLAG_VERTCENTER;





	return AUI_ERRCODE_OK;
}


template<class T,class NetShellT>
AUI_ERRCODE ns_Item<T,NetShellT>::CreateNetShellObject( T *object )
{
	if ( object )
	{
		m_netShellT = std::make_unique<NetShellT>( object );
		Assert( m_netShellT != nullptr );
		if ( !m_netShellT ) return AUI_ERRCODE_MEMALLOCFAILED;
	}

	return AUI_ERRCODE_OK;
}


template<class T,class NetShellT>
ns_Item<T,NetShellT>::~ns_Item()
{
	m_netShellT.reset();

	if ( m_icon )
	{
		aui_ui_Get()->UnloadImage( m_icon );
		m_icon = nullptr;
	}
}


template<class T,class NetShellT>
AUI_ERRCODE ns_Item<T,NetShellT>::SetIcon( MBCHAR *icon )
{
	aui_Image *prevImage = m_icon;

	if ( icon )
	{
		m_icon = aui_ui_Get()->LoadImage( icon );
		Assert( m_icon != nullptr );
		if ( !m_icon )
		{
			m_icon = prevImage;
			return AUI_ERRCODE_LOADFAILED;
		}
	}
	else
		m_icon = nullptr;

	if ( prevImage ) aui_ui_Get()->UnloadImage( prevImage );

	return AUI_ERRCODE_OK;
}





template <class T,class NetShellT>
AUI_ERRCODE ns_Item<T,NetShellT>::DrawThis(
	aui_Surface *surface,
	sint32 x,
	sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	InflateRect( &rect, -2, -2 );









	if ( m_icon )
	{




		RECT destRect = rect;










		RECT srcRect =
		{
			0,
			0,
			m_icon->TheSurface()->Width(),
			m_icon->TheSurface()->Height()
		};

		if ( destRect.left < destRect.right
		&&   destRect.top < destRect.bottom )
			aui_ui_Get()->TheBlitter()->Blt(
				surface,
				destRect.left,
				destRect.top,
				m_icon->TheSurface(),
				&srcRect,
				k_AUI_BLITTER_FLAG_CHROMAKEY );
	}

	DrawThisText(
		surface,
		&rect );

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}

#endif
