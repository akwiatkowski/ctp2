//----------------------------------------------------------------------------
// See ModernSpriteAtlas.h. P11 Stage 2 B1.
//----------------------------------------------------------------------------
#include "ctp/c3.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"

#include "gfx/gfx_utils/png_load.h"

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
