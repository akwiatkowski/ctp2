//----------------------------------------------------------------------------
// See ModernSpriteAtlas.h. P11 Stage 2 B1.
//----------------------------------------------------------------------------
#include "ctp/c3.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"

#include "gfx/gfx_utils/png_load.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_common/aui_surface.h"

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
                             int frame, int destX, int destY) const
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
            uint8 const * px = srcRow + static_cast<size_t>(col) * 4;
            if (px[3] == 0)                 // binary alpha: transparent -> skip
                continue;
            // Atlas stores R,G,B,A; write opaque at the destination depth.
            if (bpp32)
            {
                *reinterpret_cast<Pixel32 *>(dstRow + dx * 4) =
                    0xFF000000u | (static_cast<uint32>(px[0]) << 16)
                                | (static_cast<uint32>(px[1]) << 8) | px[2];
            }
            else
            {
                *reinterpret_cast<Pixel16 *>(dstRow + dx * 2) = static_cast<Pixel16>(
                    ((px[0] >> 3) << 11) | ((px[1] >> 2) << 5) | (px[2] >> 3));
            }
        }
    }

    destSurface->Unlock(base);
    return true;
}
