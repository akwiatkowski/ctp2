//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Message data
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
// HAVE_PRAGMA_ONCE
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Made GetMessageWindow const. (Oct 16th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __MESSAGEDATA_H__
#define __MESSAGEDATA_H__

#include <nlohmann/json.hpp>
#include <string>

#include "gs/gameobj/GameObj.h"
#include "gs/gameobj/ID.h"
#include "gs/world/MapPoint.h"

class SlicObject ;
class SlicButton;
template <class T> class PointerList;
class SlicEyePoint;
class MessageWindow;
class UnitDynamicArray;
class SlicSegment;

typedef sint32 AdvanceType;
typedef sint32 PLAYER_INDEX;

#define k_MAX_MSG_LEN	256

#define k_MAX_MESSAGES_KEPT	30

#include "user-robotcom/shared/diplomatictypes.h"

#if 0
enum MESSAGE_TYPE
	{
	MESSAGE_TYPE_EVENT,
	MESSAGE_TYPE_DIPLOMATIC,
	MESSAGE_TYPE_TRADE,
	MESSAGE_TYPE_MILITARY,
	MESSAGE_TYPE_KNOWLEDGE,
	MESSAGE_TYPE_WONDER,
	MESSAGE_TYPE_NULL,
	MESSAGE_TYPE_MAX
	} ;
#endif
typedef sint32 MESSAGE_TYPE;

enum MESSAGE_EYEPOINT_STYLE
	{
	MESSAGE_EYEPOINT_STYLE_NONE,
	MESSAGE_EYEPOINT_STYLE_STANDARD,
	MESSAGE_EYEPOINT_STYLE_DROPDOWN,
	MESSAGE_EYEPOINT_STYLE_LIST
	} ;

enum MESSAGE_RESPONSE_STYLE
	{
	MESSAGE_RESPONSE_STYLE_NONE,
	MESSAGE_RESPONSE_STYLE_SELECT,
	MESSAGE_RESPONSE_STYLE_SUBMIT
	} ;

class MessageData : public GameObj
	{
	private:
		friend void to_json(nlohmann::json &j, MessageData const &m);
		friend void from_json(nlohmann::json const &j, MessageData &m);

		PLAYER_INDEX	m_owner,
						m_sender ;


		BOOL m_isRead;
		MESSAGE_TYPE	m_msgType ;
		MESSAGE_TYPE    m_msgSelectedType;

		sint32			m_timestamp ;
		AdvanceType     m_advance;

		BOOL m_advanceSet;
		sint32 m_expiration;
		BOOL m_isHelpBox;
		BOOL m_isAlertBox;
		BOOL m_isInstant;
		sint32 m_class;
		BOOL m_closeDisabled;
		BOOL m_isDiplomaticResponse;
		BOOL m_useDirector;

		MBCHAR  m_caption[k_MAX_MSG_LEN];


		std::string	m_text;
		UnitDynamicArray *m_cityList ;

		ID	m_request ;
		ID  m_tradeOffer;

		PointerList<SlicButton> *m_buttonList;
		PointerList<SlicEyePoint> *m_eyePoints;

		MessageWindow *m_window;

		SlicSegment *m_slicSegment;
		std::string m_title;


		friend class NetMessage;
		friend class SlicFrame;














	public:
		MessageData(const ID id, sint32 currentYear);
		MessageData(const ID id, const PLAYER_INDEX owner, const PLAYER_INDEX sender, const MESSAGE_TYPE type, MBCHAR *s, sint32 currentYear);
		MessageData(const ID id, MessageData *copy);
		~MessageData() override;

		PLAYER_INDEX GetOwner() const { return (m_owner) ; }
		void SetOwner(PLAYER_INDEX o) { m_owner = o; }

		PLAYER_INDEX GetSender() const { return (m_sender) ; }
		MESSAGE_TYPE GetMsgType() const { return (m_msgType) ; }
		void SetMsgType(MESSAGE_TYPE type) { m_msgType = type; }
		MESSAGE_TYPE GetSelectedMsgType() const { return m_msgSelectedType; }
		void SetSelectedMsgType(MESSAGE_TYPE type) { m_msgSelectedType = type; }

		void ToString(MBCHAR *s) ;
		MBCHAR *GetMsgText() { return m_text.empty() ? nullptr : &m_text[0]; }





		void SetMsgText(MBCHAR const * s);

		const MBCHAR *GetMsgCaption() const { return m_caption; }
		void SetMsgCaption(const MBCHAR *caption);

		void SetDuration(sint32 duration, sint32 currentRound);
		sint32 GetExpiration() const;

		MESSAGE_RESPONSE_TYPE Reject() ;
		MESSAGE_RESPONSE_TYPE Accept() ;

		MESSAGE_EYEPOINT_STYLE GetEyePointStyle();
		const MBCHAR *GetEyePointName( sint32 index );
		BOOL GetEyePointMapPosition( sint32 index, MapPoint &point );

		MESSAGE_RESPONSE_STYLE GetResponseStyle();

		void SetMessageWindow( MessageWindow *window ) { m_window = window; }
		MessageWindow *GetMessageWindow( ) const { return m_window; }
		void KillMessageWindow( );


		void IgnoreMessage( );

		const MBCHAR *GetMsgOpenSound( ) { return nullptr; }
		const MBCHAR *GetMsgSound( ) { return nullptr; }


		sint32 GetNumEyePoints() const;
		void EyePointCallback( sint32 index );
		void EyeDropdownCallback( sint32 index );

		const MBCHAR *GetSubmitString() { return nullptr; }
		const MBCHAR *GetTipText() { return nullptr; }

		BOOL GetGreatLibraryButton( ) { return FALSE; }
		void GreatLibraryCallback( ) { ; }

		sint32 GetTimeStamp( ) { return m_timestamp; }

		void AddButton(SlicButton *button);
		sint32 GetNumButtons() const;
		SlicButton *GetButton(sint32 index);
		SlicButton *GetCloseEvent();

		void AddEyePoint(SlicEyePoint *eyePoint);
		SlicEyePoint *GetEyePoint(sint32 index);

		void Dump(const sint32 i) ;

		void SetSelectedAdvance(AdvanceType adv);
		AdvanceType GetSelectedAdvance() const;

		void SetIsHelpBox() { m_isHelpBox = TRUE; }
		BOOL IsHelpBox() const { return m_isHelpBox; }

		void SetIsAlertBox() { m_isAlertBox = TRUE; }
		BOOL IsAlertBox() const { return m_isAlertBox; }

		MessageData *GetGreater() { return (MessageData*)m_greater; }
		MessageData *GetLesser() { return (MessageData*)m_lesser; }

		void SetIsInstantMessage() { m_isInstant = TRUE; }
		BOOL IsInstantMessage() const { return m_isInstant; }

		BOOL IsRead() const { return m_isRead; }
		void SetRead();

		void SetSlicSegment(SlicSegment *segment) { m_slicSegment = segment; }
		const SlicSegment *GetSlicSegment() { return m_slicSegment; }

		void SetClass(sint32 mclass) { m_class = mclass; }
		sint32 GetClass() const { return m_class; }

		MBCHAR *GetTitle() { return m_title.empty() ? nullptr : &m_title[0]; }
		void SetTitle(MBCHAR *title);

		void DisableClose(BOOL dis) { m_closeDisabled = dis; }
		BOOL IsCloseDisabled() const { return m_closeDisabled; }

		void SetIsDiplomaticResponse() { m_isDiplomaticResponse = TRUE; }
		BOOL IsDiplomaticResponse() const { return m_isDiplomaticResponse; }

		void SetUseDirector() { m_useDirector = TRUE; }
		BOOL UseDirector() const { return m_useDirector; }

		void NotifySlicReload();
	} ;

#endif
