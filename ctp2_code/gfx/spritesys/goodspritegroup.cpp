//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Good sprite handling
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
// - Moved common SpriteGroup member handling to SpriteGroup.
// - Prevent crashes on failed file operations.
// - Fixed memory leaks.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/spritesys/GoodSpriteGroup.h"

#include "ctp/ctp2_utils/c3errors.h"
#include <memory>         // std::unique_ptr
#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_utils/primitives.h"
#include "gfx/spritesys/FacedSprite.h"
#include "gfx/spritesys/Sprite.h"
#include "gfx/spritesys/screenmanager.h"
#include "gs/fileio/CivPaths.h"
#include "ctp/ctp2_utils/c3files.h"
#include "gfx/spritesys/SpriteFile.h"
#include "gfx/spritesys/Anim.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"   // P11 modern-first atlas path
#include "ui/aui_common/aui_surface.h"         // aui_Surface::BitsPerPixel
#include "ui/aui_sdl/aui_sdl.h"
#include "gs/fileio/Token.h"


// Out-of-line so the unique_ptr<ModernSpriteAtlas> member is created/destroyed
// where the type is complete.
GoodSpriteGroup::GoodSpriteGroup(GROUPTYPE type) : SpriteGroup(type) {}
GoodSpriteGroup::~GoodSpriteGroup()
{
	if (m_modernAtlas)
		aui_SDL::ReleaseSpriteAtlasTexture(m_modernAtlas.get());
}

void GoodSpriteGroup::Draw(GOODACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	Assert(action > GOODACTION_NONE &&
			action < GOODACTION_MAX);
	if(action <= GOODACTION_NONE || action >= GOODACTION_MAX) {
		return;
	}

	if (m_sprites[action] == nullptr) return;

	if ((frame < 0) ||
        (static_cast<size_t>(frame) >= m_sprites[action]->GetNumFrames())
       )
    {
		frame = 0;
    }

	m_sprites[action]->SetCurrentFrame((uint16)frame);

	// Modern-first atlas draw on the interactive path (into the ScreenManager's
	// already-locked surface); falls back to the legacy RLE draw below.
	if (m_modernAtlas && outlineColor == 0)   // outline requested -> legacy
	{
		aui_Surface * surf = screenmanager_Get()->GetSurface();
		uint8 *       base = screenmanager_Get()->GetSurfBase();
		if (surf && base)
		{
			POINT const hp = m_sprites[action]->GetHotPoint();
			if (ModernSpriteDrawUnfacedLocked(*m_modernAtlas, base,
			        screenmanager_Get()->GetSurfPitch(), screenmanager_Get()->GetSurfWidth(),
			        screenmanager_Get()->GetSurfHeight(), surf->BitsPerPixel() == 32,
			        "IDLE", frame, drawX, drawY, facing, hp.x, hp.y, scale, transparency, flags))
			{
				return;
			}
		}
	}

	m_sprites[action]->Draw(drawX, drawY, facing, scale, transparency, outlineColor, flags);
}

void GoodSpriteGroup::DrawDirect(aui_Surface *surf, GOODACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	Assert(action > GOODACTION_NONE &&
			action < GOODACTION_MAX);
	if(action <= GOODACTION_NONE || action >= GOODACTION_MAX) {
		return;
	}

	if (m_sprites[action] == nullptr) return;

	m_sprites[action]->SetCurrentFrame((uint16)frame);

	// Modern-first atlas draw (any zoom); falls back to the legacy RLE draw
	// when the atlas lacks the frame or an unsupported flag is set.
	if (m_modernAtlas && outlineColor == 0)   // outline requested -> legacy
	{
		POINT const hp = m_sprites[action]->GetHotPoint();
		if (ModernSpriteDrawUnfaced(*m_modernAtlas, surf, "IDLE", frame, drawX, drawY,
		                            facing, hp.x, hp.y, scale, transparency, flags))
		{
			return;
		}
	}

	m_sprites[action]->DrawDirect(surf, drawX, drawY, facing, scale, transparency, outlineColor, flags);
}

char const * GoodSpriteGroup::s_gpuFallbackReason = "good-sprite";

bool GoodSpriteGroup::AddGpuSpriteQuad(GOODACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, Pixel16 outlineColor, uint16 flags)
{
	// One reason per rejection. A single "good-sprite" told you a good fell back
	// but not which condition did it, which is most of the diagnosis.
	if (!m_modernAtlas)              { s_gpuFallbackReason = "good-no-atlas";    return false; }
	if (action <= GOODACTION_NONE || action >= GOODACTION_MAX)
	                                 { s_gpuFallbackReason = "good-bad-action";  return false; }
	if (outlineColor != 0)           { s_gpuFallbackReason = "good-outline";     return false; }
	if (flags & ~(k_DRAWFLAGS_NORMAL | k_BIT_DRAWFLAGS_FOGGED)) { s_gpuFallbackReason = "good-drawflags";   return false; }

	ModernSpriteRect const * r = m_modernAtlas->FindRect("IDLE", 0, frame);
	if (!r)                          { s_gpuFallbackReason = "good-no-rect";     return false; }

	SDL_Texture * texture = aui_SDL::EnsureSpriteAtlasTexture(m_modernAtlas.get());
	if (!texture)                    { s_gpuFallbackReason = "good-no-texture";  return false; }

	POINT const hp = GetHotPoint(action);
	bool const reversed = facing >= 5;
	int const destX = reversed ? (drawX - static_cast<int>((r->w - hp.x) * scale))
	                         : (drawX - static_cast<int>(hp.x * scale));
	int const destY = drawY - static_cast<int>(hp.y * scale);

	aui_SDL::GpuSpriteQuad q;
	q.texture = texture;
	q.sx = r->x; q.sy = r->y; q.sw = r->w; q.sh = r->h;
	q.dx = destX; q.dy = destY;
	q.dw = static_cast<int>(r->w * scale);
	q.dh = static_cast<int>(r->h * scale);
	q.mirror = reversed;
	q.alpha = 255;
    // Braced so the unconditional AddSpriteQuad below cannot be misread
    // as part of this fogged-tint guard.
    if (flags & k_BIT_DRAWFLAGS_FOGGED) {
        q.red = q.green = q.blue = 128;
    }
	aui_SDL::AddSpriteQuad(q);
	return true;
}

POINT GoodSpriteGroup::GetHotPoint(GOODACTION action)
{
	POINT nullPoint = {0,0};
	if(action <= GOODACTION_NONE || action >= GOODACTION_MAX) {
		return nullPoint;
	}

	return m_sprites[action] ? m_sprites[action]->GetHotPoint() : nullPoint;
}

void GoodSpriteGroup::LoadBasic(MBCHAR const * filename)
{
	auto file = std::make_unique<SpriteFile>(filename);

	SPRITEFILETYPE	type;
	if (SPRITEFILEERR_OK == file->Open(&type))
	{
		auto result = file->ReadBasic(this);
		file->CloseRead();
		m_loadType = result == SPRITEFILEERR_OK ? LOADTYPE_BASIC : LOADTYPE_NONE;
	}
	ModernSpriteLoadIfEnabled(m_modernAtlas, filename);
}

void GoodSpriteGroup::LoadFull(MBCHAR const * filename)
{
	auto file = std::make_unique<SpriteFile>(filename);

	SPRITEFILETYPE	type;
	if (SPRITEFILEERR_OK == file->Open(&type))
	{
		auto result = file->ReadFull(this);
		file->CloseRead();
		m_loadType = result == SPRITEFILEERR_OK ? LOADTYPE_FULL : LOADTYPE_NONE;
	}
	ModernSpriteLoadIfEnabled(m_modernAtlas, filename);
}

void GoodSpriteGroup::Save(MBCHAR const * filename, unsigned int version_id, unsigned int compression_mode)
{
	auto file = std::make_unique<SpriteFile>(filename);

	if (SPRITEFILEERR_OK ==
			file->Create(SPRITEFILETYPE_GOOD, version_id, compression_mode)
       )
	{
		file->Write(this);
		file->CloseWrite();
	}
}

void GoodSpriteGroup::DeallocateStorage()
{
    for (int i = GOODACTION_IDLE; i < GOODACTION_MAX; i++)
    {
	    m_sprites[i].reset();
    }
}

void GoodSpriteGroup::DeallocateFullLoadAnims()
{
    for (int i = GOODACTION_IDLE; i < GOODACTION_MAX; i++)
    {
        m_anims[i].reset();
    }
}

void GoodSpriteGroup::DrawText(sint32 x, sint32 y, MBCHAR const * s)
{
	primitives_DrawText(screenmanager_Get()->GetSurface(), x, y, s, 0, false);
}

sint32 GoodSpriteGroup::Parse(uint16 id,GROUPTYPE group)
{
	MBCHAR			scriptName[k_MAX_NAME_LENGTH];
	char			prefixStr[80];

	snprintf(prefixStr, sizeof(prefixStr), ".%s%d%s", FILE_SEP, id, FILE_SEP);
	snprintf(scriptName, sizeof(scriptName), "GG%.2d.txt", id);

	auto theToken = std::make_unique<Token>(scriptName, C3DIR_SPRITES);

	if (!token_ParseKeywordNext(theToken.get(), TOKEN_GOOD_SPRITE)) {
		return k_NOT_GOOD;
	}

	printf("Good Processing '%s'\n", scriptName);

	std::unique_ptr<MBCHAR[]>    imageNames[k_MAX_NAMES];
	std::unique_ptr<MBCHAR[]>    shadowNames[k_MAX_NAMES];
	size_t		i;

    for (i = 0; i < k_MAX_NAMES; i++)
    {
        imageNames[i] =  std::make_unique<MBCHAR[]>(k_MAX_NAME_LENGTH);
        shadowNames[i] = std::make_unique<MBCHAR[]>(k_MAX_NAME_LENGTH);
    }

	if (!token_ParseAnOpenBraceNext(theToken.get())) return FALSE;

	sint32 tmp;
	if (!token_ParseValNext(theToken.get(), TOKEN_GOOD_SPRITE_IDLE, tmp)) return FALSE;
	if (tmp) {
		auto idleSprite = std::make_unique<Sprite>();
		Assert(idleSprite);
		if(!idleSprite) return FALSE;

		idleSprite->ParseFromTokens(theToken.get());

		printf(" [Idle");
		size_t numFrames = idleSprite->GetNumFrames();
		if (numFrames > k_MAX_NAMES)
			numFrames = k_MAX_NAMES;
		for(i=0; i<numFrames; i++) {
			MBCHAR			name[k_MAX_NAME_LENGTH];

			snprintf(name, sizeof(name), "%sGG%.2dS.%zu.tif", prefixStr, id, i+idleSprite->GetFirstFrame());
			// TODO(phase-2): strncpy → strlcpy — dst is char* or non-standard length, requires manual review
			strncpy(shadowNames[i].get(), name, k_MAX_NAME_LENGTH - 1);
			shadowNames[i][k_MAX_NAME_LENGTH - 1] = '\0';

			snprintf(name, sizeof(name), "%sGG%.2dA.%zu.tif", prefixStr, id, i+idleSprite->GetFirstFrame());
			// TODO(phase-2): strncpy → strlcpy — dst is char* or non-standard length, requires manual review
			strncpy(imageNames[i].get(), name, k_MAX_NAME_LENGTH - 1);
			imageNames[i][k_MAX_NAME_LENGTH - 1] = '\0';
		}

		{ MBCHAR *rawImageNames[k_MAX_NAMES], *rawShadowNames[k_MAX_NAMES];
		for (size_t ni = 0; ni < k_MAX_NAMES; ++ni) { rawImageNames[ni] = imageNames[ni].get(); rawShadowNames[ni] = shadowNames[ni].get(); }
		idleSprite->Import(numFrames, rawImageNames, rawShadowNames); }
		m_sprites[GOODACTION_IDLE] = std::move(idleSprite);
		printf("]\n");

		auto idleAnim = std::make_unique<Anim>();
		idleAnim->ParseFromTokens(theToken.get());
		m_anims[GOODACTION_IDLE] = std::move(idleAnim);
	}


    for (i = 0; i < k_MAX_NAMES; i++)
    {
        imageNames[i].reset();
        shadowNames[i].reset();
    }

	return TRUE;
}

void GoodSpriteGroup::ExportScript(MBCHAR const * name)
{
	extern TokenData	g_allTokens[];

	FILE * file = fopen(name, "w");
	if (!file) {
		c3errors_ErrorDialog("Sprite Export", "Could not open '%s' for writing.", name);
		return;
	}

	char timebuf[100];
	time_t ltime;
	time(&ltime);
	struct tm * now = localtime(&ltime);
	strftime(timebuf, 100, "%I:%M%p %m/%d/%Y", now);

	fprintf(file, "#\n");
	fprintf(file, "# This file was automatically generated by Sprite-Test\n");
	fprintf(file, "#\n");
	fprintf(file, "# %s\n", timebuf);
	fprintf(file, "#\n\n");

	fprintf(file, "%d # %s\n\n", 0, name);
	fprintf(file, "%s\n", g_allTokens[TOKEN_GOOD_SPRITE].keyword);
	fprintf(file, "{\n");

	ExportSpriteGroup(file,(GAME_ACTION)GOODACTION_IDLE,TOKEN_UNIT_SPRITE_IDLE,TOKEN_MAX);










	fprintf(file, "}\n");

	fclose(file);
}
