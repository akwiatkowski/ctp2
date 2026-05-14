#include "ctp/c3.h"
#if !defined(__AUI_USE_SDL__)
#include <ddraw.h>

void TraceErrorDD(HRESULT hErr, char *sFile, int nLine)
{
	char dderr[256];
	char err[1024];

	switch (hErr)     {
		case DDERR_ALREADYINITIALIZED : snprintf(dderr, sizeof(dderr), "DDERR_ALREADYINITIALIZED"); break;
		case DDERR_CANNOTATTACHSURFACE : snprintf(dderr, sizeof(dderr), "DDERR_CANNOTATTACHSURFACE"); break;
		case DDERR_CANNOTDETACHSURFACE : snprintf(dderr, sizeof(dderr), "DDERR_CANNOTDETACHSURFACE"); break;
		case DDERR_CURRENTLYNOTAVAIL : snprintf(dderr, sizeof(dderr), "DDERR_CURRENTLYNOTAVAIL"); break;
		case DDERR_EXCEPTION : snprintf(dderr, sizeof(dderr), "DDERR_EXCEPTION"); break;
		case DDERR_GENERIC : snprintf(dderr, sizeof(dderr), "DDERR_GENERIC"); break;
		case DDERR_HEIGHTALIGN : snprintf(dderr, sizeof(dderr), "DDERR_HEIGHTALIGN"); break;
		case DDERR_INCOMPATIBLEPRIMARY : snprintf(dderr, sizeof(dderr), "DDERR_INCOMPATIBLEPRIMARY"); break;
		case DDERR_INVALIDCAPS : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDCAPS"); break;
		case DDERR_INVALIDCLIPLIST : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDCLIPLIST"); break;
		case DDERR_INVALIDMODE : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDMODE"); break;
		case DDERR_INVALIDOBJECT : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDOBJECT"); break;
		case DDERR_INVALIDPARAMS : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDPARAMS"); break;
		case DDERR_INVALIDPIXELFORMAT : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDPIXELFORMAT"); break;
		case DDERR_INVALIDRECT : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDRECT"); break;
		case DDERR_LOCKEDSURFACES : snprintf(dderr, sizeof(dderr), "DDERR_LOCKEDSURFACES"); break;
		case DDERR_NO3D : snprintf(dderr, sizeof(dderr), "DDERR_NO3D"); break;
		case DDERR_NOALPHAHW : snprintf(dderr, sizeof(dderr), "DDERR_NOALPHAHW"); break;
		case DDERR_NOCLIPLIST : snprintf(dderr, sizeof(dderr), "DDERR_NOCLIPLIST"); break;
		case DDERR_NOCOLORCONVHW : snprintf(dderr, sizeof(dderr), "DDERR_NOCOLORCONVHW"); break;
		case DDERR_NOCOOPERATIVELEVELSET : snprintf(dderr, sizeof(dderr), "DDERR_NOCOOPERATIVELEVELSET"); break;
		case DDERR_NOCOLORKEY : snprintf(dderr, sizeof(dderr), "DDERR_NOCOLORKEY"); break;
		case DDERR_NOCOLORKEYHW : snprintf(dderr, sizeof(dderr), "DDERR_NOCOLORKEYHW"); break;
		case DDERR_NODIRECTDRAWSUPPORT : snprintf(dderr, sizeof(dderr), "DDERR_NODIRECTDRAWSUPPORT"); break;
		case DDERR_NOEXCLUSIVEMODE : snprintf(dderr, sizeof(dderr), "DDERR_NOEXCLUSIVEMODE"); break;
		case DDERR_NOFLIPHW : snprintf(dderr, sizeof(dderr), "DDERR_NOFLIPHW"); break;
		case DDERR_NOGDI : snprintf(dderr, sizeof(dderr), "DDERR_NOGDI"); break;
		case DDERR_NOMIRRORHW : snprintf(dderr, sizeof(dderr), "DDERR_NOMIRRORHW"); break;
		case DDERR_NOTFOUND : snprintf(dderr, sizeof(dderr), "DDERR_NOTFOUND"); break;
		case DDERR_NOOVERLAYHW : snprintf(dderr, sizeof(dderr), "DDERR_NOOVERLAYHW"); break;
		case DDERR_NORASTEROPHW : snprintf(dderr, sizeof(dderr), "DDERR_NORASTEROPHW"); break;
		case DDERR_NOROTATIONHW : snprintf(dderr, sizeof(dderr), "DDERR_NOROTATIONHW"); break;
		case DDERR_NOSTRETCHHW : snprintf(dderr, sizeof(dderr), "DDERR_NOSTRETCHHW"); break;
		case DDERR_NOT4BITCOLOR : snprintf(dderr, sizeof(dderr), "DDERR_NOT4BITCOLOR"); break;
		case DDERR_NOT4BITCOLORINDEX : snprintf(dderr, sizeof(dderr), "DDERR_NOT4BITCOLORINDEX"); break;
		case DDERR_NOT8BITCOLOR : snprintf(dderr, sizeof(dderr), "DDERR_NOT8BITCOLOR"); break;
		case DDERR_NOTEXTUREHW : snprintf(dderr, sizeof(dderr), "DDERR_NOTEXTUREHW"); break;
		case DDERR_NOVSYNCHW : snprintf(dderr, sizeof(dderr), "DDERR_NOVSYNCHW"); break;
		case DDERR_NOZBUFFERHW : snprintf(dderr, sizeof(dderr), "DDERR_NOZBUFFERHW"); break;
		case DDERR_NOZOVERLAYHW : snprintf(dderr, sizeof(dderr), "DDERR_NOZOVERLAYHW"); break;
		case DDERR_OUTOFCAPS : snprintf(dderr, sizeof(dderr), "DDERR_OUTOFCAPS"); break;
		case DDERR_OUTOFMEMORY : snprintf(dderr, sizeof(dderr), "DDERR_OUTOFMEMORY"); break;
		case DDERR_OUTOFVIDEOMEMORY : snprintf(dderr, sizeof(dderr), "DDERR_OUTOFVIDEOMEMORY"); break;
		case DDERR_OVERLAYCANTCLIP : snprintf(dderr, sizeof(dderr), "DDERR_OVERLAYCANTCLIP"); break;
		case DDERR_OVERLAYCOLORKEYONLYONEACTIVE : snprintf(dderr, sizeof(dderr), "DDERR_OVERLAYCOLORKEYONLYONEACTIVE"); break;
		case DDERR_PALETTEBUSY : snprintf(dderr, sizeof(dderr), "DDERR_PALETTEBUSY"); break;
		case DDERR_COLORKEYNOTSET : snprintf(dderr, sizeof(dderr), "DDERR_COLORKEYNOTSET"); break;
		case DDERR_SURFACEALREADYATTACHED : snprintf(dderr, sizeof(dderr), "DDERR_SURFACEALREADYATTACHED"); break;
		case DDERR_SURFACEALREADYDEPENDENT : snprintf(dderr, sizeof(dderr), "DDERR_SURFACEALREADYDEPENDENT"); break;
		case DDERR_SURFACEBUSY : snprintf(dderr, sizeof(dderr), "DDERR_SURFACEBUSY"); break;
		case DDERR_CANTLOCKSURFACE : snprintf(dderr, sizeof(dderr), "DDERR_CANTLOCKSURFACE"); break;
		case DDERR_SURFACEISOBSCURED : snprintf(dderr, sizeof(dderr), "DDERR_SURFACEISOBSCURED"); break;
		case DDERR_SURFACELOST : snprintf(dderr, sizeof(dderr), "DDERR_SURFACELOST"); break;
		case DDERR_SURFACENOTATTACHED : snprintf(dderr, sizeof(dderr), "DDERR_SURFACENOTATTACHED"); break;
		case DDERR_TOOBIGHEIGHT : snprintf(dderr, sizeof(dderr), "DDERR_TOOBIGHEIGHT"); break;
		case DDERR_TOOBIGSIZE : snprintf(dderr, sizeof(dderr), "DDERR_TOOBIGSIZE"); break;
		case DDERR_TOOBIGWIDTH : snprintf(dderr, sizeof(dderr), "DDERR_TOOBIGWIDTH"); break;
		case DDERR_UNSUPPORTED : snprintf(dderr, sizeof(dderr), "DDERR_UNSUPPORTED"); break;
		case DDERR_UNSUPPORTEDFORMAT : snprintf(dderr, sizeof(dderr), "DDERR_UNSUPPORTEDFORMAT"); break;
		case DDERR_UNSUPPORTEDMASK : snprintf(dderr, sizeof(dderr), "DDERR_UNSUPPORTEDMASK"); break;
		case DDERR_VERTICALBLANKINPROGRESS : snprintf(dderr, sizeof(dderr), "DDERR_VERTICALBLANKINPROGRESS"); break;
		case DDERR_WASSTILLDRAWING : snprintf(dderr, sizeof(dderr), "DDERR_WASSTILLDRAWING"); break;
		case DDERR_XALIGN : snprintf(dderr, sizeof(dderr), "DDERR_XALIGN"); break;
		case DDERR_INVALIDDIRECTDRAWGUID : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDDIRECTDRAWGUID"); break;
		case DDERR_DIRECTDRAWALREADYCREATED : snprintf(dderr, sizeof(dderr), "DDERR_DIRECTDRAWALREADYCREATED"); break;
		case DDERR_NODIRECTDRAWHW : snprintf(dderr, sizeof(dderr), "DDERR_NODIRECTDRAWHW"); break;
		case DDERR_PRIMARYSURFACEALREADYEXISTS : snprintf(dderr, sizeof(dderr), "DDERR_PRIMARYSURFACEALREADYEXISTS"); break;
		case DDERR_NOEMULATION : snprintf(dderr, sizeof(dderr), "DDERR_NOEMULATION"); break;
		case DDERR_REGIONTOOSMALL : snprintf(dderr, sizeof(dderr), "DDERR_REGIONTOOSMALL"); break;
		case DDERR_CLIPPERISUSINGHWND : snprintf(dderr, sizeof(dderr), "DDERR_CLIPPERISUSINGHWND"); break;
		case DDERR_NOCLIPPERATTACHED : snprintf(dderr, sizeof(dderr), "DDERR_NOCLIPPERATTACHED"); break;
		case DDERR_NOHWND : snprintf(dderr, sizeof(dderr), "DDERR_NOHWND"); break;
		case DDERR_HWNDSUBCLASSED : snprintf(dderr, sizeof(dderr), "DDERR_HWNDSUBCLASSED"); break;
		case DDERR_HWNDALREADYSET : snprintf(dderr, sizeof(dderr), "DDERR_HWNDALREADYSET"); break;
		case DDERR_NOPALETTEATTACHED : snprintf(dderr, sizeof(dderr), "DDERR_NOPALETTEATTACHED"); break;
		case DDERR_NOPALETTEHW : snprintf(dderr, sizeof(dderr), "DDERR_NOPALETTEHW"); break;
		case DDERR_BLTFASTCANTCLIP : snprintf(dderr, sizeof(dderr), "DDERR_BLTFASTCANTCLIP"); break;
		case DDERR_NOBLTHW : snprintf(dderr, sizeof(dderr), "DDERR_NOBLTHW"); break;
		case DDERR_NODDROPSHW : snprintf(dderr, sizeof(dderr), "DDERR_NODDROPSHW"); break;
		case DDERR_OVERLAYNOTVISIBLE : snprintf(dderr, sizeof(dderr), "DDERR_OVERLAYNOTVISIBLE"); break;
		case DDERR_NOOVERLAYDEST : snprintf(dderr, sizeof(dderr), "DDERR_NOOVERLAYDEST"); break;
		case DDERR_INVALIDPOSITION : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDPOSITION"); break;
		case DDERR_NOTAOVERLAYSURFACE : snprintf(dderr, sizeof(dderr), "DDERR_NOTAOVERLAYSURFACE"); break;
		case DDERR_EXCLUSIVEMODEALREADYSET : snprintf(dderr, sizeof(dderr), "DDERR_EXCLUSIVEMODEALREADYSET"); break;
		case DDERR_NOTFLIPPABLE : snprintf(dderr, sizeof(dderr), "DDERR_NOTFLIPPABLE"); break;
		case DDERR_CANTDUPLICATE : snprintf(dderr, sizeof(dderr), "DDERR_CANTDUPLICATE"); break;
		case DDERR_NOTLOCKED : snprintf(dderr, sizeof(dderr), "DDERR_NOTLOCKED"); break;
		case DDERR_CANTCREATEDC : snprintf(dderr, sizeof(dderr), "DDERR_CANTCREATEDC"); break;
		case DDERR_NODC : snprintf(dderr, sizeof(dderr), "DDERR_NODC"); break;
		case DDERR_WRONGMODE : snprintf(dderr, sizeof(dderr), "DDERR_WRONGMODE"); break;
		case DDERR_IMPLICITLYCREATED : snprintf(dderr, sizeof(dderr), "DDERR_IMPLICITLYCREATED"); break;
		case DDERR_NOTPALETTIZED : snprintf(dderr, sizeof(dderr), "DDERR_NOTPALETTIZED"); break;
		case DDERR_UNSUPPORTEDMODE : snprintf(dderr, sizeof(dderr), "DDERR_UNSUPPORTEDMODE"); break;
		case DDERR_NOMIPMAPHW : snprintf(dderr, sizeof(dderr), "DDERR_NOMIPMAPHW"); break;
		case DDERR_INVALIDSURFACETYPE : snprintf(dderr, sizeof(dderr), "DDERR_INVALIDSURFACETYPE"); break;
		case DDERR_DCALREADYCREATED : snprintf(dderr, sizeof(dderr), "DDERR_DCALREADYCREATED"); break;
		case DDERR_CANTPAGELOCK : snprintf(dderr, sizeof(dderr), "DDERR_CANTPAGELOCK"); break;
		case DDERR_CANTPAGEUNLOCK : snprintf(dderr, sizeof(dderr), "DDERR_CANTPAGEUNLOCK"); break;
		case DDERR_NOTPAGELOCKED : snprintf(dderr, sizeof(dderr), "DDERR_NOTPAGELOCKED"); break;
		case DDERR_NOTINITIALIZED : snprintf(dderr, sizeof(dderr), "DDERR_NOTINITIALIZED"); break;
		default : snprintf(dderr, sizeof(dderr), "Unknown Error"); break;
	}
	snprintf(err, sizeof(err), "DirectDraw Error %s\nin file %s at line %d", dderr, sFile, nLine);
	DPRINTF(k_DBG_FIX, ("%s\n", err));
}
#endif
