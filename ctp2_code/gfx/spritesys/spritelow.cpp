#include "ctp/c3.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/spriteutils.h"
#include "gfx/spritesys/Sprite.h"
#include "ui/aui_common/aui_surface.h"

#include "gfx/tilesys/tiledmap.h"



void Sprite::DrawLowClipped565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
							uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8			*surfBase;

	sint32			surfWidth = m_surfWidth;
	sint32			surfHeight = m_surfHeight;
	sint32			surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	sint32			 xstart;
	sint32			 xend;
	sint32			 ystart;
	sint32			 yend;

	if (drawX < 0) {
		xstart = 0 - drawX;
		drawX = 0;
	} else {
		xstart = 0;
	}

	if (drawY < 0) {
		ystart = 0 - drawY;
		drawY = 0;
	} else {
		ystart = 0;
	}

	if (drawX >= surfWidth - width) {

		xend = (surfWidth - drawX);
	} else {
		xend = width;
	}

	if (drawY >= surfHeight - height) {

		yend = (surfHeight - drawY);
	} else {
		yend = height;
	}

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);

	uint8			*destPixel;
	Pixel16		*srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

		sint32 j;
	sint32		len;

	sint32		xpos;

	for(j=ystart; j<yend; j++) {
		if (table[j] != k_EMPTY_TABLE_ENTRY) {
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + j * surfPitch;
			rowData = dataStart + table[j];
			tag = *rowData++;
			tag = tag & 0x0FFF;
			xpos = 0;
			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							len = (tag & 0x00FF);
							while (len) {
								len--;
								if (xpos >= xstart && xpos < xend)
									destPixel += step;
								xpos++;
							}
						break;
					case k_COPY_RUN_ID			: {
							len = (tag & 0x00FF);

							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								while (len) {
									len--;
									if (xpos >= xstart && xpos < xend)
										{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
									destPixel += step;
									rowData++;
									xpos++;
								}
							} else
								if (flags & k_BIT_DRAWFLAGS_FOGGED) {
									while (len) {
										len--;
										if (xpos >= xstart && xpos < xend)
											{ pixelutils_StorePixel(destPixel, pixelutils_Shadow_565(*rowData), bpp32); destPixel += step; rowData++; }
										else
											rowData++;
										xpos++;
									}
								} else
									if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
										while (len) {
											len--;
											if (xpos >= xstart && xpos < xend)
												{ pixelutils_StorePixel(destPixel, pixelutils_Desaturate_565(*rowData), bpp32); destPixel += step; rowData++; }
											else
												rowData++;
											xpos++;
										}
									} else {
										while (len) {
											len--;
											if (xpos >= xstart && xpos < xend)
												{ pixelutils_StorePixel(destPixel, *rowData, bpp32); destPixel += step; rowData++; }

											else
												rowData++;
											xpos++;
										}
									}
						}
						break;
					case k_SHADOW_RUN_ID		: {
							len = (tag & 0x00FF);

							while (len) {
								len--;
								if (xpos >= xstart && xpos < xend) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Shadow8888(*d); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Shadow_565(*d); } }
									destPixel += step;
								}
								xpos++;
							}
						}
						break;
					case k_FEATHERED_RUN_ID	:
						if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
							if (xpos >= xstart && xpos < xend)
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
							destPixel += step;
							rowData++;
							xpos++;
						} else {
							Pixel16 pixel = *rowData;
							if (flags & k_BIT_DRAWFLAGS_FOGGED) {
								pixel = pixelutils_Shadow_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
								pixel = pixelutils_Desaturate_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
								uint16 alpha = (tag & 0x00FF);
								if (xpos >= xstart && xpos < xend) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(pixel), *d, (uint16)alpha>>3); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_BlendFast_565(pixel, *d, (uint16)alpha>>3); } }
									destPixel += step;
								}
								rowData++;
								xpos++;
							} else {
								if (xpos >= xstart && xpos < xend)
									{ pixelutils_StorePixel(destPixel, pixel, bpp32); destPixel += step; }
								xpos++;
								rowData++;
							}
						}
						break;
					default:
						Assert(FALSE);
						break;
				}
				tag = *rowData++;
			}
		}
	}
}






void Sprite::DrawLow565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
					 uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8			*surfBase;




	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	if (drawX <= 0 - width) return;
	if (drawY <= 0 - height) return;
	if (drawX >= surfWidth) return;
	if (drawY >= surfHeight) return;


	if ((drawX < 0) ||
		(drawY < 0) ||
		(drawX >= (surfWidth-width)) ||
		(drawY >= (surfHeight-height)))
	{
		(this->*_DrawLowClipped)(frame, drawX, drawY, width, height, transparency, outlineColor, flags);
		return;
	}

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);





	uint8			*destPixel;
	Pixel16		*srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

		sint32 j;
	sint32		len;

	for(j=0; j<height; j++)
	{
		if (table[j] != k_EMPTY_TABLE_ENTRY)
		{
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + j * surfPitch;

			rowData = dataStart + table[j];

			tag = *rowData++;

			tag = tag & 0x0FFF;

			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							destPixel += (tag & 0x00FF) * step;

						break;
					case k_COPY_RUN_ID			: {
							len = (tag & 0x00FF);

							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								while (len) {
									len--;
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
									destPixel += step;
									rowData++;
								}
								goto CopyDone;
							}

							if (flags & k_BIT_DRAWFLAGS_FOGGED) {
								while (len) {
									len--;
									{ pixelutils_StorePixel(destPixel, pixelutils_Shadow_565(*rowData), bpp32); destPixel += step; rowData++; }
								}
								goto CopyDone;
							}

							if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
								while (len) {
									len--;
									{ pixelutils_StorePixel(destPixel, pixelutils_Desaturate_565(*rowData), bpp32); destPixel += step; rowData++; }
								}
								goto CopyDone;
							}

							while (len) {
								len--;
								{ pixelutils_StorePixel(destPixel, *rowData, bpp32); destPixel += step; rowData++; }
							}
						}
CopyDone:
						break;
					case k_SHADOW_RUN_ID		: {
							len = (tag & 0x00FF);

							while (len) {
								len--;
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Shadow8888(*d); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Shadow_565(*d); } }
								destPixel += step;
							}
						}
						break;
					case k_FEATHERED_RUN_ID	:
						if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
							{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
							destPixel += step;
							rowData++;
						} else {
							Pixel16 pixel = *rowData;
							if (flags & k_BIT_DRAWFLAGS_FOGGED) {
								pixel = pixelutils_Shadow_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
								pixel = pixelutils_Desaturate_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
								uint16 alpha = (tag & 0x00FF);
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(pixel), *d, (uint16)alpha>>3); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_BlendFast_565(pixel, *d, (uint16)alpha>>3); } }
								destPixel += step;
								rowData++;
							} else {
								{ pixelutils_StorePixel(destPixel, pixel, bpp32); destPixel += step; }
								rowData++;
							}
						}
						break;
					default:
						Assert(FALSE);
						break;
				}
				tag = *rowData++;
			}
		}
	}
}


void Sprite::DrawLowReversedClipped565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
									uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8			*surfBase;

	sint32			surfWidth = m_surfWidth;
	sint32			surfHeight = m_surfHeight;
	sint32			surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	sint32			 xstart;
	sint32			 xend;
	sint32			 ystart;
	sint32			 yend;

	if (drawX < 0) {
		xstart = 0 - drawX;
		drawX = 0;
	} else {
		xstart = 0;
	}

	if (drawY < 0) {
		ystart = 0 - drawY;
		drawY = 0;
	} else {
		ystart = 0;
	}

	if (drawX >= surfWidth - width) {

		xend = (surfWidth - drawX);
	} else {
		xend = width;
	}

	if (drawY >= surfHeight - height) {

		yend = (surfHeight - drawY);
	} else {
		yend = height;
	}

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);

	uint8			*destPixel;
	Pixel16		*srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

		sint32 j;
	sint32		len;

	sint32		xpos;

	for(j=ystart; j<yend; j++) {
		if (table[j] != k_EMPTY_TABLE_ENTRY) {
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + j * surfPitch + (width * step);
			rowData = dataStart + table[j];
			tag = *rowData++;
			tag = tag & 0x0FFF;
			xpos = width;
			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							len = (tag & 0x00FF);
							while (len) {
								len--;
								if (xpos >= xstart && xpos < xend)
									destPixel -= step;
								xpos--;
							}
						break;
					case k_COPY_RUN_ID			: {
							len = (tag & 0x00FF);

							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								while (len) {
									len--;
									if (xpos >= xstart && xpos < xend)
										{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
									destPixel -= step;
									rowData++;
									xpos--;
								}
							} else
								if (flags & k_BIT_DRAWFLAGS_FOGGED) {
									while (len) {
										len--;
										if (xpos >= xstart && xpos < xend)
											{ pixelutils_StorePixel(destPixel, pixelutils_Shadow_565(*rowData), bpp32); destPixel -= step; rowData++; }
										else
											rowData++;
										xpos--;
									}
								} else
									if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
										while (len) {
											len--;
											if (xpos >= xstart && xpos < xend)
												{ pixelutils_StorePixel(destPixel, pixelutils_Desaturate_565(*rowData), bpp32); destPixel -= step; rowData++; }
											else
												rowData++;
											xpos--;
										}
									} else {
										while (len) {
											len--;
											if (xpos >= xstart && xpos < xend)
												{ pixelutils_StorePixel(destPixel, *rowData, bpp32); destPixel -= step; rowData++; }

											else
												rowData++;
											xpos--;
										}
									}
						}
						break;
					case k_SHADOW_RUN_ID		: {
							len = (tag & 0x00FF);

							while (len) {
								len--;
								if (xpos >= xstart && xpos < xend) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Shadow8888(*d); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Shadow_565(*d); } }
									destPixel -= step;
								}
								xpos--;
							}
						}
						break;
					case k_FEATHERED_RUN_ID	:
						if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
							if (xpos >= xstart && xpos < xend)
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
							destPixel -= step;
							rowData++;
							xpos--;
						} else {
							Pixel16 pixel = *rowData;
							if (flags & k_BIT_DRAWFLAGS_FOGGED) {
								pixel = pixelutils_Shadow_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
								pixel = pixelutils_Desaturate_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
								uint16 alpha = (tag & 0x00FF);
								if (xpos >= xstart && xpos < xend) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(pixel), *d, (uint16)alpha>>3); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_BlendFast_565(pixel, *d, (uint16)alpha>>3); } }
									destPixel -= step;
								}
								rowData++;
								xpos--;
							} else {
								if (xpos >= xstart && xpos < xend)
									{ pixelutils_StorePixel(destPixel, pixel, bpp32); destPixel -= step; }
								xpos--;
								rowData++;
							}
						}
						break;
					default:
						Assert(FALSE);
						break;
				}
				tag = *rowData++;
			}
		}
	}
}




void Sprite::DrawLowReversed565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
							 uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8			*surfBase;




	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	if (drawX <= 0 - width) return;
	if (drawY <= 0 - height) return;
	if (drawX >= surfWidth) return;
	if (drawY >= surfHeight) return;


	if (drawX < 0 || drawY < 0 || drawX >= (surfWidth - width) || drawY >= (surfHeight-height)) {
		(this->*_DrawLowReversedClipped)(frame, drawX, drawY, width, height, transparency, outlineColor, flags);
		return;
	}

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);









	uint8			*destPixel;
	Pixel16  *srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

		sint32 j;
		sint32 len;

	for(j=0; j < height; j++) {
		if (table[j] != k_EMPTY_TABLE_ENTRY) {
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + j * surfPitch + (width * step);
			rowData = dataStart + table[j];
			tag = *rowData++;
			tag = tag & 0x0FFF;

			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							destPixel -= (tag & 0x00FF) * step;
						break;
					case k_COPY_RUN_ID			: {
							len = (tag & 0x00FF);

							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								while (len--) {

									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, transparency); } }
									destPixel -= step;
									rowData++;
								}
							} else
							if (flags & k_BIT_DRAWFLAGS_FOGGED) {
								while (len--) {
									{ pixelutils_StorePixel(destPixel, pixelutils_Shadow_565(*rowData), bpp32); destPixel -= step; rowData++; }
								}
							} else
								if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
									while (len--) {
										{ pixelutils_StorePixel(destPixel, pixelutils_Desaturate_565(*rowData), bpp32); destPixel -= step; rowData++; }
									}
								} else {
									while (len--) {
										{ pixelutils_StorePixel(destPixel, *rowData, bpp32); destPixel -= step; rowData++; }
									}
								}
						}
						break;
					case k_SHADOW_RUN_ID		: {
							len = (tag & 0x00FF);

							while (len--) {

								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Shadow8888(*d); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Shadow_565(*d); } }
								destPixel -= step;
							}
						}
						break;
					case k_FEATHERED_RUN_ID	:
						if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, (uint16)transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_BlendFast_565(*rowData, *d, (uint16)transparency); } }
								destPixel -= step;
								rowData++;
						} else {
							Pixel16 pixel = *rowData;
							if (flags & k_BIT_DRAWFLAGS_FOGGED) {
								pixel = pixelutils_Shadow_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
								pixel = pixelutils_Desaturate_565(pixel);
							}
							if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
								uint16 alpha = (tag & 0x00FF);
								if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
									pixel = pixelutils_Desaturate_565(pixel);
								}
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(pixel), *d, (uint16)alpha>>3); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_BlendFast_565(pixel, *d, (uint16)alpha>>3); } }
								destPixel -= step;
								rowData++;
							} else {
								{ pixelutils_StorePixel(destPixel, pixel, bpp32); destPixel -= step; }
								rowData++;
							}
						}
						break;
				}
				tag = *rowData++;
			}
		}
	}
}

#define k_MEDIUM_KEY	0x4208




void Sprite::DrawScaledLow565(Pixel16 *data, sint32 x, sint32 y, sint32 destWidth, sint32 destHeight,
							 uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL reverse)
{
	uint8			*surfBase;

	Pixel16		emptyRow[2];

	emptyRow[0] = (Pixel16)((k_CHROMAKEY_RUN_ID << 8) | m_width);
	emptyRow[1] = (Pixel16)(k_EOLN_ID << 8);

	RECT destRect = { x, y, x + destWidth, y + destHeight };

	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;

	surfBase = m_surfBase + (y * surfPitch) + (x * sizeof(Pixel16));

	if (destRect.left < 0) return;
	if (destRect.top < 0) return;
	if (destRect.right > surfWidth) return;
	if (destRect.bottom > surfHeight) return;

	Pixel16			*destPixel;
	Pixel16			*srcPixel = (Pixel16 *)data;

	Pixel16			*table = data+1;
	Pixel16			*dataStart = table + m_height;

	sint32				vaccum;
	sint32				 vincx;
	sint32				 vincxy;
	sint32				vend;
	sint32				vdestpos;
	sint32				 vpos1;
	sint32				 vpos2;

	vaccum = destHeight*2 - m_height;
	vincx = destHeight*2;
	vincxy = (destHeight - m_height) * 2 ;

	vpos1 = 0;
	vpos2 = (sint32)((double)(m_height - destHeight) / (double)destHeight);

	vdestpos = y;
	vend = m_height - 1;

	while ( vpos1 < vend) {
		if (vaccum < 0) {
			vaccum += vincx;
		} else {

			Pixel16		 *rowData1;
			Pixel16		 *rowData2;
			Pixel16		 pixel1;
			Pixel16		 pixel2;
			Pixel16		 pixel3;
			Pixel16		 pixel4;
			Pixel16		pixel;

			sint32		haccum;
			sint32		 hincx;
			sint32		 hincxy;
			sint32		hend;
			sint32		hpos;
			sint32		hdestpos;

			if (table[vpos1] == k_EMPTY_TABLE_ENTRY) rowData1 = emptyRow;
			else rowData1 = dataStart + table[vpos1];
			if (table[vpos2] == k_EMPTY_TABLE_ENTRY) rowData2 = emptyRow;
			else rowData2 = dataStart + table[vpos2];

			haccum = destWidth*2 - m_width;
			hincx = destWidth*2;
			hincxy = (destWidth - m_width) * 2;

			hpos = 0;
			if (reverse) hdestpos = x + destWidth;
			else hdestpos = x;
			hend = m_width-1;

			sint32		mode1;
			sint32		mode2;
			sint32		pos1 = 0;
			sint32		pos2 = 0;
			sint32		 end1;
			sint32		 end2;
			sint32		 alpha1;
			sint32		 alpha2;
			sint32		 oldend1 = 0;
			sint32		 oldend2 = 0;

			Pixel16			 firstPixel;
			Pixel16			 secondPixel;

			end1 = ReadTag(&mode1, &rowData1, &alpha1);
			end2 = ReadTag(&mode2, &rowData2, &alpha2);

			pixel1 = k_MEDIUM_KEY;
			pixel2 = k_MEDIUM_KEY;

			while (hpos < hend) {
				if (haccum < 0) {
					haccum += hincx;
				} else {
					haccum += hincxy;

					destPixel = (Pixel16 *)(surfBase + ((vdestpos-y) * surfPitch) + ((hdestpos-x) * 2));

					while (pos1 <= hpos) {
						switch (mode1) {
							case k_CHROMAKEY_RUN_ID	:
									firstPixel = k_MEDIUM_KEY;
								break;
							case k_COPY_RUN_ID			:
									if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
										firstPixel = pixelutils_Blend_565(*rowData1, *destPixel, transparency);
									} else
									if (flags & k_BIT_DRAWFLAGS_FOGGED) {
										firstPixel = pixelutils_Shadow_565(*rowData1);
									} else
									if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
										firstPixel = pixelutils_Desaturate_565(*rowData1);
									} else {
										firstPixel = *rowData1;
									}
									rowData1++;
								break;
							case k_SHADOW_RUN_ID		:
									firstPixel = pixelutils_Shadow_565(*destPixel);
								break;
							case k_FEATHERED_RUN_ID	:
									if (flags & k_BIT_DRAWFLAGS_OUTLINE) {
										firstPixel = outlineColor;
									} else {
										if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
											firstPixel = pixelutils_Blend_565(*rowData1, *destPixel, (uint16)transparency);
										} else {
											Pixel16 pixel = *rowData1;
											if (flags & k_BIT_DRAWFLAGS_FOGGED) {
												pixel = pixelutils_Shadow_565(pixel);
											}
											if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
												pixel = pixelutils_Desaturate_565(pixel);
											}
											if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
												pixel = pixelutils_Blend_565(pixel, *destPixel, (uint16)alpha1>>3);
											}
											firstPixel = pixel;
										}
									}
									rowData1++;
								break;
							default:
								Assert(mode1 == k_CHROMAKEY_RUN_ID || mode1 == k_COPY_RUN_ID || mode1 == k_SHADOW_RUN_ID || mode1 == k_FEATHERED_RUN_ID);
						}

						pos1++;

						if (pos1 >= end1) {
							oldend1 = end1;
							end1 = oldend1 + ReadTag(&mode1, &rowData1, &alpha1);
						}
					}

					while (pos2 <= hpos) {

						switch (mode2) {
							case k_CHROMAKEY_RUN_ID	:
									secondPixel = k_MEDIUM_KEY;
								break;
							case k_COPY_RUN_ID			:
									if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
										secondPixel = pixelutils_Blend_565(*rowData2, *destPixel, transparency);
									} else
									if (flags & k_BIT_DRAWFLAGS_FOGGED) {
										secondPixel = pixelutils_Shadow_565(*rowData2);
									} else
									if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
										secondPixel = pixelutils_Desaturate_565(*rowData2);
									} else {
										secondPixel = *rowData2;
									}

									rowData2++;
								break;
							case k_SHADOW_RUN_ID		:
									secondPixel = pixelutils_Shadow_565(*destPixel);
								break;
							case k_FEATHERED_RUN_ID	:
									if (flags & k_BIT_DRAWFLAGS_OUTLINE) {
										secondPixel = outlineColor;
									} else {
										if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
											secondPixel = pixelutils_Blend_565(*rowData2, *destPixel, (uint16)transparency);
										} else {
											Pixel16 pixel = *rowData2;
											if (flags & k_BIT_DRAWFLAGS_FOGGED) {
												pixel = pixelutils_Shadow_565(pixel);
											}
											if (flags & k_BIT_DRAWFLAGS_DESATURATED) {
												pixel = pixelutils_Desaturate_565(pixel);
											}
											if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
												pixel = pixelutils_Blend_565(pixel, *destPixel, (uint16)alpha2>>3);
											}
											secondPixel = pixel;
										}
									}
									rowData2++;
								break;
							default :
								Assert(mode2 == k_CHROMAKEY_RUN_ID || mode2 == k_COPY_RUN_ID || mode2 == k_SHADOW_RUN_ID || mode2 == k_FEATHERED_RUN_ID);
						}

						pos2++;

						if (pos2 >= end2) {
							oldend2 = end2;
							end2 = oldend2 + ReadTag(&mode2, &rowData2, &alpha2);
						}
					}

					pixel3 = firstPixel;
					pixel4 = secondPixel;






					if (flags & k_BIT_DRAWFLAGS_AA) {
						if (flags & k_BIT_DRAWFLAGS_AA_4_WAY) {
							if (pixel1 != k_MEDIUM_KEY || pixel2 != k_MEDIUM_KEY || pixel3 != k_MEDIUM_KEY || pixel4 != k_MEDIUM_KEY) {
								pixel =	average(pixel1, pixel2, pixel3, pixel4);

								if (pixel == 0)
									pixel = 0x0001;
								*destPixel = pixel;
							}
						} else {
							if (pixel1 != k_MEDIUM_KEY || pixel2 != k_MEDIUM_KEY) {
								pixel = average(pixel2, pixel3);
								if (pixel == 0)
									pixel = 0x0001;

								*destPixel = pixel;
							}
						}
					} else {
						if (pixel3 == 0)
							pixel3 = 0x0001;

						if (pixel3 != k_MEDIUM_KEY)
							*destPixel = pixel3;
					}
					pixel1 = pixel3;
					pixel2 = pixel4;

					if (reverse) hdestpos--;
					else hdestpos++;
				}
				hpos++;
			}

			vaccum += vincxy;
			vdestpos++;
		}
		vpos1++;
		vpos2++;
	}




}




void Sprite::DrawFlashLow565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
					 uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8			*surfBase;

	RECT destRect = { drawX, drawY, drawX + width, drawY + height };

	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);

	if (destRect.left < 0) return;
	if (destRect.top < 0) return;
	if (destRect.right > surfWidth) return;
	if (destRect.bottom > surfHeight) return;

	uint8			*destPixel;
	Pixel16  *srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

	sint32 j;
	sint32 len;

	for(j=0; j<height; j++) {
		if (table[j] != k_EMPTY_TABLE_ENTRY) {
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + j * surfPitch;
			rowData = dataStart + table[j];
			tag = *rowData++;
			tag = tag & 0x0FFF;

			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							destPixel += (tag & 0x00FF) * step;

						break;
					case k_COPY_RUN_ID			: {
							len = (tag & 0x00FF);

							while (len--) {

								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Additive8888(*d, pixelutils_16to8888(*rowData)); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Additive_565(*d, *rowData); } }
								destPixel += step;
								rowData++;
							}
						}
						break;
					case k_SHADOW_RUN_ID		: {
							len = (tag & 0x00FF);

							while (len--) {

								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Shadow8888(*d); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Shadow_565(*d); } }
								destPixel += step;
							}
						}
						break;
					case k_FEATHERED_RUN_ID	:
						{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Additive8888(*d, pixelutils_16to8888(*rowData)); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Additive_565(*d, *rowData); } }
						rowData++;
						destPixel += step;
						break;
					default:
						Assert(FALSE);
				}
				tag = *rowData++;
			}
		}
	}


}




void Sprite::DrawFlashLowReversed565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
							 uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8		*surfBase;

	RECT destRect = { drawX, drawY, drawX + width, drawY + height };

	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);

	if (destRect.left < 0) return;
	if (destRect.top < 0) return;
	if (destRect.right > surfWidth) return;
	if (destRect.bottom > surfHeight) return;

	uint8			*destPixel;
	Pixel16  *srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

	for(sint32 j=0; j < height; j++) {
		if (table[j] != k_EMPTY_TABLE_ENTRY) {
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + j * surfPitch + (width * step);

			rowData = dataStart + table[j];

			tag = *rowData++;

			tag = tag & 0x0FFF;

			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							destPixel -= (tag & 0x00FF) * step;
						break;
					case k_COPY_RUN_ID			: {
							uint16 len = (tag & 0x00FF);

							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								for (uint16 i=0; i<len; i++) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_Blend_565(*rowData, *d, transparency); } }
									destPixel -= step;
									rowData++;
								}
							} else {
								for (uint16 i=0; i<len; i++) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Additive8888(*d, pixelutils_16to8888(*rowData)); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Additive_565(*rowData, *d); } }
									destPixel -= step;
									rowData++;
								}
							}
						}
						break;
					case k_SHADOW_RUN_ID		: {
							uint16 len = (tag & 0x00FF);

							for (uint16 i=0; i<len; i++) {
								{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_Shadow8888(*d); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Shadow_565(*d); } }
								destPixel -= step;
							}
						}
						break;
					case k_FEATHERED_RUN_ID	:
						if (flags & k_BIT_DRAWFLAGS_OUTLINE) {
							{ pixelutils_StorePixel(destPixel, outlineColor, bpp32); destPixel -= step; }
							rowData++;
						} else {
							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, (uint16)transparency); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Blend_565(*rowData, *d, (uint16)transparency); } }
									destPixel -= step;
									rowData++;
							} else {
								if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
									uint16 alpha = (tag & 0x00FF);

									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, (uint16)alpha>>3); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = pixelutils_Blend_565(*rowData, *d, (uint16)alpha>>3); } }
									destPixel -= step;
									rowData++;
								} else {
									{ pixelutils_StorePixel(destPixel, *rowData, bpp32); destPixel -= step; rowData++; }
								}
							}
						}
						break;
				}
				tag = *rowData++;
			}
		}
	}


}

#define k_MEDIUM_KEY	0x4208




void Sprite::DrawFlashScaledLow565(Pixel16 *data, sint32 x, sint32 y, sint32 destWidth, sint32 destHeight,
							 uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL reverse)
{
	uint8			*surfBase;

	Pixel16		emptyRow[2];

	emptyRow[0] = (Pixel16)((k_CHROMAKEY_RUN_ID << 8) | m_width);
	emptyRow[1] = (Pixel16)(k_EOLN_ID << 8);

	RECT destRect = { x, y, x + destWidth, y + destHeight };

	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;

	surfBase = m_surfBase + (y * surfPitch) + (x * sizeof(Pixel16));

	if (destRect.left < 0) return;
	if (destRect.top < 0) return;
	if (destRect.right > surfWidth) return;
	if (destRect.bottom > surfHeight) return;

	Pixel16			*destPixel;
	Pixel16			*srcPixel = (Pixel16 *)data;

	Pixel16			*table = data+1;
	Pixel16			*dataStart = table + m_height;

	sint32				vaccum;
	sint32				 vincx;
	sint32				 vincxy;
	sint32				vend;
	sint32				vdestpos;
	sint32				 vpos1;
	sint32				 vpos2;

	vaccum = destHeight*2 - m_height;
	vincx = destHeight*2;
	vincxy = (destHeight - m_height) * 2 ;

	vpos1 = 0;
	vpos2 = (sint32)((double)(m_height - destHeight) / (double)destHeight);

	vdestpos = y;
	vend = m_height - 1;

	while ( vpos1 < vend) {
		if (vaccum < 0) {
			vaccum += vincx;
		} else {

			Pixel16		 *rowData1;
			Pixel16		 *rowData2;
			Pixel16		 pixel1;
			Pixel16		 pixel2;
			Pixel16		 pixel3;
			Pixel16		 pixel4;
			Pixel16		pixel;

			sint32		haccum;
			sint32		 hincx;
			sint32		 hincxy;
			sint32		hend;
			sint32		hpos;
			sint32		hdestpos;

			if (table[vpos1] == k_EMPTY_TABLE_ENTRY) rowData1 = emptyRow;
			else rowData1 = dataStart + table[vpos1];
			if (table[vpos2] == k_EMPTY_TABLE_ENTRY) rowData2 = emptyRow;
			else rowData2 = dataStart + table[vpos2];

			haccum = destWidth*2 - m_width;
			hincx = destWidth*2;
			hincxy = (destWidth - m_width) * 2;

			hpos = 0;
			if (reverse) hdestpos = x + destWidth;
			else hdestpos = x;
			hend = m_width-1;




			sint32		mode1;
			sint32		mode2;
			sint32		pos1 = 0;
			sint32		pos2 = 0;
			sint32		 end1;
			sint32		 end2;
			sint32		 alpha1;
			sint32		 alpha2;
			sint32		 oldend1 = 0;
			sint32		 oldend2 = 0;

			Pixel16			 firstPixel;
			Pixel16			 secondPixel;

			end1 = ReadTag(&mode1, &rowData1, &alpha1);
			end2 = ReadTag(&mode2, &rowData2, &alpha2);

			pixel1 = k_MEDIUM_KEY;
			pixel2 = k_MEDIUM_KEY;





			while (hpos < hend) {
				if (haccum < 0) {
					haccum += hincx;
				} else {
					haccum += hincxy;

					destPixel = (Pixel16 *)(surfBase + ((vdestpos-y) * surfPitch) + ((hdestpos-x) * 2));









					while (pos1 <= hpos) {
						switch (mode1) {
							case k_CHROMAKEY_RUN_ID	:
									firstPixel = k_MEDIUM_KEY;
								break;
							case k_COPY_RUN_ID			:
									if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
										firstPixel = pixelutils_Blend_565(*rowData1, *destPixel, transparency);
									} else {
										firstPixel = pixelutils_Additive_565(*rowData1, *destPixel);
									}
									rowData1++;
								break;
							case k_SHADOW_RUN_ID		:
									firstPixel = pixelutils_Shadow_565(*destPixel);
								break;
							case k_FEATHERED_RUN_ID	:
									if (flags & k_BIT_DRAWFLAGS_OUTLINE) {
										firstPixel = outlineColor;
									} else {
										if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
											firstPixel = pixelutils_Blend_565(*rowData1, *destPixel, (uint16)transparency);
										} else {
											if (flags & k_BIT_DRAWFLAGS_FEATHERING) {

												firstPixel = pixelutils_Blend_565(*rowData1, *destPixel, (uint16)alpha1>>3);
											} else {
												firstPixel = *rowData1;
											}
										}
									}
									rowData1++;
								break;
							default:
								Assert(mode1 == k_CHROMAKEY_RUN_ID || mode1 == k_COPY_RUN_ID || mode1 == k_SHADOW_RUN_ID || mode1 == k_FEATHERED_RUN_ID);
						}

						pos1++;

						if (pos1 >= end1) {
							oldend1 = end1;
							end1 = oldend1 + ReadTag(&mode1, &rowData1, &alpha1);
						}
					}

					while (pos2 <= hpos) {

						switch (mode2) {
							case k_CHROMAKEY_RUN_ID	:
									secondPixel = k_MEDIUM_KEY;
								break;
							case k_COPY_RUN_ID			:
									if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
										secondPixel = pixelutils_Blend_565(*rowData2, *destPixel, transparency);
									} else {
										secondPixel = pixelutils_Additive_565(*rowData2, *destPixel);
									}
									rowData2++;
								break;
							case k_SHADOW_RUN_ID		:
									secondPixel = pixelutils_Shadow_565(*destPixel);
								break;
							case k_FEATHERED_RUN_ID	:
									if (flags & k_BIT_DRAWFLAGS_OUTLINE) {
										secondPixel = outlineColor;
									} else {
										if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
											secondPixel = pixelutils_Blend_565(*rowData2, *destPixel, (uint16)transparency);
										} else {
											if (flags & k_BIT_DRAWFLAGS_FEATHERING) {
												secondPixel = pixelutils_Blend_565(*rowData2, *destPixel, (uint16)alpha2>>3);

											} else {
												secondPixel = *rowData2;
											}
										}
									}
									rowData2++;
								break;
							default :
								Assert(mode2 == k_CHROMAKEY_RUN_ID || mode2 == k_COPY_RUN_ID || mode2 == k_SHADOW_RUN_ID || mode2 == k_FEATHERED_RUN_ID);
						}

						pos2++;

						if (pos2 >= end2) {
							oldend2 = end2;
							end2 = oldend2 + ReadTag(&mode2, &rowData2, &alpha2);
						}
					}

					pixel3 = firstPixel;
					pixel4 = secondPixel;






					if (flags & k_BIT_DRAWFLAGS_AA) {
						if (flags & k_BIT_DRAWFLAGS_AA_4_WAY) {
							if (pixel1 != k_MEDIUM_KEY || pixel2 != k_MEDIUM_KEY || pixel3 != k_MEDIUM_KEY || pixel4 != k_MEDIUM_KEY) {
								pixel =	average(pixel1, pixel2, pixel3, pixel4);

								*destPixel = pixel;
							}
						} else {
							if (pixel1 != k_MEDIUM_KEY || pixel2 != k_MEDIUM_KEY) {
								pixel = average(pixel2, pixel3);

								*destPixel = pixel;
							}
						}
					} else {

						if (pixel3 != k_MEDIUM_KEY)
							*destPixel = pixel3;
					}
					pixel1 = pixel3;
					pixel2 = pixel4;

					if (reverse) hdestpos--;
					else hdestpos++;
				}
				hpos++;
			}

			vaccum += vincxy;
			vdestpos++;
		}
		vpos1++;
		vpos2++;
	}




}

#define k_REFLECTION_BLEND_LEVEL	4

void Sprite::DrawReflectionLow565(Pixel16 *frame, sint32 drawX, sint32 drawY, sint32 width, sint32 height,
					 uint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	uint8			*surfBase;

	RECT destRect = { drawX, drawY, drawX + width, drawY + height };

	sint32 surfWidth = m_surfWidth;
	sint32 surfHeight = m_surfHeight;
	sint32 surfPitch = m_surfPitch;
	bool const	bpp32 = m_surface && m_surface->BitsPerPixel() == 32;
	sint32 const	step  = bpp32 ? 4 : 2;

	surfBase = m_surfBase + (drawY * surfPitch) + (drawX * step);

	if (destRect.left < 0) return;
	if (destRect.top < 0) return;
	if (destRect.right > surfWidth) return;
	if (destRect.bottom > surfHeight) return;

	uint8			*destPixel;
	Pixel16  *srcPixel = (Pixel16 *)frame;

	Pixel16		*table = frame+1;
	Pixel16		*dataStart = table + height;

	for(sint32 j=0; j<height; j++) {
		if (table[j] != k_EMPTY_TABLE_ENTRY) {
			Pixel16		*rowData;
			Pixel16		tag;

			destPixel = surfBase + (height + (height-j)/2)* surfPitch;

			rowData = dataStart + table[j];

			tag = *rowData++;

			tag = tag & 0x0FFF;

			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_CHROMAKEY_RUN_ID	:
							destPixel += (tag & 0x00FF) * step;

						break;
					case k_COPY_RUN_ID			: {
							uint16 len = (tag & 0x00FF);

							if (flags & k_BIT_DRAWFLAGS_TRANSPARENCY) {
								for (uint16 i=0; i<len; i++) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, k_REFLECTION_BLEND_LEVEL); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, k_REFLECTION_BLEND_LEVEL); } }
									destPixel += step;
									rowData++;
								}
							} else {
								for (uint16 i=0; i<len; i++) {
									{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, k_REFLECTION_BLEND_LEVEL); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, k_REFLECTION_BLEND_LEVEL); } }

									destPixel += step;
									rowData++;
								}
							}
						}
						break;
					case k_SHADOW_RUN_ID		: {
							uint16 len = (tag & 0x00FF);
							destPixel += len * step;
						}
						break;
					case k_FEATHERED_RUN_ID	:
							{ if (bpp32) { Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel); *d = pixelutils_BlendFast8888(pixelutils_16to8888(*rowData), *d, k_REFLECTION_BLEND_LEVEL); } else { Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel); *d = (Pixel16)pixelutils_BlendFast_565(*rowData, *d, k_REFLECTION_BLEND_LEVEL); } }
							destPixel += step;
							rowData++;
						break;
					default:

						Assert(FALSE);
                        break;
				}
				tag = *rowData++;
			}
		}
	}
}




inline Pixel16 Sprite::average(Pixel16 pixel1, Pixel16 pixel2, Pixel16 pixel3, Pixel16 pixel4)
{
	uint16		 r1;
	uint16		 g1;
	uint16		 b1;
	uint16		 r2;
	uint16		 g2;
	uint16		 b2;
	uint16		 r3;
	uint16		 g3;
	uint16		 b3;
	uint16		 r4;
	uint16		 g4;
	uint16		 b4;
	uint16		 r0;
	uint16		 g0;
	uint16		 b0;

	if (is_565_Get()) {
		r1 = (pixel1 & 0xF800) >> 11;
		g1 = (pixel1 & 0x07E0) >> 5;
		b1 = (pixel1 & 0x001F);

		r2 = (pixel2 & 0xF800) >> 11;
		g2 = (pixel2 & 0x07E0) >> 5;
		b2 = (pixel2 & 0x001F);

		r3 = (pixel3 & 0xF800) >> 11;
		g3 = (pixel3 & 0x07E0) >> 5;
		b3 = (pixel3 & 0x001F);

		r4 = (pixel4 & 0xF800) >> 11;
		g4 = (pixel4 & 0x07E0) >> 5;
		b4 = (pixel4 & 0x001F);

		r0 = (r1 + r2 + r3 + r4) >> 2;
		g0 = (g1 + g2 + g3 + g4) >> 2;
		b0 = (b1 + b2 + b3 + b4) >> 2;

		return (r0 << 11) | (g0 << 5) | b0;
	} else {
		r1 = (pixel1 & 0x7C00) >> 10;
		g1 = (pixel1 & 0x03E0) >> 5;
		b1 = (pixel1 & 0x001F);

		r2 = (pixel2 & 0x7C00) >> 10;
		g2 = (pixel2 & 0x03E0) >> 5;
		b2 = (pixel2 & 0x001F);

		r3 = (pixel3 & 0x7C00) >> 10;
		g3 = (pixel3 & 0x03E0) >> 5;
		b3 = (pixel3 & 0x001F);

		r4 = (pixel4 & 0x7C00) >> 10;
		g4 = (pixel4 & 0x03E0) >> 5;
		b4 = (pixel4 & 0x001F);

		r0 = (r1 + r2 + r3 + r4) >> 2;
		g0 = (g1 + g2 + g3 + g4) >> 2;
		b0 = (b1 + b2 + b3 + b4) >> 2;

		return (r0 << 10) | (g0 << 5) | b0;
	}
}




inline Pixel16 Sprite::average(Pixel16 pixel1, Pixel16 pixel2)
{
	uint16		 r1;
	uint16		 g1;
	uint16		 b1;
	uint16		 r2;
	uint16		 g2;
	uint16		 b2;
	uint16		 r0;
	uint16		 g0;
	uint16		 b0;

	if (is_565_Get()) {
		r1 = (pixel1 & 0xF800) >> 11;
		g1 = (pixel1 & 0x07E0) >> 5;
		b1 = (pixel1 & 0x001F);

		r2 = (pixel2 & 0xF800) >> 11;
		g2 = (pixel2 & 0x07E0) >> 5;
		b2 = (pixel2 & 0x001F);

		r0 = (r1 + r2) >> 1;
		g0 = (g1 + g2) >> 1;
		b0 = (b1 + b2) >> 1;

		return (r0 << 11) | (g0 << 5) | b0;
	} else {
		r1 = (pixel1 & 0x7C00) >> 10;
		g1 = (pixel1 & 0x03E0) >> 5;
		b1 = (pixel1 & 0x001F);

		r2 = (pixel2 & 0x7C00) >> 10;
		g2 = (pixel2 & 0x03E0) >> 5;
		b2 = (pixel2 & 0x001F);

		r0 = (r1 + r2) >> 1;
		g0 = (g1 + g2) >> 1;
		b0 = (b1 + b2) >> 1;

		return (r0 << 10) | (g0 << 5) | b0;
	}
}
