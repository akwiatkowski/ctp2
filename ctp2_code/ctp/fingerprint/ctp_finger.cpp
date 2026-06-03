#include "ctp/c3.h"

#include "gs/fileio/CivPaths.h"
#include "ctp/fingerprint/verify.h"
#include "ctp/fingerprint/ctp_finger.h"



BOOL ctpfinger_Check()
{
	BOOL	success = FALSE;
	MBCHAR	fingerprintPath[_MAX_PATH];
	MBCHAR	userListPath[_MAX_PATH];

	if (!civpaths_Get()->FindFile(k_FINGERPRINT_ASSET_DIR, k_FINGERPRINT_ASSET, fingerprintPath))
		return FALSE;

	if (!civpaths_Get()->FindFile(k_USER_LIST_ASSET_DIR, k_USER_LIST_ASSET, userListPath))
		return FALSE;

	if (GetInfoFromFingerprint(fingerprintPath) && IsValidUser(userListPath))
		success = TRUE;

	return success;
}
