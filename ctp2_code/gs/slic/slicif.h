/*----------------------------------------------------------------------------
 *
 * Project      : Call To Power 2
 * File type    : C/C++ header
 * Description  : SLIC interpreter functions
 * Id           : $Id$
 *
 *----------------------------------------------------------------------------
 *
 * Disclaimer
 *
 * THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
 *
 * This material has been developed at apolyton.net by the Apolyton CtP2
 * Source Code Project. Contact the authors at ctp2source@apolyton.net.
 *
 *----------------------------------------------------------------------------
 *
 * Compiler flags
 *
 * __cplusplus
 *
 *----------------------------------------------------------------------------
 *
 * Modifications from the original Activision code:
 *
 * - SOP types added by Martin G�hmann to allow:
 *   - Slic database access
 *   - Slic database size access
 * - slicif_cleanup() added.
 * - Replaced slicif_is_sym by slicif_is_name function. This function is
 *   modelled slicif_find_db_index but without error message if this
 *   function fails to retrieve the database index. - Feb. 24th 2005 Martin G�hmann
 * - Added bitwise operators
 * - Added database array access. (Sep 16th 2005 Martin G�hmann)
 *
 *----------------------------------------------------------------------------
 */

#ifndef _SLICIF_H_
#define _SLICIF_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	SLIC_ERROR_OK,
	SLIC_ERROR_SYNTAX,
	SLIC_ERROR_CANT_OPEN_FILE,
	SLIC_ERROR_INCLUDE,
	SLIC_ERROR_ALREADY_USED,
	SLIC_ERROR_UNKNOWN,
	SLIC_ERROR_INTERNAL,
	SLIC_ERROR_UNKNOWN_REGION,
	SLIC_ERROR_SYMBOL_NOT_REGION,
} SLIC_ERROR;

typedef enum {
	SLIC_OBJECT_MESSAGEBOX,
	SLIC_OBJECT_TRIGGER,
	SLIC_OBJECT_FUNCTION,
	SLIC_OBJECT_HANDLEEVENT,
} SLIC_OBJECT;

typedef enum {
	SOP_TRIG,
	SOP_CALL,
	SOP_CALLR,
	SOP_EXP,
	SOP_STRING,
	SOP_ID,
	SOP_ADD,
	SOP_SUB,
	SOP_MULT,
	SOP_DIV,
	SOP_MOD,
	SOP_EQ,
	SOP_GT,
	SOP_LT,
	SOP_GTE,
	SOP_LTE,
	SOP_PUSHD,
	SOP_PUSHI,
	SOP_PUSHV,
	SOP_POP,
	SOP_ARGE,
	SOP_ARGID,
	SOP_ARGS,
	SOP_SBLK,
	SOP_END,
	SOP_JMP,
	SOP_STOP,
	SOP_NEG,
	SOP_BUTN,
	SOP_NEQ,
	SOP_ASSN,
	SOP_AND,
	SOP_OR,
	SOP_NOT,
	SOP_BNT,
	SOP_BNEV,
	SOP_SARGS,
	SOP_ARGST,
	SOP_OCLS,

	SOP_PUSHA,
	SOP_AINDX,
	SOP_ASSNA,
    SOP_RET,
	SOP_ASSNM,
	SOP_PUSHM,
	SOP_PUSHAM,
	SOP_ASSNAM,
	SOP_LINE,
	SOP_LBRK,
	SOP_EVENT,
	SOP_ASIZE,

//Added by Martin G�hmann for database support
	SOP_DBNAME,
	SOP_DBNAMEREF,
	SOP_DB,
	SOP_DBREF,
	SOP_DBARRAY,
	SOP_DBSIZE,
//Added by tombom for bitwise support
	SOP_BAND,
//And some more bitwise operators (JJB)
	SOP_BOR,
	SOP_BXOR,
	SOP_BNOT,
//Added by Martin G�hmann for database array support
	SOP_DBNAMEARRAY,
	SOP_DBNAMECONSTARRAY,

	SOP_NOP
} SOP;

#include "gs/slic/slicif_sym.h"

typedef enum {
	SF_RET_INT,
	SF_RET_VOID,
} SF_RET;

typedef enum {
	SLIC_PRI_PRE,
	SLIC_PRI_PRIMARY,
	SLIC_PRI_POST,
} SLIC_PRI;

struct PSlicObject {
	SLIC_OBJECT m_type;
	char *m_id;
	unsigned char *m_code;
	int m_codeSize;
	int *m_trigger_symbols;
	int m_num_trigger_symbols;
	int *m_parameters;
	int m_num_parameters;
	SF_RET m_return_type;

	int m_from_file;
	int m_is_alert;
	int m_is_help;
	char *m_ui_component;
	char *m_event_name;
	SLIC_PRI m_priority;
	char *m_filename;
};

struct PSlicRegion {
	int x1, y1;
	int x2, y2;
};

struct PSlicComplexRegion {
	int x1, y1;
	int x2, y2;
	struct PSlicComplexRegion *next;
};

#define k_INITIAL_SLIC_SIZE 16
#define k_MAX_CODE_SIZE 500000
#define k_INITIAL_SYM_TAB_SIZE 16
#define k_MAX_SLIC_STRING 1024
#define k_MAX_SLIC_LEVELS 256
#define k_MAX_TRIGGER_SYMBOLS 1024
#define MAX_INCLUDE_DEPTH 64
#define k_MAX_PARAMETERS 16

// Parser state.  Definitions are file-scope `static` in slicif.cpp;
// external readers (SlicEngine, SlicSegment) go through the accessors.
struct PSlicObject ** slic_object_array_Get(void);
int slic_num_entries_Get(void);

extern int slic_parse_error;

// g_slicLineNumber lives as file-scope `static` inside slic.l (the
// flex-generated lexer that increments it).  External readers use the
// accessor; the variable itself is no longer visible across TUs.
int slic_line_number_Get(void);

extern FILE *debuglog;

SLIC_ERROR slicif_run_parser(char *filename, int symStart);
void slicif_add_object(struct PSlicObject* obj);
void slicif_add_op(int op, ...); // int (not SOP): va_start on a promoted enum param is UB
void slicif_init();
void slicif_cleanup();
void slicif_set_start(int symStart);
void slicif_start();
void slicif_dump_code(unsigned char *code, int size);
void slicif_declare_sym(char *name, SLIC_SYM type);
void slicif_declare_array(char *name, SLIC_SYM type);
void slicif_declare_fixed_array(char *name, SLIC_SYM type, int size);

void slicif_start_block();
void slicif_end_block();

void slicif_start_if();
void slicif_end_if();
void slicif_add_if_clause_end(char *ptr);

void slicif_start_while();
void slicif_end_while();

int slicif_find_string(char *id);
void slicif_set_file_num(int type);
void slicif_add_region(char *name, int x1, int y1, int x2, int y2);
void slicif_start_complex_region(char *name);
void slicif_finish_complex_region();
void slicif_add_region_to_complex(char *name);
int slicif_open_first_file(char *name);
const char *slicif_current_file();

void slicif_add_parameter(SLIC_SYM type, char *name);
void slicif_function_return(SF_RET rettype);

void slicif_start_segment(char *name);

void slicif_get_local_name(char *localName, const char *name);

void slicif_add_prototype(char *name);

void slicif_start_for();
void slicif_for_expression();
void slicif_for_continue();
void slicif_start_for_body();
void slicif_end_for();

int slicif_find_const(const char *name, int *value);
void slicif_add_const(const char *name, int value);

void slicif_check_event_exists(const char *name);
char *slicif_create_name(const char *base);

void slicif_set_priority(SLIC_PRI pri);
SLIC_PRI slicif_get_priority();

char *slicif_get_segment_name_copy();
void slicif_set_event_checking(const char *eventname);

void slicif_add_local_struct(char *structtype, char *name);

void slicif_register_line(int line, int offset);

const char *slicif_get_filename();

void slicif_start_event(char *name);
void slicif_check_arg_symbol(SLIC_SYM type, const char *typeName);
void slicif_check_argument();
void slicif_check_string_argument();
void slicif_check_hard_string_argument();
void slicif_check_num_args();

int slicif_find_db(const char *dbname, void **dbptr);
int slicif_find_db_index(void *dbptr, const char *name);

int slicif_find_file(char *filename, char *fullpath);
void slicif_report_error(char *s);
int slicif_is_valid_string(char *s);

int slicif_find_db_value(void *dbptr, const char *recname, const char *valname);
int slicif_find_db_value_by_index(void *dbptr, int index, const char *valname);
int slicif_find_db_array_value(void *dbptr, const char *recname, const char *valname, int val);
int slicif_find_db_array_value_by_index(void *dbptr, int index, const char *valname, int val);

/* Added by Martin G�hmann */
int slicif_is_name(void *dbptr, const char *name);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
#include <string.h>

/**
 * @name Safe unaligned integer read/write helpers
 *
 * The SLIC bytecode compiler packs integer literals and jump offsets directly
 * into the instruction stream without any alignment padding.  On x86/x64 this
 * works because the CPU allows unaligned loads/stores, but on ARM64 (and under
 * UBSan on any architecture) a direct cast such as
 *
 *     sint32 value = *((sint32 *)bytePtr);
 *
 * is undefined behaviour when bytePtr is not 4-byte aligned.
 *
 * These helpers use memcpy() to perform the transfer safely.  Modern compilers
 * (Clang, GCC, MSVC) optimise memcpy of a scalar size into a single aligned or
 * unaligned machine instruction, so there is no runtime overhead.
 *
 * Use these helpers whenever you read or write a multi-byte integer from/to
 * the SLIC bytecode buffer (unsigned char *).
 */
//@{
/** Read a 32-bit signed integer from an unaligned source address. */
inline void slicif_read_sint32(const unsigned char *src, sint32 *dst) {
	memcpy(dst, src, sizeof(sint32));
}

/** Read a platform 'int' from an unaligned source address. */
inline void slicif_read_int(const unsigned char *src, int *dst) {
	memcpy(dst, src, sizeof(int));
}

/** Write a 32-bit signed integer to an unaligned destination address. */
inline void slicif_store_sint32(unsigned char *dst, sint32 value) {
	memcpy(dst, &value, sizeof(sint32));
}

/** Write a platform 'int' to an unaligned destination address. */
inline void slicif_store_int(unsigned char *dst, int value) {
	memcpy(dst, &value, sizeof(int));
}
//@}

class SlicNamedSymbol;
SlicNamedSymbol *slicif_get_symbol(const char *name);
#endif

#endif
