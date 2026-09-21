//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The load/save map window
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include <memory>
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_imagebase.h"
#include "ui/aui_common/aui_textbase.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_tabgroup.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3textfield.h"
#include "gfx/gfx_utils/colorset.h"
#include "ui/aui_ctp2/texttab.h"
#include "ctp/ctp2_utils/pointerlist.h"

#include "gs/database/StrDB.h"
#include "gs/database/profileDB.h"

#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/loadsavemapwindow.h"

#include "gfx/gfx_utils/pixelutils.h"

#include "ui/aui_ctp2/radarmap.h"



extern LoadSaveMapWindow			*g_loadSaveMapWindow;

LoadSaveMapWindow::LoadSaveMapWindow(AUI_ERRCODE *retval, uint32 id,
		MBCHAR *ldlBlock, sint32 bpp, AUI_WINDOW_TYPE type, bool bevel)
		: c3_PopupWindow(retval,id,ldlBlock,bpp,type,bevel)
{
	m_fileList = nullptr;
	m_gameMapInfo = nullptr;
	m_saveMapInfo = nullptr;

	m_type = LSMS_TOTAL;

	InitCommonLdl(ldlBlock);
}

AUI_ERRCODE LoadSaveMapWindow::InitCommonLdl(MBCHAR *ldlBlock)
{
	MBCHAR			tabGroupBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			tabBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];

	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;


	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Name" );
	AddTitle( block );

	AddOk( loadsavemapscreen_executePress, nullptr, "c3_PopupOk" );
	AddCancel( loadsavemapscreen_backPress );

	m_deleteButton.reset(spNew_c3_Button(
		&errcode,
		ldlBlock,
		"DeleteButton",
		loadsavemapscreen_deletePress ));

	m_nameString.reset(spNewStringTable(&errcode, "LSMSStringTable"));
	Assert(m_nameString);
	if (!m_nameString) return AUI_ERRCODE_LOADFAILED;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "TitlePanel");
	m_titlePanel = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), block);
	Assert(m_titlePanel);
	if (!m_titlePanel) return AUI_ERRCODE_LOADFAILED;

	m_gameMapText.reset(spNew_c3_Static(&errcode, block, "GameMapText"));
	Assert(m_gameMapText);
	if (!m_gameMapText) return AUI_ERRCODE_LOADFAILED;

	m_gameMapTextBox.reset(spNewTextEntry(&errcode, block, "GameMapTextBox"));
	Assert(m_gameMapTextBox);
	if (!m_gameMapTextBox) return AUI_ERRCODE_LOADFAILED;
	m_gameMapTextBox->SetIsFileName(TRUE);

	m_saveMapText.reset(spNew_c3_Static(&errcode, block, "SaveMapText"));
	Assert(m_saveMapText);
	if (!m_saveMapText) return AUI_ERRCODE_LOADFAILED;

	m_saveMapTextBox.reset(spNewTextEntry(&errcode, block, "SaveMapTextBox"));
	Assert(m_saveMapTextBox);
	if (!m_saveMapTextBox) return AUI_ERRCODE_LOADFAILED;
	m_saveMapTextBox->SetIsFileName(TRUE);

	m_noteText.reset(spNew_c3_Static(&errcode, block, "NoteText"));
	Assert(m_noteText);
	if (!m_noteText) return AUI_ERRCODE_LOADFAILED;

	m_noteTextBox.reset(spNewTextEntry(&errcode, block, "NoteTextBox"));
	Assert(m_noteTextBox);
	if (!m_noteTextBox) return AUI_ERRCODE_LOADFAILED;

	m_listOne.reset(spNew_c3_ListBox(&errcode, ldlBlock, "ListOne",
									loadsavemapscreen_ListOneHandler, (void *)this));
	Assert(m_listOne);
	if (!m_listOne) return AUI_ERRCODE_LOADFAILED;

	m_listTwo.reset(spNew_c3_ListBox(&errcode, ldlBlock, "ListTwo",
									loadsavemapscreen_ListTwoHandler, (void *)this));
	Assert(m_listTwo);
	if (!m_listTwo) return AUI_ERRCODE_LOADFAILED;

	snprintf(tabGroupBlock, sizeof(tabGroupBlock), "%s.%s", ldlBlock, "LoadTabGroup" );
	m_tabGroup = std::make_unique<aui_TabGroup>( &errcode, aui_UniqueId(), tabGroupBlock );
	Assert( AUI_NEWOK(m_tabGroup, errcode) );
	if (!m_tabGroup) return AUI_ERRCODE_LOADFAILED;

	m_tabGroup->SetDrawMask( k_AUI_REGION_DRAWFLAG_UPDATE );

	snprintf(tabBlock, sizeof(tabBlock), "%s.%s", tabGroupBlock, "MapTab");
	m_mapTab = std::make_unique<TextTab>(&errcode, aui_UniqueId(), tabBlock, nullptr);
	Assert( AUI_NEWOK(m_mapTab, errcode) );
	if ( !AUI_NEWOK(m_mapTab, errcode) ) return AUI_ERRCODE_LOADFAILED;

	snprintf(block, sizeof(block), "%s.pane.%s", tabBlock, "MapImage");
	m_mapTabImage = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), block);
	Assert(m_mapTabImage);
	if (!m_mapTabImage) return AUI_ERRCODE_LOADFAILED;




	m_mapTabImageBackup = std::make_unique<aui_Image>(
		&errcode, m_mapTabImage->GetImage()->GetFilename() );
	Assert(m_mapTabImageBackup);
	if (!m_mapTabImageBackup) return AUI_ERRCODE_LOADFAILED;

	m_mapTabImageBackup->Load();




	return AUI_ERRCODE_OK;
}

LoadSaveMapWindow::~LoadSaveMapWindow()
{
	CleanUpSaveMapInfo();

#define mycleanup(mypointer) { mypointer.reset(); }

	m_nameString.reset();

	m_titlePanel.reset();
	m_gameMapText.reset();
	m_gameMapTextBox.reset();
	m_saveMapText.reset();
	m_saveMapTextBox.reset();
	m_noteText.reset();
	m_noteTextBox.reset();

	mycleanup(m_listOne);
	mycleanup(m_listTwo);

	mycleanup(m_tabGroup);
	mycleanup(m_mapTab);
	m_mapTabImage.reset();
	mycleanup(m_mapTabImageBackup);

	mycleanup(m_deleteButton);

#undef mycleanup
}


void LoadSaveMapWindow::FillListOne()
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	if (!m_fileList) return;
	if (m_fileList->GetCount() <= 0) return;

	if (m_listOne == nullptr) return;

	m_listOne->Clear();

	PointerList<GameMapInfo>::Walker walker = PointerList<GameMapInfo>::Walker(m_fileList);

	for ( ; walker.IsValid(); walker.Next())
    {
		LSMGameMapsListItem *item = std::make_unique<LSMGameMapsListItem>
            (&errcode, const_cast<MBCHAR *>("LSMGameMapsListItem"), walker.GetObj()).release();
		Assert(errcode == AUI_ERRCODE_OK);
		if (errcode != AUI_ERRCODE_OK) return;

		m_listOne->AddItem(item);
	}
}


void LoadSaveMapWindow::FillListTwo(GameMapInfo *info)
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	Assert(m_listTwo);
	if (m_listTwo == nullptr) return;

	m_listTwo->Clear();

	Ok()->Enable(LSMS_LOAD_GAMEMAP != m_type);

    SetSaveMapInfo(nullptr);

	if ( info )
	{
		PointerList<SaveMapInfo>::Walker walker = PointerList<SaveMapInfo>::Walker(info->files.get());

		for ( ; walker.IsValid(); walker.Next())
        {
			LSMSaveMapsListItem *item = std::make_unique<LSMSaveMapsListItem>
                (&errcode, const_cast<MBCHAR *>("LSMSaveMapsListItem"), walker.GetObj()).release();
			Assert(errcode == AUI_ERRCODE_OK);
			if (errcode != AUI_ERRCODE_OK) return;

			m_listTwo->AddItem(item);
		}
	}
}

void LoadSaveMapWindow::SelectCurrentGameMap()
{
	if (!m_listOne) return;

	MBCHAR		currentGameMap[k_MAX_NAME_LEN] = {0};

	GetGameMapName(currentGameMap);

	sint32				foundItem = -1;
	LSMGameMapsListItem		*item;
	GameMapInfo			*info;

	if (m_listOne->NumItems() <= 0) return;

	for (sint32 i=0; i<m_listOne->NumItems(); i++) {
		item = (LSMGameMapsListItem *)m_listOne->GetItemByIndex(i);
		if (!item) continue;

		info = item->GetGameMapInfo();
		if (!info) continue;

		if (!strcmp(info->name, currentGameMap)) {
			foundItem = i;
			break;
		}
	}

	if (foundItem == -1) {
		foundItem = 0;
	} else {
		m_listOne->SelectItem(foundItem);
	}
}

void LoadSaveMapWindow::SelectCurrentSaveMap()
{
	if (!m_listTwo) return;

	MBCHAR		saveMapName[_MAX_PATH] = {0};
	GetSaveMapName(saveMapName);

	sint32				foundItem = -1;
	LSMSaveMapsListItem		*item;
	SaveMapInfo			*info;

	if (m_listTwo->NumItems() <= 0) return;

	for (sint32 i=0; i<m_listTwo->NumItems(); i++) {
		item = (LSMSaveMapsListItem *)m_listTwo->GetItemByIndex(i);
		if (!item) continue;

		info = item->GetSaveMapInfo();
		if (!info) continue;

		if (!strcmp(info->fileName, saveMapName)) {
			foundItem = i;
			break;
		}
	}

	if (foundItem == -1) {
		foundItem = 0;
	} else {
		m_listTwo->SelectItem(foundItem);
	}
}




void LoadSaveMapWindow::SetType(uint32 type)
{

	m_type = type;

	if((m_type>=LSMS_FIRST) && (m_type<LSMS_TOTAL)) {
		TitleText()->SetText(m_nameString->GetString(m_type));
	} else
		Assert(0);

	switch (m_type)
	{
	case LSMS_LOAD_GAMEMAP:
		EnableFields( FALSE );
		break;

	default:
		EnableFields( TRUE );
		break;
	}

	m_fileList = GameMapFile::BuildSaveMapList(C3SAVEDIR_MAP);

	FillListOne();

	SelectCurrentGameMap();
	SelectCurrentSaveMap();

	if (m_type == LSMS_SAVE_GAMEMAP) {

		if ( CreateSaveMapInfoIfNeeded( m_saveMapInfoRemember ) )
			m_saveMapInfo = m_saveMapInfoRemember.get();


		CreateSaveMapInfoIfNeeded( m_saveMapInfoToSave );

		BuildDefaultSaveMapName(
			m_gameMapInfo ? m_gameMapInfo->name : nullptr,
			m_saveMapInfoToSave->fileName);
	} else {

	}
}

BOOL LoadSaveMapWindow::CreateSaveMapInfoIfNeeded( std::unique_ptr<SaveMapInfo> &info )
{
	if ( info == nullptr) {
		info = std::make_unique<SaveMapInfo>();

		GameMapFile::GetExtendedInfoFromProfile(info.get());
		GetRadarMap(info.get());
		SetRadarMap(info.get());

		m_tabGroup->ShouldDraw(TRUE);

		return TRUE;
	}

	return FALSE;
}

void LoadSaveMapWindow::CleanUpSaveMapInfo( )
{
	m_saveMapInfoToSave.reset();
	m_saveMapInfoRemember.reset();

	m_saveMapInfo = nullptr;
	m_gameMapInfo = nullptr;
}


void LoadSaveMapWindow::GetRadarMap(SaveMapInfo *info)
{
	AUI_ERRCODE	    errcode     = AUI_ERRCODE_OK;
	sint32 const    width       = m_mapTabImage->Width();
	sint32 const    height      = m_mapTabImage->Height();
	auto radarMap = std::make_unique<RadarMap>
	    (&errcode, aui_UniqueId(), 0, 0, width, height, m_pattern->GetFilename());

	aui_Surface	*surf = radarMap->GetMapSurface();

	radarMap->RenderMap(surf);

	info->radarMapWidth = width;
	info->radarMapHeight = height;
	info->radarMapData.resize(width * height);

	Pixel16		 *buffer;
	Pixel16		 *bufferDataPtr;
	Pixel16 *   radarDataPtr = info->radarMapData.data();

	if (surf->Lock(nullptr, (LPVOID *)&buffer, 0) != AUI_ERRCODE_OK) return;
	sint32      pitch = surf->Pitch();

	for (sint32 i = 0; i < height; i++)
	{
		bufferDataPtr = buffer + i * (pitch/2);
		memcpy(radarDataPtr, bufferDataPtr, width * sizeof(Pixel16));

		if (!is_565_Get()) {
			for (sint32 j=0; j<width; j++) {
				radarDataPtr[j] = pixelutils_Convert555to565(radarDataPtr[j]);
			}
		}

		radarDataPtr += width;
	}

	surf->Unlock(buffer);
}


void LoadSaveMapWindow::SetRadarMap(SaveMapInfo *info)
{

	aui_Image		*image = m_mapTabImage->GetImage();

	Assert( image != nullptr );
	if (image == nullptr) return;

	RECT rect =
	{
		0,
		0,
		m_mapTabImageBackup->TheSurface()->Width(),
		m_mapTabImageBackup->TheSurface()->Height()
	};

	if ( !info )
	{

		c3ui_Get()->TheBlitter()->Blt(
			m_mapTabImage->GetImage()->TheSurface(),
			0, 0,
			m_mapTabImageBackup->TheSurface(),
			&rect,
			k_AUI_BLITTER_FLAG_COPY );

		m_mapTabImage->ShouldDraw();

		return;
	}

	sint32 height = info->radarMapHeight;
	sint32 width = info->radarMapWidth;

	Assert( width <= rect.right );
	if ( width > rect.right ) return;
	Assert( height <= rect.bottom );
	if ( height > rect.bottom ) return;


	aui_Surface		*surface = image->TheSurface();

	Pixel16		 *buffer;
	Pixel16		 *bufferDataPtr;

	Pixel16 *   radarDataPtr = info->radarMapData.data();

	if (surface->Lock(nullptr, (LPVOID *)&buffer, 0) != AUI_ERRCODE_OK) {
		std::unique_ptr<aui_Surface>{surface};
		std::unique_ptr<aui_Image>{image};
		return;
	}

	sint32 const    pitch = surface->Pitch();

	for (sint32 i = 0; i < height; i++)
    {
		bufferDataPtr = buffer + i * (pitch/2);
		memcpy(bufferDataPtr, radarDataPtr, width * sizeof(Pixel16));










		radarDataPtr += width;
	}

	surface->Unlock(buffer);
}

void LoadSaveMapWindow::SetGameMapName(MBCHAR *name)
{
	if (!m_gameMapTextBox) return;
	m_gameMapTextBox->SetFieldText(name);
}

void LoadSaveMapWindow::SetSaveMapName(MBCHAR *name)
{
	if (!m_saveMapTextBox) return;
	m_saveMapTextBox->SetFieldText(name);
}

void LoadSaveMapWindow::SetNote(MBCHAR *note)
{
	if (!m_noteTextBox) return;
	m_noteTextBox->SetFieldText(note);
}





BOOL LoadSaveMapWindow::GetGameMapName(MBCHAR *name)
{
	Assert(m_gameMapTextBox);
	if (!m_gameMapTextBox) return FALSE;

	m_gameMapTextBox->GetFieldText(name, _MAX_PATH);

	return TRUE;
}

BOOL LoadSaveMapWindow::GetSaveMapName(MBCHAR *name)
{
	Assert(m_saveMapTextBox);
	if (!m_saveMapTextBox) return FALSE;

	m_saveMapTextBox->GetFieldText(name, _MAX_PATH);

	return TRUE;
}

BOOL LoadSaveMapWindow::GetNote(MBCHAR *note)
{
	Assert(m_noteTextBox);
	if (!m_noteTextBox) return FALSE;

	m_noteTextBox->GetFieldText(note, _MAX_PATH);

	return TRUE;
}

void LoadSaveMapWindow::SetGameMapInfo(GameMapInfo *info)
{
	m_gameMapInfo = info;

	if (info != nullptr) {
		SetGameMapName(info->name);
		FillListTwo(info);
	}
	else
	{
		SetGameMapName(const_cast<MBCHAR *>(""));
		FillListTwo(nullptr);
	}
}

void LoadSaveMapWindow::SetSaveMapInfo(SaveMapInfo *info)
{
	m_saveMapInfo = info;

	if (info != nullptr) {
		SetSaveMapName(info->fileName);
		SetNote(info->note);

		SetRadarMap(info);
	}
	else
	{
		switch ( m_type )
		{
		case LSMS_LOAD_GAMEMAP:
			SetSaveMapName(const_cast<MBCHAR *>(""));
			SetNote(const_cast<MBCHAR *>(""));

			SetRadarMap(nullptr);
			break;

		default:
			CreateSaveMapInfoIfNeeded( m_saveMapInfoToSave );
			SetSaveMapName(m_saveMapInfoToSave->fileName);
			SetNote(m_saveMapInfoToSave->note);

			SetRadarMap(m_saveMapInfoToSave.get());
			break;
		}
	}

	m_tabGroup->ShouldDraw(TRUE);
}

void LoadSaveMapWindow::BuildDefaultSaveMapName(MBCHAR *gameMapName, MBCHAR *name)
{
	MBCHAR		saveMapName[_MAX_PATH];
	MBCHAR		theGameMapName[_MAX_PATH];

	if (gameMapName == nullptr) {
		strlcpy(theGameMapName, profiledb_Get()->GetLeaderName(), sizeof(theGameMapName));
	} else {
		strlcpy(theGameMapName, gameMapName, sizeof(theGameMapName));
	}

	theGameMapName[6] = '\0';

	if (gameMapName == nullptr)
		SetGameMapName(theGameMapName);

	snprintf(saveMapName, sizeof(saveMapName), "%s", theGameMapName);


	// single caller passes SaveMapInfo::fileName[_MAX_PATH]; src saveMapName is _MAX_PATH
	strlcpy(name, saveMapName, _MAX_PATH);

	SetSaveMapName(saveMapName);
}

void LoadSaveMapWindow::EnableFields( BOOL enable )
{
	m_gameMapTextBox->Enable( enable );
	m_saveMapTextBox->Enable( enable );
	m_noteTextBox->Enable( enable );
}










LSMGameMapsListItem::LSMGameMapsListItem(AUI_ERRCODE *retval, MBCHAR *ldlBlock, GameMapInfo *info)
:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock),
	m_itemIcon(nullptr),
	m_itemText(nullptr)
{
	m_info = info;

	m_itemIcon = spNew_c3_Static(retval, ldlBlock, "GameMapsIcon");
	if(m_itemIcon) {

		AddChild(m_itemIcon);
	}

	m_itemText = spNew_c3_Static(retval, ldlBlock, "GameMapsText");
	if(m_itemText) {

		m_itemText->SetText(info->name);

		m_itemText->Resize(Width()-m_itemIcon->Width()-5, Height());
		m_itemText->Move(m_itemIcon->Width()+5, m_itemText->Y());

		m_itemIcon->AddChild(m_itemText);
	}
}

LSMGameMapsListItem::~LSMGameMapsListItem()
= default;

sint32 LSMGameMapsListItem::Compare(c3_ListItem *item2, uint32 column)
{

	return 0;
}


LSMSaveMapsListItem::LSMSaveMapsListItem(AUI_ERRCODE *retval, MBCHAR *ldlBlock, SaveMapInfo *info)
:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock),
	m_itemIcon(nullptr),
	m_itemText(nullptr)
{
	m_info = info;

	m_itemIcon = spNew_c3_Static(retval, ldlBlock, "SaveMapsIcon");
	if(m_itemIcon) {

		AddChild(m_itemIcon);
	}

	m_itemText = spNew_c3_Static(retval, ldlBlock, "SaveMapsText");
	if(m_itemText) {

		m_itemText->SetText(info->fileName);

		m_itemText->Resize(Width()-m_itemIcon->Width()-5, Height());
		m_itemText->Move(m_itemIcon->Width()+5, m_itemText->Y());

		m_itemIcon->AddChild(m_itemText);
	}
}

LSMSaveMapsListItem::~LSMSaveMapsListItem()
= default;

sint32 LSMSaveMapsListItem::Compare(c3_ListItem *item2, uint32 column)
{

	return 0;
}
