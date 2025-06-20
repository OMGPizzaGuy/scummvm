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

#ifndef ZVISION_DETECTION_H
#define ZVISION_DETECTION_H

#include "engines/advancedDetector.h"

namespace ZVision {

enum ZVisionDebugChannels {
	kDebugScript = 1,
	kDebugLoop,
	kDebugPuzzle,
	kDebugAction,
	kDebugControl,
	kDebugEffect,
	kDebugGraphics,
	kDebugVideo,
	kDebugSound,
	kDebugSubtitle,
	kDebugFile,
	kDebugMouse,
	kDebugAssign,
	kDebugEvent
};

enum ZVisionGameId {
	GID_NONE = 0,
	GID_NEMESIS = 1,
	GID_GRANDINQUISITOR = 2
};

struct ZVisionGameDescription {
	AD_GAME_DESCRIPTION_HELPERS(desc);

	ADGameDescription desc;
	ZVisionGameId gameId;
};

#define GAMEOPTION_ORIGINAL_SAVELOAD          GUIO_GAMEOPTIONS1
#define GAMEOPTION_DOUBLE_FPS                 GUIO_GAMEOPTIONS2
#define GAMEOPTION_ENABLE_VENUS               GUIO_GAMEOPTIONS3
#define GAMEOPTION_DISABLE_ANIM_WHILE_TURNING GUIO_GAMEOPTIONS4
#define GAMEOPTION_USE_HIRES_MPEG_MOVIES      GUIO_GAMEOPTIONS5
#define GAMEOPTION_ENABLE_WIDESCREEN          GUIO_GAMEOPTIONS6
#define GAMEOPTION_HQ_PANORAMA                GUIO_GAMEOPTIONS7

} // End of namespace ZVision

#endif // ZVISION_DETECTION_H
