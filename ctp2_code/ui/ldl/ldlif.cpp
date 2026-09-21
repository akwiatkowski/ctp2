#include "ctp/c3.h"

#include <cinttypes>
#include <memory>
#include <string>

#include "ui/ldl/ldlif.h"
#include "gs/fileio/CivPaths.h"
#include "ctp/ctp2_utils/c3errors.h"

#include "gs/slic/StringHash.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "ctp/ctp2_utils/AvlTree.h"

#include "ui/aui_common/aui_ui.h"

#include "ui/ldl/ldl_data.hpp"
#include "ldl_attr.hpp"
#include "ctp/ctp2_utils/c3errors.h"

class LDLString {
	std::string m_name;
  public:
	LDLString(const char *text) {
		m_name = text;
	}

	// The returned pointer is interned: callers store it (in ldl_datablock /
	// ldl_attribute / the lexer's nameval) and compare names by pointer
	// identity, never by content, and never write through it.  std::string's
	// buffer is stable for the object's lifetime (name is never mutated after
	// construction), so c_str() preserves that identity.  const_cast keeps the
	// historic char* return type so every consumer compiles unchanged.
	char * GetName() const { return const_cast<char *>(m_name.c_str()); }
};

std::unique_ptr<StringHash<LDLString>> s_ldlStringHash;

std::unique_ptr<PointerList<ldl_datablock>> s_blockStack;
std::unique_ptr<AvlTree<ldl_datablock *>> s_blockTree;
std::unique_ptr<PointerList<ldl_datablock>> s_topLevelList;

extern "C" { void ldlif_report_error(char *text); }

void ldlif_report_error(char *text)
{
	// "%s": text is caller-supplied diagnostic text, not a template.
	c3errors_ErrorDialog("LDL", "%s", text);
}

ldl_datablock *ldlif_find_block(char const * name)
{
	Comparable<ldl_datablock *> *myKey;
	ldl_datablock dummy(aui_UI::CalculateHash(name));
	myKey = s_blockTree->Search(&dummy);
	if(myKey) {
		return myKey->Key();
	}
	return nullptr;
}

int ldlif_find_file(const char *filename, char *fullpath)
{
	if(!civpaths_Get()->FindFile(C3DIR_LAYOUT, filename, fullpath))
		return 0;
	return 1;
}

char *ldlif_getnameptr(const char *name)
{
	const LDLString *str = s_ldlStringHash->Get(name);
	if(str) {
		return str->GetName();
	}

	LDLString *newstr = std::make_unique<LDLString>(name).release();
	s_ldlStringHash->Add(newstr);
	return newstr->GetName();
}

char *ldlif_getstringptr(const char *text)
{
	return ldlif_getnameptr(text);
}

void ldlif_add_name(void **newnames, char *name, void *oldnames)
{
	PointerList<char> *namelist = (PointerList<char> *)oldnames;
	if(!namelist) {
		namelist = std::make_unique<PointerList<char>>().release();
	}
	namelist->AddHead(name);
	*newnames = (void *)namelist;
}

void ldlif_init_log()
{
#ifdef _DEBUG
	FILE *f = fopen("ldlparselog.txt", "w");
	if(f) {
		fprintf(f, "%" PRId64 "\n", static_cast<int64_t>(time(nullptr)));
		fclose(f);
	}
#endif
}
void ldlif_log(char *format, ...)
{
#ifdef _DEBUG
	va_list list;
	va_start(list, format);

	FILE *f = fopen("ldlparselog.txt", "a");
	vfprintf(f, format, list);
	fclose(f);
	va_end(list);
#endif
}

void ldlif_indent_log(int indent)
{
#ifdef _DEBUG
	int i;
	for(i = 0; i < indent; i++) {
		ldlif_log(const_cast<char *>("    "));
	}
#endif
}

void ldlif_start_block(void *names)
{
	PointerList<char> *namelist = (PointerList<char> *)names;
	Assert(namelist);

	ldl_datablock *block = std::make_unique<ldl_datablock>(namelist).release();

	if(s_blockStack->GetTail()) {
		s_blockStack->GetTail()->AddChild(block);
	}
	s_blockStack->AddTail(block);
	ldlif_add_block_to_tree(block);
}

static cmp_t ldlif_compare_blocks(ldl_datablock *b1, ldl_datablock *b2)
{
	if(b1->GetHash() < b2->GetHash()) return MIN_CMP;
	if(b1->GetHash() > b2->GetHash()) return MAX_CMP;
	return EQ_CMP;
}

void ldlif_add_block_to_tree(ldl_datablock *block)
{
	char fullname[256];
	block->GetFullName(fullname, sizeof(fullname));

	ldlif_log(const_cast<char *>("Added: %s\n"), fullname);

	block->SetHash(aui_UI::CalculateHash(fullname));
	auto cmp = std::make_unique<Comparable<ldl_datablock *>>(block, ldlif_compare_blocks);
	if(s_blockTree->Insert(cmp.get())) {
		char buf[300];
		snprintf(buf, sizeof(buf), "Duplicate block %s\n", fullname);
		ldlif_report_error(buf);
	} else {
		cmp.release();
	}

	PointerList<ldl_datablock> *childList = block->GetChildList();
	PointerList<ldl_datablock>::Walker walk(childList);
	while(walk.IsValid()) {
		ldlif_add_block_to_tree(walk.GetObj());
		walk.Next();
	}
}

void ldlif_remove_block_from_tree(ldl_datablock *block)
{
	std::unique_ptr<Comparable<ldl_datablock *>> myKey;
	char fullname[256];
	block->GetFullName(fullname, sizeof(fullname));
	ldl_datablock dummy(aui_UI::CalculateHash(fullname));
	myKey.reset(s_blockTree->Delete(&dummy));


}

void *ldlif_end_block(void *names)
{
	PointerList<char> *namelist = (PointerList<char> *)names;
	Assert(namelist);

	ldl_datablock *block = s_blockStack->RemoveTail();

	block->AddTemplateChildren();










	if(!s_blockStack->GetTail()) {
		s_topLevelList->AddTail(block);

	}

	std::unique_ptr<PointerList<char>>{namelist};
	return block;

}

void *ldlif_add_empty_block(void *names)
{
	ldlif_start_block(names);
	return ldlif_end_block(names);
}

void ldlif_add_bool_attribute(char *name, int val)
{
	ldl_attributeValue<bool> *attr = std::make_unique<ldl_attributeValue<bool>>(name, ATTRIBUTE_TYPE_BOOL, val != 0).release();
	s_blockStack->GetTail()->AddAttribute(attr);
}

void ldlif_add_int_attribute(char *name, int val)
{
	ldl_attributeValue<int> *attr = std::make_unique<ldl_attributeValue<int>>(name, ATTRIBUTE_TYPE_INT, val).release();
	s_blockStack->GetTail()->AddAttribute(attr);
}

void ldlif_add_float_attribute(char *name, double val)
{
	ldl_attributeValue<double> *attr = std::make_unique<ldl_attributeValue<double>>(name, ATTRIBUTE_TYPE_DOUBLE, val).release();
	s_blockStack->GetTail()->AddAttribute(attr);
}

void ldlif_add_string_attribute(char *name, char *val)
{
	ldl_attributeValue<char *> *attr = std::make_unique<ldl_attributeValue<char *>>(name, ATTRIBUTE_TYPE_STRING, val).release();
	s_blockStack->GetTail()->AddAttribute(attr);
}

void ldlif_allocate_stuff()
{
	s_ldlStringHash = std::make_unique<StringHash<LDLString>>(1024);
	s_blockStack = std::make_unique<PointerList<ldl_datablock>>();
	s_blockTree = std::make_unique<AvlTree<ldl_datablock *>>();
	s_topLevelList = std::make_unique<PointerList<ldl_datablock>>();
}

void ldlif_deallocate_stuff()
{
	s_blockStack.reset();

	s_topLevelList->DeleteAll();
	s_topLevelList.reset();

	s_ldlStringHash.reset();

	s_blockTree.reset();

}
