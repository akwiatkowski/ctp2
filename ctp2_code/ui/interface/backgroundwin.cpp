#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/aui_ctp2/background.h"
#include "ui/aui_ctp2/statuswindow.h"
#include "ui/interface/radarwindow.h"
#include "ui/interface/statswindow.h"

#include "ui/interface/chatbox.h"

#include "net/general/network.h"

#include "gs/gameobj/TradePool.h"
#include "gfx/tilesys/tiledmap.h"
#include "ui/aui_ctp2/c3windows.h"
#include "ctp/ctp2_utils/c3cmdline.h"

#include "ui/aui_common/aui_static.h"

#include "ui/interface/backgroundwin.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/aui_sdl/aui_sdl.h"       // aui_SDL::WorldContentOff* (margin assert)


#include "gs/database/profileDB.h"

#include "gfx/spritesys/director.h"

#include "gfx/spritesys/screenmanager.h"
static Background		*g_background = nullptr;

Background * background_Get()             { return g_background; }
void         background_Set(Background *p)    { g_background = p; }


void DumpSpanList(aui_DirtyList *list);




extern sint32 g_ScreenWidth;
extern sint32 g_ScreenHeight;

extern StatsWindow				*g_statsWindow;



extern RECT				g_backgroundViewport;

#ifdef _DEBUG
#include "gs/utility/DataCheck.h"
#endif

extern int sprite_Update(aui_Surface *surf);
extern void DisplayFrame (aui_Surface *surf);

extern sint32			g_modalWindow;


sint32 backgroundWin_Initialize(bool fullscreen)
{
	AUI_ERRCODE errcode;

	if ( g_background ) return 0;






	sint32 backgroundWidth = g_ScreenWidth + 5 * k_TILE_GRID_WIDTH / 2;

	sint32 controlPanelHeight=0;

	if (!fullscreen)
	{
		if (g_ScreenWidth < 1024)
		{

			controlPanelHeight = 100;
		}
		else
		{

			controlPanelHeight = 150;
		}
	}
	sint32 backgroundHeight = g_ScreenHeight  + 2 * k_TILE_GRID_HEIGHT;

	sint32 backgroundX = 0 - k_TILE_GRID_WIDTH;

	sint32 backgroundY = 0 - k_TILE_GRID_HEIGHT;

	sint32 widthAdjust = 0;
	sint32 heightAdjust = 0;
	if (backgroundWidth & 0x01)
	{
		widthAdjust = 1;
		backgroundWidth += widthAdjust;
	}
	if (backgroundHeight & 0x01)
	{
		heightAdjust = 1;
		backgroundHeight += heightAdjust;
	}




	SetRect(
		&g_backgroundViewport,
		k_TILE_GRID_WIDTH,
		k_TILE_GRID_HEIGHT,
		backgroundWidth - widthAdjust - 3 * k_TILE_GRID_WIDTH / 2,
		backgroundHeight - heightAdjust - k_TILE_GRID_HEIGHT );

	g_background = new Background(
		&errcode,
		k_ID_BACKGROUND,
		backgroundX, backgroundY, backgroundWidth, backgroundHeight,
		16,
		background_draw_handler );
	Assert( g_background != nullptr );
	if ( !g_background ) return -1;

	// P11 Stage 2 D: tell the UI which window is the world (background) window
	// so per-layer GPU compositing can route its composite into the world layer
	// and everything else into the UI layer. No-op unless CTP2_GPU_LAYERS is on.
	// The window, not its surface: at this point the surface does not exist yet
	// (windows create them lazily), and dynamic windows recreate theirs on
	// hide/show — the UI resolves TheSurface() live at blit time.
	if ( c3ui_Get() )
		c3ui_Get()->SetWorldWindow( g_background );

#if defined(__AUI_USE_SDL__)
    // GPU window quads and sprite coordinate conversion use the background
    // layout's margin. The CPU mirror retains painter coordinates directly.
	static_assert(aui_SDL::WorldContentOffX() == k_TILE_GRID_WIDTH,
	              "GPU world content offset X must equal the background window margin");
	static_assert(aui_SDL::WorldContentOffY() == k_TILE_GRID_HEIGHT,
	              "GPU world content offset Y must equal the background window margin");
#endif




	return 0;
}

void backgroundWin_Cleanup()
{
	if (c3ui_Get() && g_background)
    {
    	c3ui_Get()->RemoveWindow(g_background->Id());
    }

	delete g_background;
	g_background = nullptr;
}

#ifdef _PLAYTEST
sint32 g_debugOwner = k_DEBUG_OWNER_NONE;
#endif

AUI_ERRCODE background_render_map_only(Background *back)
{
	aui_Surface	*   surface = (back)    ? back->TheSurface() : nullptr;

    if (!surface || !tiledmap_Get())
    {
        // Busy initialising: postpone drawing until ready.
        return AUI_ERRCODE_INVALIDPARAM;
    }

	tiledmap_Get()->UpdateMixFromMap(surface);

	if (profiledb_Get()->IsWaterAnim())
    {
        tiledmap_Get()->DrawWater();
    }

	tradepool_Get()->Draw(surface);
	tiledmap_Get()->RepaintSprites(surface, tiledmap_Get()->GetMapViewRect(), false);

	if (director_Get())
    {
		director_Get()->GarbageCollectItems();
	}

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE background_draw_handler(LPVOID bg)
{
	Background  *   back    = reinterpret_cast<Background *>(bg);
	aui_Surface	*   surface = (back)    ? back->TheSurface() : nullptr;
	aui_Mouse *     mouse   = (c3ui_Get())  ? c3ui_Get()->TheMouse() : nullptr;

    if (!mouse || !tiledmap_Get())
    {
        // Busy initialising: postpone drawing until ready.
        return AUI_ERRCODE_INVALIDPARAM;
    }

	if (g_modalWindow > 0)
    {
		screenmanager_Get()->LockSurface(surface);
		tiledmap_Get()->DrawChatText();
		screenmanager_Get()->UnlockSurface();

		return AUI_ERRCODE_OK;
	}

	AUI_ERRCODE const mapErr = background_render_map_only(back);
	if (mapErr != AUI_ERRCODE_OK)
		return mapErr;

	tiledmap_Get()->DrawUnfinishedMove(surface);

	POINT pos;
	pos.y = mouse->Y() - back->Y();
	if (pos.y < back->Height())
	{
		tiledmap_Get()->DrawHiliteMouseTile(surface);
		tiledmap_Get()->DrawLegalMove(surface);
	}

#ifdef _PLAYTEST
	switch(g_debugOwner)
    {
#ifdef _DEBUG
	case k_DEBUG_OWNER_CRC:
		if(DataCheck *dc = datacheck_Get()) {
			dc->DisplayCRC(surface);
			tiledmap_Get()->InvalidateMix();
		}
		break;
	case k_DEBUG_OWNER_NETWORK_CHAT:
		network_Get().DisplayChat(surface);
		tiledmap_Get()->InvalidateMix();
		break;
#endif
	case k_DEBUG_OWNER_COMMANDLINE:
		command_line_Get().DisplayOutput(surface);
		tiledmap_Get()->InvalidateMix();
		break;
    case k_DEBUG_OWNER_FRAME_RATE:
        DisplayFrame (surface);
        break;
	default:
		break;
	}
#endif // _PLAYTEST

	tiledmap_Get()->CopyMixDirtyRects(back->GetDirtyList());
	tiledmap_Get()->ClearMixDirtyRects();

	return AUI_ERRCODE_OK;
}

void DumpSpanList(aui_DirtyList *list)
{
	sint32			 i;
	sint32			 j;
	FILE			*outFile;

	outFile = fopen("spans.txt", "wt");

	if (!outFile) return;

	aui_SpanList	*spanList = list->GetSpans();
	aui_SpanList	*curSpanList;

	for (i=0; i<list->GetHeight(); i++) {
		curSpanList = (spanList + i);
		if (curSpanList->num > 0) {
			fprintf(outFile, "Line %#.3d", i);
			for (j=0; j<curSpanList->num; j++) {
				fprintf(outFile, " Run %#.3d Len %#.3d", curSpanList->spans[j].run,
						curSpanList->spans[j].length);
			}
			fprintf(outFile, "\n");
		}
	}

	fclose(outFile);
}
