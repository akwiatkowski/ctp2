#include "ctp/c3.h"

#include <memory>

#include "gfx/spritesys/SpriteStateDB.h"

#include "gs/fileio/Token.h"

#include "ctp/ctp2_utils/c3files.h"

#ifdef __SPRITETEST__
	#define CHECKSERIALIZE			;
#endif

extern sint32 g_abort_parse;

SpriteStateDB::SpriteStateDB ()

{
}










SpriteStateDB::~SpriteStateDB () = default;

void SpriteStateDB::SetSize(sint32 s)

{
	m_size = s;
	m_map = std::make_unique<SpriteNameNode[]>(m_size);
}











sint32 SpriteStateDB::FindTypeIndex(char *str) const

{
    sint32 i;

    Assert(m_map);

	for (i=0; i<m_size; i++) {
        if (strcmp(m_map[i].m_name, str) == 0) {
			return i;
		}
	}
	return -1;
}

sint32 SpriteStateDB::GetDefaultVal(sint32 index) const

{

	Assert(0 <= index);
	Assert(index < m_size);

	if (index < 0 || index >= m_size) return -1;

	return m_map[index].m_default_val;
}

void SpriteStateDB::SetName(sint32 index, char str[_MAX_PATH])

{
 	Assert(0 <= index);
	Assert(index < m_size);
    Assert(m_map);
    if (index < 0 || index >= m_size || !m_map) return;

    strlcpy(m_map[index].m_name, str, sizeof(m_map[index].m_name));
}

void SpriteStateDB::SetVal(sint32 index, sint32 val)
{
 	Assert(0 <= index);
	Assert(index < m_size);
    Assert(m_map);
    if (index < 0 || index >= m_size || !m_map) return;

    m_map[index].m_default_val = val;
}













sint32 SpriteStateDB::ParseASpriteState (Token *spriteToken, sint32 count)
{
    char str[_MAX_PATH];

	if (spriteToken->GetType() == TOKEN_EOF) {
		return FALSE;
	}

	if (spriteToken->GetType() != TOKEN_STRING) {
		c3errors_ErrorDialog  (spriteToken->ErrStr(), "Sprite name expected");
        g_abort_parse = TRUE;
		return FALSE;
	} else {
   		spriteToken->GetString(str);
        SetName(count, str);
	}

    sint32 val;
    if (spriteToken->Next() != TOKEN_NUMBER) {
		c3errors_ErrorDialog  (spriteToken->ErrStr(), "Sprite default number expected");
        g_abort_parse = TRUE;
		return FALSE;
	} else {
   		spriteToken->GetNumber(val);
        SetVal(count, val);
	}

    spriteToken->Next();

    return TRUE;
}

sint32 SpriteStateDB::Parse(char *filename)

{
    sint32 n;

    auto spriteToken = std::make_unique<Token>(filename, C3DIR_GAMEDATA);

   	if (spriteToken->GetType() != TOKEN_NUMBER) {
		c3errors_ErrorDialog  (spriteToken->ErrStr(), "Missing number of ability");
        g_abort_parse = TRUE;
		return FALSE;
	} else {
		spriteToken->GetNumber(n);
		spriteToken->Next();
		if (n <0) {
			c3errors_ErrorDialog  (spriteToken->ErrStr(), "Number of sprites is negative");
            g_abort_parse = TRUE;
			return FALSE;
		}
		SetSize(n + 1);
	}

    int count = 0;

    while (ParseASpriteState(spriteToken.get(), count)) {
        count++;
    }

	Assert(count == n);
	if(count == n) {
		SetName(count, "SPRITE_MYSTERY");
		SetVal(count, 9);
	}

	if (g_abort_parse) {
		return FALSE;
	}

    return TRUE;
}
