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

#ifndef DIRECTOR_CASTMEMBER_PALETTE_H
#define DIRECTOR_CASTMEMBER_PALETTE_H

#include "director/castmember/castmember.h"

namespace Director {

class PaletteCastMember : public CastMember {
public:
	PaletteCastMember(Cast *cast, uint16 castId, Common::SeekableReadStreamEndian &stream, uint16 version);
	PaletteCastMember(Cast *cast, uint16 castId, PaletteCastMember &source);
	PaletteCastMember(Cast *cast, uint16 castId, byte *paletteData, PaletteV4 *palette);
	~PaletteCastMember();

	CastMember *duplicate(Cast *cast, uint16 castId) override;

	CastMemberID getPaletteId();
	void activatePalette();

	Common::String formatInfo() override;

	void load() override;
	void unload() override;

	uint32 writeSTXTResource(Common::MemoryWriteStream *writeStream, uint32 offset);
	uint32 getCastDataSize() override;			// This is the size of the data in the 'CASt' resource
	void writeCastData(Common::MemoryWriteStream *writeStream) override;

	PaletteV4 *_palette;
};

} // End of namespace Director

#endif
