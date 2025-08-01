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

#ifndef GLK_PC_SPEAKER_H
#define GLK_PC_SPEAKER_H

#include "common/scummsys.h"

namespace Audio {
class PCSpeaker;
}

namespace Glk {

class PCSpeaker {
private:
	Audio::PCSpeaker *_speaker;
public:
	PCSpeaker();
	~PCSpeaker();

	void speakerOn(int16 frequency, int32 length = -1);
	void speakerOff();
	void onUpdate(uint32 millis);
};

} // End of namespace Glk

#endif
