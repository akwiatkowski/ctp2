//----------------------------------------------------------------------------
// Modern-asset atlas: manifest + decoded RGBA atlas image, with frame-rect
// lookup (P11 Stage 2 B1). Ties ModernSpriteManifest (the validated JSON shape)
// to png_load (the decoded pixels). Does not touch rendering — it is the data
// layer the modern-first sprite draw route consumes.
//----------------------------------------------------------------------------
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __MODERNSPRITEATLAS_H__
#define __MODERNSPRITEATLAS_H__

#include "gfx/spritesys/ModernSpriteManifest.h"

#include <cstdint>
#include <string>
#include <vector>

class ModernSpriteAtlas
{
public:
	// Load the manifest at manifestPath and the RGBA atlas PNG it names
	// (resolved relative to the manifest's own directory). Returns nullptr on
	// any parse / image / consistency error, with a human-readable reason in
	// error. Caller owns the returned object.
	static ModernSpriteAtlas * Load(char const * manifestPath, std::string & error);

	int Width()  const { return m_width;  }
	int Height() const { return m_height; }
	// Tightly-packed RGBA (row-major, top-down, 4 bytes/pixel); size == W*H*4.
	std::vector<uint8_t> const & Rgba() const { return m_rgba; }
	ModernSpriteManifest const & Manifest() const { return m_manifest; }

	// Find the atlas rect for a given action / facing / frame. Returns nullptr
	// if there is no such action or frame. Action match is case-sensitive on
	// the manifest name (e.g. "MOVE", "IDLE").
	ModernSpriteRect const * FindRect(char const * action, int facing, int frame) const;

	// Composite the frame (action/facing/frame) into destSurface with its
	// top-left at (destX, destY). The atlas carries binary alpha (opaque or
	// fully transparent), so this is a chroma-key-style copy: transparent
	// atlas pixels are skipped, opaque ones written (expanded to the dest
	// depth). Clipped to the surface. Returns false if the frame is unknown or
	// the surface can't be locked.
	//
	// The three mutually-exclusive per-pixel draw flags the legacy RLE draw
	// applies (k_BIT_DRAWFLAGS_TRANSPARENCY / _FOGGED / _DESATURATED, from
	// Sprite.h) are honoured here for parity: TRANSPARENCY alpha-blends the
	// sprite over the destination by `transparency`, FOGGED shadows it, and
	// DESATURATED greys it. On the 32-bit destination the effects run in full
	// 8888 (pixelutils_*8888) straight from the atlas RGB — no 565 round-trip,
	// so the modern path keeps its extra colour fidelity rather than collapsing
	// back to the legacy 16-bit precision; the 16-bit branch uses the 565
	// helpers to match that surface's depth. Outline/feathering remain
	// legacy-only.
	//
	// When `mirror` is set the frame is drawn horizontally flipped (source
	// columns read right-to-left). The atlas only stores facings 0..4; the
	// caller reflects facings 5..7 onto their stored counterpart and sets
	// `mirror` — the same trick FacedSprite uses for reversed facings.
	bool Blit(class aui_Surface * destSurface, char const * action, int facing,
	          int frame, int destX, int destY,
	          uint16 transparency = 0, uint16 flags = 0, bool mirror = false) const;

private:
	ModernSpriteManifest m_manifest;
	std::vector<uint8_t> m_rgba;
	int                  m_width  = 0;
	int                  m_height = 0;
};

// True when the modern-first atlas sprite path is opted into via the
// CTP2_MODERN_SPRITES environment variable (unset/"0" => legacy path). The
// legacy (now 32-bit) render stays the default so this cannot regress it.
bool ModernSpritesEnabled();

// Given a legacy sprite filename ("GU04.SPR", "GX22.SPR", ...), return the path
// to its generated atlas manifest under ~/.ctp2/assets/current/<base>.json, or
// an empty string if the modern cache has no manifest for it. The "current"
// pointer is maintained by tools/assets/spr_export.py --modern-assets.
std::string ModernAssetManifestPath(char const * spriteFileName);

// Load the generated atlas for `spriteFileName` into `slot` when the modern
// path is enabled (CTP2_MODERN_SPRITES) and a manifest exists; otherwise leave
// `slot` null so the legacy RLE draw is used. Never fatal — any failure just
// leaves `slot` null. Shared by the Unit / Good / Effect sprite groups.
void ModernSpriteLoadIfEnabled(std::unique_ptr<ModernSpriteAtlas> & slot,
                               char const * spriteFileName);

// Draw a NON-FACED sprite frame (goods, effects) from `atlas`, replicating
// Sprite::DrawDirect's hot-point placement and facing>=5 horizontal reversal.
// (hotX,hotY) is the sprite's hot point. Returns false — caller should fall
// back to the legacy draw — when the frame is absent or an unsupported flag
// (k_BIT_DRAWFLAGS_ADDITIVE, which the binary-alpha atlas blit cannot blend)
// is set. Unit sprites use their own faced path in UnitSpriteGroup instead.
bool ModernSpriteDrawUnfaced(ModernSpriteAtlas const & atlas, class aui_Surface * surf,
                             char const * action, int frame, int drawX, int drawY,
                             int facing, int hotX, int hotY,
                             uint16 transparency, uint16 flags);

#endif // __MODERNSPRITEATLAS_H__
