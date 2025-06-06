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

#ifndef IMAGE_PNG_H
#define IMAGE_PNG_H

#include "common/scummsys.h"
#include "common/textconsole.h"
#include "graphics/palette.h"
#include "graphics/pixelformat.h"
#include "image/image_decoder.h"

namespace Common {
class SeekableReadStream;
class WriteStream;
}

namespace Graphics {
struct Surface;
}

namespace Image {

/**
 * @defgroup image_png PNG decoder
 * @ingroup image
 *
 * @brief Decoder for PNG images.
 *
 * This decoder has a dependency on the libpng library.
 *
 * Used in engines:
 * - Sword25
 * - TwinE
 * - Wintermute
 * @{
 */

class PNGDecoder : public ImageDecoder {
public:
	PNGDecoder();
	~PNGDecoder();

	bool loadStream(Common::SeekableReadStream &stream) override;
	void destroy() override;
	const Graphics::Surface *getSurface() const override { return _outputSurface; }
	const Graphics::Palette &getPalette() const override { return _palette; }
	bool hasTransparentColor() const override { return _hasTransparentColor; }
	uint32 getTransparentColor() const override { return _transparentColor; }
	void setSkipSignature(bool skip) { _skipSignature = skip; }
	void setKeepTransparencyPaletted(bool keep) { _keepTransparencyPaletted = keep; }
private:
	Graphics::PixelFormat getByteOrderRgbaPixelFormat(bool isAlpha) const;

	Graphics::Palette _palette;

	// flag to skip the png signature check for headless png files
	bool _skipSignature;

	// Flag to keep paletted images paletted, even when the image has transparency
	bool _keepTransparencyPaletted;
	bool _hasTransparentColor;
	uint32 _transparentColor;

	Graphics::Surface *_outputSurface;
};

/**
 * Outputs a compressed PNG stream of the given input surface.
  *
 *  @param out  Stream to which to write the PNG image.
 *  @param input The surface to save as a PNG image..
 *  @param palette    The palette (in RGB888), if the source format has a bpp of 1.
 */
bool writePNG(Common::WriteStream &out, const Graphics::Surface &input, const byte *palette = nullptr);
/** @} */
} // End of namespace Image

#endif
