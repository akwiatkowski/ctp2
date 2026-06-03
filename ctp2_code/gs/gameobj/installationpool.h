#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _INSTALLATIONPOOL_H_
#define _INSTALLATIONPOOL_H_

#include "gs/gameobj/ObjPool.h"
#include "gs/gameobj/installation.h"

#include <nlohmann/json.hpp>

class CivArchive;

class InstallationPool : public ObjPool
{
public:
	InstallationPool();
	InstallationPool(CivArchive &archive);

	InstallationData *AccessInstallation(const Installation id)
	{
		return (InstallationData*)Access(id);
	}

	InstallationData *GetInstallation(const Installation id)
	{
		return (InstallationData*)Get(id);
	}

	Installation Create(sint32 owner,
						MapPoint &pnt,
						sint32 type);
	void Remove(Installation id);

	void Serialize(CivArchive &archive) override;
	void RebuildQuadTree();

	// JSON bridge — mirrors InstallationPool::Serialize.  Persists
	// ObjPool key counter + every live InstallationData entry.
	friend void to_json(nlohmann::json &j, InstallationPool const &p);
	friend void from_json(nlohmann::json const &j, InstallationPool &p);
};

// g_theInstallationPool's lifecycle (new / archive-load / Cleanup) lives
// in gs/utility/gameinit.cpp; the variable is now file-scope `static`
// there.  External readers go through installationpool_Get().
InstallationPool * installationpool_Get();
void               installationpool_Set(InstallationPool *p);

#endif
