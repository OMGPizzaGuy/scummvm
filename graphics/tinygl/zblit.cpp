/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/array.h"

#include "graphics/tinygl/zblit.h"
#include "graphics/tinygl/zgl.h"
#include "graphics/tinygl/pixelbuffer.h"
#include "graphics/tinygl/zdirtyrect.h"
#include "graphics/tinygl/gl.h"

#include "graphics/blit.h"

#include <math.h>

namespace TinyGL {

Common::Point transformPoint(float x, float y, int rotation);
Common::Rect rotateRectangle(int x, int y, int width, int height, int rotation, int originX, int originY);

struct BlitImage {
public:
	BlitImage() : _isDisposed(false), _version(0), _binaryTransparent(false), _opaque(true), _zBuffer(false), _refcount(1) { }

	void loadData(const Graphics::Surface &surface, uint32 colorKey, bool applyColorKey, bool zBuffer) {
		_lines.clear();

		_zBuffer = zBuffer;
		if (_zBuffer) {
			_surface.copyFrom(surface);
			return;
		}

		int size = surface.w * surface.h;
		Graphics::PixelBuffer buffer(surface.format, (byte *)const_cast<void *>(surface.getPixels()));

		_opaque = true;
		if (surface.format.aBits() > 0) {
			for (int x = 0; x < size; x++) {
				uint8 r, g, b, a;
				buffer.getARGBAt(x, a, r, g, b);
				if (a != 0xFF) {
					_opaque = false;
				}
			}
		}

		if (_opaque && !applyColorKey) {
			GLContext *c = gl_get_context();
			_surface.convertFrom(surface, c->fb->getPixelFormat());

			_binaryTransparent = false;
		} else {
			const Graphics::PixelFormat textureFormat = Graphics::PixelFormat::createFormatRGBA32();
			_surface.convertFrom(surface, textureFormat);

			Graphics::PixelBuffer dataBuffer(textureFormat, (byte *)const_cast<void *>(_surface.getPixels()));

			if (applyColorKey) {
				for (int x = 0; x < size; x++) {
					if (buffer.getValueAt(x) == colorKey) {
						// Color keyed pixels become transparent white.
						dataBuffer.setPixelAt(x, 0, 255, 255, 255);
						_opaque = false;
					}
				}
			}

			// Create opaque lines data.
			// A line of pixels can not wrap more that one line of the image, since it would break
			// blitting of bitmaps with a non-zero x position.
			Graphics::PixelBuffer srcBuf = dataBuffer;
			_binaryTransparent = true;
			for (int y = 0; y < surface.h; y++) {
				int start = -1;
				for (int x = 0; x < surface.w; ++x) {
					// We found a transparent pixel, so save a line from 'start' to the pixel before this.
					uint8 r, g, b, a;
					srcBuf.getARGBAt(x, a, r, g, b);
					if (a != 0 && a != 0xFF) {
						_binaryTransparent = false;
					}
					if (a == 0 && start >= 0) {
						_lines.push_back(Line(start, y, x - start, srcBuf.getRawBuffer(start), textureFormat));
						start = -1;
					} else if (a != 0 && start == -1) {
						start = x;
					}
				}
				// end of the bitmap line. if start is an actual pixel save the line.
				if (start >= 0) {
					_lines.push_back(Line(start, y, surface.w - start, srcBuf.getRawBuffer(start), textureFormat));
				}
				srcBuf.shiftBy(surface.w);
			}
		}

		_version++;
	}

	int getVersion() const {
		return _version;
	}

	~BlitImage() {
		_surface.free();
	}

	struct Line {
		int _x;
		int _y;
		int _bpp;
		int _length;
		byte *_pixels;

		Line() : _x(0), _y(0), _length(0), _pixels(nullptr) { }
		Line(int x, int y, int length, byte *pixels, const Graphics::PixelFormat &textureFormat) :
				_x(x), _y(y), _bpp(gl_get_context()->fb->getPixelBufferBpp()), _length(length) {
			_pixels = (byte *)gl_zalloc(_length * _bpp);
			Graphics::crossBlit(_pixels, pixels, _length * _bpp, _length * _bpp, _length, 1,
			                    gl_get_context()->fb->getPixelFormat(), textureFormat);
		}

		Line &operator=(const Line &other) {
			if (this == &other)
				return *this;
			_x = other._x;
			_y = other._y;
			if (_length != other._length || _bpp != other._bpp) {
				_pixels = (byte *)gl_realloc(_pixels, other._length * other._bpp);
				_length = other._length;
				_bpp = other._bpp;
			}
			memcpy(_pixels, other._pixels, _length * _bpp);
			return *this;
		}

		Line(const Line& other) : _x(other._x), _y(other._y), _bpp(other._bpp), _length(other._length) {
			_pixels = (byte *)gl_zalloc(_length * _bpp);
			memcpy(_pixels, other._pixels, _length * _bpp);
		}

		~Line() {
			gl_free(_pixels);
		}
	};

	bool clipBlitImage(TinyGL::GLContext *c, int &srcX, int &srcY, int &srcWidth, int &srcHeight, int &width, int &height, int &dstX, int &dstY, int &clampWidth, int &clampHeight) {
		if (srcWidth == 0 || srcHeight == 0) {
			srcWidth = _surface.w;
			srcHeight = _surface.h;
		}

		if (width == 0 && height == 0) {
			width = srcWidth;
			height = srcHeight;
		}

		const Common::Rect &clippingRect = c->fb->getClippingRectangle();

		if (dstX >= clippingRect.right || dstY >= clippingRect.bottom)
			return false;

		if (dstX + width < clippingRect.left || dstY + height < clippingRect.top) {
			return false;
		}

		if (dstX < clippingRect.left) {
			srcX += (clippingRect.left - dstX);
			width -= (clippingRect.left - dstX);
			dstX = clippingRect.left;
		}

		if (dstY < clippingRect.top) {
			srcY += (clippingRect.top - dstY);
			height -= (clippingRect.top - dstY);
			dstY = clippingRect.top;
		}

		if (width < 0 || height < 0) {
			return false;
		}

		if (dstX + width >= clippingRect.right) {
			clampWidth = clippingRect.right - dstX;
		} else {
			clampWidth = width;
		}

		if (dstY + height >= clippingRect.bottom) {
			clampHeight = clippingRect.bottom - dstY;
		} else {
			clampHeight = height;
		}

		return true;
	}

	// Blits an image to the z buffer.
	// The function only supports clipped blitting without any type of transformation or tinting.
	void tglBlitZBuffer(int dstX, int dstY) {
		TinyGL::GLContext *c = TinyGL::gl_get_context();
		assert(_zBuffer);

		int clampWidth, clampHeight;
		int width = _surface.w, height = _surface.h;
		int srcWidth = 0, srcHeight = 0;
		int srcX = 0, srcY = 0;
		if (clipBlitImage(c, srcX, srcY, srcWidth, srcHeight, width, height, dstX, dstY, clampWidth, clampHeight) == false)
			return;

		int fbWidth = c->fb->getPixelBufferWidth();

		Graphics::PixelBuffer srcBuf(_surface.format, (byte *)const_cast<void *>(_surface.getPixels())); // Blit image buffer
		Graphics::PixelBuffer dstBuf(_surface.format, (byte *)const_cast<uint *>(c->fb->getZBuffer())); // TinyGL z buffer

		srcBuf.shiftBy(srcY * _surface.w);

		dstBuf.shiftBy(dstY * fbWidth);
		for (int y = 0; y < clampHeight; y++) {
			dstBuf.copyBuffer(dstX, srcX, clampWidth, srcBuf);
			dstBuf.shiftBy(fbWidth);
			srcBuf.shiftBy(_surface.w);
		}
	}

	void tglBlitOpaque(int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight);

	template <bool kDisableColoring, bool kDisableBlending, bool kEnableAlphaBlending>
	void tglBlitRLE(int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight, float aTint, float rTint, float gTint, float bTint);

	template <bool kDisableBlending, bool kDisableColoring, bool kFlipVertical, bool kFlipHorizontal>
	void tglBlitSimple(int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight, float aTint, float rTint, float gTint, float bTint);

	template <bool kDisableBlending, bool kDisableColoring, bool kFlipVertical, bool kFlipHorizontal>
	void tglBlitScale(int dstX, int dstY, int width, int height, int srcX, int srcY, int srcWidth, int srcHeight, float aTint, float rTint, float gTint, float bTint);

	template <bool kDisableBlending, bool kDisableColoring, bool kFlipVertical, bool kFlipHorizontal>
	void tglBlitRotoScale(int dstX, int dstY, int width, int height, int srcX, int srcY, int srcWidth, int srcHeight, int rotation,
	                      int originX, int originY, float aTint, float rTint, float gTint, float bTint);

	//Utility function that calls the correct blitting function.
	template <bool kDisableBlending, bool kDisableColoring, bool kDisableTransform, bool kFlipVertical, bool kFlipHorizontal, bool kEnableAlphaBlending, bool kEnableOpaqueBlit>
	void tglBlitGeneric(const BlitTransform &transform) {
		assert(!_zBuffer);

		if (kDisableTransform) {
			if (kEnableOpaqueBlit && kDisableColoring && kFlipVertical == false && kFlipHorizontal == false) {
				tglBlitOpaque(transform._destinationRectangle.left, transform._destinationRectangle.top,
					transform._sourceRectangle.left, transform._sourceRectangle.top,
					transform._sourceRectangle.width() , transform._sourceRectangle.height());
			} else if ((kDisableBlending || kEnableAlphaBlending) && kFlipVertical == false && kFlipHorizontal == false) {
				tglBlitRLE<kDisableColoring, kDisableBlending, kEnableAlphaBlending>(transform._destinationRectangle.left,
					transform._destinationRectangle.top, transform._sourceRectangle.left, transform._sourceRectangle.top,
					transform._sourceRectangle.width() , transform._sourceRectangle.height(), transform._aTint,
					transform._rTint, transform._gTint, transform._bTint);
			} else {
				tglBlitSimple<kDisableBlending, kDisableColoring, kFlipVertical, kFlipHorizontal>(transform._destinationRectangle.left,
					transform._destinationRectangle.top, transform._sourceRectangle.left, transform._sourceRectangle.top,
					transform._sourceRectangle.width() , transform._sourceRectangle.height(),
					transform._aTint, transform._rTint, transform._gTint, transform._bTint);
			}
		} else {
			if (transform._rotation == 0) {
				tglBlitScale<kDisableBlending, kDisableColoring, kFlipVertical, kFlipHorizontal>(transform._destinationRectangle.left,
					transform._destinationRectangle.top, transform._destinationRectangle.width(), transform._destinationRectangle.height(),
					transform._sourceRectangle.left, transform._sourceRectangle.top, transform._sourceRectangle.width(), transform._sourceRectangle.height(),
					transform._aTint, transform._rTint, transform._gTint, transform._bTint);
			} else {
				tglBlitRotoScale<kDisableBlending, kDisableColoring, kFlipVertical, kFlipHorizontal>(transform._destinationRectangle.left,
					transform._destinationRectangle.top, transform._destinationRectangle.width(), transform._destinationRectangle.height(),
					transform._sourceRectangle.left, transform._sourceRectangle.top, transform._sourceRectangle.width(),
					transform._sourceRectangle.height(), transform._rotation, transform._originX, transform._originY, transform._aTint,
					transform._rTint, transform._gTint, transform._bTint);
			}
		}
	}

	int getWidth() const { return _surface.w; }
	int getHeight() const { return _surface.h; }
	void incRefCount() { _refcount++; }
	void dispose() { if (--_refcount == 0) _isDisposed = true; }
	bool isDisposed() const { return _isDisposed; }
	bool isOpaque() const { return _opaque; }
private:
	bool _isDisposed;
	bool _binaryTransparent;
	bool _opaque;
	bool _zBuffer;
	Common::Array<Line> _lines;
	Graphics::Surface _surface;
	int _version;
	int _refcount;
};

} // end of namespace TinyGL


void tglGetBlitImageSize(TinyGL::BlitImage *blitImage, int &width, int &height) {
	width = blitImage->getWidth();
	height = blitImage->getHeight();
}

void tglIncBlitImageRef(TinyGL::BlitImage *blitImage) {
	blitImage->incRefCount();
}

int tglGetBlitImageVersion(TinyGL::BlitImage *blitImage) {
	return blitImage->getVersion();
}

TinyGL::BlitImage *tglGenBlitImage() {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	TinyGL::BlitImage *image = new TinyGL::BlitImage();
	c->_blitImages.push_back(image);
	return image;
}

void tglUploadBlitImage(TinyGL::BlitImage *blitImage, const Graphics::Surface& surface, uint32 colorKey, bool applyColorKey, bool zBuffer) {
	if (blitImage != nullptr) {
		blitImage->loadData(surface, colorKey, applyColorKey, zBuffer);
	}
}

void tglDeleteBlitImage(TinyGL::BlitImage *blitImage) {
	if (blitImage != nullptr) {
		blitImage->dispose();
	}
}

namespace TinyGL {

void BlitImage::tglBlitOpaque(int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight) {
	GLContext *c = gl_get_context();

	int clampWidth, clampHeight;
	int width = srcWidth, height = srcHeight;
	if (clipBlitImage(c, srcX, srcY, srcWidth, srcHeight, width, height, dstX, dstY, clampWidth, clampHeight) == false)
		return;

	int fbPitch = c->fb->getPixelBufferPitch();
	int fbBpp = c->fb->getPixelBufferBpp();
	byte *fbBuf = c->fb->getPixelBuffer() + (dstX * fbBpp) + (dstY * fbPitch);

	Graphics::crossBlit(fbBuf, (const byte *)_surface.getBasePtr(srcX, srcY),
	                    fbPitch, _surface.pitch, clampWidth, clampHeight,
	                    c->fb->getPixelFormat(), _surface.format);
}

// This function uses RLE encoding to skip transparent bitmap parts
// This blit only supports tinting but it will fall back to simpleBlit
// if flipping is required (or anything more complex than that, including rotationd and scaling).
template <bool kDisableColoring, bool kDisableBlending, bool kEnableAlphaBlending>
void BlitImage::tglBlitRLE(int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight, float aTint, float rTint, float gTint, float bTint) {
	GLContext *c = gl_get_context();

	int clampWidth, clampHeight;
	int width = srcWidth, height = srcHeight;
	if (clipBlitImage(c, srcX, srcY, srcWidth, srcHeight, width, height, dstX, dstY, clampWidth, clampHeight) == false)
		return;

	if (aTint <= 0.0f)
		return;

	int fbWidth = c->fb->getPixelBufferWidth();

	Graphics::PixelBuffer srcBuf(_surface.format, (byte *)_surface.getPixels());
	srcBuf.shiftBy(srcX + (srcY * _surface.w));

	Graphics::PixelBuffer dstBuf(c->fb->getPixelFormat(), c->fb->getPixelBuffer());
	dstBuf.shiftBy(dstY * fbWidth + dstX);

	int kBytesPerPixel = c->fb->getPixelFormat().bytesPerPixel;

	uint32 lineIndex = 0;
	int maxY = srcY + clampHeight;
	int maxX = srcX + clampWidth;
	while (lineIndex < _lines.size() && _lines[lineIndex]._y < srcY) {
		lineIndex++;
	}

	if (_binaryTransparent || (kDisableBlending || !kEnableAlphaBlending)) { // If bitmap is binary transparent or if  we need complex forms of blending (not just alpha) we need to use writePixel, which is slower
		while (lineIndex < _lines.size() && _lines[lineIndex]._y < maxY) {
			const BlitImage::Line &l = _lines[lineIndex];
			if (l._x < maxX && l._x + l._length > srcX) {
				int length = l._length;
				int skipStart = (l._x < srcX) ? (srcX - l._x) : 0;
				length -= skipStart;
				int skipEnd   = (l._x + l._length > maxX) ? (l._x + l._length - maxX) : 0;
				length -= skipEnd;
				int xStart = MAX(l._x - srcX, 0);
				if (kDisableColoring) {
					memcpy(dstBuf.getRawBuffer((l._y - srcY) * fbWidth + xStart),
						l._pixels + skipStart * kBytesPerPixel, length * kBytesPerPixel);
				} else {
					for(int x = xStart; x < xStart + length; x++) {
						byte aDst, rDst, gDst, bDst;
						srcBuf.getARGBAt((l._y - srcY) * _surface.w + x, aDst, rDst, gDst, bDst);
						c->fb->writePixel((dstX + x) + (dstY + (l._y - srcY)) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
					}
				}
			}
			lineIndex++;
		}
	} else { // Otherwise can use setPixel in some cases which speeds up things quite a bit
		while (lineIndex < _lines.size() && _lines[lineIndex]._y < maxY) {
			const BlitImage::Line &l = _lines[lineIndex];
			if (l._x < maxX && l._x + l._length > srcX) {
				int length = l._length;
				int skipStart = (l._x < srcX) ? (srcX - l._x) : 0;
				length -= skipStart;
				int skipEnd   = (l._x + l._length > maxX) ? (l._x + l._length - maxX) : 0;
				length -= skipEnd;
				int xStart = MAX(l._x - srcX, 0);
				if (kDisableColoring && (kEnableAlphaBlending == false || kDisableBlending)) {
					memcpy(dstBuf.getRawBuffer((l._y - srcY) * fbWidth + xStart),
						l._pixels + skipStart * kBytesPerPixel, length * kBytesPerPixel);
				} else {
					for(int x = xStart; x < xStart + length; x++) {
						byte aDst, rDst, gDst, bDst;
						srcBuf.getARGBAt((l._y - srcY) * _surface.w + x, aDst, rDst, gDst, bDst);
						if (kDisableColoring) {
							if (aDst != 0xFF) {
								c->fb->writePixel((dstX + x) + (dstY + (l._y - srcY)) * fbWidth, aDst, rDst, gDst, bDst);
							} else {
								dstBuf.setPixelAt(x + (l._y - srcY) * fbWidth, aDst, rDst, gDst, bDst);
							}
						} else {
							c->fb->writePixel((dstX + x) + (dstY + (l._y - srcY)) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
						}
					}
				}
			}
			lineIndex++;
		}
	}
}

// This blit function is called when flipping is needed but transformation isn't.
template <bool kDisableBlending, bool kDisableColoring, bool kFlipVertical, bool kFlipHorizontal>
void BlitImage::tglBlitSimple(int dstX, int dstY, int srcX, int srcY, int srcWidth, int srcHeight, float aTint, float rTint, float gTint, float bTint) {
	GLContext *c = gl_get_context();

	int clampWidth, clampHeight;
	int width = srcWidth, height = srcHeight;
	if (clipBlitImage(c, srcX, srcY, srcWidth, srcHeight, width, height, dstX, dstY, clampWidth, clampHeight) == false)
		return;

	Graphics::PixelBuffer srcBuf(_surface.format, (byte *)_surface.getPixels());

	if (kFlipVertical) {
		srcBuf.shiftBy(((srcHeight - srcY - 1) * _surface.w));
	} else {
		srcBuf.shiftBy((srcY * _surface.w));
	}

	Graphics::PixelBuffer dstBuf(c->fb->getPixelFormat(), c->fb->getPixelBuffer());
	int fbWidth = c->fb->getPixelBufferWidth();

	for (int y = 0; y < clampHeight; y++) {
		for (int x = 0; x < clampWidth; ++x) {
			byte aDst, rDst, gDst, bDst;
			if (kFlipHorizontal) {
				srcBuf.getARGBAt(srcX + clampWidth - x, aDst, rDst, gDst, bDst);
			} else {
				srcBuf.getARGBAt(srcX + x, aDst, rDst, gDst, bDst);
			}

			// Those branches are needed to favor speed: avoiding writePixel always yield a huge performance boost when blitting images.
			if (kDisableColoring) {
				if (kDisableBlending && aDst != 0) {
					dstBuf.setPixelAt((dstX + x) + (dstY + y) * fbWidth, aDst, rDst, gDst, bDst);
				} else {
					c->fb->writePixel((dstX + x) + (dstY + y) * fbWidth, aDst, rDst, gDst, bDst);
				}
			} else {
				if (kDisableBlending && aDst * aTint != 0) {
					dstBuf.setPixelAt((dstX + x) + (dstY + y) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
				} else {
					c->fb->writePixel((dstX + x) + (dstY + y) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
				}
			}
		}
		if (kFlipVertical) {
			srcBuf.shiftBy(-_surface.w);
		} else {
			srcBuf.shiftBy(_surface.w);
		}
	}
}

// This function is called when scale is needed: it uses a simple nearest
// filter to scale the blit image before copying it to the screen.
template <bool kDisableBlending, bool kDisableColoring, bool kFlipVertical, bool kFlipHorizontal>
void BlitImage::tglBlitScale(int dstX, int dstY, int width, int height, int srcX, int srcY, int srcWidth, int srcHeight,
	                     float aTint, float rTint, float gTint, float bTint) {
	GLContext *c = gl_get_context();

	int clampWidth, clampHeight;
	if (clipBlitImage(c, srcX, srcY, srcWidth, srcHeight, width, height, dstX, dstY, clampWidth, clampHeight) == false)
		return;

	Graphics::PixelBuffer srcBuf(_surface.format, (byte *)_surface.getPixels());
	srcBuf.shiftBy(srcX + (srcY * _surface.w));

	Graphics::PixelBuffer dstBuf(c->fb->getPixelFormat(), c->fb->getPixelBuffer());
	int fbWidth = c->fb->getPixelBufferWidth();

	for (int y = 0; y < clampHeight; y++) {
		for (int x = 0; x < clampWidth; ++x) {
			byte aDst, rDst, gDst, bDst;
			int xSource, ySource;
			if (kFlipVertical) {
				ySource = clampHeight - y - 1;
			} else {
				ySource = y;
			}

			if (kFlipHorizontal) {
				xSource = clampWidth - x - 1;
			} else {
				xSource = x;
			}

			srcBuf.getARGBAt(((ySource * srcHeight) / height) * _surface.w + ((xSource * srcWidth) / width), aDst, rDst, gDst, bDst);

			if (kDisableColoring) {
				if (kDisableBlending && aDst != 0) {
					dstBuf.setPixelAt((dstX + x) + (dstY + y) * fbWidth, aDst, rDst, gDst, bDst);
				} else {
					c->fb->writePixel((dstX + x) + (dstY + y) * fbWidth, aDst, rDst, gDst, bDst);
				}
			} else {
				if (kDisableBlending && aDst * aTint != 0) {
					dstBuf.setPixelAt((dstX + x) + (dstY + y) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
				} else {
					c->fb->writePixel((dstX + x) + (dstY + y) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
				}
			}
		}
	}
}

/*

The below two functions are adapted from SDL_rotozoom.c,
taken from SDL_gfx-2.0.18.

Its copyright notice:

=============================================================================
SDL_rotozoom.c: rotozoomer, zoomer and shrinker for 32bit or 8bit surfaces

Copyright (C) 2001-2012  Andreas Schiffler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.

Andreas Schiffler -- aschiffler at ferzkopp dot net
=============================================================================


The functions have been adapted for different structures and coordinate
systems.

*/

template <bool kDisableBlending, bool kDisableColoring, bool kFlipVertical, bool kFlipHorizontal>
void BlitImage::tglBlitRotoScale(int dstX, int dstY, int width, int height, int srcX, int srcY, int srcWidth, int srcHeight, int rotation,
	                         int originX, int originY, float aTint, float rTint, float gTint, float bTint) {
	GLContext *c = gl_get_context();

	int clampWidth, clampHeight;
	if (clipBlitImage(c, srcX, srcY, srcWidth, srcHeight, width, height, dstX, dstY, clampWidth, clampHeight) == false)
		return;

	Graphics::PixelBuffer srcBuf(_surface.format, (byte *)_surface.getPixels());
	srcBuf.shiftBy(srcX + (srcY * _surface.w));
	int fbWidth = c->fb->getPixelBufferWidth();

	Graphics::PixelBuffer dstBuf(c->fb->getPixelFormat(), c->fb->getPixelBuffer());

	// Transform destination rectangle accordingly.
	Common::Rect destinationRectangle = rotateRectangle(dstX, dstY, width, height, rotation, originX, originY);

	if (dstX + destinationRectangle.width() > fbWidth)
		clampWidth = fbWidth - dstX;
	else
		clampWidth = destinationRectangle.width();
	if (dstY + destinationRectangle.height() > c->fb->getPixelBufferHeight())
		clampHeight = c->fb->getPixelBufferHeight() - dstY;
	else
		clampHeight = destinationRectangle.height();

	uint32 invAngle = 360 - (rotation % 360);
	float invCos = cos(invAngle * (float)M_PI / 180.0f);
	float invSin = sin(invAngle * (float)M_PI / 180.0f);

	int icosx = (int)(invCos * (65536.0f * srcWidth / width));
	int isinx = (int)(invSin * (65536.0f * srcWidth / width));
	int icosy = (int)(invCos * (65536.0f * srcHeight / height));
	int isiny = (int)(invSin * (65536.0f * srcHeight / height));

	int xd = (srcX + originX) << 16;
	int yd = (srcY + originY) << 16;
	int cx = originX * ((float)width / srcWidth);
	int cy = originY * ((float)height / srcHeight);

	int ax = -icosx * cx;
	int ay = -isiny * cx;
	int sw = width - 1;
	int sh = height - 1;

	for (int y = 0; y < clampHeight; y++) {
		int t = cy - y;
		int sdx = ax + (isinx * t) + xd;
		int sdy = ay - (icosy * t) + yd;
		for (int x = 0; x < clampWidth; ++x) {
			byte aDst, rDst, gDst, bDst;

			int dx = (sdx >> 16);
			int dy = (sdy >> 16);

			if (kFlipHorizontal) {
				dx = sw - dx;
			}

			if (kFlipVertical) {
				dy = sh - dy;
			}

			if ((dx >= 0) && (dy >= 0) && (dx < srcWidth) && (dy < srcHeight)) {
				srcBuf.getARGBAt(dy * _surface.w + dx, aDst, rDst, gDst, bDst);
				if (kDisableColoring) {
					if (kDisableBlending && aDst != 0) {
						dstBuf.setPixelAt((dstX + x) + (dstY + y) * fbWidth, aDst, rDst, gDst, bDst);
					} else {
						c->fb->writePixel((dstX + x) + (dstY + y) * fbWidth, aDst, rDst, gDst, bDst);
					}
				} else {
					if (kDisableBlending && aDst * aTint != 0) {
						dstBuf.setPixelAt((dstX + x) + (dstY + y) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
					} else {
						c->fb->writePixel((dstX + x) + (dstY + y) * fbWidth, aDst * aTint, rDst * rTint, gDst * gTint, bDst * bTint);
					}
				}
			}
			sdx += icosx;
			sdy += isiny;
		}
	}
}

} // end of namespace TinyGL

void tglBlit(TinyGL::BlitImage *blitImage, int x, int y) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	TinyGL::BlitTransform transform(x, y);
	c->issueDrawCall(new TinyGL::BlittingDrawCall(blitImage, transform, TinyGL::BlittingDrawCall::BlitMode_Regular));
}

void tglBlit(TinyGL::BlitImage *blitImage, const TinyGL::BlitTransform &transform) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	c->issueDrawCall(new TinyGL::BlittingDrawCall(blitImage, transform, TinyGL::BlittingDrawCall::BlitMode_Regular));
}

void tglBlitFast(TinyGL::BlitImage *blitImage, int x, int y) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	TinyGL::BlitTransform transform(x, y);
	c->issueDrawCall(new TinyGL::BlittingDrawCall(blitImage, transform, TinyGL::BlittingDrawCall::BlitMode_Fast));
}

void tglBlitZBuffer(TinyGL::BlitImage *blitImage, int x, int y) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	TinyGL::BlitTransform transform(x, y);
	c->issueDrawCall(new TinyGL::BlittingDrawCall(blitImage, transform, TinyGL::BlittingDrawCall::BlitMode_ZBuffer));
}


namespace TinyGL {
namespace Internal {

template <bool kEnableAlphaBlending, bool kEnableOpaqueBlit, bool kDisableColor, bool kDisableTransform, bool kDisableBlend>
void tglBlit(BlitImage *blitImage, const BlitTransform &transform) {
	if (transform._flipHorizontally) {
		if (transform._flipVertically) {
			blitImage->tglBlitGeneric<kDisableBlend, kDisableColor, kDisableTransform, true, true, kEnableAlphaBlending, kEnableOpaqueBlit>(transform);
		} else {
			blitImage->tglBlitGeneric<kDisableBlend, kDisableColor, kDisableTransform, false, true, kEnableAlphaBlending, kEnableOpaqueBlit>(transform);
		}
	} else if (transform._flipVertically) {
		blitImage->tglBlitGeneric<kDisableBlend, kDisableColor, kDisableTransform, true, false, kEnableAlphaBlending, kEnableOpaqueBlit>(transform);
	} else {
		blitImage->tglBlitGeneric<kDisableBlend, kDisableColor, kDisableTransform, false, false, kEnableAlphaBlending, kEnableOpaqueBlit>(transform);
	}
}

template <bool kEnableAlphaBlending, bool kEnableOpaqueBlit, bool kDisableColor, bool kDisableTransform>
void tglBlit(BlitImage *blitImage, const BlitTransform &transform, bool disableBlend) {
	if (disableBlend) {
		tglBlit<kEnableAlphaBlending, kEnableOpaqueBlit, kDisableColor, kDisableTransform, true>(blitImage, transform);
	} else {
		tglBlit<kEnableAlphaBlending, kEnableOpaqueBlit, kDisableColor, kDisableTransform, false>(blitImage, transform);
	}
}

template <bool kEnableAlphaBlending, bool kEnableOpaqueBlit, bool kDisableColor>
void tglBlit(BlitImage *blitImage, const BlitTransform &transform, bool disableTransform, bool disableBlend) {
	if (disableTransform) {
		tglBlit<kEnableAlphaBlending, kEnableOpaqueBlit, kDisableColor, true>(blitImage, transform, disableBlend);
	} else {
		tglBlit<kEnableAlphaBlending, kEnableOpaqueBlit, kDisableColor, false>(blitImage, transform, disableBlend);
	}
}

template <bool kEnableAlphaBlending, bool kEnableOpaqueBlit>
void tglBlit(BlitImage *blitImage, const BlitTransform &transform, bool disableColor, bool disableTransform, bool disableBlend) {
	if (disableColor) {
		tglBlit<kEnableAlphaBlending, kEnableOpaqueBlit, true>(blitImage, transform, disableTransform, disableBlend);
	} else {
		tglBlit<kEnableAlphaBlending, kEnableOpaqueBlit, false>(blitImage, transform, disableTransform, disableBlend);
	}
}

template <bool kEnableAlphaBlending>
void tglBlit(BlitImage *blitImage, const BlitTransform &transform, bool enableOpaqueBlit, bool disableColor, bool disableTransform, bool disableBlend) {
	if (enableOpaqueBlit) {
		tglBlit<kEnableAlphaBlending, true>(blitImage, transform, disableColor, disableTransform, disableBlend);
	} else {
		tglBlit<kEnableAlphaBlending, false>(blitImage, transform, disableColor, disableTransform, disableBlend);
	}
}

void tglBlit(BlitImage *blitImage, const BlitTransform &transform) {
	GLContext *c = gl_get_context();
	bool disableColor = transform._aTint == 1.0f && transform._bTint == 1.0f && transform._gTint == 1.0f && transform._rTint == 1.0f;
	bool disableTransform = transform._destinationRectangle.width() == 0 && transform._destinationRectangle.height() == 0 && transform._rotation == 0;
	bool disableBlend = c->blending_enabled == false;
	bool enableAlphaBlending = c->source_blending_factor == TGL_SRC_ALPHA && c->destination_blending_factor == TGL_ONE_MINUS_SRC_ALPHA;
	bool enableOpaqueBlit = blitImage->isOpaque()
	                    && (c->source_blending_factor == TGL_ONE || c->source_blending_factor == TGL_SRC_ALPHA)
	                    && (c->destination_blending_factor == TGL_ZERO || c->destination_blending_factor == TGL_ONE_MINUS_SRC_ALPHA);

	if (enableAlphaBlending) {
		tglBlit<true>(blitImage, transform, enableOpaqueBlit, disableColor, disableTransform, disableBlend);
	} else {
		tglBlit<false>(blitImage, transform, enableOpaqueBlit, disableColor, disableTransform, disableBlend);
	}
}

void tglBlitFast(BlitImage *blitImage, int x, int y) {
	BlitTransform transform(x, y);
	if (blitImage->isOpaque()) {
		blitImage->tglBlitGeneric<true, true, true, false, false, false, true>(transform);
	} else {
		blitImage->tglBlitGeneric<true, true, true, false, false, false, false>(transform);
	}
}

void tglBlitZBuffer(BlitImage *blitImage, int x, int y) {
	blitImage->tglBlitZBuffer(x, y);
}

void tglCleanupImages() {
	GLContext *c = gl_get_context();
	Common::List<BlitImage *>::iterator it = c->_blitImages.begin();
	while (it != c->_blitImages.end()) {
		if ((*it)->isDisposed()) {
			delete (*it);
			it = c->_blitImages.erase(it);
		} else {
			++it;
		}
	}
}

} // end of namespace Internal

Common::Point transformPoint(float x, float y, int rotation) {
	float rotateRad = rotation * (float)M_PI / 180.0f;
	Common::Point newPoint;
	newPoint.x = x * cos(rotateRad) - y * sin(rotateRad);
	newPoint.y = x * sin(rotateRad) + y * cos(rotateRad);
	return newPoint;
}

Common::Rect rotateRectangle(int x, int y, int width, int height, int rotation, int originX, int originY) {
	Common::Point nw, ne, sw, se;
	nw = transformPoint(x - originX, y - originY, rotation);
	ne = transformPoint(x + width - originX, y - originY, rotation);
	sw = transformPoint(x + width - originX, y + height - originY, rotation);
	se = transformPoint(x - originX, y + height - originY, rotation);

	float top = MIN(nw.y, MIN(ne.y, MIN(sw.y, se.y)));
	float bottom = MAX(nw.y, MAX(ne.y, MAX(sw.y, se.y)));
	float left = MIN(nw.x, MIN(ne.x, MIN(sw.x, se.x)));
	float right = MAX(nw.x, MAX(ne.x, MAX(sw.x, se.x)));

	Common::Rect res;
	res.top = (int32)(floor(top)) + originY;
	res.bottom = (int32)(ceil(bottom)) + originY;
	res.left = (int32)(floor(left)) + originX;
	res.right = (int32)(ceil(right)) + originX;

	return res;
}

} // end of namespace TinyGL
