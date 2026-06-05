#include "ctp/c3.h"
#include "gs/slic/SlicConditional.h"
#include "gs/slic/sliccmd.h"
#include "sliccmd.tab.h"

SlicConditional::SlicConditional(const char *expression)
{
	strlcpy(m_expression, expression, sizeof(m_expression));
}

SlicConditional::~SlicConditional()
= default;

sint32 SlicConditional::Eval()
{
	char output[1024];
	char catString = 0;

	sliccmd_int_result = 0;
	sliccmd_parse(SLICCMD_EVAL, m_expression, output, 1024, 0, &catString);
	return sliccmd_int_result;
}

void SlicConditional::SetExpression(const char *expression)
{
	strlcpy(m_expression, expression, sizeof(m_expression));
}
