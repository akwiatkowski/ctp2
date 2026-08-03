#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __WONDERMOVIEWINDOW_H__
#define __WONDERMOVIEWINDOW_H__

#include <memory>

class aui_MovieButton;
class c3_Static;
class Sequence;
class ctp2_HyperTextBox;

class WonderMovieWindow : public C3Window
{
public:

	WonderMovieWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD );
	WonderMovieWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 bpp,
		MBCHAR *pattern,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD );

	~WonderMovieWindow() override;

	virtual AUI_ERRCODE InitCommonLdl(MBCHAR *ldlBlock);
	virtual AUI_ERRCODE InitCommon();

	AUI_ERRCODE Idle() override;

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	void SetMovie(const MBCHAR *filename);
	void SetWonderName(MBCHAR *name);
	void SetText(const MBCHAR *text);

  std::weak_ptr<Sequence> GetSequence() { return m_sequence;}
	void SetSequence(std::weak_ptr<Sequence> seq) { m_sequence = seq; }

private:
	std::unique_ptr<aui_MovieButton>	m_movieButton;
	std::unique_ptr<c3_Static>	m_wonderName;
	std::weak_ptr<Sequence> m_sequence;
	std::unique_ptr<c3_Static>	m_topBorder;
	std::unique_ptr<c3_Static>	m_leftBorder;
	std::unique_ptr<c3_Static>	m_rightBorder;
	std::unique_ptr<c3_Static>	m_bottomBorder;
	std::unique_ptr<ctp2_HyperTextBox>	m_textBox;
};

#endif
