#ifndef __INST_DB_H__
#define __INST_DB_H__

// Stub InstallationDatabase for COM wrapper compatibility.
// The original InstallationDatabase was removed during refactoring.
// This stub allows gs/outcom/ COM wrappers to compile.

class InstallationRecord {
public:
	sint32 GetObsolete(sint32) const { return 0; }
	sint32 GetEnabling() const { return 0; }
};

class InstallationDatabase {
public:
	InstallationRecord *Get(sint32) const { return NULL; }

	sint32 EnableInstallation(sint32) const { return 0; }
	double GetVisionRange(sint32) const { return 0.0; }
	uint32 GetVisibilityClasses(sint32) const { return 0; }
	BOOL IsAirfield(sint32) const { return FALSE; }
	BOOL IsFort(sint32) const { return FALSE; }
	sint32 GetAttack(sint32) const { return 0; }
	sint32 GetFirepower(sint32) const { return 0; }
};

#endif
