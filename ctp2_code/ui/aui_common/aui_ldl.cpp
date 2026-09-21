//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Activision User Interface - ldl handling
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
// - Fixed memory leaks.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_common/aui_ldl.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_dimension.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "robot/aibackdoor/avl.h"

#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3windows.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/ctp2_menubar.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "ui/aui_ctp2/ctp2_listitem.h"
#include "ui/aui_ctp2/ctp2_dropdown.h"
#include "ui/aui_ctp2/ctp2_spinner.h"
#include "ui/aui_ctp2/ctp2_hypertextbox.h"
#include "ui/aui_ctp2/c3_hypertextbox.h"
#include "ui/aui_common/aui_switch.h"
#include "gfx/gfx_utils/colorset.h"
#include "ui/aui_ctp2/ctp2_Switch.h"
#include "ui/aui_ctp2/ctp2_textfield.h"
#include "ui/aui_ctp2/ctp2_MenuButton.h"
#include "ui/aui_ctp2/ctp2_TabGroup.h"
#include "ui/aui_ctp2/ctp2_Tab.h"
#include "ui/aui_ctp2/ctp2_TabButton.h"
#include "ui/aui_ctp2/ctp2_TabGroup.h"
#include "ui/aui_ctp2/radarmap.h"
#include "ui/aui_ctp2/linegraph.h"
#include "ctp/ctp2_utils/AvlTree.h"
#include "ui/ldl/ldl_user.h"
#include <unistd.h>

std::unique_ptr<ldl> aui_Ldl::s_ldl;

std::unique_ptr<AvlTree<aui_LdlObject *>>	aui_Ldl::s_objectListByObject;
std::unique_ptr<AvlTree<aui_LdlObject *>>	aui_Ldl::s_objectListByString;

sint32						aui_Ldl::s_ldlRefCount = 0;

aui_LdlObject				*aui_Ldl::s_objectList = nullptr;
aui_LdlObject				*aui_Ldl::s_objectListTail = nullptr;










static cmp_t CompareByObject(aui_LdlObject *obj1, aui_LdlObject *obj2)
{
	if (obj1->object < obj2->object) return MIN_CMP;
	if (obj1->object > obj2->object) return MAX_CMP;

	return EQ_CMP;
}

static cmp_t CompareByString(aui_LdlObject *obj1, aui_LdlObject *obj2)
{
	if (obj1->hash < obj2->hash) return MIN_CMP;
	if (obj1->hash > obj2->hash) return MAX_CMP;

	return EQ_CMP;
}

aui_Ldl::aui_Ldl
(
	AUI_ERRCODE *   retval,
	MBCHAR const *  ldlFilename
)
:
	aui_Base    ()
{
	*retval = InitCommon( ldlFilename );
}


AUI_ERRCODE aui_Ldl::InitCommon( MBCHAR const *ldlFilename )
{
	Assert(ldlFilename != nullptr);
	if (!ldlFilename) return AUI_ERRCODE_INVALIDPARAM;

	if (!s_objectListByObject)
	{
		s_objectListByObject = std::make_unique<AvlTree<aui_LdlObject *>>();
		Assert( s_objectListByObject != nullptr );
		if ( !s_objectListByObject ) return AUI_ERRCODE_MEMALLOCFAILED;
    }

    if (!s_objectListByString)
    {
		s_objectListByString = std::make_unique<AvlTree<aui_LdlObject *>>();
		Assert( s_objectListByString != nullptr );
		if ( !s_objectListByString ) return AUI_ERRCODE_MEMALLOCFAILED;
	}

	MBCHAR outDir[ MAX_PATH + 1 ];
#ifdef HAVE_UNISTD_H
	getcwd(outDir, MAX_PATH);
#else
	GetCurrentDirectory( MAX_PATH, outDir );
#endif
	strlcat(outDir, FILE_SEP "ldl_out", sizeof(outDir));

    s_ldl.reset();
	s_ldl = std::make_unique<ldl>( ldlFilename, outDir );
	Assert( s_ldl != nullptr );
	if ( !s_ldl ) return AUI_ERRCODE_MEMALLOCFAILED;

	s_ldlRefCount++;

	return AUI_ERRCODE_OK;
}


aui_Ldl::~aui_Ldl()
{
    if (0 == --s_ldlRefCount)
    {
    	s_ldl.reset();
	    s_objectListByObject.reset();
	    s_objectListByString.reset();

        aui_LdlObject * nextObject  = nullptr;
	    for (aui_LdlObject * curObject = s_objectList; curObject; curObject = nextObject)
	    {
		    nextObject = curObject->next;
		    DeleteLdlObject(curObject);
	    }
    }
}


bool aui_Ldl::IsValid(MBCHAR const * ldlBlock)
{
	return FindDataBlock(ldlBlock) != nullptr;
}


AUI_ERRCODE aui_Ldl::MakeSureBlockExists(MBCHAR const *ldlBlock)
{
	Assert( ldlBlock != nullptr );
	if ( !ldlBlock ) return AUI_ERRCODE_INVALIDPARAM;

	ldl_datablock *block = s_ldl->FindDataBlock( ldlBlock );
	if ( !block )
	{





#if 1





		block = std::make_unique<ldl_datablock>(s_ldl.get(), ldlBlock).release();
		Assert( block != nullptr );
		if ( !block ) return AUI_ERRCODE_MEMALLOCFAILED;

		block->AddAttribute( k_AUI_LDL_HANCHOR, "left" );

		block->AddAttribute( k_AUI_LDL_VANCHOR, "top" );

		block->AddAttribute( k_AUI_LDL_HABSPOSITION, 0 );
		block->AddAttribute( k_AUI_LDL_VABSPOSITION, 0 );
		block->AddAttribute( k_AUI_LDL_HABSSIZE, 100 );
		block->AddAttribute( k_AUI_LDL_VABSSIZE, 100 );

		BOOL success = s_ldl->WriteData();
		Assert( success );
		if ( !success ) return AUI_ERRCODE_LDLFILEWRITEFAILED;
#else
		return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;
#endif
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Ldl::MakeSureDefaultTemplateExists( )
{
	ldl_datablock *format = s_ldl->FindDataBlock( k_AUI_LDL_DEFAULTTEMPLATE );
	if ( !format )
	{

#if 1

		format = std::make_unique<ldl_datablock>( s_ldl.get(), k_AUI_LDL_DEFAULTTEMPLATE ).release();
		Assert( format != nullptr );
		if ( !format ) return AUI_ERRCODE_MEMALLOCFAILED;

		format->AddAttribute( k_AUI_LDL_HANCHOR, "left" );

		format->AddAttribute( k_AUI_LDL_VANCHOR, "top" );

		format->AddAttribute( k_AUI_LDL_HABSPOSITION, 0 );
		format->AddAttribute( k_AUI_LDL_VABSPOSITION, 0 );
		format->AddAttribute( k_AUI_LDL_HABSSIZE, 100 );
		format->AddAttribute( k_AUI_LDL_VABSSIZE, 100 );

		BOOL success = s_ldl->WriteData();
		Assert( success );
		if ( !success ) return AUI_ERRCODE_LDLFILEWRITEFAILED;
#else
		return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;
#endif
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Ldl::Associate(void *object, MBCHAR const * ldlBlock)
{

	if ( !s_objectListByObject || !s_objectListByString ) return AUI_ERRCODE_INVALIDPARAM;

	Assert( object != nullptr );
	if ( !object ) return AUI_ERRCODE_INVALIDPARAM;
	Assert( ldlBlock != nullptr );
	if ( !ldlBlock ) return AUI_ERRCODE_INVALIDPARAM;

	if ( GetBlock( object ) ) return AUI_ERRCODE_OK;

	auto ldlObject = std::make_unique<aui_LdlObject>();
	Assert( ldlObject != nullptr );
	if ( !ldlObject ) return AUI_ERRCODE_MEMALLOCFAILED;

	ldlObject->ldlBlock = std::make_unique<MBCHAR[]>( strlen( ldlBlock ) + 1 );
	Assert( ldlObject->ldlBlock != nullptr );
	if ( !ldlObject->ldlBlock ) return AUI_ERRCODE_MEMALLOCFAILED;

	ldlObject->object = object;
	strlcpy( ldlObject->ldlBlock.get(), ldlBlock, strlen( ldlBlock ) + 1 );
	ldlObject->hash = aui_UI::CalculateHash( ldlBlock );




	auto objByObject = std::make_unique<Comparable<aui_LdlObject *>>(ldlObject.get(), CompareByObject);
	if(!s_objectListByObject->Insert(objByObject.get()))
		objByObject.release();

	auto objByString = std::make_unique<Comparable<aui_LdlObject *>>(ldlObject.get(), CompareByString);
	if(!s_objectListByString->Insert(objByString.get()))
		objByString.release();

	return AppendLdlObject(ldlObject.release());
}

AUI_ERRCODE aui_Ldl::AppendLdlObject(aui_LdlObject *object)
{
	if (nullptr == object)
		return AUI_ERRCODE_INVALIDPARAM;

	object->next = nullptr;
	object->prev = s_objectListTail;

	if (s_objectListTail)
    {
		s_objectListTail->next = object;
    }
    else
    {
		s_objectList = object;
	}

	s_objectListTail = object;

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Ldl::RemoveLdlObject(aui_LdlObject *object)
{
    if (nullptr == object)
		return AUI_ERRCODE_INVALIDPARAM;

	if (s_objectList == object)
    {
        // Object at begin
        s_objectList = object->next;

		if (s_objectList)
        {
			s_objectList->prev  = nullptr;
		}
        else
        {
			s_objectListTail    = nullptr;
		}
	}
    else if (s_objectListTail == object)
    {
        // Object at end
		s_objectListTail = object->prev;
	    s_objectListTail->next  = object->next;
    }
    else
    {
        // Object in between
		object->prev->next = object->next;
		object->next->prev = object->prev;
	}

    object->next    = nullptr;
    object->prev    = nullptr;
	return AUI_ERRCODE_OK;
}


void aui_Ldl::DeleteLdlObject( aui_LdlObject *ldlObject )
{
	// ldlObject->ldlBlock is a unique_ptr, auto-freed
	std::unique_ptr<aui_LdlObject> deleter(ldlObject);
}


AUI_ERRCODE aui_Ldl::Remove( void *object )
{
	if (!object)
		return AUI_ERRCODE_INVALIDPARAM;

	if ( !s_objectListByObject || !s_objectListByString)
		return AUI_ERRCODE_INVALIDPARAM;

	aui_LdlObject		dummyObject;
	dummyObject.object = object;

	Comparable<aui_LdlObject *>	*   myKey =
        s_objectListByObject->Search(&dummyObject);

	if (myKey) {
		aui_LdlObject		*foundObject = myKey->Key();

		if (foundObject) {

			std::unique_ptr<Comparable<aui_LdlObject *>> byString(s_objectListByString->Delete(foundObject));

			std::unique_ptr<Comparable<aui_LdlObject *>> byObject(s_objectListByObject->Delete(foundObject));

			RemoveLdlObject(foundObject);

			DeleteLdlObject(foundObject);
		}
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_Ldl::Remove(MBCHAR const * ldlBlock)
{

	if ( !s_objectListByObject || !s_objectListByString) return AUI_ERRCODE_INVALIDPARAM;


	uint32 hash = aui_UI::CalculateHash( ldlBlock );


	aui_LdlObject		myObject;
	myObject.hash = hash;

	std::unique_ptr<Comparable<aui_LdlObject *>> myKey(
		s_objectListByString->Delete(&myObject));

	if (myKey) {

		aui_LdlObject	*ldlObject = myKey->Key();

		if (ldlObject) {
			s_objectListByString->Delete(ldlObject);

			RemoveLdlObject(ldlObject);

			DeleteLdlObject(ldlObject);
		}
	}

	return AUI_ERRCODE_OK;
}


MBCHAR *aui_Ldl::GetBlock( void *object )
{

	if ( !s_objectListByObject ) return nullptr;


	aui_LdlObject	dummyObject;
	dummyObject.object = object;

	Comparable<aui_LdlObject *> *myKey =
        s_objectListByObject->Search(&dummyObject);

	if (myKey) {

		aui_LdlObject *obj = myKey->Key();
		Assert(obj);
		if (obj) {
			return obj->ldlBlock.get();
		}
	}

	return nullptr;
}


void *aui_Ldl::GetObject( const MBCHAR *ldlBlock )
{

	if ( !s_objectListByString ) return nullptr;

	uint32 hash = aui_UI::CalculateHash( ldlBlock );

	aui_LdlObject	dummyObject;
	dummyObject.hash = hash;

	Comparable<aui_LdlObject *>	*myKey =
        s_objectListByString->Search(&dummyObject);

	if (myKey) {

		aui_LdlObject *obj = myKey->Key();
		Assert(obj);
		if (obj) {
			return obj->object;
		}
	}

	return nullptr;
}

void *aui_Ldl::GetObject(const MBCHAR *parentBlock, const MBCHAR *regionBlock)
{
	Assert(parentBlock && regionBlock);
	if (parentBlock == nullptr || regionBlock == nullptr)
		return nullptr;

	MBCHAR		ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(ldlBlock, sizeof(ldlBlock), "%s.%s", parentBlock, regionBlock);

	return GetObject(ldlBlock);
}


AUI_ERRCODE aui_Ldl::SetupHeirarchyFromRoot(MBCHAR const * rootBlock )
{

	if ( !s_objectListByString ) return AUI_ERRCODE_INVALIDPARAM;

	Assert( rootBlock != nullptr );
	if ( !rootBlock ) return AUI_ERRCODE_INVALIDPARAM;

	uint32 hash = aui_UI::CalculateHash( rootBlock );

	aui_LdlObject dummyObject;
	dummyObject.hash = hash;

	Comparable<aui_LdlObject *> *myKey =
        s_objectListByString->Search(&dummyObject);

	if (myKey) {
		aui_LdlObject	*obj = myKey->Key();

		if (obj)
        {
			size_t len = strlen( rootBlock );

			for
            (
                aui_LdlObject * curObject = obj->next;
                curObject;
                curObject = curObject->next
            )
            {
				if ( strnicmp( rootBlock, curObject->ldlBlock.get(), len ) == 0 )
				{

					AUI_ERRCODE errcode = SetupHeirarchyFromLeaf(
						curObject->ldlBlock.get(),
						(aui_Region *)curObject->object );
					Assert( AUI_SUCCESS(errcode) );
					if ( !AUI_SUCCESS(errcode) ) return errcode;
				}
			}

			return AUI_ERRCODE_OK;
		}
	}

	Assert( FALSE );
	return AUI_ERRCODE_HACK;
}


AUI_ERRCODE aui_Ldl::SetupHeirarchyFromLeaf(MBCHAR * leafBlock, aui_Region * object)
{

	if ( !s_objectListByObject || !s_objectListByString ) return AUI_ERRCODE_INVALIDPARAM;

	Assert( leafBlock != nullptr && object != nullptr );
	if ( !leafBlock || !object )
		return AUI_ERRCODE_INVALIDPARAM;

	MBCHAR * lastDot = strrchr(leafBlock, '.');
	if ( !lastDot )
		return AUI_ERRCODE_OK;

	if ( object->GetParent() )
		return AUI_ERRCODE_OK;

	*lastDot = '\0';

	aui_Region *parent = (aui_Region *)GetObject( leafBlock );
	Assert( parent != nullptr );
	if ( !parent )
		return AUI_ERRCODE_HACK;

	parent->AddChild( object );

	AUI_ERRCODE errcode = SetupHeirarchyFromLeaf( leafBlock, parent );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) )
		return errcode;

	*lastDot = '.';

	return AUI_ERRCODE_OK;
}








aui_Region *aui_Ldl::BuildHierarchyFromRoot(MBCHAR const * rootBlock)
{

	Assert(s_objectListByObject);
	Assert(s_objectListByString);

	if ( !s_objectListByObject || !s_objectListByString )
		return nullptr;

	Assert(rootBlock);
	if (rootBlock == nullptr)
		return nullptr;


	ldl_datablock * dataBlock = s_ldl->FindDataBlock(rootBlock, nullptr);
	Assert(dataBlock);
	if (!dataBlock)
		return nullptr;

	MBCHAR const		*objTypeString = dataBlock->GetString(k_AUI_LDL_OBJECTTYPE);
	if (!objTypeString)
		return nullptr;


	bool			isAtomic = dataBlock->GetBool(k_AUI_LDL_ATOMIC);


	char fullname[256];
	dataBlock->GetFullName(fullname, sizeof(fullname));

	aui_Region *    myRegion    = nullptr;
	AUI_ERRCODE		err         = BuildObjectFromType(objTypeString, fullname, &myRegion);
	Assert(err == AUI_ERRCODE_OK);
	if (err != AUI_ERRCODE_OK)
		return nullptr;

	if (!isAtomic) {
		err = BuildHierarchyFromLeaf(dataBlock, myRegion);
		Assert(err == AUI_ERRCODE_OK);
		if (err != AUI_ERRCODE_OK)
			return nullptr;
	}

	err = SetupHeirarchyFromRoot(rootBlock);
	Assert(err == AUI_ERRCODE_OK);
	if (err != AUI_ERRCODE_OK)
		return nullptr;

	if (dataBlock->GetBool(k_AUI_LDL_DETACH))
    {
		DetachHierarchy(myRegion);
	}

    myRegion->DoneInstantiating();

	return myRegion;
}






AUI_ERRCODE aui_Ldl::BuildHierarchyFromLeaf(ldl_datablock *parent, aui_Region *region)
{
	Assert(s_objectListByObject && s_objectListByString && parent);
	if ( !s_objectListByObject || !s_objectListByString || !parent)
		return AUI_ERRCODE_INVALIDPARAM;

	PointerList<ldl_datablock> *    childList = parent->GetChildList();
	for
    (
	    PointerList<ldl_datablock>::Walker walk(childList);
        walk.IsValid();
        walk.Next()
    )
    {
	    ldl_datablock * dataBlock = walk.GetObj();
		Assert(dataBlock);
		if (dataBlock == nullptr)
			return AUI_ERRCODE_INVALIDPARAM;




		MBCHAR const		*objTypeString = dataBlock->GetString(k_AUI_LDL_OBJECTTYPE);
		if (!objTypeString)
			return AUI_ERRCODE_INVALIDPARAM;


		BOOL			isAtomic = dataBlock->GetBool(k_AUI_LDL_ATOMIC);


		char fullname[256];
		dataBlock->GetFullName(fullname, sizeof(fullname));

		aui_Region *    myRegion;
		AUI_ERRCODE		err = BuildObjectFromType(objTypeString, fullname, &myRegion);
		Assert(err == AUI_ERRCODE_OK);
		if (err != AUI_ERRCODE_OK)
			return AUI_ERRCODE_INVALIDPARAM;




		if (!isAtomic) {
			err = BuildHierarchyFromLeaf(dataBlock, myRegion);
			Assert(err == AUI_ERRCODE_OK);
			if (err != AUI_ERRCODE_OK)
				return err;
		}

		region->AddChild(myRegion);
	}

	return AUI_ERRCODE_OK;
}































AUI_ERRCODE aui_Ldl::BuildObjectFromType(MBCHAR const *typeString,
										 MBCHAR const *ldlName,
										 aui_Region **theObject)
{
	AUI_ERRCODE		retval = AUI_ERRCODE_OK;
	std::unique_ptr<aui_Region>	region;

	if (!stricmp(typeString, "C3Window")) {
		region = std::make_unique<C3Window>(&retval, aui_UniqueId(), ldlName, 16);
	} else
	if(!stricmp(typeString, "ctp2_Window")) {
		region = std::make_unique<ctp2_Window>(&retval, aui_UniqueId(), ldlName, 16);
	} else
	if (!stricmp(typeString, "C3Slider")) {
		region = std::make_unique<C3Slider>(&retval, aui_UniqueId(), ldlName);
	} else
	if (!stricmp(typeString, "c3_Button")) {
		region = std::make_unique<c3_Button>(&retval, aui_UniqueId(), ldlName);
	} else
	if (!stricmp(typeString, "c3_Static")) {
		region = std::make_unique<c3_Static>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_Static")) {
		region = std::make_unique<ctp2_Static>(&retval, aui_UniqueId(), ldlName);
	} else
	if (!stricmp(typeString, "ctp2_Button")) {
		region = std::make_unique<ctp2_Button>(&retval, aui_UniqueId(), ldlName);
	} else
	if (!stricmp(typeString, "ctp2_ListBox")) {
		region = std::make_unique<ctp2_ListBox>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_LineGraph")) {
		region = std::make_unique<LineGraph>(&retval, aui_UniqueId(), ldlName);
	} else
	if (!stricmp(typeString, "ctp2_ListItem")) {
		region = std::make_unique<ctp2_ListItem>(&retval, ldlName);
	} else
	if (!stricmp(typeString, "ctp2_DropDown")) {
		region = std::make_unique<ctp2_DropDown>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString,  "ctp2_Spinner")) {
		region = std::make_unique<ctp2_Spinner>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_HyperTextBox")) {
		region = std::make_unique<ctp2_HyperTextBox>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "c3_HyperTextBox")) {
		region = std::make_unique<c3_HyperTextBox>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_MenuBar")) {
		region = std::make_unique<ctp2_MenuBar>(&retval, aui_UniqueId(), ldlName, 16);
	} else
	if(!stricmp(typeString, "aui_switch")) {
		region = std::make_unique<aui_Switch>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_Switch")) {
		region = std::make_unique<ctp2_Switch>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_TextField")) {
		region = std::make_unique<ctp2_TextField>(&retval, aui_UniqueId(), ldlName);
	} else
	if(!stricmp(typeString, "ctp2_MenuButton")) {
		region = std::make_unique<ctp2_MenuButton>(&retval, aui_UniqueId(), ldlName);
	}
	if(!stricmp(typeString, "ctp2_Tab")) {
		region = std::make_unique<ctp2_Tab>(&retval, aui_UniqueId(), ldlName);
	}
	if(!stricmp(typeString, "ctp2_TabGroup")) {
		region = std::make_unique<ctp2_TabGroup>(&retval, aui_UniqueId(), ldlName);
	}
	if(!stricmp(typeString, "ctp2_TabButton")) {
		region = std::make_unique<ctp2_TabButton>(&retval, aui_UniqueId(), ldlName);
	}
	if(!stricmp(typeString, "RadarMap")) {
		region = std::make_unique<RadarMap>(&retval, aui_UniqueId(), ldlName);
	}

	Assert(region);
	Assert(retval == AUI_ERRCODE_OK);
	if (!region || retval != AUI_ERRCODE_OK) {

		c3errors_ErrorDialog("aui_Ldl::BuildObjectFromType()",
							 "Auto-instantiate of '%s' failed on '%s'",
							 typeString,
							 ldlName);

		*theObject = nullptr;

		return AUI_ERRCODE_INVALIDPARAM;
	}

	*theObject = region.release();

	return AUI_ERRCODE_OK;
}






AUI_ERRCODE aui_Ldl::DeleteHierarchyFromRoot(MBCHAR const * rootBlock)
{

	Assert(s_objectListByObject);
	Assert(s_objectListByString);

	if ( !s_objectListByObject || !s_objectListByString )
		return AUI_ERRCODE_INVALIDPARAM;

	Assert(rootBlock);
	if (rootBlock == nullptr) return AUI_ERRCODE_INVALIDPARAM;


	ldl_datablock * dataBlock   = s_ldl->FindDataBlock(rootBlock, nullptr);
	Assert(dataBlock);
	if (!dataBlock) return AUI_ERRCODE_INVALIDPARAM;


	AUI_ERRCODE		errcode     = DeleteHierarchyFromLeaf(dataBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK)
		return errcode;

	char fullname[256];
	dataBlock->GetFullName(fullname, sizeof(fullname));
	aui_Region *region = (aui_Region *)GetObject(fullname);
	Assert(region);
	if (region) {
		std::unique_ptr<aui_Region> deleter(region);
	} else {
		return AUI_ERRCODE_INVALIDPARAM;
	}

	return AUI_ERRCODE_OK;
}







AUI_ERRCODE aui_Ldl::DeleteHierarchyFromLeaf(ldl_datablock *parent)
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	aui_Region		*region;
	ldl_datablock *dataBlock;

	if (parent == nullptr) return AUI_ERRCODE_OK;

	PointerList<ldl_datablock> *childList = parent->GetChildList();
	PointerList<ldl_datablock>::Walker walk(childList);
	while(walk.IsValid()) {
		dataBlock = walk.GetObj();




		BOOL			isAtomic = dataBlock->GetBool(k_AUI_LDL_ATOMIC);

		if (!isAtomic) {
			errcode = DeleteHierarchyFromLeaf(dataBlock);
			Assert(errcode == AUI_ERRCODE_OK);
			if (errcode != AUI_ERRCODE_OK)
				return errcode;
		}

		char fullname[256];
		region = (aui_Region *)GetObject(dataBlock->GetFullName(fullname, sizeof(fullname)));
		Assert(region);
		if (!region)
			return AUI_ERRCODE_INVALIDPARAM;

		std::unique_ptr<aui_Region> deleter(region);
		walk.Next();
	}

	return errcode;
}








AUI_ERRCODE aui_Ldl::SetActionFuncAndCookie
(
    MBCHAR const *                          ldlBlock,
	aui_Control::ControlActionCallback *    actionFunc,
	void *                                  cookie
)
{
	aui_Control	*   control = (aui_Control *) aui_Ldl::GetObject(ldlBlock);
    AUI_ERRCODE		errcode = control
                              ? control->SetActionFuncAndCookie(actionFunc, cookie)
                              : AUI_ERRCODE_NOCONTROL;
	Assert(errcode == AUI_ERRCODE_OK);

	return errcode;
}

AUI_ERRCODE aui_Ldl::SetActionFuncAndCookie
(
    MBCHAR const *                          parentBlock,
    MBCHAR const *                          regionBlock,
	aui_Control::ControlActionCallback *    actionFunc,
	void *                                  cookie
)
{
	Assert(parentBlock && regionBlock);
	if (!parentBlock || !regionBlock)
		return AUI_ERRCODE_INVALIDPARAM;

	MBCHAR		ldlBlock[k_AUI_LDL_MAXBLOCK + 1];
	snprintf(ldlBlock, sizeof(ldlBlock), "%s.%s", parentBlock, regionBlock);

	return SetActionFuncAndCookie(ldlBlock, actionFunc, cookie);
}


AUI_ERRCODE aui_Ldl::DetachHierarchy(aui_Region *region)
{
	if (region)
    {
	    sint32 numChildren = region->NumChildren();

	    for (sint32 i = 0; i < numChildren; i++)
        {
		    DetachHierarchy(region->GetChildByIndex(i));
	    }

	    Remove((void *)region);
    }

	return AUI_ERRCODE_OK;
}

sint32 aui_Ldl::GetIntDependent(MBCHAR const * strPtr)
{
	Assert( strPtr != nullptr );
	if ( !strPtr ) return AUI_ERRCODE_INVALIDPARAM;

	sint32 width = aui_ui_Get()->Width();
	sint32 height = aui_ui_Get()->Height();

	for ( ; strPtr; ++strPtr)
	{
		sint32 w;
		sint32 h;
		sint32 value;
		if ( sscanf( strPtr, "%dx%d?%d", &w, &h, &value ) != 3 )
			if ( sscanf( strPtr, "%dX%d?%d", &w, &h, &value ) != 3 )
				break;

		if ( w == width && h == height ) return value;

        strPtr = strchr(strPtr, ':');
	}

	return 0;
}

void aui_Ldl::ModifyAttributes( MBCHAR const * ldlBlock, aui_Dimension * dimension)
{
	ldl_datablock * format = s_ldl->FindDataBlock(ldlBlock);

	if (format && dimension)
    {
		sint32 x      = dimension->HorizontalPositionData();
		sint32 y      = dimension->VerticalPositionData();
		sint32 width  = dimension->HorizontalSizeData();
		sint32 height = dimension->VerticalSizeData();

		if ( dimension->GetHorizontalPositionType() == AUI_DIMENSION_HPOSITION_ABSOLUTE ) {
			if ( format->AttributeNameTaken( k_AUI_LDL_HABSPOSITION ) )
				format->SetValue( k_AUI_LDL_HABSPOSITION, x );
			else
				format->AddAttribute( k_AUI_LDL_HABSPOSITION, x );
		} else {
			if ( format->AttributeNameTaken( k_AUI_LDL_HRELPOSITION ) )
				format->SetValue( k_AUI_LDL_HRELPOSITION, x );
			else
				format->AddAttribute( k_AUI_LDL_HRELPOSITION, x );
		}

		if ( dimension->GetVerticalPositionType() == AUI_DIMENSION_VPOSITION_ABSOLUTE ) {
			if ( format->AttributeNameTaken( k_AUI_LDL_VABSPOSITION ) )
				format->SetValue( k_AUI_LDL_VABSPOSITION, y );
			else
				format->AddAttribute( k_AUI_LDL_VABSPOSITION, y );
		} else {
			if ( format->AttributeNameTaken( k_AUI_LDL_VRELPOSITION ) )
				format->SetValue( k_AUI_LDL_VRELPOSITION, y );
			else
				format->AddAttribute( k_AUI_LDL_VRELPOSITION, y );
		}

		if ( dimension->GetHorizontalSizeType() == AUI_DIMENSION_HSIZE_ABSOLUTE ) {
			if ( format->AttributeNameTaken( k_AUI_LDL_HABSSIZE ) )
				format->SetValue( k_AUI_LDL_HABSSIZE, width );
			else
				format->AddAttribute( k_AUI_LDL_HABSSIZE, width );
		} else {
			if ( format->AttributeNameTaken( k_AUI_LDL_HRELSIZE ) )
				format->SetValue( k_AUI_LDL_HRELSIZE, width );
			else
				format->AddAttribute( k_AUI_LDL_HRELSIZE, width );
		}

		if ( dimension->GetVerticalSizeType() == AUI_DIMENSION_VSIZE_ABSOLUTE ) {
			if ( format->AttributeNameTaken( k_AUI_LDL_VABSSIZE ) )
				format->SetValue( k_AUI_LDL_VABSSIZE, height );
			else
				format->AddAttribute( k_AUI_LDL_VABSSIZE, height );
		} else {
			if ( format->AttributeNameTaken( k_AUI_LDL_VRELSIZE ) )
				format->SetValue( k_AUI_LDL_VRELSIZE, height );
			else
				format->AddAttribute( k_AUI_LDL_VRELSIZE, height );
		}

	}
}

ldl_datablock * aui_Ldl::FindDataBlock(MBCHAR const * ldlBlock)
{
    return s_ldl ? s_ldl->FindDataBlock(ldlBlock) : nullptr;
}
