//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : SLIC interpreter functions
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
// _DEBUG
// - Generate debug version
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added slic database access by Martin G�hmann
// - Added a way to find out the size of a slic database by Martin G�hmann
// - slicif_cleanup() added.
// - Fixed slic database access after a reload by Martin G�hmann.
// - Types corrected.
// - Added debugging code for '**' operator
// - Replaced slicif_is_sym by slicif_is_name function. This function is
//   modelled slicif_find_db_index but without error message if this
//   function fails to retrieve the database index. - Feb. 24th 2005 Martin G�hmann
// - Added debugging code for bitwise operator
// - Prevented crash with invalid Slic input.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Added database array access. (Sep 16th 2005 Martin G�hmann)
// - Slic file paths are even saved if slic debugging is off, so that there is
//   no problem if slic debugging is set on without reloading slic. (24-Feb-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/slic/slicif.h"

FILE *debuglog = nullptr;
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <vector>

#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSymbol.h"
#include "gs/events/GameEventManager.h"
#include "gs/slic/SlicStruct.h"
#include "gs/slic/SlicNamedSymbol.h"
#include "gs/slic/SlicArray.h"
#include "gs/database/profileDB.h"

#include "gs/slic/SlicDBConduit.h"

#define k_MAX_PAREN_LEVEL 128

template<typename T, typename PtrT>
static void slicif_store(PtrT ptr, T value) {
    memcpy(ptr, &value, sizeof(T));
}

template<typename T, typename PtrT>
static void slicif_emit(PtrT &ptr, T value) {
    memcpy(ptr, &value, sizeof(T));
    ptr += sizeof(T);
}

template<typename T, typename PtrT>
static T slicif_read(PtrT &ptr) {
    T value;
    memcpy(&value, ptr, sizeof(T));
    ptr += sizeof(T);
    return value;
}


static std::vector<struct PSlicObject *> g_slicObjectArray;
static int g_slicNumEntries = 0;

struct PSlicObject ** slic_object_array_Get(void) { return g_slicObjectArray.data(); }
int slic_num_entries_Get(void) { return g_slicNumEntries; }

namespace
{
    char const NAME_STRUCT_INVALID[]    = "*invalid struct*";
} // namespace

static std::unique_ptr<unsigned char[]> s_code;
static int s_allocated_code = 0;
static unsigned char *s_code_ptr;
static int s_trigger_symbols[k_MAX_TRIGGER_SYMBOLS];
static int s_num_trigger_symbols;
static int s_parameters[k_MAX_PARAMETERS];
static int s_num_parameters;
static int s_parameter_index = 0;
static int s_temp_name_counter = 0;

static unsigned char *s_block_ptr[k_MAX_SLIC_LEVELS];
static int s_level = 0;
static int s_found_trigger;

static int s_file_num = 0;

static int s_if_level = 0;
struct {
	int count;
	unsigned char *array[k_MAX_SLIC_LEVELS];
} s_if_stack[k_MAX_SLIC_LEVELS];

struct LoopStackDescriptor {
	int expression;
	int increment;
};

static int s_while_level = 0;
static LoopStackDescriptor s_while_stack[k_MAX_SLIC_LEVELS];

static char s_current_segment_name[1024];
static SF_RET s_function_return_type = (SF_RET)0;

static int s_event_checking = 0;

static int s_arg_counts[GEA_End];

static int s_inSegment = 0;

static int s_parenLevel = 0;

static GAME_EVENT s_currentEvent;
static size_t s_currentEventArgument[k_MAX_PAREN_LEVEL];

static bool s_argValuePushed = false;
static SlicSymbolData *s_argSymbol;
static int s_argMemberIndex;

#define k_MAX_FUNCTION_NAME 256

extern "C" void yyerror(char *s);

void slicif_init()
{
	int i;

	for(i = 0; i < g_slicNumEntries; i++) {
		if(g_slicObjectArray[i]) {
			if(g_slicObjectArray[i]->m_id) {
				free(g_slicObjectArray[i]->m_id);
			}
			if(g_slicObjectArray[i]->m_trigger_symbols) {
				free(g_slicObjectArray[i]->m_trigger_symbols);
			}
			if(g_slicObjectArray[i]->m_parameters) {
				free(g_slicObjectArray[i]->m_parameters);
			}
			free(g_slicObjectArray[i]);
		}
	}

	g_slicObjectArray.clear();
	g_slicNumEntries = 0;
	s_temp_name_counter = 0;
}

//----------------------------------------------------------------------------
//
// Name       : slicif_cleanup
//
// Description: Clean up used (heap) memory
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
void slicif_cleanup()
{
	slicif_init();
	s_code.reset();
	s_code_ptr			= nullptr;
	s_allocated_code	= 0;
}

void slicif_start()
{
	s_level = 0;
	s_num_trigger_symbols = 0;
	s_found_trigger = 0;
	s_if_level = 0;
	s_while_level = 0;
	s_num_parameters = 0;
	s_parameter_index = 0;
	s_event_checking = 0;
	s_inSegment = 0;
	s_currentEvent = GEV_MAX;
	s_argValuePushed = false;
	s_argSymbol = nullptr;
	s_argMemberIndex = -1;
	s_parenLevel = 0;
	if(!s_code) {
		s_code = std::make_unique<unsigned char[]>(1000);
		s_allocated_code = 1000;
	}
	s_code_ptr = s_code.get();
}

void slicif_set_start(int symStart)
{
}

void slicif_add_object(struct PSlicObject *obj)
{
	if(g_slicObjectArray.empty()) {
		g_slicObjectArray.resize(k_INITIAL_SLIC_SIZE);
	}

	if(g_slicNumEntries >= (int)g_slicObjectArray.size()) {
		g_slicObjectArray.resize(g_slicObjectArray.size() * 2);
	}

	slicif_add_op(SOP_STOP);
	obj->m_code          = (unsigned char *)malloc(s_code_ptr - s_code.get());
	memcpy(obj->m_code, s_code.get(), s_code_ptr - s_code.get());
	obj->m_codeSize      = s_code_ptr - s_code.get();
	obj->m_from_file     = s_file_num;
	const char *filename = slicif_get_filename();
	obj->m_filename      = (char *)malloc(strlen(filename) + 1);
	strlcpy(obj->m_filename, filename, strlen(filename) + 1);

	g_slicObjectArray[g_slicNumEntries] = obj;
	g_slicNumEntries++;

	if(obj->m_type == SLIC_OBJECT_TRIGGER) {
		obj->m_trigger_symbols = (int *)malloc(s_num_trigger_symbols * sizeof(int));
		obj->m_num_trigger_symbols = s_num_trigger_symbols;
		memcpy(obj->m_trigger_symbols, s_trigger_symbols, s_num_trigger_symbols * sizeof(int));
	} else {
		obj->m_trigger_symbols = nullptr;
		obj->m_num_trigger_symbols = 0;
		obj->m_ui_component = nullptr;
	}

	if(obj->m_type == SLIC_OBJECT_FUNCTION) {
		SlicSymbolData *funcSym;

		obj->m_parameters = (int *)malloc(s_num_parameters * sizeof(int));
		obj->m_num_parameters = s_num_parameters;
		memcpy(obj->m_parameters, s_parameters, s_num_parameters * sizeof(int));
		obj->m_return_type = s_function_return_type;

		funcSym = slicengine_Get()->GetOrMakeSymbol(obj->m_id);
		Assert(funcSym->GetType() == SLIC_SYM_FUNC || funcSym->GetType() == SLIC_SYM_UFUNC || funcSym->GetType() == SLIC_SYM_UNDEFINED);
		if(funcSym->GetType() == SLIC_SYM_UNDEFINED)
			funcSym->SetType(SLIC_SYM_UFUNC);

	} else {
		obj->m_parameters = nullptr;
		obj->m_num_parameters = 0;
		obj->m_return_type = SF_RET_VOID;
	}

	if(obj->m_type != SLIC_OBJECT_HANDLEEVENT) {
		obj->m_event_name = nullptr;
	} else {
		Assert(obj->m_priority == SLIC_PRI_PRE || obj->m_priority == SLIC_PRI_POST);
	}

#ifdef _DEBUG
	slicif_dump_code(s_code.get(), s_code_ptr - s_code.get());
#endif

	slicif_start();
}

void slicif_declare_sym(char *name, SLIC_SYM type)
{
	char buf[1024];
	char realname[1024];

	SlicNamedSymbol *sym;

	if(s_inSegment) {
		slicif_get_local_name(realname, name);
		sym = slicengine_Get()->GetOrMakeSymbol(realname);
	} else {
		sym = slicengine_Get()->GetOrMakeSymbol(name);
	}

	if(sym) {
		if(sym->GetType() != SLIC_SYM_UNDEFINED) {
			snprintf(buf, sizeof(buf), "Symbol '%s' already has a type", name);
			yyerror(buf);
		} else {
			SlicStructDescription *desc = slicengine_Get()->GetStructDescription(type);
			if(desc) {
				sym->SetType(SLIC_SYM_STRUCT);
				sym->SetStruct(std::make_unique<SlicStructInstance>(desc).release());
			} else {
				sym->SetType(type);
			}
		}
	} else {
		snprintf(buf, sizeof(buf), "Couldn't create symbol '%s'", name);
		yyerror(buf);
	}
}

void slicif_declare_array(char *name, SLIC_SYM type)
{
	char buf[1024];
	SlicNamedSymbol *sym = slicengine_Get()->GetOrMakeSymbol(name);
	if(sym) {
		if(sym->GetType() != SLIC_SYM_UNDEFINED) {
			snprintf(buf, sizeof(buf), "Symbol '%s' in array declaration already has a type", name);
			yyerror(buf);
		} else {
			sym->SetType(SLIC_SYM_ARRAY);
			SlicStructDescription *desc = slicengine_Get()->GetStructDescription(type);
			if(desc) {
				sym->SetArrayType(SLIC_SYM_STRUCT);
				sym->GetArray()->SetStructTemplate(desc);
			} else {
				sym->SetArrayType(type);
			}
		}
	} else {
		snprintf(buf, sizeof(buf), "Couldn't create array symbol '%s'", name);
		yyerror(buf);
	}
}

void slicif_declare_fixed_array(char *name, SLIC_SYM type, int size)
{
	char buf[1024];
	SlicNamedSymbol *sym = slicengine_Get()->GetOrMakeSymbol(name);
	if(sym) {
		if(sym->GetType() != SLIC_SYM_UNDEFINED) {
			snprintf(buf, sizeof(buf), "Symbol '%s' in array declaration already has a type", name);
			yyerror(buf);
		} else {
			sym->SetType(SLIC_SYM_ARRAY);
			sym->GetArray()->FixSize(size);

			SlicStructDescription *desc = slicengine_Get()->GetStructDescription(type);
			if(desc) {
				sym->SetArrayType(SLIC_SYM_STRUCT);
				sym->GetArray()->SetStructTemplate(desc);
			} else {
				sym->SetArrayType(type);
			}
		}
	} else {
		snprintf(buf, sizeof(buf), "Couldn't create array symbol '%s'", name);
		yyerror(buf);
	}
}

//----------------------------------------------------------------------------
//
// Name       : slicif_add_op
//
// Description: This function is used to compile slic code as far as it is
//              known.
//
// Parameters : SOP op and as many as you want.
//
// Globals    : This function is so big no idea.
//
// Returns    : -
//
// Remark(s)  : The number of arguments depends on the given SOP type
//              if you define new operatiors you have of course modify
//              or implement the according behaviour of that function.
//
//----------------------------------------------------------------------------
void slicif_add_op(int op, ...)
{
	va_list vl;
	double dval;
	int ival;
	char *name;
	SlicNamedSymbol *symval;
	int offset;
	char *sptr;
	char internalName[k_MAX_FUNCTION_NAME];

	//Added by Martin G�hmann for database access
	SlicDBInterface *conduit;
	char *dbName;
//	string dbName;

	char errbuf[1024];

	va_start(vl, op);

	if(s_code_ptr - s_code.get() > (s_allocated_code - 40)) {

		auto newcode = std::make_unique<unsigned char[]>(s_allocated_code * 2);
		s_allocated_code *= 2;
		memcpy(newcode.get(), s_code.get(), s_code_ptr - s_code.get());
		s_code_ptr = newcode.get() + (s_code_ptr - s_code.get());

		sint32 i;
		sint32 j;
		for(i = 0; i <= s_level; i++) {
			s_block_ptr[i] = newcode.get() + (s_block_ptr[i] - s_code.get());
		}

		for(i = 0; i <= s_if_level; i++) {
			for(j = 0; j < s_if_stack[i].count; j++) {
				s_if_stack[i].array[j] = newcode.get() + (s_if_stack[i].array[j] - s_code.get());
			}
		}

		s_code = std::move(newcode);
	}

	*s_code_ptr++ = (unsigned char)op;
	switch(op) {
		case SOP_PUSHD:

			dval = va_arg(vl, double);
			slicif_emit(s_code_ptr, (double)dval);
			break;
		case SOP_PUSHI:

			ival = va_arg(vl, int);
			slicif_emit(s_code_ptr, (int)ival);
			if(!s_argValuePushed && s_parenLevel > 0) {
				s_argValuePushed = true;
			}
			s_argSymbol = nullptr;
			break;
		case SOP_PUSHV:

			name = va_arg(vl, char*);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
			}
			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = symval;
				s_argMemberIndex = -1;
			} else {

				s_argSymbol = nullptr;
			}

			break;

		case SOP_PUSHA:

			name = va_arg(vl, char*);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol '%s' is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
			}
			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));
			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = symval;
				s_argMemberIndex = -1;
			} else {

				s_argSymbol = nullptr;
			}
			break;
		case SOP_PUSHM:
		{

			char *structname = va_arg(vl, char *);
			name = va_arg(vl, char *);
			symval = slicif_get_symbol(structname);
			sint32 member = 0;
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", structname);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(structname);
			} else if(symval->GetType() != SLIC_SYM_STRUCT) {
				snprintf(errbuf, sizeof(errbuf), "%s is not a structure", structname);
				yyerror(errbuf);
			} else {
				SlicStructDescription *theStruct = symval->GetStruct()->GetDescription();
				member = (int)theStruct->GetMemberIndex(name);
				if(member < 0) {
					snprintf(errbuf, sizeof(errbuf), "%s is not a member of %s", name, structname);
					yyerror(errbuf);
				}
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			slicif_emit(s_code_ptr, (int)member);

			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = symval;
				s_argMemberIndex = member;
			} else {

				s_argSymbol = nullptr;
			}
			break;
		}
		case SOP_PUSHAM:
		{

			char *structname = va_arg(vl, char *);
			name = va_arg(vl, char *);

			symval = slicif_get_symbol(structname);
			sint32 member = 0;
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", structname);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(structname);
			} else if(symval->GetType() != SLIC_SYM_ARRAY) {
				snprintf(errbuf, sizeof(errbuf), "%s is not an array", structname);
				yyerror(errbuf);
			} else {
				SlicStructDescription *theStruct = symval->GetArray()->GetStructTemplate();
				member = (int)theStruct->GetMemberIndex(name);
				if(member < 0) {
					snprintf(errbuf, sizeof(errbuf), "%s is not a member of %s", name, structname);
					yyerror(errbuf);
				}
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			slicif_emit(s_code_ptr, (int)member);

			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = symval;
				s_argMemberIndex = member;
			} else {

				s_argSymbol = nullptr;
			}
			break;
		}
		case SOP_SARGS:

			s_currentEventArgument[++s_parenLevel] = 0;
			s_argValuePushed = false;
			break;
		case SOP_ARGE:

			slicif_check_argument();
			s_argValuePushed = false;
			s_argSymbol = nullptr;
			break;
		case SOP_ARGID:
			name = va_arg(vl, char*);
			symval = slicengine_Get()->GetOrMakeSymbol(name);
			if(symval->GetType() != SLIC_SYM_ID) {
				if(symval->GetType() == SLIC_SYM_UNDEFINED) {
					symval->SetType(SLIC_SYM_ID);
				} else {
					snprintf(errbuf, sizeof(errbuf), "%s already defined", name);
					yyerror(errbuf);
				}
			}
			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			s_argSymbol = symval;

			slicif_check_argument();
			s_argValuePushed = false;
			s_argSymbol = nullptr;
			break;
		case SOP_ARGS:
			ival = va_arg(vl, int);
			slicif_emit(s_code_ptr, (int)ival);

			slicif_check_string_argument();
			s_argValuePushed = false;
			s_argSymbol = nullptr;

			break;
		case SOP_ARGST:
			name = va_arg(vl, char *);
			symval = slicengine_Get()->GetOrMakeSymbol(name);
			if(symval->GetType() == SLIC_SYM_UNDEFINED) {
				symval->SetType(SLIC_SYM_STRING);
			} else {

				Assert(symval->GetType() == SLIC_SYM_STRING);
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			slicif_check_hard_string_argument();
			s_argValuePushed = false;
			s_argSymbol = nullptr;
			break;

		case SOP_CALL:
		case SOP_CALLR:
			slicif_check_num_args();
			s_parenLevel--;

			name    = va_arg(vl, char*);
            symval  = slicengine_Get()->GetSymbol(name);

			if (symval)
            {
				if(symval->GetType() != SLIC_SYM_FUNC &&
				   symval->GetType() != SLIC_SYM_UFUNC) {
					snprintf(errbuf, sizeof(errbuf), "%s is not a function", name);
					yyerror(errbuf);
				}
			} else {
				snprintf(internalName, sizeof(internalName), "_%s", name);
				if(!slicengine_Get()->GetFunction(internalName)) {
					snprintf(errbuf, sizeof(errbuf), "No function named %s", name);
					yyerror(errbuf);
				}

                symval = slicengine_Get()->GetSymbol(internalName);
				if (!symval) {
					symval = slicengine_Get()->GetOrMakeSymbol(internalName);
					symval->SetType(SLIC_SYM_FUNC);
				}
			}
			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));
			break;
		case SOP_EVENT:
		{
			slicif_check_num_args();
			s_parenLevel--;

			s_currentEvent = GEV_MAX;

			name = va_arg(vl, char *);
			GAME_EVENT ev = GameEventManager::GetEventIndex(name);
			slicif_emit(s_code_ptr, (int)(int)ev);
			break;
		}

		case SOP_SBLK:
			ival = va_arg(vl, int);
			s_block_ptr[ival] = s_code_ptr;
			slicif_emit(s_code_ptr, (int)ival);
			break;
		case SOP_END:

			ival = va_arg(vl, int);

			slicif_emit(s_code_ptr, (int)(-1));

			offset = s_code_ptr - s_code.get();
			slicif_store(s_block_ptr[ival], (int)offset);
			s_block_ptr[ival][-1] = SOP_JMP;

			break;
		case SOP_BUTN:
			offset = s_block_ptr[s_level] - s_code.get();
			slicif_emit(s_code_ptr, (int)(offset + sizeof(int)));

			ival = va_arg(vl, int);
			slicif_emit(s_code_ptr, (int)ival);
			break;
		case SOP_OCLS:
			offset = s_block_ptr[s_level] - s_code.get();
			slicif_emit(s_code_ptr, (int)(offset + sizeof(int)));
			break;
		case SOP_BNT:
		case SOP_BNEV:

			sptr = (char *)(s_block_ptr[s_level] - 1);
			*sptr = static_cast<char>(op);
			sptr++;
			slicif_store(sptr, (int)((int)(s_code_ptr - s_code.get()) - 1));


			s_block_ptr[s_level] = s_code_ptr - 5;


			slicif_add_if_clause_end((char *)(s_code_ptr - 5));
			*(s_code_ptr - 6) = SOP_JMP;


			s_code_ptr -= 1;

			break;

		case SOP_JMP:
			ival = va_arg(vl, int);
			slicif_emit(s_code_ptr, (int)ival);
			break;
		case SOP_ASSN:

			name = va_arg(vl, char*);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
			}

			if(symval->GetType() == SLIC_SYM_UNDEFINED) {
				snprintf(errbuf, sizeof(errbuf), "Variable '%s' used in assignment has unknown type", name);
				yyerror(errbuf);
				symval->SetType(SLIC_SYM_IVAR);
			} else if(symval->IsParameter()) {
				snprintf(errbuf, sizeof(errbuf), "Function parameters are read-only");
				yyerror(errbuf);
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));
			break;
		case SOP_ASSNA:


			name = va_arg(vl, char *);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
			}

			if(symval->GetType() != SLIC_SYM_ARRAY) {
				snprintf(errbuf, sizeof(errbuf), "Symbol '%s' used in array assignment is not an array", name);
				yyerror(errbuf);
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));
			break;
		case SOP_ASSNM:
		case SOP_ASSNAM:
		{

			int member;
			char *structname = va_arg(vl, char*);
			name = va_arg(vl, char*);
			symval = slicif_get_symbol(structname);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", structname);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(structname);
			}

			if(symval->GetType() != SLIC_SYM_STRUCT) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is not a struct", structname);
				yyerror(errbuf);
				member = -1;
			} else {

				SlicStructDescription *theStruct = symval->GetStruct()->GetDescription();
				member = (int)theStruct->GetMemberIndex(name);
				if(member < 0) {
					snprintf(errbuf, sizeof(errbuf), "Struct %s has no member named %s", structname, name);
					yyerror(errbuf);
				}
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			slicif_emit(s_code_ptr, (int)member);

			break;
		}
		case SOP_TRIG:
			s_found_trigger = 1;
			break;
		case SOP_LINE:
			ival = va_arg(vl, int);
			slicif_emit(s_code_ptr, (int)ival);

			ival = va_arg(vl, int);
			slicif_emit(s_code_ptr, (int)ival);

			slicif_emit(s_code_ptr, (void *)nullptr);

			break;
		case SOP_ASIZE:
			name = va_arg(vl, char *);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
			}

			if(symval->GetType() != SLIC_SYM_ARRAY) {
				snprintf(errbuf, sizeof(errbuf), ".# operator only works on arrays");
				yyerror(errbuf);
			}

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));
			break;
//Added by Martin G�hmann for database support
		case SOP_DBNAME:
		{
			conduit = va_arg(vl, SlicDBInterface *);
			Assert(conduit);

			name = va_arg(vl, char*);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
			}

			//Save the database name to the code data, by saving every
			//single char including the /0 char.
			int i;
			dbName = const_cast<char*>(conduit->GetName());
			for(i = 0; dbName[i] != '\0'; ++i){
				*((char*)s_code_ptr) = dbName[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = dbName[i];
			s_code_ptr += sizeof(char);

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = symval;
				s_argMemberIndex = -1;
			} else {

				s_argSymbol = nullptr;
			}

			break;
		}
		case SOP_DBNAMEREF:
		case SOP_DBNAMEARRAY:
		{
			conduit = va_arg(vl, SlicDBInterface *);
			Assert(conduit);

			//Get variable name in the argument list.
			name = va_arg(vl, char*);
			symval = slicif_get_symbol(name);
			if(!symval) {
				snprintf(errbuf, sizeof(errbuf), "Symbol %s is undefined", name);
				yyerror(errbuf);
				symval = slicengine_Get()->GetOrMakeSymbol(name);
				//variable name is now free and can be reused.
			}

			//Get referenced name of a flag from the according database.
			name = va_arg(vl, char*);
			if(!conduit->IsTokenInDB(name)){
				snprintf(errbuf, sizeof(errbuf), "Token %s not found in %s", name, conduit->GetName());
				yyerror(errbuf);
			}

			//Save the database name to the code data, by saving every
			//single char including the /0 char.
			int i;
			dbName = const_cast<char*>(conduit->GetName());
			for(i = 0; dbName[i] != '\0'; ++i){
				*((char*)s_code_ptr) = dbName[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = dbName[i];
			s_code_ptr += sizeof(char);

			slicif_emit(s_code_ptr, (int)(symval->GetIndex()));

			//Do the same thing with the record member name:
			for(i = 0; name[i] != '\0'; ++i){
				*((char*)s_code_ptr) = name[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = name[i];
			s_code_ptr += sizeof(char);

			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = symval;
				s_argMemberIndex = -1;
			} else {

				s_argSymbol = nullptr;
			}

			break;
		}
		case SOP_DBNAMECONSTARRAY:
		{
			conduit = va_arg(vl, SlicDBInterface *);
			Assert(conduit);

			//Get variable name in the argument list.
			ival = va_arg(vl, int);

			//Get referenced name of a flag from the according database.
			name = va_arg(vl, char*);
			if(!conduit->IsTokenInDB(name)){
				snprintf(errbuf, sizeof(errbuf), "Token %s not found in %s", name, conduit->GetName());
				yyerror(errbuf);
			}

			//Save the database name to the code data, by saving every
			//single char including the /0 char.
			int i;
			dbName = const_cast<char*>(conduit->GetName());
			for(i = 0; dbName[i] != '\0'; ++i){
				*((char*)s_code_ptr) = dbName[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = dbName[i];
			s_code_ptr += sizeof(char);

			slicif_emit(s_code_ptr, (int)ival);

			//Do the same thing with the record member name:
			for(i = 0; name[i] != '\0'; ++i){
				*((char*)s_code_ptr) = name[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = name[i];
			s_code_ptr += sizeof(char);

			if(!s_argValuePushed && (s_parenLevel > 0)) {

				s_argValuePushed = true;
				s_argSymbol = nullptr;
				s_argMemberIndex = -1;
			} else {

				s_argSymbol = nullptr;
			}

			break;
		}
		case SOP_DB:
		{
			conduit = va_arg(vl, SlicDBInterface *);
			Assert(conduit);

			//Save the database name to the code data, by saving every
			//single char including the /0 char.
			int i;
			dbName = const_cast<char*>(conduit->GetName());
			for(i = 0; dbName[i] != '\0'; ++i){
				*((char*)s_code_ptr) = dbName[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = dbName[i];
			s_code_ptr += sizeof(char);

			if(!s_argValuePushed && s_parenLevel > 0) {
				s_argValuePushed = true;
			}
			s_argSymbol = nullptr;

			break;
		}
		case SOP_DBREF:
		case SOP_DBARRAY:
		{
			conduit = va_arg(vl, SlicDBInterface *);
			Assert(conduit);

			name = va_arg(vl, char *);
			if(!conduit->IsTokenInDB(name)){
				snprintf(errbuf, sizeof(errbuf), "Token %s not found in %s", name, conduit->GetName());
				yyerror(errbuf);
			}

			//Save the database name to the code data, by saving every
			//single char including the /0 char.
			int i;
			dbName = const_cast<char*>(conduit->GetName());
			for(i = 0; dbName[i] != '\0'; ++i){
				*((char*)s_code_ptr) = dbName[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = dbName[i];
			s_code_ptr += sizeof(char);

			//Do the same thing with the record member name:
			for(i = 0; name[i] != '\0'; ++i){
				*((char*)s_code_ptr) = name[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = name[i];
			s_code_ptr += sizeof(char);

			if(!s_argValuePushed && s_parenLevel > 0) {
				s_argValuePushed = true;
			}
			s_argSymbol = nullptr;

			break;
		}
		case SOP_DBSIZE:
		{
			//Added by Martin G�hmann to figure out via
			//slic how many records the database contains
			conduit = va_arg(vl, SlicDBInterface *);
			Assert(conduit);

			//Save the database name to the code data, by saving every
			//single char including the /0 char.
			int i;
			dbName = const_cast<char*>(conduit->GetName());
			for(i = 0; dbName[i] != '\0'; ++i){
				*((char*)s_code_ptr) = dbName[i];
				s_code_ptr += sizeof(char);
			}
			*((char*)s_code_ptr) = dbName[i];
			s_code_ptr += sizeof(char);

			if(!s_argValuePushed && s_parenLevel > 0) {
				s_argValuePushed = true;
			}
			s_argSymbol = nullptr;
			break;
		}
		default:
			break;
	}
	va_end(vl);
}

void slicif_start_block()
{
	if(s_level > 0) {
		slicif_add_op(SOP_SBLK, s_level);
	}
	++s_level;
}

void slicif_end_block()
{
	--s_level;
	if(s_level > 0) {
		slicif_add_op(SOP_END, s_level);
	}
}

void slicif_start_if()
{
	++s_if_level;
	s_if_stack[s_if_level].count = 0;
}

void slicif_add_if_clause_end(char *ptr)
{
	s_if_stack[s_if_level].array[s_if_stack[s_if_level].count++] = (unsigned char *)ptr;
}

void slicif_end_if()
{
	int i;
	for(i = 0; i < s_if_stack[s_if_level].count; i++) {
		slicif_store(s_if_stack[s_if_level].array[i], (int)(s_code_ptr - s_code.get()));
	}
	--s_if_level;
}

void slicif_start_while()
{
	++s_while_level;
	s_while_stack[s_while_level].expression = s_code_ptr - s_code.get();
	s_while_stack[s_while_level].increment = -1;
}

void slicif_end_while()
{
	char *sptr;

	s_code_ptr -= 5;

	slicif_add_op(SOP_JMP, s_while_stack[s_while_level].expression);


	sptr = (char *)(s_block_ptr[s_level] - 1);
	*sptr = SOP_BNT;
	sptr++;
	slicif_store(sptr, (int)((int)(s_code_ptr - s_code.get())));

	s_while_level--;
}

#ifdef _DEBUG
//----------------------------------------------------------------------------
//
// Name       : slicif_dump_code
//
// Description: This function is for debug purposes only and dumps error
//              messages to a file presumably
//
// Parameters : unsigned char* code
//              int codeSize
//
// Globals    : This function is so big no idea.
//
// Returns    : -
//
// Remark(s)  : This function is used for debbug purposes only, it should
//              also be modified if you add new SOP types.
//
//              As far as known this function is also used and slic compiling
//              time.
//
//----------------------------------------------------------------------------
void slicif_dump_code(unsigned char* code, int codeSize)
{
	unsigned char* codePtr = code;
	double dval;
	int ival;
	int ival2;
	SlicNamedSymbol *symval;
	//Added by Martin G�hmann for database access
//	SlicDBInterface *conduit;
	char* name;
	const char* dbName;

	

	while(codePtr < code + codeSize) {
		SOP op = (SOP)*codePtr;
		fprintf(debuglog, "%04lx: ", codePtr - code);
		codePtr++;
		switch(op) {
			case SOP_PUSHI:
				fprintf(debuglog, "pushi ");
				ival = slicif_read<int>(codePtr);
				fprintf(debuglog, "%d\n", ival);
				break;
			case SOP_PUSHD:
				fprintf(debuglog, "pushd ");
				dval = slicif_read<double>(codePtr);
				fprintf(debuglog, "%lf\n", dval);
				break;
			case SOP_PUSHV:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
					return;
				}
				fprintf(debuglog, "pushv %s(%d)\n", symval->GetName(), ival);
				break;
			case SOP_PUSHA:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
					return;
				}
				fprintf(debuglog, "pusha %s(%d)\n", symval->GetName(), ival);
				break;
			case SOP_PUSHM:
				{
				    ival = slicif_read<int>(codePtr);
				    symval = slicengine_Get()->GetSymbol(ival);

				    ival2 = slicif_read<int>(codePtr);

                    SlicStructInstance *    gotStruct   = symval->GetStruct();
                    char const *            memberName  =
                        gotStruct
                        ? gotStruct->GetDescription()->GetMemberName(ival2)
                        : NAME_STRUCT_INVALID;
			        fprintf(debuglog, "pushm %s(%d).%s(%d)\n",
                            symval->GetName(), symval->GetIndex(), memberName, ival2
                           );
                }
				break;
			case SOP_PUSHAM:

				ival = slicif_read<int>(codePtr);
				symval = slicengine_Get()->GetSymbol(ival);

				if(!symval) {
					fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
					return;
				}

				ival2 = slicif_read<int>(codePtr);
				fprintf(debuglog, "pusham %s(%d)[].%s(%d)\n", symval->GetName(), symval->GetIndex(),
						symval->GetArray()->GetStructTemplate()->GetMemberName(ival2), ival2);
				break;
			case SOP_AINDX: fprintf(debuglog, "aindx\n"); break;
			case SOP_ADD:  fprintf(debuglog, "add\n"); break;
			case SOP_SUB:  fprintf(debuglog, "sub\n"); break;
			case SOP_MULT: fprintf(debuglog, "mult\n"); break;
			case SOP_EXP: fprintf(debuglog, "pow\n"); break;
			case SOP_BAND: fprintf(debuglog, "band\n"); break;
			case SOP_BOR: fprintf(debuglog, "bor\n"); break;
			case SOP_BXOR: fprintf(debuglog, "bxor\n"); break;
			case SOP_BNOT: fprintf(debuglog, "bnot\n"); break;
			case SOP_DIV:  fprintf(debuglog, "div\n"); break;
			case SOP_MOD:  fprintf(debuglog, "mod\n"); break;
			case SOP_EQ:   fprintf(debuglog, "eq\n"); break;
			case SOP_GT:   fprintf(debuglog, "gt\n"); break;
			case SOP_LT:   fprintf(debuglog, "lt\n"); break;
			case SOP_GTE:  fprintf(debuglog, "gte\n"); break;
			case SOP_LTE:  fprintf(debuglog, "lte\n"); break;
			case SOP_POP:  fprintf(debuglog, "pop\n"); break;
			case SOP_TRIG: fprintf(debuglog, "trig\n"); break;
			case SOP_ARGE: fprintf(debuglog, "arge\n"); break;
			case SOP_NEQ:  fprintf(debuglog, "neq\n"); break;
			case SOP_ARGID:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad Mojo, NULL symbol %d\n", ival);
					return;
				}
				if(symval->GetType() != SLIC_SYM_ID) {
					fprintf(debuglog, "Bad Mojo, symbol %s is not of type ID\n",
							symval->GetName());
					return;
				}
				fprintf(debuglog, "argid %s(%d)\n", symval->GetName(), ival);
				break;
			case SOP_ARGS:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(symval->GetType() != SLIC_SYM_SVAR) {
					fprintf(debuglog, "Bad Mojo, string id arg doesn't have string id type\n");
					return;
				}
				fprintf(debuglog, "args  %d(%s)\n", ival, symval->GetName());
				break;
			case SOP_ARGST:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(symval->GetType() != SLIC_SYM_STRING) {
					fprintf(debuglog, "Bad Mojo, string arg doesn't have string type\n");
					return;
				}
				fprintf(debuglog, "argst %d(%s)\n", ival, symval->GetName());
				break;
			case SOP_CALL:
			case SOP_CALLR:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad Mojo, NULL symbol %d\n", ival);
					return;
				}
				if(symval->GetType() != SLIC_SYM_FUNC &&
				   symval->GetType() != SLIC_SYM_UFUNC) {
					fprintf(debuglog, "Bad Mojo, symbol %s is not a function\n",
							symval->GetName());
					return;
				}
				fprintf(debuglog, "call%c %s(%d)\n", (op == SOP_CALL) ? ' ' : 'r',
						symval->GetName(), ival);
				break;
			case SOP_EVENT:
				ival = slicif_read<int>(codePtr);

				fprintf(debuglog, "event %s\n", GameEventManager::GetEventName((GAME_EVENT)ival));
				break;
			case SOP_SBLK:
				fprintf(debuglog, "dangling SBLK\n");
				return;
			case SOP_END:
				fprintf(debuglog, "end\n");
				codePtr += sizeof(int);
				break;
			case SOP_JMP:
				ival = slicif_read<int>(codePtr);
				fprintf(debuglog, "jmp   0x%04x\n", ival);
				break;
			case SOP_BNT:
				ival = slicif_read<int>(codePtr);
				fprintf(debuglog, "bnt   0x%04x\n", ival);
				break;
			case SOP_BNEV:
				ival = slicif_read<int>(codePtr);
				fprintf(debuglog, "bnev  0x%04x\n", ival);
				break;
			case SOP_BUTN:
				ival = slicif_read<int>(codePtr);
				ival2 = slicif_read<int>(codePtr);





				symval = slicengine_Get()->GetSymbol(ival2);
				if(symval->GetType() != SLIC_SYM_SVAR) {
					fprintf(debuglog, "Bad Mojo, button string arg doesn't have string type\n");
					return;
				}
				fprintf(debuglog, "butn  0x%04x,%d(%s)\n", ival,
						symval->GetIndex(), symval->GetName());
				break;
			case SOP_OCLS:
				ival = slicif_read<int>(codePtr);
				fprintf(debuglog, "ocls  0x%04x\n", ival);
				break;
			case SOP_STOP:
				fprintf(debuglog, "stop\n");
				break;
			case SOP_NEG:
				fprintf(debuglog, "neg\n");
				break;
			case SOP_ASSN:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
					return;
				}
				fprintf(debuglog, "assn  %s(%d)\n", symval->GetName(), ival);
				break;
			case SOP_ASSNA:
				ival = slicif_read<int>(codePtr);




				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
				} else {
					fprintf(debuglog, "assna %s(%d)\n", symval->GetName(), ival);
				}
				break;
			case SOP_AND: fprintf(debuglog, "and\n"); break;
			case SOP_OR:  fprintf(debuglog, "or\n"); break;
			case SOP_NOT: fprintf(debuglog, "not\n"); break;
			case SOP_SARGS: fprintf(debuglog, "sargs\n"); break;
			case SOP_RET:   fprintf(debuglog, "ret\n"); break;
			case SOP_LINE:
				ival = slicif_read<int>(codePtr);

				ival2 = slicif_read<int>(codePtr);

				codePtr += sizeof(void *);
				fprintf(debuglog, "line  %d/%d\n", ival, ival2);
				break;
			case SOP_ASIZE:
				ival = slicif_read<int>(codePtr);
				symval = slicengine_Get()->GetSymbol(ival);
				if(!symval) {
					fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
				} else {
					fprintf(debuglog, "asize %s(%d)\n", symval->GetName(), ival);
				}
				break;
//Added by Martin G�hmann for database support
			case SOP_DBNAME:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				ival = slicif_read<int>(codePtr);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {

					symval = slicengine_Get()->GetSymbol(ival);
					if(!symval) {
						fprintf(debuglog, "%s\n", dbName);
						fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
						return;
					}
					fprintf(debuglog, "%s %s(%d)\n", dbName, symval->GetName(), ival);
				}
				break;
			}
			case SOP_DBNAMEREF:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				ival = slicif_read<int>(codePtr);

				//Get the member name:
				name = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {

					symval = slicengine_Get()->GetSymbol(ival);
					if(!symval) {
						fprintf(debuglog, "%s\n", dbName);
						fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
						return;
					}
					fprintf(debuglog, "%s(%s).%s, %s == (%d)\n", dbName, symval->GetName(), name, symval->GetName(), ival);
				}
				break;
			}
			case SOP_DBNAMEARRAY:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				ival = slicif_read<int>(codePtr);

				//Get the member name:
				name = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {
					symval = slicengine_Get()->GetSymbol(ival);
					if(!symval) {
						fprintf(debuglog, "%s\n", dbName);
						fprintf(debuglog, "Bad mojo, NULL symbol %d\n", ival);
						return;
					}
					fprintf(debuglog, "%s(%s).%s[], %s == (%d)\n", dbName, symval->GetName(), name, symval->GetName(), ival);
				}
				break;
			}
			case SOP_DBNAMECONSTARRAY:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				ival = slicif_read<int>(codePtr);

				//Get the member name:
				name = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {
					fprintf(debuglog, "%s(%s).%s[]\n", dbName, slicengine_Get()->GetDBConduit(dbName)->GetRecordNameByIndex(ival), name);
				}
				break;
			}
			case SOP_DB:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {
					fprintf(debuglog, "%s\n", dbName);
				}
				break;
			}
			case SOP_DBREF:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				//Get the member name:
				name = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {
					fprintf(debuglog, "%s(..).%s\n", dbName, name);
				}
				break;
			}
			case SOP_DBARRAY:
			{
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				//Get the member name:
				name = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {
					fprintf(debuglog, "%s(..).%s[]\n", dbName, name);
				}
				break;
			}
			case SOP_DBSIZE:
			{
				//Added by Martin G�hmann to figure out via
				//slic how many records the database contains
				//Get the database name:
				dbName = ((char*)codePtr);
				while(*((char*)codePtr) != '\0'){
					codePtr += sizeof(char);
				}
				codePtr += sizeof(char);

				if(!slicengine_Get()->GetDBConduit(dbName)) {
					fprintf(debuglog, "%s\n", dbName);
					fprintf(debuglog, "Bad mojo, NULL db\n");
				} else {
					fprintf(debuglog, "%s\n", dbName);
				}
				break;
			}
			default:
				fprintf(debuglog, "???\n");
				break;
		}
	}
}
#endif

int slicif_find_string(char *id)
{
	char errbuf[1024];

	SlicNamedSymbol *sym = slicengine_Get()->GetOrMakeSymbol(id + 3);
	if(sym->GetType() == SLIC_SYM_UNDEFINED) {
		sym->SetType(SLIC_SYM_SVAR);
	} else if(sym->GetType() != SLIC_SYM_SVAR) {
		snprintf(errbuf, sizeof(errbuf), "%s is not a string variable", id + 3);
		yyerror(errbuf);
	}

	if(!slicif_is_valid_string(id + 3)) {
		snprintf(errbuf, sizeof(errbuf), "%s not found in string databse", id + 3);
		yyerror(errbuf);
	}

	return sym->GetIndex();
}

void slicif_set_file_num(int num)
{
	s_file_num = num;
}

void slicif_add_region(char *name, int x1, int y1, int x2, int y2)
{
}

void slicif_start_complex_region(char *name)
{
}

void slicif_finish_complex_region()
{

}

void slicif_add_region_to_complex(char *name)
{
}

void slicif_start_segment(char *name)
{
	s_inSegment = 1;
	strlcpy(s_current_segment_name, name, sizeof(s_current_segment_name));
	slicif_register_line(slic_line_number_Get(), -1);
}

char *slicif_get_segment_name_copy()
{
	char *name = (char *)malloc(strlen(s_current_segment_name) + 1);
	strlcpy(name, s_current_segment_name, strlen(s_current_segment_name) + 1);
	return name;
}

void slicif_add_parameter(SLIC_SYM type, char *name)
{
	char namebuf[1024];
	slicif_get_local_name(namebuf, name);

	SlicSymbolData * sym = slicengine_Get()->GetSymbol(namebuf);

	if (sym)
    {
		char errbuf[1024];
		snprintf(errbuf, sizeof(errbuf), "'%s' already has a local definition", name);
		yyerror(errbuf);
	} else {

		SlicParameterSymbol *psym = slicengine_Get()->GetParameterSymbol(namebuf, s_parameter_index++);
		Assert(psym);
		Assert(psym->IsParameter());
		SlicStructDescription *desc = slicengine_Get()->GetStructDescription(type);
		if(desc) {
			psym->SetType(SLIC_SYM_STRUCT);
			psym->SetStruct(std::make_unique<SlicStructInstance>(desc, psym).release());
		} else {
			psym->SetType(type);
		}
		s_parameters[s_num_parameters] = psym->GetIndex();
		s_num_parameters++;
	}
}

void slicif_function_return(SF_RET rettype)
{
	s_function_return_type = rettype;
}

void slicif_get_local_name(char *localName, const char *name)
{
	snprintf(localName, sizeof(localName), "%s#%s", s_current_segment_name, name);
}

void slicif_add_prototype(char *name)
{
	SlicSymbolData * sym = slicengine_Get()->GetOrMakeSymbol(name);
	char errbuf[1024];
	if(sym->GetType() != SLIC_SYM_UNDEFINED) {
		snprintf(errbuf, sizeof(errbuf), "Symbol '%s' is already defined", name);
		yyerror(errbuf);
	} else {
		sym->SetType(SLIC_SYM_UFUNC);
	}

	if(s_num_parameters > 0) {

		snprintf(errbuf, sizeof(errbuf), "Prototypes should not define arguments\n");
		yyerror(errbuf);
	}
}

void slicif_start_for()
{
}

void slicif_for_expression()
{
	++s_while_level;
	s_while_stack[s_while_level].expression = s_code_ptr - s_code.get();
	s_while_stack[s_while_level].increment = -1;
}

void slicif_for_continue()
{

	slicif_add_op(SOP_JMP, -1);
	s_while_stack[s_while_level].increment = s_code_ptr - s_code.get();
}

void slicif_start_for_body()
{

	slicif_add_op(SOP_JMP, s_while_stack[s_while_level].expression);
}

void slicif_end_for()
{
	s_code_ptr -= 5;

	slicif_add_op(SOP_JMP, s_while_stack[s_while_level].increment);

	char * sptr = (char *)(s_block_ptr[s_level] - 1);
	*sptr = SOP_BNT;
	sptr++;
	slicif_store(sptr, (int)((int)(s_code_ptr - s_code.get())));

	sptr = (char*)(s_code.get() +  s_while_stack[s_while_level].increment - 5);
	sptr++;
	slicif_store(sptr, (int)((int)(s_block_ptr[s_level] - 1 - s_code.get())));

	s_while_level--;
}

int slicif_find_const(const char *name, int *value)
{
	return (int)slicengine_Get()->FindConst(name, (sint32*)value);
}

void slicif_add_const(const char *name, int value)
{
	slicengine_Get()->AddConst(name, (sint32)value);
}

void slicif_check_event_exists(const char *name)
{
	GAME_EVENT ev = GameEventManager::GetEventIndex(name);
	if(ev >= GEV_MAX) {
		char errbuf[1024];
		snprintf(errbuf, sizeof(errbuf), "No event named %s", name);
		yyerror(errbuf);
	}
}

char *slicif_create_name(const char *base)
{
	size_t const nameSize = strlen(base) + 10;
	char *name = (char *)malloc(nameSize);
	snprintf(name, nameSize, "%s!%08x", base, s_temp_name_counter++);
	return name;
}

static SLIC_PRI s_priority;
void slicif_set_priority(SLIC_PRI pri)
{
	s_priority = pri;
}

SLIC_PRI slicif_get_priority()
{
	return s_priority;
}

void slicif_set_event_checking(const char *eventname)
{
	s_event_checking = 1;
	const char *argString = GameEventManager::GetArgString(GameEventManager::GetEventIndex(eventname));

	memset(s_arg_counts, 0, sizeof(s_arg_counts));

	while(*argString) {
		Assert(*argString == '%' || *argString == '&' || *argString == '$');
		argString++;
		if(!*argString)
			break;

		GAME_EVENT_ARGUMENT argType = GameEventManager::ArgCharToIndex(*argString);
		s_arg_counts[argType]++;
		argString++;
	}
}

void slicif_add_valid_builtin(char *name)
{
}

void slicif_add_local_struct(char *structtype, char *name)
{
}

void slicif_register_line(int line, int offset)
{






	if(s_inSegment && profiledb_Get()->IsDebugSlic()) {
		slicif_add_op(SOP_LINE, line, offset);
	}
}

SlicNamedSymbol *slicif_get_symbol(const char *name)
{
	char localname[1024];
	slicif_get_local_name(localname, name);

	SlicNamedSymbol *sym = slicengine_Get()->GetSymbol(localname);
	if(sym)
		return sym;

	sym = slicengine_Get()->GetSymbol(name);
	return sym;
}

void slicif_start_event(char *name)
{
	char errbuf[1024];

	s_currentEvent = GameEventManager::GetEventIndex(name);
	if(s_currentEvent >= GEV_MAX) {
		snprintf(errbuf, sizeof(errbuf), "Event %s does not exist", name);
		yyerror(errbuf);
		return;
	}

	slicif_add_op(SOP_SARGS);
}

void slicif_check_arg_symbol(SLIC_SYM type, const char *typeName)
{
	char errbuf[1024];
	SLIC_SYM symType;

	if(!s_argSymbol) {


		if(type != SLIC_SYM_IVAR && type != SLIC_SYM_PLAYER && type != SLIC_SYM_LOCATION &&
			type != SLIC_SYM_CITY && type != SLIC_SYM_UNIT && type != SLIC_SYM_ARMY) {
			snprintf(errbuf, sizeof(errbuf), "Argument %zu requires a symbol", s_currentEventArgument[s_parenLevel] + 1);
			yyerror(errbuf);
		}
		return;
	}

	if(s_argSymbol->GetType() == SLIC_SYM_ARRAY) {
		if(s_argSymbol->GetArray()->GetType() == SS_TYPE_INT) {
			symType = SLIC_SYM_IVAR;
		} else {
			auto structDataSym = s_argSymbol->GetArray()->GetStructTemplate()->CreateDataSymbol();
			symType = structDataSym->GetType();
		}
	} else if(s_argSymbol->GetType() == SLIC_SYM_STRUCT) {
		if(s_argMemberIndex < 0) {
			symType = s_argSymbol->GetStruct()->GetDataSymbol()->GetType();
		} else {
			symType = s_argSymbol->GetStruct()->GetMemberSymbol(s_argMemberIndex)->GetType();
		}
	} else {
		symType = s_argSymbol->GetType();
	}

	if(type == SLIC_SYM_PLAYER && symType == SLIC_SYM_IVAR) {

		return;
	}
	if(symType != type) {
		snprintf(errbuf, sizeof(errbuf), "Type mismatch for argument %zu, expected %s", s_currentEventArgument[s_parenLevel] + 1, typeName);
		yyerror(errbuf);
		return;
	}
}

void slicif_check_argument()
{
	if(s_currentEvent < GEV_MAX) {
		char argChar = GameEventManager::ArgChar(s_currentEvent, s_currentEventArgument[s_parenLevel]);
		switch(argChar) {
			case GEAC_ARMY:
				slicif_check_arg_symbol(SLIC_SYM_ARMY, "army_t");
				break;
			case GEAC_UNIT:
				slicif_check_arg_symbol(SLIC_SYM_UNIT, "unit_t");
				break;
			case GEAC_CITY:
				slicif_check_arg_symbol(SLIC_SYM_CITY, "city_t");
				break;
			case GEAC_POP:
				slicif_check_arg_symbol(SLIC_SYM_POP, "pop_t");
				break;
			case GEAC_GOLD:
				slicif_check_arg_symbol(SLIC_SYM_IVAR, "int_t");
				break;
			case GEAC_PATH:
				slicif_check_arg_symbol(SLIC_SYM_PATH, "path_t");
				break;
			case GEAC_MAPPOINT:
				slicif_check_arg_symbol(SLIC_SYM_LOCATION, "location_t");
				break;
			case GEAC_PLAYER:
				slicif_check_arg_symbol(SLIC_SYM_PLAYER, "int_t");
				break;
			case GEAC_INT:
				slicif_check_arg_symbol(SLIC_SYM_IVAR, "int_t");
				break;
			case GEAC_DIRECTION:
				slicif_check_arg_symbol(SLIC_SYM_IVAR, "int_t");
				break;

			case GEAC_ADVANCE:
				slicif_check_arg_symbol(SLIC_SYM_IVAR, "int_t");
				break;
			case GEAC_WONDER:
				slicif_check_arg_symbol(SLIC_SYM_IVAR, "int_t");
				break;
			case GEAC_IMPROVEMENT:
				slicif_check_arg_symbol(SLIC_SYM_IMPROVEMENT, "improvement_t");
				break;
		}
		s_currentEventArgument[s_parenLevel]++;
	}
}

void slicif_check_num_args()
{
	char errbuf[1024];

	if((s_currentEvent < GEV_MAX) && s_parenLevel == 1) {
		if((s_currentEventArgument[s_parenLevel]) != GameEventManager::GetNumArgs(s_currentEvent)) {
			snprintf(errbuf, sizeof(errbuf), "Wrong number of arguments for event %s, expected %zu",
			        GameEventManager::GetEventName(s_currentEvent),
			        GameEventManager::GetNumArgs(s_currentEvent));
			yyerror(errbuf);
			return;
		}
	}
}

void slicif_check_string_argument()
{
}

void slicif_check_hard_string_argument()
{
}

//----------------------------------------------------------------------------
//
// Name       : slicif_find_db
//
// Description: Handels slic database access.
//
// Parameters : const char *dbname: A name of a database in slic.
//              void **dbptr:       A pointer that can be converted into
//                                  a SlicDBInterface interface object.
//
// Globals    : -
//
// Returns    : Returns whether there is a database with such a name
//              given by dbname. So finally 1 or 0.
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
int slicif_find_db(const char *dbname, void **dbptr)
{
	SlicDBInterface *conduit = slicengine_Get()->GetDBConduit(dbname);
	if(conduit) {
		*dbptr = (void *)conduit;
		return TRUE;
	} else {
		dbptr = nullptr;
		return FALSE;
	}
}

//----------------------------------------------------------------------------
//
// Name       : slicif_find_db_index
//
// Description: Handels slic database access.
//
// Parameters : void *dbptr:      Represents a database for instance UnitDB.
//              const char *name: Represents name that can be found in the
//                                given database for instance UNIT_SETTLER.
//
// Globals    : -
//
// Returns    : The database index of the given name.For instance
//              UnitDB(UNIT_SETTLER) gives the the datbase index of the
//              unit with name UNIT_SETTLER.
//
// Remark(s)  : This function is called at copiling time.
//
//----------------------------------------------------------------------------
int slicif_find_db_index(void *dbptr, const char *name)
{
	SlicDBInterface * conduit = reinterpret_cast<SlicDBInterface *>(dbptr);
	Assert(conduit);
	if(!conduit)
		return 0;

	sint32 index = conduit->GetIndex(name);
	if(index < 0) {
		char errbuf[1024];
		snprintf(errbuf, sizeof(errbuf), "%s not found in %s", name, conduit->GetName());
		yyerror(errbuf);
	}
	return index;
}

//----------------------------------------------------------------------------
//
// Name       : slicif_find_db_value
//
// Description: Handels slic database access.
//
// Parameters : void *dbptr:         Represents a database for instance UnitDB.
//              const char *recname: Represents name that can be found in the
//                                   given database for instance UNIT_SETTLER.
//              const char *valname: Represents a flag of an entry in the given
//                                   database for instance MaxMovePoints.
//
// Globals    : -
//
// Returns    : Gets the value of a flag of a given entry in a given
//              database. For instance UnitDB(UNIT_SETTLER).MaxMovePoints
//              gives the value of the MaxMovePoints flag of the entry
//              in UnitDB with the internal name UNIT_SETTLER.
//
// Remark(s)  : This function is called at copiling time in normal slic
//              function but called at run time from the Great Libary.
//
//----------------------------------------------------------------------------
int slicif_find_db_value(void *dbptr, const char *recname, const char *valname)
{
	SlicDBInterface * conduit = reinterpret_cast<SlicDBInterface *>(dbptr);
	char errbuf[1024];
	Assert(conduit);
	if(!conduit)
		return 0;

	sint32 index;
	if((index = conduit->GetIndex(recname)) < 0) {
		snprintf(errbuf, sizeof(errbuf), "%s not found in %s", recname, conduit->GetName());
		yyerror(errbuf);
		return 0;
	}

	return conduit->GetValue(index, valname);
}

//----------------------------------------------------------------------------
//
// Name       : slicif_find_db_value_by_index
//
// Description: Handels slic database access.
//
// Parameters : void *dbptr:         Represents a database for instance UnitDB.
//              int index:           Represents an index in the given database.
//              const char *valname: Represents a flag of an entry in the given
//                                   database for instance MaxMovePoints.
//
// Globals    : -
//
// Returns    : Gets the value of a flag of a given entry in a given
//              database. For instance UnitDB(0).MaxMovePoints gives
//              the value of the MaxMovePoints flag of the first entry in
//              UnitDB.
//
// Remark(s)  : This function is called both at copiling time and run time
//              in normal slic function but called at run time from the
//              Great Libary.
//
//
//----------------------------------------------------------------------------
int slicif_find_db_value_by_index(void *dbptr, int index, const char *valname)
{
	SlicDBInterface * conduit = reinterpret_cast<SlicDBInterface *>(dbptr);
	Assert(conduit);
	if(!conduit)
		return 0;

	return conduit->GetValue(index, valname);
}

//----------------------------------------------------------------------------
//
// Name       : slicif_find_db_array_value
//
// Description: Handels slic database access.
//
// Parameters : void *dbptr:         Represents a database for instance UnitDB.
//              const char *recname: Represents name that can be found in the
//                                   given database for instance UNIT_SETTLER.
//              const char *valname: Represents a flag of an entry in the given
//                                   database for instance MaxMovePoints.
//              sint32 val:          Represents an array index.
//
// Globals    : -
//
// Returns    : Gets the value of a flag of a given entry in a given
//              database. For instance UnitDB(UNIT_SETTLER).MaxMovePoints
//              gives the value of the MaxMovePoints flag of the entry
//              in UnitDB with the internal name UNIT_SETTLER.
//
// Remark(s)  : This function is called at copiling time in normal slic
//              function but called at run time from the Great Libary.
//
//----------------------------------------------------------------------------
int slicif_find_db_array_value(void *dbptr, const char *recname, const char *valname, int val)
{
	SlicDBInterface * conduit = reinterpret_cast<SlicDBInterface *>(dbptr);
	char errbuf[1024];
	Assert(conduit);
	if(!conduit)
		return 0;

	sint32 index;
	if((index = conduit->GetIndex(recname)) < 0) {
		snprintf(errbuf, sizeof(errbuf), "%s not found in %s", recname, conduit->GetName());
		yyerror(errbuf);
		return 0;
	}

	return conduit->GetValue(index, valname, val);
}

//----------------------------------------------------------------------------
//
// Name       : slicif_find_db_array_value_by_index
//
// Description: Handels slic database access.
//
// Parameters : void *dbptr:         Represents a database for instance UnitDB.
//              int index:           Represents an index in the given database.
//              const char *valname: Represents a flag of an entry in the given
//                                   database for instance MaxMovePoints.
//              sint32 val:          Represents an array index.
//
// Globals    : -
//
// Returns    : Gets the value of a flag of a given entry in a given
//              database. For instance UnitDB(0).MaxMovePoints gives
//              the value of the MaxMovePoints flag of the first entry in
//              UnitDB.
//
// Remark(s)  : This function is called both at copiling time and run time
//              in normal slic function but called at run time from the
//              Great Libary.
//
//
//----------------------------------------------------------------------------
int slicif_find_db_array_value_by_index(void *dbptr, int index, const char *valname, int val)
{
	SlicDBInterface * conduit = reinterpret_cast<SlicDBInterface *>(dbptr);
	Assert(conduit);
	if(!conduit)
		return 0;

	return conduit->GetValue(index, valname, val);
}


//----------------------------------------------------------------------------
//
// Name       : slicif_is_name
//
// Description: Retrieves the database index of the given, same as
//              slicif_find_db_index but without an error message
//              if it fails.
//
// Parameters : const char *name
//              void *dbptr
//
// Globals    : -
//
// Returns    : The database index of an given name in the given
//              database.
//
// Remark(s)  : This function is only used at compiling time, to determine
//              wheather a given name represents a name in the given
//              database. That is double work as it is also done in the
//              slicif_find_db_index if it is called. But as it is only
//              done at compile time, the additional time needed shouldn't
//              be a problem.
//
//----------------------------------------------------------------------------
int slicif_is_name(void *dbptr, const char *name)
{
	SlicDBInterface * conduit = reinterpret_cast<SlicDBInterface *>(dbptr);

	Assert(conduit);
	if(!conduit)
		return 0;

	return conduit->GetIndex(name);
}
