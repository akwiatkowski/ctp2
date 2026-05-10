#pragma once
#ifndef __IC3TRADE_H__
#define __IC3TRADE_H__

#include <OBJBASE.H>

DEFINE_GUID(CLSID_IC3TradeOffer,
			0xcfed4800,
			0x6f1b,
			0x11d2,
			0x8d, 0x13, 0x0, 0x60, 0x97, 0xde, 0x50, 0xd5);

#undef INTERFACE
#define INTERFACE IC3TradeOffer

typedef sint32 PLAYER_INDEX;
#include "gs/gameobj/TradeRoute.h"

DECLARE_INTERFACE_(IC3TradeOffer, IUnknown)
{
	STDMETHOD_(uint32, GetID) (THIS) PURE;
	STDMETHOD_(PLAYER_INDEX, GetOwner) (THIS) PURE;
	STDMETHOD_(uint32, GetFromCity) (THIS) PURE;
	STDMETHOD_(ROUTE_TYPE, GetOfferType) (THIS) PURE;
	STDMETHOD_(sint32, GetOfferResource) (THIS) PURE;
	STDMETHOD_(sint32, GetAskingGold) (THIS) PURE;

	STDMETHOD_(void, Accept) (THIS_ uint32 destCity) PURE;
};

#endif
