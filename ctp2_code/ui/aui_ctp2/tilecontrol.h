#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TILECONTROL_H__
#define __TILECONTROL_H__


class aui_Surface;
class MapPoint;


class TileControl : public aui_Control
{
public:

	TileControl(AUI_ERRCODE *retval,
						uint32 id,
						MBCHAR const *ldlBlock );
	TileControl(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~TileControl() override = default;

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	void SetMouseTile(const MapPoint &p) { m_currentTile = p; };

protected:
	MapPoint			m_currentTile;

	sint32 DrawTile(aui_Surface *surface,
			sint32 i,
			sint32 j,
			sint32 x,
			sint32 y);
};

#endif
