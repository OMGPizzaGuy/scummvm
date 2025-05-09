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

#include "backends/platform/3ds/osystem.h"
#include "backends/platform/3ds/sprite.h"
#include "common/algorithm.h"
#include "common/util.h"

namespace N3DS {

Sprite::Sprite()
	: textureTransferFlags(0)
	, dirtyPixels(true)
	, dirtyMatrix(true)
	, actualWidth(0)
	, actualHeight(0)
	, posX(0)
	, posY(0)
	, offsetX(0)
	, offsetY(0)
	, scaleX(1.f)
	, scaleY(1.f)
{
	Mtx_Identity(&modelview);

	vertices = (vertex *)linearAlloc(sizeof(vertex) * 4);
}

Sprite::~Sprite() {
	free();
	linearFree(vertices);
}

void Sprite::create(uint16 width, uint16 height, const GfxMode3DS *mode, bool vram) {
	int16 wPow = MAX<uint16>(Common::nextHigher2(width), 64u);
	int16 hPow = MAX<uint16>(Common::nextHigher2(height), 64u);

	bool pwrW_hChanged = (wPow != w) || (hPow != h);
	bool sfcBppChanged = (mode->surfaceFormat.bytesPerPixel != format.bytesPerPixel);
	bool texFmtChanged = (mode->textureFormat != texture.fmt);

	bool srfDataReinitNeeded = (pwrW_hChanged || sfcBppChanged || !pixels);
	bool textureReinitNeeded = (pwrW_hChanged || texFmtChanged || !texture.data);

	actualWidth = width;
	actualHeight = height;
	format = mode->surfaceFormat;
	textureTransferFlags = mode->textureTransferFlags;
	w = wPow;
	h = hPow;
	pitch = w * format.bytesPerPixel;
	dirtyPixels = true;

	if (width && height) {
		// Don't needlessly reinitialize surface pixels.
		if (srfDataReinitNeeded) {
			linearFree(pixels);
			pixels = linearAlloc(h * pitch);
		}
		// Don't needlessly reinitialize C3D_Tex data.
		if (textureReinitNeeded) {
			C3D_TexDelete(&texture);
			if (vram) {
				if (!C3D_TexInitVRAM(&texture, w, h, mode->textureFormat))
					C3D_TexInit(&texture, w, h, mode->textureFormat);
			} else
				C3D_TexInit(&texture, w, h, mode->textureFormat);
		}
		assert(pixels && texture.data);
		clear();
	}

	float x = 0.f, y = 0.f;
	float u = (float)width/w;
	float v = (float)height/h;
	vertex tmp[4] = {
		{{x,       y,        0.5f}, {0, 0}},
		{{x+width, y,        0.5f}, {u, 0}},
		{{x,       y+height, 0.5f}, {0, v}},
		{{x+width, y+height, 0.5f}, {u, v}},
	};
	memcpy(vertices, tmp, sizeof(vertex) * 4);
}

void Sprite::free() {
	linearFree(pixels);
	C3D_TexDelete(&texture);
	pixels = 0;
	w = h = pitch = 0;
	actualWidth = actualHeight = 0;
	format = Graphics::PixelFormat();
}

void Sprite::convertToInPlace(const Graphics::PixelFormat &dstFormat, const byte *palette) {
	//
}

void Sprite::transfer() {
	if (pixels && dirtyPixels) {
		dirtyPixels = false;
		GSPGPU_FlushDataCache(pixels, w * h * format.bytesPerPixel);
		C3D_SyncDisplayTransfer((u32*)pixels, GX_BUFFER_DIM(w, h), (u32*)texture.data, GX_BUFFER_DIM(w, h), textureTransferFlags);
	}
}

void Sprite::render() {
	C3D_TexBind(0, &texture);

	C3D_BufInfo *bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vertices, sizeof(vertex), 2, 0x10);
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, 0, 4);
}

void Sprite::clear(uint32 color) {
	dirtyPixels = true;
	memset(pixels, color, w * h * format.bytesPerPixel);
}

void Sprite::setScale (float x, float y) {
	if (x != scaleX || y != scaleY) {
		scaleX = x;
		scaleY = y;
		dirtyMatrix = true;
	}
}

void Sprite::setPosition(int x, int y) {
	if (x != posX || y != posY) {
		posX = x;
		posY = y;
		dirtyMatrix = true;
	}
}

void Sprite::setOffset(uint16 x, uint16 y) {
	offsetX = x;
	offsetY = y;
	dirtyMatrix = true;
}

C3D_Mtx* Sprite::getMatrix() {
	if (dirtyMatrix) {
		dirtyMatrix = false;
		Mtx_Identity(&modelview);
		Mtx_Scale(&modelview, scaleX, scaleY, 1.f);
		Mtx_Translate(&modelview, posX - offsetX, posY - offsetY, 0, true);
	}
	return &modelview;
}

void Sprite::setFilteringMode(bool enableLinearFiltering) {
	GPU_TEXTURE_FILTER_PARAM filteringMode = enableLinearFiltering ? GPU_LINEAR : GPU_NEAREST;
	C3D_TexSetFilter(&texture, filteringMode, filteringMode);
}

} // namespace N3DS
