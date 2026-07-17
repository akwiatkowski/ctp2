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

    uint8 * base = nullptr;
    if (destSurface->Lock(nullptr, reinterpret_cast<LPVOID *>(&base), 0) != AUI_ERRCODE_OK
        || !base)
    {
        return false;
    }

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

            if (bpp32)
            {
                // Full-fidelity 8888 straight from the atlas RGB (no 565 round-trip).
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
                // Legacy 16-bit destination: match that surface's depth via 565.
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
