//----------------------------------------------------------------------------
// See ModernSpriteAtlas.h. P11 Stage 2 B1.
//----------------------------------------------------------------------------
#include "ctp/c3.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"

#include "gfx/gfx_utils/png_load.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/Sprite.h"           // k_BIT_DRAWFLAGS_* per-pixel effect bits
#include "ui/aui_common/aui_surface.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace
{
    // Directory portion of a path (everything up to and including the last '/'),
    // or empty if the path has no separator. Used to resolve the atlas PNG,
    // which the manifest names relative to its own location.
    std::string DirOf(char const * path)
    {
        std::string const p(path ? path : "");
        std::string::size_type const slash = p.find_last_of("/\\");
        return (slash == std::string::npos) ? std::string() : p.substr(0, slash + 1);
    }

    // Composite one opaque source RGBA pixel (px) onto the dest pixel at
    // dstRow + dx, honouring the mutually-exclusive draw-flag effects (same
    // precedence as the legacy RLE draw: transparency, then fog, then
    // desaturate). bpp32 selects the full-fidelity 8888 path vs the 565 path
    // that matches a legacy 16-bit surface. Shared by Blit and BlitScaled.
    inline void WriteEffectPixel(uint8 * dstRow, int dx, uint8 const * px, bool bpp32,
                                 bool transp, bool fogged, bool desatur, uint16 transparency)
    {
        if (bpp32)
        {
            Pixel32 const src = 0xFF000000u | (static_cast<uint32>(px[0]) << 16)
                                            | (static_cast<uint32>(px[1]) << 8) | px[2];
            Pixel32 * const d = reinterpret_cast<Pixel32 *>(dstRow + dx * 4);
            if      (transp)  *d = pixelutils_BlendFast8888(src, *d, transparency);
            else if (fogged)  *d = pixelutils_Shadow8888(src);
            else if (desatur) *d = pixelutils_Desaturate8888(src);
            else              *d = src;
        }
        else
        {
            Pixel16 const src = static_cast<Pixel16>(
                ((px[0] >> 3) << 11) | ((px[1] >> 2) << 5) | (px[2] >> 3));
            Pixel16 * const d = reinterpret_cast<Pixel16 *>(dstRow + dx * 2);
            if      (transp)  *d = static_cast<Pixel16>(pixelutils_BlendFast_565(src, *d, transparency));
            else if (fogged)  *d = pixelutils_Shadow_565(src);
            else if (desatur) *d = pixelutils_Desaturate_565(src);
            else              *d = src;
        }
    }
}

ModernSpriteAtlas * ModernSpriteAtlas::Load(char const * manifestPath, std::string & error)
{
    auto atlas = std::make_unique<ModernSpriteAtlas>();

    if (!ModernSpriteManifestLoad(manifestPath, atlas->m_manifest, error))
        return nullptr;

    std::string const pngPath = DirOf(manifestPath) + atlas->m_manifest.atlasPng;
    if (!png_load_rgba(pngPath.c_str(), atlas->m_width, atlas->m_height, atlas->m_rgba))
    {
        error = "failed to load atlas image: " + pngPath;
        return nullptr;
    }

    // The manifest was validated against its declared atlas size; make sure the
    // actual PNG matches, otherwise every rect lookup would read out of bounds.
    if (atlas->m_width != atlas->m_manifest.atlasWidth
        || atlas->m_height != atlas->m_manifest.atlasHeight)
    {
        error = "atlas image size does not match manifest";
        return nullptr;
    }

    return atlas.release();
}

ModernSpriteRect const * ModernSpriteAtlas::FindRect(char const * action, int facing, int frame) const
{
    for (ModernSpriteAction const & a : m_manifest.actions)
    {
        if (a.name != action)
            continue;
        for (ModernSpriteFrame const & f : a.frames)
        {
            if (f.facing == facing && f.frame == frame)
                return &f.rect;
        }
    }
    return nullptr;
}

bool ModernSpriteAtlas::Blit(aui_Surface * destSurface, char const * action, int facing,
                             int frame, int destX, int destY,
                             uint16 transparency, uint16 flags, bool mirror) const
{
    if (!destSurface)
        return false;
    ModernSpriteRect const * r = FindRect(action, facing, frame);
    if (!r)
        return false;

    LPVOID lockedBits = nullptr;
    if (destSurface->Lock(nullptr, &lockedBits, 0) != AUI_ERRCODE_OK || !lockedBits)
        return false;
    uint8 * base = static_cast<uint8 *>(lockedBits);

    sint32 const pitch = destSurface->Pitch();
    sint32 const destW = destSurface->Width();
    sint32 const destH = destSurface->Height();
    bool   const bpp32 = destSurface->BitsPerPixel() == 32;

    // Which per-pixel effect to run (mutually exclusive, same precedence as the
    // legacy RLE draw in spritelow.cpp: transparency, then fog, then desaturate).
    bool const transp   = (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) != 0;
    bool const fogged   = !transp && (flags & k_BIT_DRAWFLAGS_FOGGED) != 0;
    bool const desatur  = !transp && !fogged && (flags & k_BIT_DRAWFLAGS_DESATURATED) != 0;

    for (int row = 0; row < r->h; ++row)
    {
        int const dy = destY + row;
        if (dy < 0 || dy >= destH)
            continue;
        uint8 const * srcRow = m_rgba.data()
            + (static_cast<size_t>(r->y + row) * m_width + r->x) * 4;
        uint8 *       dstRow = base + static_cast<size_t>(dy) * pitch;
        for (int col = 0; col < r->w; ++col)
        {
            int const dx = destX + col;
            if (dx < 0 || dx >= destW)
                continue;
            // Reversed facings sample the row right-to-left (horizontal flip).
            int const srcCol = mirror ? (r->w - 1 - col) : col;
            uint8 const * px = srcRow + static_cast<size_t>(srcCol) * 4;
            if (px[3] == 0)                 // binary alpha: transparent -> skip
                continue;
            WriteEffectPixel(dstRow, dx, px, bpp32, transp, fogged, desatur, transparency);
        }
    }

    destSurface->Unlock(base);
    return true;
}

bool ModernSpriteAtlas::BlitScaled(aui_Surface * destSurface, char const * action, int facing,
                                   int frame, int destX, int destY, int destW, int destH,
                                   uint16 transparency, uint16 flags, bool mirror) const
{
    if (!destSurface || destW <= 0 || destH <= 0)
        return false;
    ModernSpriteRect const * r = FindRect(action, facing, frame);
    if (!r || r->w <= 0 || r->h <= 0)
        return false;

    LPVOID lockedBits = nullptr;
    if (destSurface->Lock(nullptr, &lockedBits, 0) != AUI_ERRCODE_OK || !lockedBits)
        return false;
    uint8 * base = static_cast<uint8 *>(lockedBits);

    sint32 const pitch   = destSurface->Pitch();
    sint32 const surfW   = destSurface->Width();
    sint32 const surfH   = destSurface->Height();
    bool   const bpp32   = destSurface->BitsPerPixel() == 32;

    bool const transp   = (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) != 0;
    bool const fogged   = !transp && (flags & k_BIT_DRAWFLAGS_FOGGED) != 0;
    bool const desatur  = !transp && !fogged && (flags & k_BIT_DRAWFLAGS_DESATURATED) != 0;

    // Nearest-neighbour scale of the source rect into destW x destH, matching
    // the legacy DrawScaledLow stepping (integer source index per dest pixel).
    for (int row = 0; row < destH; ++row)
    {
        int const dy = destY + row;
        if (dy < 0 || dy >= surfH)
            continue;
        int const sy = row * r->h / destH;
        uint8 const * srcRow = m_rgba.data()
            + (static_cast<size_t>(r->y + sy) * m_width + r->x) * 4;
        uint8 *       dstRow = base + static_cast<size_t>(dy) * pitch;
        for (int col = 0; col < destW; ++col)
        {
            int const dx = destX + col;
            if (dx < 0 || dx >= surfW)
                continue;
            int const sc     = col * r->w / destW;
            int const srcCol = mirror ? (r->w - 1 - sc) : sc;
            uint8 const * px = srcRow + static_cast<size_t>(srcCol) * 4;
            if (px[3] == 0)
                continue;
            WriteEffectPixel(dstRow, dx, px, bpp32, transp, fogged, desatur, transparency);
        }
    }

    destSurface->Unlock(base);
    return true;
}

bool ModernSpritesEnabled()
{
    char const * e = getenv("CTP2_MODERN_SPRITES");
    return e && e[0] && strcmp(e, "0") != 0;
}

std::string ModernAssetManifestPath(char const * spriteFileName)
{
    if (!spriteFileName || !spriteFileName[0])
        return std::string();

    // base = filename without directory or extension (e.g. "GU04.SPR" -> "GU04").
    std::string name(spriteFileName);
    std::string::size_type const slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);
    std::string::size_type const dot = name.find_last_of('.');
    if (dot != std::string::npos)
        name = name.substr(0, dot);

    char const * home = getenv("HOME");
    if (!home || !home[0])
        return std::string();

    // The converter maintains ~/.ctp2/assets/current -> <fingerprint> (symlink),
    // with a "current.txt" holding the fingerprint where symlinks are absent.
    std::string const root = std::string(home) + "/.ctp2/assets/";
    std::string candidate = root + "current/" + name + ".json";
    if (FILE * f = fopen(candidate.c_str(), "r")) { fclose(f); return candidate; }

    if (FILE * ptr = fopen((root + "current.txt").c_str(), "r"))
    {
        char fp[256] = {0};
        if (fgets(fp, sizeof(fp), ptr))
        {
            size_t len = strlen(fp);
            while (len > 0 && (fp[len - 1] == '\n' || fp[len - 1] == '\r'))
                fp[--len] = '\0';
            candidate = root + fp + "/" + name + ".json";
            fclose(ptr);
            if (FILE * f = fopen(candidate.c_str(), "r")) { fclose(f); return candidate; }
            return std::string();
        }
        fclose(ptr);
    }
    return std::string();
}

void ModernSpriteLoadIfEnabled(std::unique_ptr<ModernSpriteAtlas> & slot,
                               char const * spriteFileName)
{
    if (!ModernSpritesEnabled())
        return;
    std::string const manifest = ModernAssetManifestPath(spriteFileName);
    if (manifest.empty())
        return;
    std::string error;
    slot.reset(ModernSpriteAtlas::Load(manifest.c_str(), error));
}

bool ModernSpriteDrawUnfaced(ModernSpriteAtlas const & atlas, aui_Surface * surf,
                             char const * action, int frame, int drawX, int drawY,
                             int facing, int hotX, int hotY, double scale,
                             uint16 transparency, uint16 flags)
{
    // The atlas blit is a binary-alpha copy; it cannot do the additive "flash"
    // blend Sprite::DrawDirect uses for k_BIT_DRAWFLAGS_ADDITIVE. Leave those to
    // the legacy path.
    if (flags & k_BIT_DRAWFLAGS_ADDITIVE)
        return false;

    ModernSpriteRect const * r = atlas.FindRect(action, 0, frame);
    if (!r)
        return false;

    // Match Sprite::DrawDirect: reversed facings measure the hot point from the
    // frame's right edge and draw mirrored; the origin scales with the zoom.
    bool const reversed = facing >= 5;
    int  const destX    = reversed ? (drawX - static_cast<int>((r->w - hotX) * scale))
                                   : (drawX - static_cast<int>(hotX * scale));
    int  const destY    = drawY - static_cast<int>(hotY * scale);

    if (scale > 0.999 && scale < 1.001)
        return atlas.Blit(surf, action, 0, frame, destX, destY, transparency, flags, reversed);

    int const destW = static_cast<int>(r->w * scale);
    int const destH = static_cast<int>(r->h * scale);
    return atlas.BlitScaled(surf, action, 0, frame, destX, destY, destW, destH,
                            transparency, flags, reversed);
}
