#include "GameWatch/gwciv/GWCivRecord.h"

class GWUnitRecord {
public:
	char name[128];
	int cost;
	int benefit;

	char builderAIP[80];
	char killerAIP[80];
	char killedAIP[80];

	GWUnitRecord *next;
};

DllExport GWCivRecord::GWCivRecord() : GWRecord()
{

	head = NULL;
}


DllExport GWCivRecord::GWCivRecord(void *data, long numOfBytes) : GWRecord()
{

	head = NULL;

	long size = numOfBytes / sizeof(GWUnitRecord);

	GWUnitRecord *unitArray = (GWUnitRecord*)data;


	for(int index = 0; index < size; index++) {

		GWUnitRecord *record = new GWUnitRecord;

		strlcpy(record->name, unitArray[index].name, sizeof(record->name));
		record->cost = unitArray[index].cost;
		record->benefit = unitArray[index].benefit;
		record->next = NULL;

		AddUnitRecord(record);
	}
}

DllExport GWCivRecord::~GWCivRecord()
{

	while(head) {

		GWUnitRecord *record = head;

		head = record->next;

		delete record;
	}
}


DllExport GWRecord* GWCivRecord::Merge(GWRecord *record)
{

	GWCivRecord *newRecord = new GWCivRecord;

	GWUnitRecord *current = head;

	while(current) {

		newRecord->UnitBuilt(current->name, current->cost, current->builderAIP);
		newRecord->UnitKill(current->name, current->benefit, current->killerAIP, current->killedAIP);

		current = current->next;
	}

	current = ((GWCivRecord*)record)->head;
	while(current) {

		newRecord->UnitBuilt(current->name, current->cost, current->builderAIP);
		newRecord->UnitKill(current->name, current->benefit, current->killerAIP, current->killedAIP);

		current = current->next;
	}

	return(newRecord);
}




DllExport char *GWCivRecord::Export(char *baseName, char *stamp)
{

	char fileName[1024];
	snprintf(fileName, sizeof(fileName), "%s.txt", baseName);

	FILE *exportFile = fopen(fileName, "w");
	if(!exportFile) return(NULL);

	if(stamp) fprintf(exportFile, "Unique identifier: %s\n\n", stamp);
	else fprintf(exportFile, "Summary\n\n");

	GWUnitRecord *oldList;
	GWUnitRecord *current;




































































































	oldList = head;

	head = NULL;

	while(oldList) {

		GWUnitRecord *nextRecord = oldList->next;

		oldList->next = NULL;

		SortUnitRecord(oldList);

		oldList = nextRecord;
	}

	fprintf(exportFile, "------------------------------------------------------------------------------\n");
	fprintf(exportFile, "Name                                               Benefit      Cost     Ratio\n");
	fprintf(exportFile, "------------------------------------------------------------------------------\n\n");

	current = head;

	while(current) {

		float strength = current->cost ? (float(current->benefit) / float(current->cost)) : 0.0f;

		fprintf(exportFile, "%-48.48s%10d%10d%10.4g\n", current->name, current->benefit, current->cost, strength);

		current = current->next;
	}

	fclose(exportFile);

	return(strdup(fileName));
}


DllExport void GWCivRecord::GetData(void **data, long *numOfBytes)
{

	long counter = 0;
	GWUnitRecord *current = head;

	while(current) {
		counter++;
		current = current->next;
	}


	dataBuffer.assign(counter, GWUnitRecord());

	*numOfBytes = counter * sizeof(GWUnitRecord);

	counter = 0;
	current = head;

	while(current) {

		strlcpy(dataBuffer[counter].name, current->name, sizeof(dataBuffer[counter].name));
		dataBuffer[counter].cost = current->cost;
		dataBuffer[counter].benefit = current->benefit;
		strlcpy(dataBuffer[counter].builderAIP, current->builderAIP, sizeof(dataBuffer[counter].builderAIP));
		strlcpy(dataBuffer[counter].killerAIP, current->killerAIP, sizeof(dataBuffer[counter].killerAIP));
		strlcpy(dataBuffer[counter].killedAIP, current->killedAIP, sizeof(dataBuffer[counter].killedAIP));

		dataBuffer[counter].next = NULL;

		current = current->next;

		counter++;
	}

	*data = (void*)dataBuffer.data();
}


DllExport void GWCivRecord::UnitBuilt(char *unitName, int unitCost, char *builderAIP)
{

	if(!unitName) return;


	GWUnitRecord *unitRecord = FindUnitRecord(unitName);

	if(unitRecord) {
		unitRecord->cost += unitCost;
	} else {

		unitRecord = new GWUnitRecord;

		strlcpy(unitRecord->name, unitName, sizeof(unitRecord->name));
		unitRecord->cost = unitCost;
		unitRecord->benefit = 0;
		unitRecord->next = NULL;

		if (builderAIP)
			strlcpy(unitRecord->builderAIP, builderAIP, sizeof(unitRecord->builderAIP));
		else
			strlcpy(unitRecord->builderAIP, "Unknown", sizeof(unitRecord->builderAIP));

		AddUnitRecord(unitRecord);
	}
}




DllExport void GWCivRecord::UnitKill(char *unitName, int destroyedUnitCost, char *killerAIP, char *killedAIP)
{

	if(!unitName) return;


	GWUnitRecord *unitRecord = FindUnitRecord(unitName);

	if(unitRecord) {
		unitRecord->benefit += destroyedUnitCost;
	} else {

		unitRecord = new GWUnitRecord;

		strlcpy(unitRecord->name, unitName, sizeof(unitRecord->name));
		unitRecord->cost = 0;
		unitRecord->benefit = destroyedUnitCost;
		unitRecord->next = NULL;

		if (killerAIP)
			strlcpy(unitRecord->killerAIP, killerAIP, sizeof(unitRecord->killerAIP));
		else
			strlcpy(unitRecord->killerAIP, "Unknown", sizeof(unitRecord->killerAIP));

		if (killedAIP)
			strlcpy(unitRecord->killedAIP, killedAIP, sizeof(unitRecord->killedAIP));
		else
			strlcpy(unitRecord->killedAIP, "Unknown", sizeof(unitRecord->killedAIP));

		AddUnitRecord(unitRecord);
	}
}

DllExport GWUnitRecord *GWCivRecord::FindUnitRecord(char *name)
{

	GWUnitRecord *searchRecord = head;

	while(searchRecord) {

		if(!strcmp(name, searchRecord->name)) return(searchRecord);

		searchRecord = searchRecord->next;
	}

	return(NULL);
}


DllExport GWUnitRecord *GWCivRecord::FindUnitRecordForBuilder(char *name, char *builderAIP)
{

	GWUnitRecord *searchRecord = head;

	while(searchRecord) {

		if(!strcmp(name, searchRecord->name)
			&&
			!strcmp(name, searchRecord->builderAIP))
			return(searchRecord);

		searchRecord = searchRecord->next;
	}

	return(NULL);
}

DllExport GWUnitRecord *GWCivRecord::FindUnitRecordForKiller(char *name, char *killerAIP)
{

	GWUnitRecord *searchRecord = head;

	while(searchRecord) {

		if(!strcmp(name, searchRecord->name)
			&&
			!strcmp(name, searchRecord->killerAIP))
			return(searchRecord);

		searchRecord = searchRecord->next;
	}

	return(NULL);
}


DllExport void GWCivRecord::AddUnitRecord(GWUnitRecord *record)
{

	record->next = head;

	head = record;
}

DllExport void GWCivRecord::SortUnitRecord(GWUnitRecord *record)
{

	GWUnitRecord *current = head;


	if(!current || (strcmp(record->name, current->name) < 0)) {

		record->next = head;

		head = record;

		return;
	}


	while(current->next) {

		if(strcmp(record->name, current->next->name) < 0) break;

		current = current->next;
	}

	record->next = current->next;
	current->next = record;
}

DllExport void GWCivRecord::SortUnitRecordByBuilder(GWUnitRecord *record)
{

	GWUnitRecord *current = head;


	if(!current || (strcmp(record->name, current->name) < 0)) {

		record->next = head;

		head = record;

		return;
	}


	while(current->next) {

		if(strcmp(record->builderAIP, current->next->builderAIP) < 0
			&& strcmp(record->name, current->next->name) < 0) break;

		current = current->next;
	}

	record->next = current->next;
	current->next = record;
}

DllExport void GWCivRecord::SortUnitRecordByKiller(GWUnitRecord *record)
{

	GWUnitRecord *current = head;


	if(!current || (strcmp(record->name, current->name) < 0)) {

		record->next = head;

		head = record;

		return;
	}


	while(current->next) {

		if(strcmp(record->killerAIP, current->next->killerAIP) < 0
			&& strcmp(record->name, current->next->name) < 0) break;

		current = current->next;
	}

	record->next = current->next;
	current->next = record;
}
