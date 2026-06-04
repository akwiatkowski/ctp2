//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Checks weather the databases match in MP.
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
// - Replaced old civilisation database by new one. (Aug 20th 2005 Martin G�hmann)
// - Fixed SerializeDBs method to fix the database in saync check. (Aug 25th 2005 Martin G�hmann)
// - Added the risk database for sync check. (Aug 29th 2005 Martin G�hmann)
// - Replaced old difficulty database by new one. (April 29th 2006 Martin G�hmann)
// - Replaced old pollution database by new one. (July 15th 2006 Martin G�hmann)
// - Replaced old global warming database by new one. (July 15th 2006 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "net/general/network.h"
#include "net/general/net_crc.h"
#include "net/io/net_util.h"
#include "robot/aibackdoor/civarchive.h"
#include "gs/utility/Checksum.h"

// Database includes
#include "gs/database/StrDB.h"
#include "gs/database/profileDB.h"

#include "AdvanceRecord.h"
#include "AdvanceBranchRecord.h"
#include "AdvanceListRecord.h"
#include "AgeRecord.h"
#include "AgeCityStyleRecord.h"
#include "BuildingRecord.h"
#include "BuildingBuildListRecord.h"
#include "BuildListSequenceRecord.h"
#include "CitySizeRecord.h"
#include "CityStyleRecord.h"
#include "CivilisationRecord.h"
#include "ConstRecord.h"
#include "DifficultyRecord.h"
#include "DiplomacyRecord.h"
#include "DiplomacyProposalRecord.h"
#include "DiplomacyThreatRecord.h"
#include "EndGameObjectRecord.h"
#include "FeatRecord.h"
#include "GlobalWarmingRecord.h"
#include "GoalRecord.h"
#include "GovernmentRecord.h"
#include "IconRecord.h"
#include "ImprovementListRecord.h"
#include "OrderRecord.h"
#include "gs/database/UVDB.h"                      // Ozone database
#include "PersonalityRecord.h"
#include "PollutionRecord.h"
#include "PopRecord.h"
#include "ResourceRecord.h"
#include "RiskRecord.h"
#include "SoundRecord.h"
#include "SpecialAttackInfoRecord.h"
#include "SpecialEffectRecord.h"
#include "TerrainRecord.h"
#include "SpriteRecord.h"
#include "StrategyRecord.h"
#include "TerrainRecord.h"
#include "TerrainImprovementRecord.h"
#include "UnitRecord.h"
#include "UnitBuildListRecord.h"
#include "WonderRecord.h"
#include "WonderBuildListRecord.h"

#include "ctp/civapp.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"



extern OzoneDatabase           *g_theUVDB;


//----------------------------------------------------------------------------
//
// Name       : NetFeatTracker::NetFeatTracker
//
// Description: Constructor
//
// Parameters : sint32 startat: First database to check
//              sint32 stopat:  Last database to check
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : No idea why there should be just a subset of the databases
//              to check actual they should be checked all
//
//----------------------------------------------------------------------------
NetCRC::NetCRC(sint32 startat, sint32 stopat)
:	Packetizer()
{
	m_startAt = startat;
	m_stopAt = stopat;
}

//----------------------------------------------------------------------------
//
// Name       : NetFeatTracker::NetFeatTracker
//
// Description: Constructor
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
NetCRC::NetCRC()
:	Packetizer()
{
	m_startAt = 0;
	m_stopAt = k_MAX_DBS - 1;
}

//----------------------------------------------------------------------------
//
// Name       : CHECKDB(db)
//
// Description: Macro for databse checking
//
// Parameters : db: The databse to check
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : Database must be serializable
//
//----------------------------------------------------------------------------
#define CHECKDB(db) \
	if(dbnum < m_startAt || dbnum > m_stopAt) { \
		dbnum++; \
	} else { \
		Assert(dbnum < k_MAX_DBS); \
		archive = new CivArchive; \
		check = new CheckSum; \
		archive->SetStore(); \
		db->Serialize(*archive); \
		check->AddData(archive->GetStream(), archive->StreamLen()); \
		check->Done(m_db_crc[dbnum][0], m_db_crc[dbnum][1], m_db_crc[dbnum][2], m_db_crc[dbnum][3]); \
		delete archive; \
		delete check; \
		dbnum++; \
		numchecked++; \
	}

//----------------------------------------------------------------------------
//
// Name       : NetCRC::SerializeDBs
//
// Description: Serializes the databases and creates check sums from the
//              archieves, so that the databases can be checked for synchronicity.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
sint32 NetCRC::SerializeDBs()
{
    // Phase 0.C-4: body gutted — used CivArchive to serialize every DB.
    // Net code is disabled and CivArchive is going away.
    return 0;
}

//----------------------------------------------------------------------------
//
// Name       : NetFeatTracker::Packetize
//
// Description: Generate an application data packet to transmit.
//
// Parameters : buf         : buffer to store the message
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetCRC::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_CRC_ID);

	sint32 num = SerializeDBs();

	PUSHLONG(m_startAt);
	PUSHLONG(m_stopAt);
	PUSHLONG(num);
	for(sint32 j = 0; j < num; j++) {
		for(sint32 i = 0; i < 4; i++) {
			PUSHLONG(m_db_crc[j+m_startAt][i]);
		}
	}
}

//----------------------------------------------------------------------------
//
// Name       : NetFeatTracker::Unpacketize
//
// Description: Retrieve the data from a received application data packet.
//
// Parameters : id          : Sender identification?
//              buf         : Buffer with received message
//              size        : Length of received message (in bytes)
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetCRC::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	uint16 packid;
	uint16 pos = 0;

	PULLID(packid);
	Assert(packid == k_PACKET_CRC_ID);

	PULLLONG(m_startAt);
	PULLLONG(m_stopAt);
	sint32 num = SerializeDBs();
	sint32 remoteNum;
	PULLLONG(remoteNum);

	if(num != remoteNum) {
		// No idea what this should do, this can only happen if the
		// remote executable is compiled from a different version
		// of the source code.
		Error("Number of databases doesn't even match, sheesh!");
		return;
	}

	sint32 i;
	sint32 j;
	uint32 part;
	BOOL alreadybad;

	for(j = 0; j < num; j++) {
		alreadybad = FALSE;
		for(i = 0; i < 4; i++) {
			PULLLONG(part);
			if(!alreadybad && (part != m_db_crc[j+m_startAt][i])) {
				char buf[2048];
				snprintf(buf, sizeof(buf), "Database #%d is out of synch", j + m_startAt);
				Error(buf);
				alreadybad = TRUE;

			}
		}
	}
}

//----------------------------------------------------------------------------
//
// Name       : NetFeatTracker::Packetize
//
// Description: Generate an application data packet to transmit.
//
// Parameters : buf         : buffer to store the message
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetCRC::Error(char *buf)
{
	extern void network_AbortCallback( sint32 type );

	DPRINTF(k_DBG_NET, ("NetCRC: %s\n", buf));
	const char *str = stringdb_Get()->GetNameStr("str_ldl_mp_dbase_out_of_synch");
	char nonConstStr[1024];
	if (str) {
		strncpy(nonConstStr, str, sizeof(nonConstStr) - 1);
		nonConstStr[sizeof(nonConstStr) - 1] = '\0';
	} else {
		strncpy(nonConstStr, "Databases out of sync, returning to lobby", sizeof(nonConstStr) - 1);
		nonConstStr[sizeof(nonConstStr) - 1] = '\0';
	}
	c3_RemoveAbortMessage();
	civapp_Get()->ProcessGraphicsCallback();
	c3_AbortMessage(nonConstStr, k_UTILITY_ABORT, network_AbortCallback );
	g_network.SetCRCError();
}
