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

#ifndef CHAMBER_H
#define CHAMBER_H
#define RUNCOMMAND_RESTART 1337

#include "common/random.h"
#include "common/serializer.h"
#include "common/rendermode.h"
#include "engines/engine.h"
#include "gui/debugger.h"

namespace Audio {
class SoundHandle;
class PCSpeaker;
}

struct ADGameDescription;

namespace Chamber {

class ChamberEngine : public Engine {
private:
	// We need random numbers
	Common::RandomSource *_rnd;

public:
	ChamberEngine(OSystem *syst, const ADGameDescription *desc);
	~ChamberEngine();

	Common::Language getLanguage() const;

	Common::Error run() override;
	Common::Error init();
	Common::Error execute();
	bool hasFeature(EngineFeature f) const override;
	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override { return true; }
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override { return true; }
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override;
	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override;
	void syncGameStream(Common::Serializer &s);

	byte readKeyboardChar();

	void initSound();
	void deinitSound();

public:
	bool _shouldQuit;
	bool _shouldRestart;
	bool _prioritycommand_1;
	bool _prioritycommand_2;

	Common::RenderMode _videoMode;

	byte *_pxiData;

	uint16 _screenW; ///< Screen Width
	uint16 _screenH; ///< Screen Height
	uint8 _screenBits; ///< Bits per pixel
	uint16 _line_offset; ///< Memory offset of blanks
	uint16 _line_offset2; ///< Memory offset of blanks
	uint8 _screenPPB; ///< Pixels per byte
	uint16 _screenBPL; ///< Bytes per line
	uint8 _fontHeight; ///< Font height
	uint8 _fontWidth; ///< Font height


	Audio::PCSpeaker *_speaker;

private:
	const ADGameDescription *_gameDescription;
};

void init(void);

extern ChamberEngine *g_vm;

} // End of namespace Chamber

#endif
