//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : JSON savegame entry points (Phase A scaffold)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/fileio/json_save.h"

#include <fstream>
#include <iostream>

namespace json_save {

bool SaveJson(char const *path)
{
    nlohmann::json doc;
    doc["magic"]          = MAGIC;
    doc["schema_version"] = SCHEMA_VERSION;

    std::ofstream out(path);
    if (!out)
    {
        std::cerr << "[json_save] SaveJson: cannot open '" << path
                  << "' for writing\n";
        return false;
    }
    // Pretty-print with 2-space indent for diffability — Phase A budget
    // accepts the size cost; gzip compression lands later.
    out << doc.dump(2);
    return out.good();
}

bool LoadJson(char const *path)
{
    std::ifstream in(path);
    if (!in)
    {
        std::cerr << "[json_save] LoadJson: cannot open '" << path
                  << "' for reading\n";
        return false;
    }

    nlohmann::json doc;
    try
    {
        in >> doc;
    }
    catch (nlohmann::json::parse_error const &e)
    {
        std::cerr << "[json_save] LoadJson: parse error at '" << path
                  << "': " << e.what() << "\n";
        return false;
    }

    // Hard-break on magic / schema mismatch.  Decision #1 in the
    // migration plan: no migrators during migration phases.
    if (!doc.contains("magic") || doc["magic"] != MAGIC)
    {
        std::cerr << "[json_save] LoadJson: bad magic in '" << path
                  << "' (expected \"" << MAGIC << "\")\n";
        return false;
    }
    if (!doc.contains("schema_version")
        || doc["schema_version"].get<int>() != SCHEMA_VERSION)
    {
        std::cerr << "[json_save] LoadJson: schema_version mismatch in '"
                  << path << "' (expected " << SCHEMA_VERSION << ")\n";
        return false;
    }

    return true;
}

}  // namespace json_save
