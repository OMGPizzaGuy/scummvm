/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * Additional copyright for this file:
 * Copyright (C) 1995-1997 Presto Studios, Inc.
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

#include "common/system.h"
#include "video/qt_decoder.h"
#include "video/theora_decoder.h"

#include "pegasus/cursor.h"
#include "pegasus/energymonitor.h"
#include "pegasus/gamestate.h"
#include "pegasus/interface.h"
#include "pegasus/pegasus.h"
#include "pegasus/ai/ai_area.h"
#include "pegasus/items/biochips/arthurchip.h"
#include "pegasus/items/biochips/biochipitem.h"
#include "pegasus/neighborhood/caldoria/caldoria.h"
#include "pegasus/neighborhood/caldoria/caldoria4dsystem.h"
#include "pegasus/neighborhood/caldoria/caldoriabomb.h"
#include "pegasus/neighborhood/caldoria/caldoriamessages.h"
#include "pegasus/neighborhood/caldoria/caldoriamirror.h"
#include "pegasus/neighborhood/tsa/fulltsa.h"

namespace Pegasus {

enum {
	kVidPhoneAngle = 30,
	kReplicatorAngle = 50,
	kDrawersAngle = -30,
	kCaldoria53Angle = 45,
	kCaldoria55Angle = -45
};

enum {
	kCreateCornbread = 1000,
	kWashingMachineZoomIn = 1001,
	kWashingMachineLoop = 1002,
	kWashingMachineZoomOut = 1003
};

enum {
	kCaldoriaLaundryZoomInHotSpotID = 10000,
	kCaldoriaLaundryZoomOutHotSpotID = 10001,
	kCaldoriaCornbreadHotSpotID = 10002
};

enum {
	kSinclairInterruptionTime1 = 2955,
	kSinclairInterruptionTime2 = 6835,
	kSinclairInterruptionTime3 = 9835,
	kSinclairInterruptionTime4 = 12555
};

enum {
	kPullbackInterruptFilter = kFilterAllInput,
	kRecalibrationInterruptFilter = kFilterAllInput
};

enum {
	kCaldoriaLaundryIntro1In = 0,
	kCaldoriaLaundryIntro1Out = 2645,

	kCaldoriaLaundryIntro2In = 2645,
	kCaldoriaLaundryIntro2Out = 4933,

	kCaldoriaReplicatorIntroIn = 4933,
	kCaldoriaReplicatorIntroOut = 6557,

	kCaldoriaReplicatorWrongChoiceIn = 6557,
	kCaldoriaReplicatorWrongChoiceOut = 8586,

	kCaldoriaReplicatorOJChoiceIn = 8586,
	kCaldoriaReplicatorOJChoiceOut = 11687,

	kCaldoriaMessagesIntroIn = 11687,
	kCaldoriaMessagesIntroOut = 13641,

	kCaldoriaFirstMessageIn = 13641,
	kCaldoriaFirstMessageOut = 14203,

	kCaldoriaSecondMessageIn = 14203,
	kCaldoriaSecondMessageOut = 14750,

	kCaldoriaDoorCloseIn = 14750,
	kCaldoriaDoorCloseOut = 15472,

	kCaldoriaElevatorCloseIn = 15472,
	kCaldoriaElevatorCloseOut = 16336,

	kCaldoriaShowerCloseIn = 16336,
	kCaldoriaShowerCloseOut = 17101,

	kCaldoriaGTDoorCloseIn = 17101,
	kCaldoriaGTDoorCloseOut = 18523,

	kCaldoriaNobodyHomeIn = 18523,
	kCaldoriaNobodyHomeOut = 21469,

	kCaldoriaNoOtherFloorIn = 21469,
	kCaldoriaNoOtherFloorOut = 28013,

	kCaldoriaDrinkOJIn = 33910,
	kCaldoriaDrinkOJOut = 35846,

	kCaldoriaNoOtherDestinationIn = 35846,
	kCaldoriaNoOtherDestinationOut = 37877,

	kCaldoriaUhghIn = 37877,
	kCaldoriaUhghOut = 38025,

	kCaldoriaSinclairShootsOSIn = 38025,
	kCaldoriaSinclairShootsOSOut = 40649,

	kCaldoriaScreamingAfterIn = 40649,
	kCaldoriaScreamingAfterOut = 47661
};

enum {
	k4FloorTime = 0,

	k4To1Start = 40,
	k4To1Stop = 7720,

	k4To5Start = 7720,
	k4To5Stop = 10280,

	k4To2Time = 10280,

	k4To3Time = 10320,

	k1FloorTime = 10360,

	k1To4Start = 10400,
	k1To4Stop = 18080,

	k1To5Start = 18080,
	k1To5Stop = 28320,

	k1To2Time = 28320,

	k1To3Time = 28360,

	k5FloorTime = 28400,

	k5To1Start = 28440,
	k5To1Stop = 38680,

	k5To4Start = 38680,
	k5To4Stop = 41240,

	k5To2Time = 41240,

	k5To3Time = 41280
};

// FuseFunction functions...

const NotificationFlags kSinclairLoopDoneFlag = kLastNeighborhoodNotificationFlag << 1;

SinclairCallBack::SinclairCallBack(Caldoria *caldoria) {
	_caldoria = caldoria;
}

void SinclairCallBack::callBack() {
	_caldoria->checkInterruptSinclair();
}

Caldoria::Caldoria(InputHandler* nextHandler, PegasusEngine *owner)
		: Neighborhood(nextHandler, owner, "Caldoria", kCaldoriaID), _extraMovie(kNoDisplayElement),
		_laundryZoomInSpot(kCaldoriaLaundryZoomInHotSpotID), _laundryZoomOutSpot(kCaldoriaLaundryZoomOutHotSpotID),
		_cornbreadSpot(kCaldoriaCornbreadHotSpotID), _sinclairInterrupt(this), _lookingAtLaundry(false) {
	setIsItemTaken(kKeyCard);
	setIsItemTaken(kOrangeJuiceGlassEmpty);
	GameState.setTakenItemID(kOrangeJuiceGlassFull, GameState.isTakenItemID(kOrangeJuiceGlassEmpty));
	_zoomOutSpot = nullptr;
	_gunSprite = nullptr;
}

Caldoria::~Caldoria() {
	_sinclairInterrupt.releaseCallBack();
	if (_vm->isDVD()) {
		_vm->getAllHotspots().remove(&_laundryZoomInSpot);
		_vm->getAllHotspots().remove(&_laundryZoomOutSpot);
		_vm->getAllHotspots().remove(&_cornbreadSpot);
	}
}

void Caldoria::init() {
	Neighborhood::init();

	if (_vm->isDVD()) {
		_laundryZoomInSpot.setArea(Common::Rect(384, 120, 576, 320));
		_laundryZoomInSpot.setHotspotFlags(kNeighborhoodSpotFlag | kZoomInSpotFlag);
		_vm->getAllHotspots().push_back(&_laundryZoomInSpot);

		_laundryZoomOutSpot.setArea(Common::Rect(64, 64, 576, 320));
		_laundryZoomOutSpot.setHotspotFlags(kNeighborhoodSpotFlag | kZoomOutSpotFlag);
		_vm->getAllHotspots().push_back(&_laundryZoomOutSpot);

		_cornbreadSpot.setArea(Common::Rect(270, 233, 381, 298));
		_cornbreadSpot.setHotspotFlags(kNeighborhoodSpotFlag | kClickSpotFlag);
		_vm->getAllHotspots().push_back(&_cornbreadSpot);
	}

	// We need this notification flag as well.
	_neighborhoodNotification.notifyMe(this, kSinclairLoopDoneFlag, kSinclairLoopDoneFlag);

	_extraMovieCallBack.setNotification(&_neighborhoodNotification);

	_sinclairInterrupt.initCallBack(&_navMovie, kCallBackAtTime);

	forceStridingStop(kCaldoria55, kSouth, kAltCaldoriaSinclairDown);
	forceStridingStop(kCaldoria50, kNorth, kAltCaldoriaSinclairDown);
}

void Caldoria::start() {
	g_energyMonitor->stopEnergyDraining();

	if (!GameState.getCaldoriaSeenPullback()) {
		Input input;
		Common::Path wakeModeMoviePath;
		InputDevice.getInput(input, kPullbackInterruptFilter);
		if (_vm->isDVD() && JMPPPInput::isEasterEggModifierInput(input)) {
			wakeModeMoviePath = "Images/Caldoria/A00WM";
		} else {
			wakeModeMoviePath = "Images/Caldoria/A00WN";
		}

		_vm->_gfx->doFadeOutSync(kOneSecond * kFifteenTicksPerSecond, kFifteenTicksPerSecond);

		Video::VideoDecoder *pullbackMovie = nullptr;
		uint16 pullbackX, pullbackY;

#ifdef USE_THEORADEC
		if (_vm->isDVD()) {
			// Updated larger version
			pullbackMovie = new Video::TheoraDecoder();

			if (!pullbackMovie->loadFile("Images/Caldoria/Pullback.ogg")) {
				delete pullbackMovie;
			pullbackMovie = nullptr;
			}
		}
#endif

		if (!pullbackMovie) {
			pullbackMovie = new Video::QuickTimeDecoder();

			if (!pullbackMovie->loadFile("Images/Caldoria/Pullback.movie"))
				error("Could not load Pullback.movie");
		}

		pullbackMovie->setVolume(MIN<uint>(_vm->getSoundFXLevel(), 0xFF));

		// Draw the first frame so we can fade to it
		const Graphics::Surface *frame = nullptr;

		if (_vm->isDVD()) {
			uint16 newHeight = (uint16)((640.0f / (float)pullbackMovie->getWidth()) * (float)pullbackMovie->getHeight());
			pullbackX = 0;
			pullbackY = (480 - newHeight) / 2;

			_vm->_gfx->enableUpdates();
		} else {
			pullbackX = 80;
			pullbackY = 112;

			// Draw the first frame so we can fade to it
			frame = pullbackMovie->decodeNextFrame();
			assert(frame);
			assert(frame->format == g_system->getScreenFormat());
			g_system->copyRectToScreen((const byte *)frame->getPixels(), frame->pitch, pullbackX, pullbackY, frame->w, frame->h);
			_vm->_gfx->doFadeInSync(kTwoSeconds * kFifteenTicksPerSecond, kFifteenTicksPerSecond);
		}

		bool saveAllowed = _vm->swapSaveAllowed(false);
		bool openAllowed = _vm->swapLoadAllowed(false);

		bool skipped = false;

		pullbackMovie->start();

		while (!_vm->shouldQuit() && !pullbackMovie->endOfVideo()) {
			if (pullbackMovie->needsUpdate()) {
				frame = pullbackMovie->decodeNextFrame();

				if (frame) {
					g_system->copyRectToScreen((const byte *)frame->getPixels(), frame->pitch, pullbackX, pullbackY, frame->w, frame->h);
					g_system->updateScreen();
				}
			}

			InputDevice.getInput(input, kPullbackInterruptFilter);
			if ((input.anyInput() || _vm->saveRequested() || _vm->loadRequested()) && !GameState.getEasterEgg()) {
				skipped = true;
				break;
			}

			g_system->delayMillis(10);
		}

		delete pullbackMovie;

		if (_vm->shouldQuit())
			return;

		_vm->swapSaveAllowed(saveAllowed);
		_vm->swapLoadAllowed(openAllowed);

		ExtraTable::Entry entry;

		if (!skipped) {
			if (_vm->isDVD()) {
				Video::VideoDecoder* wakeModeMovie = nullptr;
#ifdef USE_THEORADEC
				wakeModeMovie = new Video::TheoraDecoder();
				if (!wakeModeMovie->loadFile(wakeModeMoviePath.append(".ogg"))) {
					delete wakeModeMovie;
					wakeModeMovie = nullptr;
				}
#endif
				if (!wakeModeMovie) {
					wakeModeMovie = new Video::QuickTimeDecoder();
					if (!wakeModeMovie->loadFile(wakeModeMoviePath.append(".movie")))
						error("Could not load Jonny Ego movie");
				}

				wakeModeMovie->setVolume(MIN<uint>(_vm->getSoundFXLevel(), 0xFF));

				wakeModeMovie->start();

				while (!_vm->shouldQuit() && !wakeModeMovie->endOfVideo()) {
					if (wakeModeMovie->needsUpdate()) {
						frame = wakeModeMovie->decodeNextFrame();

						if (frame) {
							g_system->copyRectToScreen((const byte *)frame->getPixels(), frame->pitch, 0, 0, frame->w, frame->h);
							g_system->updateScreen();
						}
					}

					InputDevice.getInput(input, kPullbackInterruptFilter);
					if ((input.anyInput() || _vm->saveRequested() || _vm->loadRequested()) && !GameState.getEasterEgg()) {
						skipped = true;
						break;
					}

					g_system->delayMillis(10);
				}

				delete wakeModeMovie;

				if (_vm->shouldQuit())
					return;
			} else {
				_vm->_gfx->doFadeOutSync(kThreeSeconds * kFifteenTicksPerSecond, kFifteenTicksPerSecond, false);
				g_system->delayMillis(3 * 1000 / 2);
				getExtraEntry(kCaldoria00WakeUp1, entry);
				_navMovie.setTime(entry.movieStart);
				_navMovie.redrawMovieWorld();
				_navMovie.show();
				_vm->refreshDisplay();
				_vm->_gfx->doFadeInSync(kOneSecond * kFifteenTicksPerSecond, kFifteenTicksPerSecond, false);
			}
		} else {
			getExtraEntry(kCaldoria00WakeUp1, entry);
			_navMovie.setTime(entry.movieStart);
			_navMovie.redrawMovieWorld();
			_navMovie.show();
		}

		GameState.setCaldoriaSeenPullback(true);
	}

	Neighborhood::start();
}

void Caldoria::throwAwayInterface() {
	Neighborhood::throwAwayInterface();
	if (_vm->isDVD()) {
		_vm->getAllHotspots().remove(&_laundryZoomInSpot);
		_vm->getAllHotspots().remove(&_laundryZoomOutSpot);
		_vm->getAllHotspots().remove(&_cornbreadSpot);
	}
}

void Caldoria::flushGameState() {
	GameState.setCaldoriaFuseTimeLimit(_utilityFuse.getTimeRemaining());
}

class AIBombActiveCondition : public AICondition {
public:
	AIBombActiveCondition() {}

	bool fireCondition() override;
};

// Return true if player is on 53 east and Sinclair is shot.
bool AIBombActiveCondition::fireCondition() {
	return GameState.getCurrentRoom() == kCaldoria53 && GameState.getCurrentDirection() == kEast &&
			GameState.getCaldoriaSinclairShot();
}

void Caldoria::setUpAIRules() {
	Neighborhood::setUpAIRules();

	if (g_AIArea) {
		if (GameState.allTimeZonesFinished()) {
			AIPlayMessageAction *messageAction = new AIPlayMessageAction("Images/AI/Caldoria/X49NB1", false);
			AILocationCondition *locCondition = new AILocationCondition(1);
			locCondition->addLocation(MakeRoomView(kCaldoria49, kNorth));
			AIRule *rule = new AIRule(locCondition, messageAction);
			g_AIArea->addAIRule(rule);

			messageAction = new AIPlayMessageAction("Images/AI/Caldoria/X56EH1", false);
			AIBombActiveCondition *activeCondition = new AIBombActiveCondition();
			rule = new AIRule(activeCondition, messageAction);
			g_AIArea->addAIRule(rule);
		} else {
			AIPlayMessageAction *messageAction = new AIPlayMessageAction("Images/AI/Caldoria/XAB2", false);
			AITimerCondition *timerCondition = new AITimerCondition(kLateWarning3TimeLimit, 1, true);
			AILocationCondition *locCondition = new AILocationCondition(1);
			locCondition->addLocation(MakeRoomView(kCaldoria44, kEast));
			AINotCondition *notCondition = new AINotCondition(locCondition);
			AIAndCondition *andCondition = new AIAndCondition(timerCondition, notCondition);
			AIRule *rule = new AIRule(andCondition, messageAction);
			g_AIArea->addAIRule(rule);

			messageAction = new AIPlayMessageAction("Images/AI/Caldoria/XAB1", false);
			timerCondition = new AITimerCondition(kLateWarning2TimeLimit, 1, true);
			locCondition = new AILocationCondition(1);
			locCondition->addLocation(MakeRoomView(kCaldoria44, kEast));
			notCondition = new AINotCondition(locCondition);
			andCondition = new AIAndCondition(timerCondition, notCondition);
			rule = new AIRule(andCondition, messageAction);
			g_AIArea->addAIRule(rule);

			messageAction = new AIPlayMessageAction("Images/AI/Caldoria/XA44EB", false);
			locCondition = new AILocationCondition(3);
			locCondition->addLocation(MakeRoomView(kCaldoria01, kNorth));
			locCondition->addLocation(MakeRoomView(kCaldoria01, kEast));
			locCondition->addLocation(MakeRoomView(kCaldoria01, kSouth));
			rule = new AIRule(locCondition, messageAction);
			g_AIArea->addAIRule(rule);

			messageAction = new AIPlayMessageAction("Images/AI/Caldoria/X42WH1", false);
			AICondition *condition = makeLocationAndDoesntHaveItemCondition(kCaldoria44, kEast, kKeyCard);
			rule = new AIRule(condition, messageAction);
			g_AIArea->addAIRule(rule);

			AIActivateRuleAction *ruleAction = new AIActivateRuleAction(rule);
			locCondition = new AILocationCondition(1);
			locCondition->addLocation(MakeRoomView(kCaldoria42, kEast));
			rule = new AIRule(locCondition, ruleAction);
			g_AIArea->addAIRule(rule);
		}
 	}
}

uint16 Caldoria::getDateResID() const {
	return kDate2318ID;
}

TimeValue Caldoria::getViewTime(const RoomID room, const DirectionConstant direction) {
	ExtraTable::Entry extra;
	uint32 extraID = 0xffffffff;

	switch (room) {
	case kCaldoria00:
		if (direction == kEast && _privateFlags.getFlag(kCaldoriaPrivate4DSystemOpenFlag))
			extraID = k4DEnvironOpenView;
		break;
	case kCaldoriaDrawers:
		if (direction == kNorth && _privateFlags.getFlag(kCaldoriaPrivateRightDrawerOpenFlag)) {
			if (GameState.isTakenItemID(kKeyCard))
				extraID = kRightDrawerOpenViewNoKeys;
			else
				extraID = kRightDrawerOpenViewWithKeys;
		}
		break;
	case kCaldoria16:
		if (direction == kSouth && GameState.getCaldoriaSeenSinclairInElevator())
			extraID = kCaldoria16SouthViewWithElevator;
		break;
	case kCaldoriaReplicator:
		if (GameState.getCaldoriaMadeOJ() && !(GameState.isTakenItemID(kOrangeJuiceGlassEmpty) || GameState.isTakenItemID(kOrangeJuiceGlassFull)))
			extraID = kReplicatorNorthViewWithOJ;
		break;
	case kCaldoriaKiosk:
	case kCaldoriaBinoculars:
		return 0xffffffff;
	case kCaldoria48:
		if (direction == kNorth && GameState.getCaldoriaRoofDoorOpen())
			extraID = kCa48NorthExplosion;
		break;
	default:
		break;
	}

	if (extraID == 0xffffffff)
		return Neighborhood::getViewTime(room, direction);

	getExtraEntry(extraID, extra);
	return extra.movieEnd - 1;
}

void Caldoria::startSpotOnceOnly(TimeValue startTime, TimeValue stopTime) {
	switch (GameState.getCurrentRoomAndView()) {
	case MakeRoomView(kCaldoria13, kEast):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen13CarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen13CarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria14, kEast):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen14CarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen14CarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria18, kWest):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen18CarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen18CarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria23, kSouth):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen23CarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen23CarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria33, kSouth):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen33CarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen33CarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria36, kNorth):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen36CarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen36CarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria41, kNorth):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen41NorthCarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen41NorthCarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria41, kEast):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen41EastCarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen41EastCarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	case MakeRoomView(kCaldoria41, kWest):
		if (!_privateFlags.getFlag(kCaldoriaPrivateSeen41WestCarFlag) && _vm->getRandomBit() == 0) {
			_privateFlags.setFlag(kCaldoriaPrivateSeen41WestCarFlag, true);
			Neighborhood::startSpotOnceOnly(startTime, stopTime);
		} else {
			showViewFrame(getViewTime(GameState.getCurrentRoom(), GameState.getCurrentDirection()));
		}
		break;
	default:
		Neighborhood::startSpotOnceOnly(startTime, stopTime);
		break;
	}
}

void Caldoria::findSpotEntry(const RoomID room, const DirectionConstant direction, SpotFlags flags, SpotTable::Entry &entry) {
	Neighborhood::findSpotEntry(room, direction, flags, entry);

	switch (room) {
	case kCaldoria00:
		if (direction == kEast && (!GameState.getCaldoriaINNAnnouncing() || GameState.getCaldoriaSeenINN()))
			entry.clear();
		break;
	case kCaldoriaVidPhone:
		if (direction == kNorth && GameState.getCaldoriaSeenMessages())
			entry.clear();
		break;
	case kCaldoria44:
		if (direction == kEast && GameState.getLastRoom() != kCaldoria42)
			entry.clear();
		break;
	default:
		break;
	}
}

void Caldoria::startExitMovie(const ExitTable::Entry &exitEntry) {
	switch (GameState.getCurrentRoom()) {
	case kCaldoria05:
	case kCaldoria07:
		if (GameState.getCurrentDirection() == kWest)
			closeCroppedMovie();
		// fall through
	case kCaldoria11:
		if (GameState.getCurrentDirection() == kEast)
			closeCroppedMovie();
		break;
	case kCaldoria13:
	case kCaldoria14:
		if (GameState.getCurrentDirection() == kNorth)
			closeCroppedMovie();
		break;
	default:
		break;
	}

	Neighborhood::startExitMovie(exitEntry);
}

void Caldoria::startZoomMovie(const ZoomTable::Entry &zoomEntry) {
	switch (GameState.getCurrentRoom()) {
	case kCaldoria12:
		if (GameState.getCurrentDirection() == kNorth)
			closeCroppedMovie();
		break;
	default:
		break;
	}

	Neighborhood::startZoomMovie(zoomEntry);
}

void Caldoria::startDoorOpenMovie(const TimeValue startTime, const TimeValue stopTime) {
	if (GameState.getCurrentRoom() == kCaldoria27 || GameState.getCurrentRoom() == kCaldoria28 || GameState.getCurrentRoom() == kCaldoria45)
		// Must be opening elevator door.
		closeCroppedMovie();

	if (GameState.getCurrentRoom() == kCaldoria44 && GameState.getLastRoom() != kCaldoria42)
		startExtraSequence(kArriveAtCaldoriaFromTSA, kDoorOpenCompletedFlag, false);
	else
		Neighborhood::startDoorOpenMovie(startTime, stopTime);
}

void Caldoria::startTurnPush(const TurnDirection turnDirection, const TimeValue newViewTime, const DirectionConstant destDirection) {
	switch (GameState.getCurrentRoom()) {
	case kCaldoria05:
	case kCaldoria07:
		if (GameState.getCurrentDirection() == kWest)
			closeCroppedMovie();
		break;
	case kCaldoria11:
		if (GameState.getCurrentDirection() == kEast)
			closeCroppedMovie();
		break;
	case kCaldoria12:
	case kCaldoria13:
	case kCaldoria14:
	case kCaldoria27:
	case kCaldoria28:
	case kCaldoria45:
		if (GameState.getCurrentDirection() == kNorth)
			closeCroppedMovie();
		break;
	case kCaldoria48:
		if (_croppedMovie.isSurfaceValid())
			closeCroppedMovie();
		break;
	default:
		break;
	}

	Neighborhood::startTurnPush(turnDirection, newViewTime, destDirection);
}

void Caldoria::bumpIntoWall() {
	requestSpotSound(kCaldoriaUhghIn, kCaldoriaUhghOut, kFilterNoInput, 0);
	Neighborhood::bumpIntoWall();
}

void Caldoria::closeDoorOffScreen(const RoomID room, const DirectionConstant direction) {
	switch (room) {
	case kCaldoria08:
		if (direction == kNorth)
			playSpotSoundSync(kCaldoriaShowerCloseIn, kCaldoriaShowerCloseOut);
		else
			playSpotSoundSync(kCaldoriaDoorCloseIn, kCaldoriaDoorCloseOut);
		break;
	case kCaldoria09:
		playSpotSoundSync(kCaldoriaShowerCloseIn, kCaldoriaShowerCloseOut);
		break;
	case kCaldoria16:
	case kCaldoria38:
	case kCaldoria46:
	case kCaldoria27:
	case kCaldoria28:
	case kCaldoria45:
		playSpotSoundSync(kCaldoriaElevatorCloseIn, kCaldoriaElevatorCloseOut);
		break;
	case kCaldoria44:
	case kCaldoria42:
		if (GameState.getCurrentRoom() == kCaldoria42)
			playSpotSoundSync(kCaldoriaGTDoorCloseIn, kCaldoriaGTDoorCloseOut);
		break;
	default:
		playSpotSoundSync(kCaldoriaDoorCloseIn, kCaldoriaDoorCloseOut);
		break;
	}
}

int16 Caldoria::getStaticCompassAngle(const RoomID room, const DirectionConstant dir) {
	int16 result = Neighborhood::getStaticCompassAngle(room, dir);

	switch (room) {
	case kCaldoriaVidPhone:
		result += kVidPhoneAngle;
		break;
	case kCaldoriaReplicator:
		result += kReplicatorAngle;
		break;
	case kCaldoriaDrawers:
		result += kDrawersAngle;
		break;
	case kCaldoria53:
		result += kCaldoria53Angle;
		break;
	case kCaldoria55:
		result += kCaldoria55Angle;
		break;
	default:
		break;
	}

	return result;
}

void Caldoria::getExitCompassMove(const ExitTable::Entry &exitEntry, FaderMoveSpec &compassMove) {
	Neighborhood::getExitCompassMove(exitEntry, compassMove);

	switch (MakeRoomView(exitEntry.room, exitEntry.direction)) {
	case MakeRoomView(kCaldoria08, kNorth):
	case MakeRoomView(kCaldoria09, kSouth):
		compassMove.insertFaderKnot((exitEntry.movieStart + exitEntry.movieEnd) >> 1, compassMove.getNthKnotValue(0) + 30);
		break;
	case MakeRoomView(kCaldoria10, kEast):
		compassMove.insertFaderKnot(exitEntry.movieStart + 4 * kCaldoriaFrameDuration, 90);
		compassMove.insertFaderKnot(exitEntry.movieStart + 19 * kCaldoriaFrameDuration, -90);
		break;
	case MakeRoomView(kCaldoria42, kWest):
		compassMove.makeTwoKnotFaderSpec(_navMovie.getScale(), exitEntry.movieStart, -90, exitEntry.movieEnd, 90);
		compassMove.insertFaderKnot(exitEntry.movieStart + 3 * kCaldoriaFrameDuration, -90);
		compassMove.insertFaderKnot(exitEntry.movieStart + 33 * kCaldoriaFrameDuration, 90);
		break;
	case MakeRoomView(kCaldoria54, kEast):
		if (getCurrentAlternate() != kAltCaldoriaSinclairDown) {
			compassMove.insertFaderKnot(exitEntry.movieStart + 16 * kCaldoriaFrameDuration, 135);
			compassMove.insertFaderKnot(exitEntry.movieEnd, 135);
		}
		break;
	case MakeRoomView(kCaldoria55, kNorth):
		compassMove.makeTwoKnotFaderSpec(_navMovie.getScale(), exitEntry.movieStart, 315, exitEntry.movieEnd, 270);
		break;
	default:
		break;
	}
}

void Caldoria::getZoomCompassMove(const ZoomTable::Entry &zoomEntry, FaderMoveSpec &compassMove) {
	Neighborhood::getZoomCompassMove(zoomEntry, compassMove);

	switch (zoomEntry.hotspot) {
	case kCaBathroomToiletSpotID:
		compassMove.insertFaderKnot(zoomEntry.movieStart + 4 * kCaldoriaFrameDuration, 90);
		compassMove.insertFaderKnot(zoomEntry.movieStart + 19 * kCaldoriaFrameDuration, -90);
		compassMove.insertFaderKnot(zoomEntry.movieEnd, -90);
		break;
	default:
		break;
	}
}

void Caldoria::getExtraCompassMove(const ExtraTable::Entry &entry, FaderMoveSpec &compassMove) {
	switch (entry.extra) {
	case kCaldoria00WakeUp1:
		compassMove.makeTwoKnotFaderSpec(_navMovie.getScale(), entry.movieStart, 90, entry.movieEnd, 180);
		compassMove.insertFaderKnot(entry.movieStart + 1000, 90);
		compassMove.insertFaderKnot(entry.movieStart + 1640, 120);
		compassMove.insertFaderKnot(entry.movieStart + 2240, 135);
		compassMove.insertFaderKnot(entry.movieStart + 2640, 180);
		break;
	case kCaldoria00WakeUp2:
		compassMove.makeTwoKnotFaderSpec(_navMovie.getScale(), entry.movieStart, 180, entry.movieEnd, 90);
		compassMove.insertFaderKnot(entry.movieStart + 560, 90);
		break;
	case kCaldoria56BombStage1:
		compassMove.makeTwoKnotFaderSpec(_navMovie.getScale(), entry.movieStart, 90, entry.movieEnd, 10);
		compassMove.insertFaderKnot(entry.movieStart + 31 * kCaldoriaFrameDuration, 60);
		compassMove.insertFaderKnot(entry.movieStart + 49 * kCaldoriaFrameDuration, 60);
		compassMove.insertFaderKnot(entry.movieStart + 66 * kCaldoriaFrameDuration, 10);
		break;
	case kCaldoria56BombStage7:
		compassMove.makeTwoKnotFaderSpec(_navMovie.getScale(), entry.movieStart, 10, entry.movieEnd, 90);
		compassMove.insertFaderKnot(entry.movieStart + 131 * kCaldoriaFrameDuration, 10);
		compassMove.insertFaderKnot(entry.movieStart + 148 * kCaldoriaFrameDuration, 60);
		compassMove.insertFaderKnot(entry.movieStart + 165 * kCaldoriaFrameDuration, 60);
		compassMove.insertFaderKnot(entry.movieEnd - 5 * kCaldoriaFrameDuration, 90);
		break;
	default:
		Neighborhood::getExtraCompassMove(entry, compassMove);
		break;
	}
}

void Caldoria::loadAmbientLoops() {
	RoomID room = GameState.getCurrentRoom();

	if (_vm->isDVD()) {
		// Updated sounds in the DVD version
		if (room == kCaldoria00 && GameState.getCaldoriaWokenUp())
			loadLoopSound1("Sounds/Caldoria/Apartment Music.32K.aiff", 0x100 / 4);
		else if (room >= kCaldoria01 && room <= kCaldoria14)
			loadLoopSound1("Sounds/Caldoria/Apartment Music.32K.aiff", 0x100 / 4);
		else if (room == kCaldoria27 || room == kCaldoria28 || room == kCaldoria45)
			loadLoopSound1("Sounds/Caldoria/Elevator Loop.32K.aiff", 0x100 / 5);
		else if (room == kCaldoria44)
			loadLoopSound1("Sounds/Caldoria/TSA Hum Loop.44K.aiff");
		else if (room >= kCaldoria15 && room <= kCaldoria48)
			loadLoopSound1("Sounds/Caldoria/Industrial Nuage.44K.aiff", 2 * 0x100 / 3);
		else if (room >= kCaldoria49 && room <= kCaldoria56)
			loadLoopSound1("Sounds/Caldoria/A50NLB00.32K.AIFF", 0x100 / 4);
	} else {
		if (room == kCaldoria00 && GameState.getCaldoriaWokenUp())
			loadLoopSound1("Sounds/Caldoria/Apartment Music.AIFF", 0x100 / 4);
		else if (room >= kCaldoria01 && room <= kCaldoria14)
			loadLoopSound1("Sounds/Caldoria/Apartment Music.AIFF", 0x100 / 4);
		else if (room == kCaldoria27 || room == kCaldoria28 || room == kCaldoria45)
			loadLoopSound1("Sounds/Caldoria/Elevator Loop.AIFF", 0x100 / 5);
		else if (room == kCaldoria44)
			loadLoopSound1("Sounds/Caldoria/TSA Hum Loop.AIFF");
		else if (room >= kCaldoria15 && room <= kCaldoria48)
			loadLoopSound1("Sounds/Caldoria/Industrial Nuage.aiff", 2 * 0x100 / 3);
		else if (room >= kCaldoria49 && room <= kCaldoria56)
			loadLoopSound1("Sounds/Caldoria/A50NLB00.22K.AIFF", 0x100 / 4);
	}
}

void Caldoria::checkContinuePoint(const RoomID room, const DirectionConstant direction) {
	switch (MakeRoomView(room, direction)) {
	case MakeRoomView(kCaldoria06, kSouth):
	case MakeRoomView(kCaldoria13, kNorth):
	case MakeRoomView(kCaldoria16, kSouth):
	case MakeRoomView(kCaldoria38, kEast):
	case MakeRoomView(kCaldoria38, kWest):
	case MakeRoomView(kCaldoria40, kNorth):
	case MakeRoomView(kCaldoria44, kEast):
	case MakeRoomView(kCaldoria48, kNorth):
	case MakeRoomView(kCaldoria49, kNorth):
		makeContinuePoint();
		break;
	default:
		break;
	}
}

void Caldoria::spotCompleted() {
	Neighborhood::spotCompleted();
	switch (GameState.getCurrentRoom()) {
	case kCaldoriaBinoculars:
		startExtraSequence(kBinocularsZoomInOnShip, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaToilet:
		if (g_arthurChip)
			g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA47", kArthurCaldoriaReadPaper);
		break;
	}
}

void Caldoria::arriveAt(const RoomID room, const DirectionConstant direction) {
	switch (room) {
	case kCaldoria56:
		if (!GameState.getCaldoriaGunAimed())
			// Fall through...
	case kCaldoria49:
	case kCaldoria50:
	case kCaldoria51:
	case kCaldoria52:
	case kCaldoria53:
	case kCaldoria54:
	case kCaldoria55:
		if (GameState.getCaldoriaSinclairShot())
			setCurrentAlternate(kAltCaldoriaSinclairDown);
		break;
	default:
		break;
	}

	Neighborhood::arriveAt(room, direction);
	Input dummy;
	Sound flushSound;

	switch (room) {
	case kCaldoria00:
		arriveAtCaldoria00();
		break;
	case kCaldoria05:
		if (direction == kWest && GameState.getCaldoriaINNAnnouncing())
			loopCroppedMovie("Images/Caldoria/A05 Light Loop", kCaldoriaA05LightLoopLeft, kCaldoriaA05LightLoopTop);
		break;
	case kCaldoria07:
		if (direction == kWest && GameState.getCaldoriaINNAnnouncing())
			loopCroppedMovie("Images/Caldoria/A07 Light Loop", kCaldoriaA07LightLoopLeft, kCaldoriaA07LightLoopTop);
		break;
	case kCaldoria09:
		_lastExtra = 0xffffffff;
		break;
	case kCaldoria10:
		if (_vm->isDVD() && direction == kWest) {
			_vm->_cursor->hide();
			flushSound.initFromAIFFFile("Sounds/Caldoria/Apartment Toilet.32K.aiff");
			flushSound.setVolume(_vm->getSoundFXLevel());
			flushSound.playSound();
			while (flushSound.isPlaying() && !_vm->shouldQuit()) {
				InputDevice.getInput(dummy, kFilterNoInput);

				_vm->checkCallBacks();
				_vm->refreshDisplay();
				_vm->_system->delayMillis(10);
			}
			if (_vm->shouldQuit())
				return;
			_vm->_cursor->hideUntilMoved();
		} else if (direction == kEast) {
			if (g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA42", kArthurCaldoriaReachedToilet);
		}
		break;
	case kCaldoriaToilet:
		GameState.setScoringReadPaper(true);
		break;
	case kCaldoriaReplicator:
		setCurrentActivation(kActivateReplicatorReady);
		requestSpotSound(kCaldoriaReplicatorIntroIn, kCaldoriaReplicatorIntroOut, kFilterNoInput, 0);
		break;
	case kCaldoria11:
		setCurrentAlternate(kAltCaldoriaNormal);
		if (direction == kEast && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A11 Message Machine Loop", kCaldoria11MessageLoopLeft, kCaldoria11MessageLoopTop);
		break;
	case kCaldoria12:
		if (direction == kNorth && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A12 Message Machine Loop", kCaldoria12MessageLoopLeft, kCaldoria12MessageLoopTop);
		break;
	case kCaldoriaDrawers:
		setCurrentActivation(kActivateDrawersClosed);
		break;
	case kCaldoria13:
		GameState.setCaldoriaINNAnnouncing(true);
		if (direction == kNorth && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A13 Message Machine Loop", kCaldoria13MessageLoopLeft, kCaldoria13MessageLoopTop);
		break;
	case kCaldoria14:
		if (direction == kNorth && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A14 Message Machine Loop", kCaldoria14MessageLoopLeft, kCaldoria14MessageLoopTop);
		break;
	case kCaldoria08:
		if (direction == kWest)
			setCurrentActivation(kActivateMirrorReady);
		// Fall through...
	case kCaldoria15:
		GameState.setCaldoriaINNAnnouncing(true);
		break;
	case kCaldoria27:
	case kCaldoria28:
	case kCaldoria45:
		if (GameState.getCurrentDirection() == kNorth)
			openDoor();
		break;
	case kCaldoriaBinoculars:
		GameState.setScoringLookThroughTelescope(true);
		break;
	case kCaldoriaKiosk:
		GameState.setScoringSawCaldoriaKiosk(true);
		startExtraSequenceSync(kCaldoriaKioskVideo, kFilterAllInput);
		downButton(dummy);
		break;
	case kCaldoria44:
		arriveAtCaldoria44();
		break;
	case kCaldoria49:
		arriveAtCaldoria49();
		break;
	case kCaldoria48:
		if (direction == kNorth && !GameState.getCaldoriaDoorBombed() && GameState.isTakenItemID(kCardBomb) && g_arthurChip)
			g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA12", kArthurCaldoriaRoofDoor);
		break;
	case kCaldoria53:
		if (direction == kEast && !GameState.getCaldoriaSinclairShot())
			zoomToSinclair();
		break;
	case kCaldoria50:
		if (direction == kNorth && !GameState.getCaldoriaSinclairShot())
			setUpSinclairLoops();
		break;
	case kCaldoria54:
		if (direction == kSouth && !GameState.getCaldoriaSinclairShot())
			setUpSinclairLoops();
		break;
	case kCaldoria56:
		arriveAtCaldoria56();
		break;
	case kCaldoriaDeathRoom:
		arriveAtCaldoriaDeath();
		break;
	default:
		break;
	}

	checkSinclairShootsOS();
	setUpRoofTop();
}

void Caldoria::doAIRecalibration() {
	GameState.setCaldoriaDidRecalibration(true);

	Input input;
	InputDevice.getInput(input, kPullbackInterruptFilter);
	if (_vm->isDVD() && JMPPPInput::isEasterEggModifierInput(input)) {
		Video::VideoDecoder *video = nullptr;

		_vm->_cursor->hide();

#ifdef USE_THEORADEC
		video = new Video::TheoraDecoder();
		if (!video->loadFile("Images/Caldoria/A00EA.ogg")) {
			delete video;
			video = nullptr;
		}
#endif

		if (!video) {
			video = new Video::QuickTimeDecoder();
			if (!video->loadFile("Images/Caldoria/A00EA.movie"))
				error("Could not load Month-O-Matic video");
		}

		video->setVolume(MIN<uint>(_vm->getSoundFXLevel(), 0xFF));
		video->start();

		while (!_vm->shouldQuit() && !video->endOfVideo()) {
			if (video->needsUpdate()) {
				const Graphics::Surface *frame = video->decodeNextFrame();

				if (frame) {
					g_system->copyRectToScreen((const byte *)frame->getPixels(), frame->pitch, kNavAreaLeft, kNavAreaTop, frame->w, frame->h);
					g_system->updateScreen();
				}
			}

			InputDevice.pumpEvents();

			g_system->delayMillis(10);
		}

		delete video;

		if (_vm->shouldQuit())
			return;

		arriveAt(kCaldoria01, kEast);
	} else if (_vm->isChattyAI()) {
		if (!g_AIArea->playAIMovie(kRightAreaSignature, "Images/AI/Caldoria/XA01EB1", true, kRecalibrationInterruptFilter))
			return;

		g_interface->calibrateEnergyBar();
		if (!g_AIArea->playAIMovie(kRightAreaSignature, "Images/AI/Caldoria/XA01EB4", true, kRecalibrationInterruptFilter))
			return;

		g_interface->raiseInventoryDrawerSync();
		if (!g_AIArea->playAIMovie(kRightAreaSignature, "Images/AI/Caldoria/XA01EB6", true, kRecalibrationInterruptFilter)) {
			g_interface->lowerInventoryDrawerSync();
			return;
		}

		g_interface->lowerInventoryDrawerSync();
		g_interface->raiseBiochipDrawerSync();

		if (!g_AIArea->playAIMovie(kRightAreaSignature, "Images/AI/Caldoria/XA01EB5", true, kRecalibrationInterruptFilter)) {
			g_interface->lowerBiochipDrawerSync();
			return;
		}

		g_interface->lowerBiochipDrawerSync();

		g_AIArea->playAIMovie(kRightAreaSignature, "Images/AI/Caldoria/XA01EB8", false, kRecalibrationInterruptFilter);
	}
}

void Caldoria::arriveAtCaldoria00() {
	if (GameState.getCurrentDirection() == kEast) {
		if (GameState.getCaldoriaWokenUp()) {
			if (!GameState.getCaldoriaDidRecalibration())
				doAIRecalibration();
			setCurrentActivation(kActivate4DClosed);
		} else {
			// Good morning, sleeping beauty
			ExtraTable::Entry extra;
			getExtraEntry(kCaldoria00WakeUp1, extra);

			if (_navMovie.getTime() != extra.movieStart) {
				_navMovie.setTime(extra.movieStart);
				_navMovie.redrawMovieWorld();
			}

			startExtraSequenceSync(kCaldoria00WakeUp1, kFilterNoInput);
			GameState.setCaldoriaWokenUp(true);
			playCroppedMovieOnce("Images/Caldoria/VidPhone.movie", kCaldoriaVidPhoneLeft, kCaldoriaVidPhoneTop, kFilterAllInput);
			startExtraSequence(kCaldoria00WakeUp2, kExtraCompletedFlag, kFilterNoInput);
		}
	}
}

bool Caldoria::wantsCursor() {
	return GameState.getCaldoriaDidRecalibration();
}

void Caldoria::arriveAtCaldoria44() {
	if (GameState.getLastNeighborhood() != kCaldoriaID) {
		openDoor();
	} else {
		setCurrentActivation(kActivateReadyForCard);
		loopExtraSequence(kCaldoriaTransporterArrowLoop, 0);
	}
}

void Caldoria::arriveAtCaldoria49() {
	if (GameState.getLastRoom() == kCaldoria48)
		setCurrentAlternate(kAltCaldoriaNormal);

	// Need to force the loop to play.
	if (GameState.getCurrentDirection() == kNorth && !GameState.getCaldoriaSinclairShot()) {
		GameState.setCaldoriaFuseTimeLimit(kSinclairShootsTimeLimit);
		startExtraSequence(kCa49NorthVoiceAnalysis, kExtraCompletedFlag, kFilterNoInput);
	}
}

void Caldoria::arriveAtCaldoria56() {
	if (!GameState.getCaldoriaBombDisarmed()) {
		_privateFlags.setFlag(kCaldoriaPrivateZoomingToBombFlag, true);

		if (GameState.getCurrentDirection() == kNorth) {
			turnRight();
		} else if (GameState.getCurrentDirection() == kSouth) {
			turnLeft();
		} else if (GameState.getCurrentDirection() == kEast) {
			_privateFlags.setFlag(kCaldoriaPrivateZoomingToBombFlag, false);
			newInteraction(kCaldoriaBombInteractionID);
		}
	}
}

void Caldoria::arriveAtCaldoriaDeath() {
	if (GameState.getLastRoom() == kCaldoria49) {
		if (GameState.getCaldoriaSinclairShot()) {
			die(kDeathNuclearExplosion);
		} else {
			playSpotSoundSync(kCaldoriaSinclairShootsOSIn, kCaldoriaSinclairShootsOSOut);
			playSpotSoundSync(kCaldoriaScreamingAfterIn, kCaldoriaScreamingAfterOut);
			die(kDeathSinclairShotDelegate);
		}
	} else {
		die(kDeathShotBySinclair);
	}
}

void Caldoria::setUpRoofTop() {
	switch (GameState.getCurrentRoom()) {
	case kCaldoria48:
		if (GameState.getCurrentDirection() == kNorth) {
			if (GameState.getCaldoriaRoofDoorOpen()) {
				setCurrentAlternate(kAltCaldoriaRoofDoorBlown);
			} else if (GameState.getCaldoriaDoorBombed()) {
				// Long enough for AI hints...?
				_utilityFuse.primeFuse(kCardBombCountDownTime);
				_utilityFuse.setFunctor(new Common::Functor0Mem<void, Caldoria>(this, &Caldoria::doorBombTimerExpired));
				_utilityFuse.lightFuse();

				loopCroppedMovie("Images/Caldoria/A48 Bomb Loop", kCaldoria48CardBombLoopLeft, kCaldoria48CardBombLoopTop);
			} else {
				setCurrentActivation(kActivateRoofSlotEmpty);
			}
		}
		break;
	case kCaldoria56:
		if (GameState.getCurrentDirection() == kEast && GameState.getCaldoriaGunAimed())
			startExtraSequence(kCa53EastShootSinclair, kExtraCompletedFlag, false);
		else
			// Fall through...
	case kCaldoria49:
	case kCaldoria50:
	case kCaldoria51:
	case kCaldoria52:
	case kCaldoria53:
	case kCaldoria54:
	case kCaldoria55:
		if (!GameState.getCaldoriaSinclairShot()) {
			if (GameState.getCaldoriaSawVoiceAnalysis() && !_utilityFuse.isFuseLit()) {
				_utilityFuse.primeFuse(GameState.getCaldoriaFuseTimeLimit());
				_utilityFuse.setFunctor(new Common::Functor0Mem<void, Caldoria>(this, &Caldoria::sinclairTimerExpired));
				_utilityFuse.lightFuse();
			}
		} else {
			setCurrentAlternate(kAltCaldoriaSinclairDown);
		}
		break;
	default:
		break;
	}
}

void Caldoria::leftButton(const Input &input) {
	if (!(GameState.getCurrentRoomAndView() == MakeRoomView(kCaldoria11, kWest) && _lookingAtLaundry))
		Neighborhood::leftButton(input);
}

void Caldoria::rightButton(const Input &input) {
	if (!(GameState.getCurrentRoomAndView() == MakeRoomView(kCaldoria11, kWest) && _lookingAtLaundry))
		Neighborhood::rightButton(input);
}

void Caldoria::downButton(const Input &input) {
	switch (GameState.getCurrentRoomAndView()) {
	case MakeRoomView(kCaldoria01, kEast):
		GameState.setCaldoriaWokenUp(true);
		startExtraSequence(kCaldoria00SitDown, kExtraCompletedFlag, kFilterNoInput);
		break;
	case MakeRoomView(kCaldoria11, kWest):
		if (_lookingAtLaundry)
			startExtraSequence(kWashingMachineZoomOut, kExtraCompletedFlag, kFilterNoInput);
		break;
	default:
		Neighborhood::downButton(input);
		break;
	}
}

void Caldoria::turnTo(const DirectionConstant direction) {
	Neighborhood::turnTo(direction);

	switch (GameState.getCurrentRoom()) {
	case kCaldoria00:
		if (direction == kEast)
			setCurrentActivation(kActivate4DClosed);
		break;
	case kCaldoria01:
		if (direction == kEast) {
			GameState.setCaldoriaWokenUp(true);
			startExtraSequence(kCaldoria00SitDown, kExtraCompletedFlag, kFilterNoInput);
		}
		break;
	case kCaldoria05:
		if (direction == kWest && GameState.getCaldoriaINNAnnouncing())
			loopCroppedMovie("Images/Caldoria/A05 Light Loop", kCaldoriaA05LightLoopLeft, kCaldoriaA05LightLoopTop);
		break;
	case kCaldoria07:
		if (direction == kWest && GameState.getCaldoriaINNAnnouncing())
			loopCroppedMovie("Images/Caldoria/A07 Light Loop", kCaldoriaA07LightLoopLeft, kCaldoriaA07LightLoopTop);
		break;
	case kCaldoria08:
		if (direction == kWest)
			setCurrentActivation(kActivateMirrorReady);
		break;
	case kCaldoria09:
		_lastExtra = 0xffffffff;
		break;
	case kCaldoria10:
		if (direction == kEast && g_arthurChip)
			g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA42", kArthurCaldoriaReachedToilet);
		break;
	case kCaldoria11:
		if (direction == kEast && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A11 Message Machine Loop", kCaldoria11MessageLoopLeft, kCaldoria11MessageLoopTop);
		break;
	case kCaldoria12:
		if (direction == kNorth && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A12 Message Machine Loop", kCaldoria12MessageLoopLeft, kCaldoria12MessageLoopTop);
		break;
	case kCaldoria13:
		if (direction == kNorth && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A13 Message Machine Loop", kCaldoria13MessageLoopLeft, kCaldoria13MessageLoopTop);
		break;
	case kCaldoria14:
		if (direction == kNorth && !GameState.getCaldoriaSeenMessages())
			loopCroppedMovie("Images/Caldoria/A14 Message Machine Loop", kCaldoria14MessageLoopLeft, kCaldoria14MessageLoopTop);
		break;
	case kCaldoria27:
	case kCaldoria28:
	case kCaldoria45:
		if (direction == kNorth)
			openElevatorMovie();
		else
			closeCroppedMovie();
		break;
	case kCaldoria48:
		if (direction == kNorth && !GameState.getCaldoriaDoorBombed()) {
			setCurrentActivation(kActivateRoofSlotEmpty);
			if (GameState.isTakenItemID(kCardBomb) && g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA12", kArthurCaldoriaRoofDoor);
		}
		break;
	case kCaldoria53:
		if (GameState.getCurrentDirection() == kEast && !GameState.getCaldoriaSinclairShot())
			zoomToSinclair();
		break;
	case kCaldoria50:
		if (direction == kNorth && !GameState.getCaldoriaSinclairShot())
			setUpSinclairLoops();
		break;
	case kCaldoria54:
		if (direction == kSouth && !GameState.getCaldoriaSinclairShot())
			setUpSinclairLoops();
		break;
	case kCaldoria56:
		if (_privateFlags.getFlag(kCaldoriaPrivateZoomingToBombFlag)) {
			_privateFlags.setFlag(kCaldoriaPrivateZoomingToBombFlag, false);
			newInteraction(kCaldoriaBombInteractionID);
		} else if (GameState.getCaldoriaBombDisarmed()) {
			_vm->playEndMessage();
		}
		break;
	default:
		break;
	}

	checkSinclairShootsOS();
}

void Caldoria::zoomTo(const Hotspot *zoomOutSpot) {
	// Need to set _zoomOutSpot here because we may come through
	// this function another way, say by pressing the down arrow,
	// that doesn't involve the ClickInHotSpot function.
	_zoomOutSpot = zoomOutSpot;

	if (_vm->isDVD() && zoomOutSpot->getObjectID() == kCaldoriaReplicatorOutSpotID) {
		GameState.setEasterEgg(false);
		Neighborhood::zoomTo(zoomOutSpot);
	} else if (zoomOutSpot->getObjectID() == kCaldoriaDrawersOutSpotID) {
		if (_privateFlags.getFlag(kCaloriaPrivateLeftDrawerOpenFlag)) {
			_privateFlags.setFlag(kCaloriaPrivateLeftDrawerOpenFlag, false);
			startExtraSequence(kLeftDrawerClose, kExtraCompletedFlag, kFilterNoInput);
		} else if (_privateFlags.getFlag(kCaldoriaPrivateRightDrawerOpenFlag)) {
			_privateFlags.setFlag(kCaldoriaPrivateRightDrawerOpenFlag, false);
			if (GameState.isTakenItemID(kKeyCard))
				startExtraSequence(kRightDrawerCloseNoKeys, kExtraCompletedFlag, false);
			else
				startExtraSequence(kRightDrawerCloseWithKeys, kExtraCompletedFlag, false);
		} else {
			Neighborhood::zoomTo(zoomOutSpot);
		}
	} else {
		Neighborhood::zoomTo(zoomOutSpot);
	}
}

void Caldoria::setUpSinclairLoops() {
	_navMovie.stop();
	scheduleNavCallBack(kSinclairLoopDoneFlag);
	_sinclairLoopCount = 0;
	_numSinclairLoops = 2;
	_navMovie.start();
}

void Caldoria::zoomToSinclair() {
	_utilityFuse.stopFuse();
	_privateFlags.setFlag(kCaldoriaPrivateReadyToShootFlag, true);
	setCurrentActivation(kActivateZoomedOnSinclair);

	ExtraTable::Entry entry;
	getExtraEntry(kCa53EastZoomToSinclair, entry);
	_sinclairInterrupt.scheduleCallBack(kTriggerTimeFwd, entry.movieStart + kSinclairInterruptionTime1, _navMovie.getScale());
	startExtraSequence(kCa53EastZoomToSinclair, kExtraCompletedFlag, kFilterAllInput);
}

void Caldoria::startExtraSequence(const ExtraID extraID, const NotificationFlags flags, const InputBits interruptionFilter) {
	short left = kNavAreaLeft, top = kNavAreaTop;
	DisplayOrder displayOrder = kNavMovieOrder + 1;
	TimeValue segmentStart = 0, segmentStop = 0;
	bool loopSequence = false;
	Common::Rect pushBounds;
	NotificationFlags extraFlags;

	switch (extraID) {
	case kCreateCornbread:
	case kWashingMachineZoomIn:
	case kWashingMachineLoop:
	case kWashingMachineZoomOut:
		_turnPush.getBounds(pushBounds);

		switch (extraID) {
		case kCreateCornbread:
			_extraMovie.initFromMovieFile("Images/Caldoria/A12RC.movie");
			left = pushBounds.left;
			top = pushBounds.top;
			displayOrder = kNavMovieOrder + 1;
			segmentStart = 0;
			segmentStop = _extraMovie.getDuration();
			loopSequence = false;
			break;
		case kWashingMachineZoomIn:
			_extraMovie.initFromMovieFile("Images/Caldoria/A11WAS.movie");
			left = pushBounds.left;
			top = pushBounds.top;
			displayOrder = kNavMovieOrder + 1;
			segmentStart = 0;
			segmentStop = 5480;
			loopSequence = false;
			break;
		case kWashingMachineLoop:
			// The washing machine movie will already be loaded after zooming in
			left = pushBounds.left;
			top = pushBounds.top;
			displayOrder = kNavMovieOrder + 1;
			segmentStart = 5480;
			segmentStop = 9880;
			loopSequence = true;
			break;
		case kWashingMachineZoomOut:
			// The washing machine movie will still be loaded after looping
			left = pushBounds.left;
			top = pushBounds.top;
			displayOrder = kNavMovieOrder + 1;
			segmentStart = 9880;
			segmentStop = 11200;
			loopSequence = false;
			break;
		default:
			break;
		}

		_lastExtra = extraID;
		_turnPush.hide();

		if (!loopSequence && g_AIArea)
			g_AIArea->lockAIOut();

		extraFlags = flags;
		_interruptionFilter = interruptionFilter;
		// Stop the nav movie before doing anything else
		_navMovie.stop();
		_navMovie.stopDisplaying();

		_extraMovie.setVolume(_vm->getSoundFXLevel());
		_extraMovie.moveElementTo(left, top);
		_extraMovie.setDisplayOrder(displayOrder);
		_extraMovie.startDisplaying();
		_extraMovie.show();
		_extraMovie.setFlags(0);
		_extraMovie.setSegment(segmentStart, segmentStop);
		_extraMovie.setTime(segmentStart);
		if (loopSequence)
			_extraMovie.setFlags(kLoopTimeBase);
		else
			extraFlags |= kNeighborhoodMovieCompletedFlag;
		_extraMovieCallBack.cancelCallBack();
		_extraMovieCallBack.initCallBack(&_extraMovie, kCallBackAtExtremes);
		if (extraFlags != 0) {
			_extraMovieCallBack.setCallBackFlag(extraFlags);
			_extraMovieCallBack.scheduleCallBack(kTriggerAtStop, 0, 0);
		}
		_extraMovie.start();
		break;
	default:
		Neighborhood::startExtraSequence(extraID, flags, interruptionFilter);
		break;
	}
}

void Caldoria::receiveNotification(Notification *notification, const NotificationFlags flags) {
	Neighborhood::receiveNotification(notification, flags);

	if ((flags & kExtraCompletedFlag) != 0) {
		InventoryItem *item;
		_interruptionFilter = kFilterAllInput;

		switch (_lastExtra) {
		case kCaldoria00WakeUp2:
			makeContinuePoint();
			// Force ArriveAt to do its thing...
			GameState.setCurrentRoom(kNoRoomID);
			arriveAt(kCaldoria00, kEast);
			break;
		case k4DEnvironOpenToINN:
			GameState.setCaldoriaSeenINN(true);
			GameState.setScoringSawINN(true);
			// fall through
		case k4DEnvironOpen:
			_privateFlags.setFlag(kCaldoriaPrivate4DSystemOpenFlag, true);
			setCurrentActivation(kActivate4DOpen);
			newInteraction(kCaldoria4DInteractionID);
			break;
		case kCaldoriaShowerUp:
			GameState.setScoringTookShower(true);
			GameState.setCaldoriaDoneHygiene(true);
			break;
		case kLeftDrawerClose:
		case kRightDrawerCloseNoKeys:
		case kRightDrawerCloseWithKeys:
			if (_zoomOutSpot && _zoomOutSpot->getObjectID() == kCaldoriaDrawersOutSpotID) {
				Input input;
				clickInHotspot(input, _zoomOutSpot);
			}
			break;
		case kCreateOrangeJuice:
			GameState.setCaldoriaMadeOJ(true);
			setCurrentActivation(kActivateOJOnThePad);
			GameState.setEasterEgg(false);
			break;
		case kCreateCornbread:
			_extraMovie.moveElementTo(0, 0);
			_vm->_gfx->setCurSurface(_navMovie.getSurface());
			_extraMovie.copyToCurrentPort();
			_vm->_gfx->setCurSurface(_vm->_gfx->getWorkArea());
			_extraMovie.stopDisplaying();
			_extraMovie.releaseMovie();
			_navMovie.startDisplaying();
			GameState.setEasterEgg(true);
			if (g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBB39", kArthurCaldoriaCreatedCornbread);
			break;
		case kWashingMachineZoomIn:
			_lookingAtLaundry = true;
			startExtraSequence(kWashingMachineLoop, kExtraCompletedFlag, kFilterAllInput);
			break;
		case kWashingMachineZoomOut:
			_extraMovie.stopDisplaying();
			_extraMovie.releaseMovie();
			_navMovie.startDisplaying();
			_lookingAtLaundry = false;
			break;
		case kCaldoria00SitDown:
			arriveAt(kCaldoria00, kEast);
			break;
		case kCaldoria16ElevatorUp:
			startExtraSequence(kCaldoria16ElevatorDown, kExtraCompletedFlag, kFilterNoInput);
			break;
		case kCaldoria16ElevatorDown:
			GameState.setCaldoriaSeenSinclairInElevator(true);
			_privateFlags.setFlag(kCaldoriaPrivateCanOpenElevatorDoorFlag, true);
			openDoor();
			break;
		case kCaldoriaFourthToGround:
		case kCaldoriaRoofToGround:
			arriveAt(kCaldoria28, GameState.getCurrentDirection());
			break;
		case kCaldoriaFourthToRoof:
		case kCaldoriaGroundToRoof:
			arriveAt(kCaldoria45, GameState.getCurrentDirection());
			break;
		case kCaldoriaGroundToFourth:
		case kCaldoriaRoofToFourth:
			arriveAt(kCaldoria27, GameState.getCurrentDirection());
			break;
		case kCaGTCardSwipe:
			item = (InventoryItem *)_vm->getAllItems().findItemByID(kKeyCard);
			_vm->addItemToInventory(item);
			setCurrentActivation(kActivateReadyToTransport);
			break;
		case kCaGTFryTheFly:
		case kCaGTGoToTSA:
			_vm->jumpToNewEnvironment(kFullTSAID, kTSA00, kNorth);
			break;
		case kCaGTGoToTokyo:
			playDeathExtra(kCaGTArriveAtTokyo, kDeathUncreatedInCaldoria);
			break;
		case kCaGTGoToBeach:
			playDeathExtra(kCaGTArriveAtBeach, kDeathUncreatedInCaldoria);
			break;
		case kCa48NorthExplosion:
			// Current biochip must be the shield if we got here.
			_vm->getCurrentBiochip()->setItemState(kShieldNormal);
			if (g_arthurChip) {
				if (_vm->getRandomBit())
					g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA41", kArthurCaldoriaBlownDoor);
				else
					g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBB42", kArthurCaldoriaBlownDoor);
			}
			break;
		case kBinocularsZoomInOnShip:
			setCurrentActivation(kActivateFocusedOnShip);
			if (g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA03", kArthurCaldoriaLookThroughTelescope);
			break;
		case kCa49NorthVoiceAnalysis:
			_utilityFuse.primeFuse(kSinclairShootsTimeLimit);
			_utilityFuse.setFunctor(new Common::Functor0Mem<void, Caldoria>(this, &Caldoria::sinclairTimerExpired));
			_utilityFuse.lightFuse();
			GameState.setCaldoriaSawVoiceAnalysis(true);
			if (_vm->isDVD() && g_AIArea)
				g_AIArea->checkRules();
			if (g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA20", kArthurCaldoriaSawVoiceAnalysis);
			break;
		case kCa53EastZoomToSinclair:
			if (GameState.getCaldoriaSinclairShot()) {
				delete _gunSprite;
				_gunSprite = nullptr;
				startExtraSequence(kCa53EastShootSinclair, kExtraCompletedFlag, false);
			} else {
				playDeathExtra(kCa53EastDeath2, kDeathSinclairShotDelegate);
			}
			break;
		case kCa53EastShootSinclair:
			_vm->addItemToInventory((InventoryItem *)_vm->getAllItems().findItemByID(kStunGun));
			startExtraSequence(kCa53EastZoomOutFromSinclair, kExtraCompletedFlag, false);
			GameState.setScoringStunnedSinclair(true);
			break;
		case kCa53EastZoomOutFromSinclair:
			setCurrentAlternate(kAltCaldoriaSinclairDown);
			updateViewFrame();
			makeContinuePoint();
			break;
		default:
			break;
		}
	} else if ((flags & kSpotSoundCompletedFlag) != 0) {
		switch (GameState.getCurrentRoom()) {
		case kCaldoria11:
			if (g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA46", kArthurCaldoriaZoomedToLaundry);
			break;
		case kCaldoria20:
		case kCaldoria21:
		case kCaldoria26:
		case kCaldoria29:
		case kCaldoria34:
		case kCaldoria35:
			updateViewFrame();
			if ((GameState.getCurrentRoom() == kCaldoria34 || GameState.getCurrentRoom() == kCaldoria35) && g_arthurChip)
				g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA44", kArthurCaldoriaSawVacantApartment);
			break;
		case kCaldoria27:
		case kCaldoria28:
		case kCaldoria45:
			updateElevatorMovie();
			break;
		case kCaldoriaReplicator:
			if (_spotSounds.getStart() == kCaldoriaReplicatorWrongChoiceIn) {
				if (g_arthurChip)
					g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA16", kArthurCaldoriaSelectedStickyBuns);
			} else {
				emptyOJGlass();
				if (g_arthurChip)
					g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA45", kArthurCaldoriaDrankOJ);
			}
			break;
		default:
			break;
		}
	} else if ((flags & kSinclairLoopDoneFlag) != 0) {
		if (++_sinclairLoopCount == _numSinclairLoops) {
			switch (GameState.getCurrentRoom()) {
			case kCaldoria50:
				playDeathExtra(kCa50SinclairShoots, kDeathShotBySinclair);
				break;
			case kCaldoria54:
				playDeathExtra(kCa54SouthDeath, kDeathShotBySinclair);
				break;
			default:
				break;
			}
		} else {
			_navMovie.stop();
			scheduleNavCallBack(kSinclairLoopDoneFlag);
			_navMovie.start();
		}
	}

	g_AIArea->checkMiddleArea();
}

InputBits Caldoria::getInputFilter() {
	InputBits result = Neighborhood::getInputFilter();

	switch (GameState.getCurrentRoom()) {
	case kCaldoria00:
		if (_privateFlags.getFlag(kCaldoriaPrivate4DSystemOpenFlag))
			result &= ~kFilterAllDirections;
		break;
	case kCaldoriaBinoculars:
		if (getCurrentActivation() == kActivateNotFocusedOnShip)
			result &= ~(kFilterDownButton | kFilterDownAuto);
		break;
	case kCaldoria53:
		if (_privateFlags.getFlag(kCaldoriaPrivateReadyToShootFlag) && !GameState.getCaldoriaSinclairShot())
			result &= ~kFilterAllDirections;
		break;
	case kCaldoria48:
		if (GameState.getCaldoriaDoorBombed())
			result &= ~kFilterAllDirections;
		break;
	default:
		break;
	}

	return result;
}

void Caldoria::activateHotspots() {
	Neighborhood::activateHotspots();

	switch (GameState.getCurrentRoom()) {
	case kCaldoriaDrawers:
		if (getCurrentActivation() == kActivateRightOpen) {
			if (GameState.isTakenItemID(kKeyCard)) {
				_vm->getAllHotspots().activateOneHotspot(kCaldoriaRightDrawerNoKeysCloseSpotID);
				_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRightDrawerWithKeysCloseSpotID);
			} else {
				_vm->getAllHotspots().activateOneHotspot(kCaldoriaRightDrawerWithKeysCloseSpotID);
				_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRightDrawerNoKeysCloseSpotID);
			}
		}
		break;
	case kCaldoriaReplicator:
		if (GameState.getCaldoriaMadeOJ())
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaMakeOJSpotID);
		if (GameState.getEasterEgg())
			_vm->getAllHotspots().activateOneHotspot(kCaldoriaCornbreadHotSpotID);
		break;
	case kCaldoria11:
		if (_vm->isDVD() && GameState.getCurrentDirection() == kWest) {
			if (_lookingAtLaundry)
				_vm->getAllHotspots().activateOneHotspot(kCaldoriaLaundryZoomOutHotSpotID);
			else
				_vm->getAllHotspots().activateOneHotspot(kCaldoriaLaundryZoomInHotSpotID);
		}
		break;
	case kCaldoria27:
		if (GameState.isCurrentDoorOpen()) {
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaFourthFloorElevator1);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaFourthFloorElevator2);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaFourthFloorElevator3);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaFourthFloorElevator4);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaFourthFloorElevator5);
		}
		break;
	case kCaldoria28:
		if (GameState.isCurrentDoorOpen()) {
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaGroundElevator1);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaGroundElevator2);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaGroundElevator3);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaGroundElevator4);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaGroundElevator5);
		}
		break;
	case kCaldoria45:
		if (GameState.isCurrentDoorOpen()) {
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRoofElevator1);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRoofElevator2);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRoofElevator3);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRoofElevator4);
			_vm->getAllHotspots().deactivateOneHotspot(kCaldoriaRoofElevator5);
		}
		break;
	default:
		break;
	}
}

void Caldoria::clickInHotspot(const Input &input, const Hotspot *spot) {
	switch (spot->getObjectID()) {
	case kCa4DEnvironOpenSpotID:
		if (!GameState.getCaldoriaINNAnnouncing() || GameState.getCaldoriaSeenINN()) {
			startExtraSequence(k4DEnvironOpen, kExtraCompletedFlag, kFilterNoInput);
		} else {
			// This trick depends on the following sequences being in order in the
			// world movie:
			// k4DEnvironOpenToINN
			// k4DINNInterruption
			// k4DINNIntro
			// k4DINNMarkJohnson
			// k4DINNMeganLove
			// k4DINNFadeOut
			// k4DEnvironOpenFromINN
			loadLoopSound1("");
			loadLoopSound2("");
			startExtraLongSequence(k4DEnvironOpenToINN, k4DEnvironOpenFromINN, kExtraCompletedFlag, kFilterNoInput);
		}
		break;
	case kCa4DEnvironCloseSpotID:
		((Caldoria4DSystem *)_currentInteraction)->shutDown4DSystem();
		break;
	case kCaBathroomMirrorSpotID:
		newInteraction(kCaldoriaMirrorInteractionID);
		break;
	case kCaShowerSpotID:
		requestExtraSequence(kCaldoriaShowerTitle, 0, kFilterNoInput);
		requestExtraSequence(kCaldoriaShowerButton, 0, kFilterNoInput);
		requestExtraSequence(kCaldoriaShowerDown, 0, kFilterNoInput);
		requestExtraSequence(kCaldoriaShowerUp, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaLeftDrawerOpenSpotID:
		_privateFlags.setFlag(kCaloriaPrivateLeftDrawerOpenFlag, true);
		setCurrentActivation(kActivateLeftOpen);
		startExtraSequence(kLeftDrawerOpen, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaLeftDrawerCloseSpotID:
		_privateFlags.setFlag(kCaloriaPrivateLeftDrawerOpenFlag, false);
		setCurrentActivation(kActivateDrawersClosed);
		startExtraSequence(kLeftDrawerClose, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaRightDrawerOpenSpotID:
		_privateFlags.setFlag(kCaldoriaPrivateRightDrawerOpenFlag, true);
		setCurrentActivation(kActivateRightOpen);
		if (GameState.isTakenItemID(kKeyCard))
			startExtraSequence(kRightDrawerOpenNoKeys, kExtraCompletedFlag, kFilterNoInput);
		else
			startExtraSequence(kRightDrawerOpenWithKeys, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaRightDrawerWithKeysCloseSpotID:
		_privateFlags.setFlag(kCaldoriaPrivateRightDrawerOpenFlag, false);
		setCurrentActivation(kActivateDrawersClosed);
		startExtraSequence(kRightDrawerCloseWithKeys, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaRightDrawerNoKeysCloseSpotID:
		_privateFlags.setFlag(kCaldoriaPrivateRightDrawerOpenFlag, false);
		setCurrentActivation(kActivateDrawersClosed);
		startExtraSequence(kRightDrawerCloseNoKeys, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaMakeStickyBunsSpotID:
		requestSpotSound(kCaldoriaReplicatorWrongChoiceIn, kCaldoriaReplicatorWrongChoiceOut, kFilterNoInput, kSpotSoundCompletedFlag);
		break;
	case kCaldoriaMakeOJSpotID:
		if (_vm->isDVD() && JMPPPInput::isEasterEggModifierInput(input)) {
			startExtraSequence(kCreateCornbread, kExtraCompletedFlag, kFilterNoInput);
		} else {
			requestSpotSound(kCaldoriaReplicatorOJChoiceIn, kCaldoriaReplicatorOJChoiceOut, kFilterNoInput, 0);
			requestExtraSequence(kCreateOrangeJuice, kExtraCompletedFlag, kFilterNoInput);
		}
		break;
	case kCaldoriaCornbreadHotSpotID:
		doArthurJoyride();
		break;
	case kCaldoriaLaundryZoomInHotSpotID:
		if (_vm->isDVD()) {
			startExtraSequence(kWashingMachineZoomIn, kExtraCompletedFlag, kFilterNoInput);
			requestDelay(30, 10, kFilterNoInput, kDelayCompletedFlag);
			requestSpotSound(kCaldoriaLaundryIntro1In, kCaldoriaLaundryIntro2Out, kFilterNoInput, kSpotSoundCompletedFlag);
		}
		break;
	case kCaldoriaLaundryZoomOutHotSpotID:
		startExtraSequence(kWashingMachineZoomOut, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaBedroomVidPhoneActivationSpotID:
		newInteraction(kCaldoriaMessagesInteractionID);
		break;
	case kCaldoriaFourthFloorElevatorSpotID:
		if (!GameState.getCaldoriaSeenSinclairInElevator()) {
			startExtraSequence(kCaldoria16ElevatorUp, kExtraCompletedFlag, kFilterNoInput);
		} else {
			_privateFlags.setFlag(kCaldoriaPrivateCanOpenElevatorDoorFlag, true);
			openDoor();
		}
		break;
	case kCaldoriaGroundElevatorSpotID:
		_privateFlags.setFlag(kCaldoriaPrivateCanOpenElevatorDoorFlag, true);
		openDoor();
		break;
	case kCaldoriaRoofElevatorSpotID:
		_privateFlags.setFlag(kCaldoriaPrivateCanOpenElevatorDoorFlag, true);
		openDoor();
		break;
	case kCaldoriaFourthFloorElevator1:
	case kCaldoriaFourthFloorElevator2:
	case kCaldoriaFourthFloorElevator3:
	case kCaldoriaFourthFloorElevator4:
	case kCaldoriaFourthFloorElevator5:
		// Assumes that elevator hot spots are consecutive.
		takeElevator(4, spot->getObjectID() - kCaldoriaFourthFloorElevator1 + 1);
		break;
	case kCaldoriaGroundElevator1:
	case kCaldoriaGroundElevator2:
	case kCaldoriaGroundElevator3:
	case kCaldoriaGroundElevator4:
	case kCaldoriaGroundElevator5:
		// Assumes that elevator hot spots are consecutive.
		takeElevator(1, spot->getObjectID() - kCaldoriaGroundElevator1 + 1);
		break;
	case kCaldoriaRoofElevator1:
	case kCaldoriaRoofElevator2:
	case kCaldoriaRoofElevator3:
	case kCaldoriaRoofElevator4:
	case kCaldoriaRoofElevator5:
		// Assumes that elevator hot spots are consecutive.
		takeElevator(5, spot->getObjectID() - kCaldoriaRoofElevator1 + 1);
		break;
	case kCaldoriaGTTokyoSpotID:
		startExtraSequence(kCaGTGoToTokyo, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaGTTSASpotID:
		GameState.setScoringGoToTSA(true);
		startExtraLongSequence(kCaGTFryTheFly, kCaGTGoToTSA, kExtraCompletedFlag, false);
		break;
	case kCaldoriaGTBeachSpotID:
		startExtraSequence(kCaGTGoToBeach, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaGTOtherSpotID:
		showExtraView(kCaGTOtherChoice);
		playSpotSoundSync(kCaldoriaNoOtherDestinationIn, kCaldoriaNoOtherDestinationOut);
		showExtraView(kCaGTCardSwipe);
		break;
	case kCaldoriaZoomInOnShipSpotID:
		startExtraSequence(kBinocularsZoomInOnShip, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoriaRoofDoorSpotID:
		startExtraSequence(kCa48NorthRooftopClosed, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kCaldoria20DoorbellSpotID:
	case kCaldoria21DoorbellSpotID:
	case kCaldoria26DoorbellSpotID:
	case kCaldoria29DoorbellSpotID:
	case kCaldoria34DoorbellSpotID:
	case kCaldoria35DoorbellSpotID:
		clickOnDoorbell(spot->getObjectID());
		break;
	default:
		Neighborhood::clickInHotspot(input, spot);
		break;
	}
}

void Caldoria::clickOnDoorbell(const HotSpotID doorBellSpotID) {
	uint32 extra;
	Sound doorbellSound;
	ExtraTable::Entry entry;
	Input input;

	switch (doorBellSpotID) {
	case kCaldoria20DoorbellSpotID:
		if (_vm->isDVD())
			doorbellSound.initFromAIFFFile("Sounds/Caldoria/AH5.AIFF");
		extra = kCaldoria20Doorbell;
		break;
	case kCaldoria21DoorbellSpotID:
		if (_vm->isDVD())
			doorbellSound.initFromAIFFFile("Sounds/Caldoria/AH4.AIFF");
		extra = kCaldoria21Doorbell;
		break;
	case kCaldoria26DoorbellSpotID:
		if (_vm->isDVD())
			doorbellSound.initFromAIFFFile("Sounds/Caldoria/AH3.AIFF");
		extra = kCaldoria26Doorbell;
		break;
	case kCaldoria29DoorbellSpotID:
		if (_vm->isDVD())
			doorbellSound.initFromAIFFFile("Sounds/Caldoria/AH1.AIFF");
		extra = kCaldoria29Doorbell;
		break;
	case kCaldoria34DoorbellSpotID:
		extra = kCaldoria34Doorbell;
		break;
	case kCaldoria35DoorbellSpotID:
		extra = kCaldoria35Doorbell;
		break;
	default:
		error("Invalid doorbell hotspot");
	}

	getExtraEntry(extra, entry);
	showViewFrame(entry.movieStart);
	if (_vm->isDVD() && doorBellSpotID != kCaldoria34DoorbellSpotID && doorBellSpotID != kCaldoria35DoorbellSpotID) {
		_vm->_cursor->hide();
		doorbellSound.setVolume(_vm->getSoundFXLevel());
		doorbellSound.playSound();
		while (doorbellSound.isPlaying() && !_vm->shouldQuit()) {
			InputDevice.getInput(input, kFilterNoInput);

			_vm->checkCallBacks();
			_vm->refreshDisplay();
			_vm->_system->delayMillis(10);
		}
		if (_vm->shouldQuit())
			return;
		_vm->_cursor->hideUntilMoved();
		updateViewFrame();
	} else {
		requestSpotSound(kCaldoriaNobodyHomeIn, kCaldoriaNobodyHomeOut, kFilterNoInput, kSpotSoundCompletedFlag);
	}
}

void Caldoria::cantMoveThatWay(CanOpenDoorReason reason) {
	switch (reason) {
	case kCantMoveDoorClosed:
	case kCantMoveDoorLocked:
		openDoor();
		break;
	case kCantMoveBlocked:
		switch (GameState.getCurrentRoomAndView()) {
		case MakeRoomView(kCaldoria20, kWest):
		case MakeRoomView(kCaldoria21, kEast):
		case MakeRoomView(kCaldoria26, kSouth):
		case MakeRoomView(kCaldoria29, kSouth):
		case MakeRoomView(kCaldoria34, kWest):
		case MakeRoomView(kCaldoria35, kEast):
			cantOpenDoor(kCantOpenLocked);
			break;
		default:
			zoomUpOrBump();
			break;
		}
		break;
	default:
		bumpIntoWall();
		break;
	}
}

CanOpenDoorReason Caldoria::canOpenDoor(DoorTable::Entry &entry) {
	switch (GameState.getCurrentRoom()) {
	case kCaldoria16:
	case kCaldoria38:
	case kCaldoria46:
		if (GameState.getCurrentDirection() == kSouth && !_privateFlags.getFlag(kCaldoriaPrivateCanOpenElevatorDoorFlag))
			return kCantOpenLocked;
		break;
	default:
		break;
	}

	return Neighborhood::canOpenDoor(entry);
}

void Caldoria::doorOpened() {
	Neighborhood::doorOpened();
	_privateFlags.setFlag(kCaldoriaPrivateCanOpenElevatorDoorFlag, false);
}

GameInteraction *Caldoria::makeInteraction(const InteractionID interactionID) {
	switch (interactionID) {
	case kCaldoria4DInteractionID:
		return new Caldoria4DSystem(this);
	case kCaldoriaBombInteractionID:
		return new CaldoriaBomb(this, _vm);
	case kCaldoriaMessagesInteractionID:
		return new CaldoriaMessages(this, kCaldoriaMessagesNotificationID, _vm);
	case kCaldoriaMirrorInteractionID:
		return new CaldoriaMirror(this);
	default:
		break;
	}

	return nullptr;
}

void Caldoria::newInteraction(const InteractionID interactionID) {
	Neighborhood::newInteraction(interactionID);

	if (!_currentInteraction) {
		if (_privateFlags.getFlag(kCaldoriaPrivate4DSystemOpenFlag)) {
			_privateFlags.setFlag(kCaldoriaPrivate4DSystemOpenFlag, false);
			setCurrentActivation(kActivate4DClosed);
			startExtraSequence(k4DEnvironClose, kExtraCompletedFlag, kFilterNoInput);
		} else if (GameState.getCaldoriaBombDisarmed()) {
			turnLeft();
		}
	}
}

// Only called when trying to pick up an item and the player can't (because
// the inventory is too full or because the player lets go of the item before
// dropping it into the inventory).
Hotspot *Caldoria::getItemScreenSpot(Item *item, DisplayElement *element) {
	HotSpotID destSpotID = kNoHotSpotID;

	switch (item->getObjectID()) {
	case kKeyCard:
		destSpotID = kCaldoriaKeyCardSpotID;
		break;
	case kOrangeJuiceGlassEmpty:
	case kOrangeJuiceGlassFull:
		destSpotID = kCaldoriaOrangeJuiceSpotID;
		break;
	default:
		break;
	}

	if (destSpotID == kNoHotSpotID)
		return Neighborhood::getItemScreenSpot(item, element);

	return _vm->getAllHotspots().findHotspotByID(destSpotID);
}

void Caldoria::pickedUpItem(Item *item) {
	switch (item->getObjectID()) {
	case kKeyCard:
		GameState.setScoringGotKeyCard(true);
		break;
	case kOrangeJuiceGlassFull:
		setCurrentActivation(kActivateReplicatorReady);
		requestSpotSound(kCaldoriaDrinkOJIn, kCaldoriaDrinkOJOut, kFilterNoInput, kSpotSoundCompletedFlag);
		break;
	case kStunGun:
		GameState.setCaldoriaGunAimed(false);
		break;
	default:
		break;
	}
}

void Caldoria::dropItemIntoRoom(Item *item, Hotspot *dropSpot) {
	Input input;
	Sound cardBombVoice;

	switch (item->getObjectID()) {
	case kKeyCard:
		Neighborhood::dropItemIntoRoom(item, dropSpot);
		if (dropSpot->getObjectID() == kCaldoriaGTCardDropSpotID)
			startExtraSequence(kCaGTCardSwipe, kExtraCompletedFlag, kFilterNoInput);
		break;
	case kOrangeJuiceGlassEmpty:
		Neighborhood::dropItemIntoRoom(item, dropSpot);
		if (dropSpot->getObjectID() == kCaldoriaOrangeJuiceDropSpotID) {
			GameState.setCaldoriaMadeOJ(false);
			startExtraSequence(kDisposeOrangeJuice, kExtraCompletedFlag, kFilterNoInput);
		}
		break;
	case kCardBomb:
		GameState.setCaldoriaDoorBombed(true);
		setCurrentActivation(kActivateHotSpotAlways);
		Neighborhood::dropItemIntoRoom(item, dropSpot);
		// Long enough for AI hints...?
		_utilityFuse.primeFuse(kCardBombCountDownTime);
		_utilityFuse.setFunctor(new Common::Functor0Mem<void, Caldoria>(this, &Caldoria::doorBombTimerExpired));
		_utilityFuse.lightFuse();
		GameState.setCaldoriaFuseTimeLimit(kCardBombCountDownTime);
		loopCroppedMovie("Images/Caldoria/A48 Bomb Loop", kCaldoria48CardBombLoopLeft, kCaldoria48CardBombLoopTop);
		if (_vm->isDVD()) {
			InputDevice.getInput(input, kFilterAllInput);
			if (JMPPPInput::isEasterEggModifierInput(input))
				cardBombVoice.initFromAIFFFile("Sounds/Caldoria/Card Bomb.Geno.aiff");
			else
				cardBombVoice.initFromAIFFFile("Sounds/Caldoria/Card Bomb.44K.aiff");
			cardBombVoice.setVolume(_vm->getSoundFXLevel());
			cardBombVoice.playSound();
			while (cardBombVoice.isPlaying() && !_vm->shouldQuit()) {
				InputDevice.getInput(input, kFilterNoInput);

				_vm->checkCallBacks();
				_vm->refreshDisplay();
				_vm->_system->delayMillis(10);
			}
			if (_vm->shouldQuit())
				return;
		}
		GameState.setScoringUsedCardBomb(true);
		if (g_arthurChip)
			g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA19", kArthurCaldoriaUsedCardBomb);
		break;
	case kStunGun:
		GameState.setCaldoriaGunAimed(true);
		GameState.setCaldoriaSinclairShot(true);
		_gunSprite = item->getDragSprite(0);
		_gunSprite->setCurrentFrameIndex(1);
		_gunSprite->setDisplayOrder(kDragSpriteOrder);
		_gunSprite->moveElementTo(kCaldoriaGunSpriteLeft, kCaldoriaGunSpriteTop);
		_gunSprite->startDisplaying();
		_gunSprite->show();
		if (g_arthurChip)
			g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA17", kArthurCaldoriaStunningSinclair);
		break;
	default:
		Neighborhood::dropItemIntoRoom(item, dropSpot);
		break;
	}
}

void Caldoria::setSoundFXLevel(const uint16 level) {
	Neighborhood::setSoundFXLevel(level);

	if (_extraMovie.isMovieValid())
		_extraMovie.setVolume(level);
}

void Caldoria::playMissingFloorSound() {
	Input input;
	Sound elevatorVoice;

	InputDevice.getInput(input, kFilterAllInput);
	if (_vm->isDVD() && JMPPPInput::isEasterEggModifierInput(input)) {
		_vm->_cursor->hide();
		elevatorVoice.initFromAIFFFile("Sounds/Caldoria/Elevator Denied.32K.aiff");
		elevatorVoice.setVolume(_vm->getSoundFXLevel());
		elevatorVoice.playSound();
		while (elevatorVoice.isPlaying() && !_vm->shouldQuit()) {
			InputDevice.getInput(input, kFilterNoInput);

			_vm->checkCallBacks();
			_vm->refreshDisplay();
			_vm->_system->delayMillis(10);
		}
		if (_vm->shouldQuit())
			return;
		_vm->_cursor->hideUntilMoved();
		updateElevatorMovie();
	} else {
		requestSpotSound(kCaldoriaNoOtherFloorIn,
							kCaldoriaNoOtherFloorOut,
							kFilterNoInput,
							kSpotSoundCompletedFlag);
	}
}

void Caldoria::takeElevator(uint startFloor, uint endFloor) {
	_croppedMovie.stop();
	_croppedMovie.setSegment(0, _croppedMovie.getDuration());

	switch (startFloor) {
	case 1:
		switch (endFloor) {
		case 1:
			// Do nothing.
			break;
		case 2:
			_croppedMovie.setTime(k1To2Time);
			_croppedMovie.redrawMovieWorld();
			playMissingFloorSound();
			break;
		case 3:
			_croppedMovie.setTime(k1To3Time);
			_croppedMovie.redrawMovieWorld();
			playMissingFloorSound();
			break;
		case 4:
			_croppedMovie.setSegment(k1To4Start, k1To4Stop);
			_croppedMovie.setTime(k1To4Start);
			startExtraSequence(kCaldoriaGroundToFourth, kExtraCompletedFlag, false);
			_croppedMovie.start();
			break;
		case 5:
			_croppedMovie.setSegment(k1To5Start, k1To5Stop);
			_croppedMovie.setTime(k1To5Start);
			startExtraSequence(kCaldoriaGroundToRoof, kExtraCompletedFlag, false);
			_croppedMovie.start();
			break;
		default:
			break;
		}
		break;
	case 4:
		switch (endFloor) {
		case 1:
			_croppedMovie.setSegment(k4To1Start, k4To1Stop);
			_croppedMovie.setTime(k4To1Start);
			startExtraSequence(kCaldoriaFourthToGround, kExtraCompletedFlag, false);
			_croppedMovie.start();
			break;
		case 2:
			_croppedMovie.setTime(k4To2Time);
			_croppedMovie.redrawMovieWorld();
			playMissingFloorSound();
			break;
		case 3:
			_croppedMovie.setTime(k4To3Time);
			_croppedMovie.redrawMovieWorld();
			playMissingFloorSound();
			break;
		case 4:
			// Do nothing.
			break;
		case 5:
			_croppedMovie.setSegment(k4To5Start, k4To5Stop);
			_croppedMovie.setTime(k4To5Start);
			startExtraSequence(kCaldoriaFourthToRoof, kExtraCompletedFlag, false);
			_croppedMovie.start();
			break;
		default:
			break;
		}
		break;
	case 5:
		switch (endFloor) {
		case 1:
			_croppedMovie.setSegment(k5To1Start, k5To1Stop);
			_croppedMovie.setTime(k5To1Start);
			startExtraSequence(kCaldoriaRoofToGround, kExtraCompletedFlag, false);
			_croppedMovie.start();
			break;
		case 2:
			_croppedMovie.setTime(k5To2Time);
			_croppedMovie.redrawMovieWorld();
			playMissingFloorSound();
			break;
		case 3:
			_croppedMovie.setTime(k5To3Time);
			_croppedMovie.redrawMovieWorld();
			playMissingFloorSound();
			break;
		case 4:
			_croppedMovie.setSegment(k5To4Start, k5To4Stop);
			_croppedMovie.setTime(k5To4Start);
			startExtraSequence(kCaldoriaRoofToFourth, kExtraCompletedFlag, false);
			_croppedMovie.start();
			break;
		case 5:
			// Do nothing.
			break;
		default:
			break;
		}
		break;
	default:
		break;
	};
}

void Caldoria::updateElevatorMovie() {
	TimeValue time = 0xffffffff;

	if (GameState.getCurrentDirection() == kNorth) {
		switch (GameState.getCurrentRoom()) {
		case kCaldoria27:
			time = k4FloorTime;
			break;
		case kCaldoria28:
			time = k1FloorTime;
			break;
		case kCaldoria45:
			time = k5FloorTime;
			break;
		default:
			break;
		}
	}

	_croppedMovie.stop();

	if (time == 0xffffffff) {
		_croppedMovie.hide();
	} else {
		_croppedMovie.stop();
		_croppedMovie.setSegment(0, _croppedMovie.getDuration());
		_croppedMovie.setTime(time);
		_croppedMovie.redrawMovieWorld();
		_croppedMovie.show();

		// *** Why do I need this?
		// clone2727: "don't ask me!"
		_navMovie.redrawMovieWorld();
	}
}

void Caldoria::openElevatorMovie() {
	if (!_croppedMovie.isSurfaceValid())
		openCroppedMovie("Images/Caldoria/Caldoria Elevator.movie", kCaldoriaElevatorLeft, kCaldoriaElevatorTop);

	updateElevatorMovie();
}

void Caldoria::emptyOJGlass() {
	GameState.setTakenItemID(kOrangeJuiceGlassFull, false);
	GameState.setTakenItemID(kOrangeJuiceGlassEmpty, true);
	_vm->removeItemFromInventory((InventoryItem *)_vm->getAllItems().findItemByID(kOrangeJuiceGlassFull));
	_vm->addItemToInventory((InventoryItem *)_vm->getAllItems().findItemByID(kOrangeJuiceGlassEmpty));
}

void Caldoria::doorBombTimerExpired() {
	closeCroppedMovie();

	if (GameState.getShieldOn()) {
		_vm->getCurrentBiochip()->setItemState(kShieldCardBomb);
		setCurrentAlternate(kAltCaldoriaRoofDoorBlown);
		startExtraSequence(kCa48NorthExplosion, kExtraCompletedFlag, kFilterNoInput);
		GameState.setScoringShieldedCardBomb(true);
		GameState.setCaldoriaDoorBombed(false);
		GameState.setCaldoriaRoofDoorOpen(true);
	} else {
		playDeathExtra(kCa48NorthExplosionDeath, kDeathCardBomb);
	}
}

void Caldoria::sinclairTimerExpired() {
	_privateFlags.setFlag(kCaldoriaPrivateSinclairTimerExpiredFlag, true);
	checkSinclairShootsOS();
}

void Caldoria::checkSinclairShootsOS() {
	if (_privateFlags.getFlag(kCaldoriaPrivateSinclairTimerExpiredFlag))
		switch (GameState.getCurrentRoomAndView()) {
		case MakeRoomView(kCaldoria49, kNorth):
		case MakeRoomView(kCaldoria49, kSouth):
		case MakeRoomView(kCaldoria49, kEast):
		case MakeRoomView(kCaldoria49, kWest):
		case MakeRoomView(kCaldoria50, kSouth):
		case MakeRoomView(kCaldoria50, kEast):
		case MakeRoomView(kCaldoria50, kWest):
		case MakeRoomView(kCaldoria51, kNorth):
		case MakeRoomView(kCaldoria51, kSouth):
		case MakeRoomView(kCaldoria51, kWest):
		case MakeRoomView(kCaldoria52, kNorth):
		case MakeRoomView(kCaldoria52, kSouth):
		case MakeRoomView(kCaldoria52, kWest):
		case MakeRoomView(kCaldoria53, kNorth):
		case MakeRoomView(kCaldoria53, kSouth):
		case MakeRoomView(kCaldoria53, kWest):
		case MakeRoomView(kCaldoria54, kNorth):
		case MakeRoomView(kCaldoria54, kEast):
		case MakeRoomView(kCaldoria54, kWest):
			playSpotSoundSync(kCaldoriaSinclairShootsOSIn, kCaldoriaSinclairShootsOSOut);
			playSpotSoundSync(kCaldoriaScreamingAfterIn, kCaldoriaScreamingAfterOut);
			die(kDeathSinclairShotDelegate);
			break;
		default:
			break;
		}
}

void Caldoria::checkInterruptSinclair() {
	if (GameState.getCaldoriaSinclairShot()) {
		_navMovie.stop();
		_neighborhoodNotification.setNotificationFlags(kExtraCompletedFlag, kExtraCompletedFlag);
		g_AIArea->unlockAI();
	} else {
		uint32 currentTime = _navMovie.getTime();

		ExtraTable::Entry entry;
		getExtraEntry(kCa53EastZoomToSinclair, entry);

		if (currentTime < entry.movieStart + kSinclairInterruptionTime2)
			_sinclairInterrupt.scheduleCallBack(kTriggerTimeFwd, entry.movieStart + kSinclairInterruptionTime2,
					_navMovie.getScale());
		else if (currentTime < entry.movieStart + kSinclairInterruptionTime3)
			_sinclairInterrupt.scheduleCallBack(kTriggerTimeFwd, entry.movieStart + kSinclairInterruptionTime3,
					_navMovie.getScale());
		else if (currentTime < entry.movieStart + kSinclairInterruptionTime4)
			_sinclairInterrupt.scheduleCallBack(kTriggerTimeFwd, entry.movieStart + kSinclairInterruptionTime4,
					_navMovie.getScale());
	}
}

Common::Path Caldoria::getBriefingMovie() {
	Common::Path movieName = Neighborhood::getBriefingMovie();

	if (movieName.empty()) {
		if (GameState.allTimeZonesFinished())
			return "Images/AI/Caldoria/XA02";

		return "Images/AI/Caldoria/XA01";
	}

	return movieName;
}

Common::Path Caldoria::getEnvScanMovie() {
	Common::Path movieName = Neighborhood::getEnvScanMovie();

	if (movieName.empty()) {
		RoomID room = GameState.getCurrentRoom();

		if (room >= kCaldoria00 && room <= kCaldoria14) {
			// Inside apartment.
			if (GameState.getCaldoriaDoneHygiene())
				return "Images/AI/Caldoria/XAE2";

			return "Images/AI/Caldoria/XAE1";
		} else if (room >= kCaldoria15 && room <= kCaldoria48) {
			// Wandering the halls...
			return "Images/AI/Caldoria/XAE3";
		} else {
			// Must be the roof.
			return "Images/AI/Caldoria/XAEH2";
		}
	}

	return movieName;
}

uint Caldoria::getNumHints() {
	uint numHints = Neighborhood::getNumHints();

	if (numHints == 0) {
		switch (GameState.getCurrentRoomAndView()) {
		case MakeRoomView(kCaldoria44, kEast):
			if (!GameState.isTakenItemID(kKeyCard) && GameState.getOpenDoorRoom() == kNoRoomID)
				numHints = 1;
			break;
		case MakeRoomView(kCaldoria48, kNorth):
			if (!GameState.getCaldoriaRoofDoorOpen()) {
				if (_croppedMovie.isRunning()) // Bomb must be looping.
					numHints = 3;
				else if (GameState.isTakenItemID(kCardBomb))
					numHints = 1;
			}
			break;
#if 0
		// The hint file is missing
		case MakeRoomView(kCaldoria49, kEast):
		case MakeRoomView(kCaldoria54, kEast):
			numHints = 1;
			break;
#endif
		case MakeRoomView(kCaldoria49, kNorth):
			numHints = 1;
			break;
		default:
			break;
		}
	}

	return numHints;
}

Common::Path Caldoria::getHintMovie(uint hintNum) {
	Common::Path movieName = Neighborhood::getHintMovie(hintNum);

	if (movieName.empty()) {
		switch (GameState.getCurrentRoomAndView()) {
		case MakeRoomView(kCaldoria44, kEast):
			return "Images/AI/Caldoria/X42WH2";
		case MakeRoomView(kCaldoria48, kNorth):
			if (_croppedMovie.isRunning()) { // Bomb must be looping.
				if (hintNum == 1)
					return "Images/AI/Caldoria/X48ND1";
				else if (hintNum == 2)
					return "Images/AI/Caldoria/X48ND2";
				else if (GameState.isTakenItemID(kShieldBiochip))
					return "Images/AI/Caldoria/X48ND3";

				// *** Doesn't work yet, need global movies.
				break;
			}

			return "Images/AI/Globals/XGLOB1A";
#if 0
		// The hint file is missing
		case MakeRoomView(kCaldoria49, kEast):
		case MakeRoomView(kCaldoria54, kEast):
			return "Images/AI/Caldoria/X49E";
#endif
		case MakeRoomView(kCaldoria49, kNorth):
			return "Images/AI/Caldoria/X49NB2";
		default:
			break;
		}
	}

	return movieName;
}

void Caldoria::updateCursor(const Common::Point &where, const Hotspot *cursorSpot) {
	if (cursorSpot) {
		switch (cursorSpot->getObjectID()) {
		case kCa4DEnvironCloseSpotID:
			_vm->_cursor->setCurrentFrameIndex(2);
			return;
		case kCaldoriaKioskSpotID:
			_vm->_cursor->setCurrentFrameIndex(3);
			return;
		default:
			break;
		}
	}

	Neighborhood::updateCursor(where, cursorSpot);
}

Common::Path Caldoria::getNavMovieName() {
	return "Images/Caldoria/Caldoria.movie";
}

Common::Path Caldoria::getSoundSpotsName() {
	return "Sounds/Caldoria/Caldoria Spots";
}

void Caldoria::doArthurJoyride() {
	Video::VideoDecoder *video = nullptr;
	BiochipItem *item;

	setNextHandler(_vm);
	throwAwayInterface();
	loadLoopSound1("");

	_vm->_cursor->hide();

#ifdef USE_THEORADEC
	video = new Video::TheoraDecoder();
	if (!video->loadFile("Images/Caldoria/A12RD.ogg")) {
		delete video;
		video = nullptr;
	}
#endif

	if (!video) {
		video = new Video::QuickTimeDecoder();
		if (!video->loadFile("Images/Caldoria/A12RD.movie"))
			error("Could not load joyride video");
	}

	video->setVolume(MIN<uint>(_vm->getSoundFXLevel(), 0xFF));
	video->start();

	while (!_vm->shouldQuit() && !video->endOfVideo()) {
		if (video->needsUpdate()) {
			const Graphics::Surface *frame = video->decodeNextFrame();

			if (frame) {
				g_system->copyRectToScreen((const byte *)frame->getPixels(), frame->pitch, 0, 0, frame->w, frame->h);
				g_system->updateScreen();
			}
		}

		InputDevice.pumpEvents();

		g_system->delayMillis(10);
	}

	delete video;

	if (_vm->shouldQuit())
		return;

	reinstateMonocleInterface();
	loadAmbientLoops();
	updateViewFrame();
	if (!_vm->playerHasItemID(kArthurBiochip)) {
		item = (BiochipItem *)_vm->getAllItems().findItemByID(kArthurBiochip);
		_vm->addItemToBiochips(item);
		if (g_arthurChip)
			g_arthurChip->playArthurMovieForEvent("Images/AI/Globals/XGLOBA06", kArthurCaldoriaFinishedJoyride);
	}
	g_interface->setCurrentBiochipID(kArthurBiochip);
	GameState.setEasterEgg(false);
}

} // End of namespace Pegasus
