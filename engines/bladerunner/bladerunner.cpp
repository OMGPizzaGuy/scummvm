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
#include "bladerunner/bladerunner.h"

#include "bladerunner/actor.h"
#include "bladerunner/actor_dialogue_queue.h"
#include "bladerunner/ambient_sounds.h"
#include "bladerunner/audio_cache.h"
#include "bladerunner/audio_mixer.h"
#include "bladerunner/audio_player.h"
#include "bladerunner/audio_speech.h"
#include "bladerunner/chapters.h"
#include "bladerunner/combat.h"
#include "bladerunner/crimes_database.h"
#include "bladerunner/debugger.h"
#include "bladerunner/dialogue_menu.h"
#include "bladerunner/framelimiter.h"
#include "bladerunner/font.h"
#include "bladerunner/game_flags.h"
#include "bladerunner/game_info.h"
#include "bladerunner/image.h"
#include "bladerunner/item_pickup.h"
#include "bladerunner/items.h"
#include "bladerunner/lights.h"
#include "bladerunner/mouse.h"
#include "bladerunner/music.h"
#include "bladerunner/outtake.h"
#include "bladerunner/obstacles.h"
#include "bladerunner/overlays.h"
#include "bladerunner/regions.h"
#include "bladerunner/savefile.h"
#include "bladerunner/scene.h"
#include "bladerunner/scene_objects.h"
#include "bladerunner/screen_effects.h"
#include "bladerunner/set.h"
#include "bladerunner/script/ai_script.h"
#include "bladerunner/script/init_script.h"
#include "bladerunner/script/kia_script.h"
#include "bladerunner/script/police_maze.h"
#include "bladerunner/script/scene_script.h"
#include "bladerunner/settings.h"
#include "bladerunner/shape.h"
#include "bladerunner/slice_animations.h"
#include "bladerunner/slice_renderer.h"
#include "bladerunner/subtitles.h"
#include "bladerunner/suspects_database.h"
#include "bladerunner/text_resource.h"
#include "bladerunner/time.h"
#include "bladerunner/ui/elevator.h"
#include "bladerunner/ui/end_credits.h"
#include "bladerunner/ui/esper.h"
#include "bladerunner/ui/kia.h"
#include "bladerunner/ui/scores.h"
#include "bladerunner/ui/spinner.h"
#include "bladerunner/ui/vk.h"
#include "bladerunner/vqa_decoder.h"
#include "bladerunner/waypoints.h"
#include "bladerunner/zbuffer.h"

#include "backends/keymapper/action.h"
#include "backends/keymapper/keymapper.h"

#include "common/array.h"
#include "common/config-manager.h"
#include "common/error.h"
#include "common/events.h"
#include "common/savefile.h"
#include "common/system.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/translation.h"
#include "common/compression/unzip.h"

#include "gui/message.h"

#include "engines/util.h"
#include "engines/advancedDetector.h"

#include "graphics/thumbnail.h"
#include "audio/mididrv.h"

namespace BladeRunner {

const char *BladeRunnerEngine::kGameplayKeymapId = "bladerunner-gameplay";
const char *BladeRunnerEngine::kKiaKeymapId = "bladerunner-kia";
const char *BladeRunnerEngine::kCommonKeymapId = "bladerunner-common";

BladeRunnerEngine::BladeRunnerEngine(OSystem *syst, const ADGameDescription *desc)
	: Engine(syst),
	  _rnd("bladerunner") {
	_newGameRandomSeed = _rnd.getSeed();

	_windowIsActive     = true;
	_gameIsRunning      = true;
	_gameJustLaunched   = true;

	_vqaIsPlaying       = false;
	_vqaStopIsRequested = false;

	_actorIsSpeaking           = false;
	_actorSpeakStopIsRequested = false;

	_subtitlesEnabled             = false;
	_showSubtitlesForTextCrawl    = false;

	_surfaceFrontCreated          = false;
	_surfaceBackCreated           = false;

	_sitcomMode                   = false;
	_shortyMode                   = false;
	_noDelayMillisFramelimiter    = false;
	_framesPerSecondMax           = false;
	_disableStaminaDrain          = false;
	_spanishCreditsCorrection     = false;
	_cutContent                   = Common::String(desc->gameId).contains("bladerunner-final");
	_enhancedEdition              = Common::String(desc->gameId).contains("bladerunner-ee");
	_validBootParam               = false;

	_playerLosesControlCounter = 0;
	_extraCPos = 0;
	_extraCNotify = 0;

	_playerActorIdle = false;
	_playerDead      = false;

	_gameOver               = false;
	_gameAutoSaveTextId     = -1;
	_gameIsAutoSaving       = false;
	_gameIsLoading          = false;
	_sceneIsLoading         = false;

	_runningActorId         = -1;
	_isWalkingInterruptible = false;
	_interruptWalking       = false;

	_walkSoundId      = -1;
	_walkSoundVolume  = 0;
	_walkSoundPan     = 0;

	_language = desc->language;
	switch (desc->language) {
	case Common::EN_ANY:
		_languageCode = "E";
		break;
	case Common::DE_DEU:
		_languageCode = "G";
		break;
	case Common::FR_FRA:
		_languageCode = "F";
		break;
	case Common::IT_ITA:
		_languageCode = "I";
		break;
	case Common::RU_RUS:
		_languageCode = "E"; // Russian version is built on top of English one
		break;
	case Common::ES_ESP:
		_languageCode = "S";
		break;
	default:
		_languageCode = "E";
	}

	_screenEffects           = nullptr;
	_combat                  = nullptr;
	_actorDialogueQueue      = nullptr;
	_settings                = nullptr;
	_itemPickup              = nullptr;
	_lights                  = nullptr;
	_obstacles               = nullptr;
	_sceneScript             = nullptr;
	_time                    = nullptr;
	_framelimiter            = nullptr;
	_gameInfo                = nullptr;
	_waypoints               = nullptr;
	_gameVars                = nullptr;
	_cosTable1024            = nullptr;
	_sinTable1024            = nullptr;
	_view                    = nullptr;
	_sceneObjects            = nullptr;
	_gameFlags               = nullptr;
	_items                   = nullptr;
	_audioCache              = nullptr;
	_audioMixer              = nullptr;
	_audioPlayer             = nullptr;
	_music                   = nullptr;
	_audioSpeech             = nullptr;
	_ambientSounds           = nullptr;
	_chapters                = nullptr;
	_overlays                = nullptr;
	_zbuffer                 = nullptr;
	_playerActor             = nullptr;
	_textActorNames          = nullptr;
	_textCrimes              = nullptr;
	_textClueTypes           = nullptr;
	_textKIA                 = nullptr;
	_textSpinnerDestinations = nullptr;
	_textVK                  = nullptr;
	_textOptions             = nullptr;
	_dialogueMenu            = nullptr;
	_suspectsDatabase        = nullptr;
	_kia                     = nullptr;
	_endCredits              = nullptr;
	_spinner                 = nullptr;
	_scores                  = nullptr;
	_elevator                = nullptr;
	_mainFont                = nullptr;
	_subtitles               = nullptr;
	_esper                   = nullptr;
	_vk                      = nullptr;
	_policeMaze              = nullptr;
	_mouse                   = nullptr;
	_sliceAnimations         = nullptr;
	_sliceRenderer           = nullptr;
	_crimesDatabase          = nullptr;
	_scene                   = nullptr;
	_aiScripts               = nullptr;
	_shapes                  = nullptr;
	for (int i = 0; i != kActorCount; ++i) {
		_actors[i]           = nullptr;
	}
	_debugger                = nullptr;

	walkingReset();

	_actorUpdateCounter  = 0;
	_actorUpdateTimeLast = 0;

	_currentKeyDown.keycode = Common::KEYCODE_INVALID;
	_keyRepeatTimeLast = 0;
	_keyRepeatTimeDelay = 0;

	_activeCustomEvents.clear();
	_customEventRepeatTimeLast = 0;
	_customEventRepeatTimeDelay = 0;

	_isNonInteractiveDemo = desc->flags & ADGF_DEMO;

	_archive = nullptr;
}

BladeRunnerEngine::~BladeRunnerEngine() {
	shutdown();
}

bool BladeRunnerEngine::hasFeature(EngineFeature f) const {
	return
		f == kSupportsReturnToLauncher ||
		f == kSupportsLoadingDuringRuntime ||
		f == kSupportsSavingDuringRuntime;
}

bool BladeRunnerEngine::canLoadGameStateCurrently(Common::U32String *msg) {
	return
		playerHasControl() &&
		_gameIsRunning &&
		!_isNonInteractiveDemo &&
		!_actorIsSpeaking &&
		!_vqaIsPlaying &&
		!_gameJustLaunched &&
		!_sceneScript->isInsideScript() &&
		!_aiScripts->isInsideScript() &&
		!_kia->isOpen() &&
		!_spinner->isOpen() &&
		!_vk->isOpen() &&
		!_elevator->isOpen();
}

Common::Error BladeRunnerEngine::loadGameState(int slot) {
	Common::InSaveFile *saveFile = BladeRunner::SaveFileManager::openForLoading(_targetName, slot);
	if (saveFile == nullptr || saveFile->err()) {
		delete saveFile;
		return Common::kReadingFailed;
	}

	BladeRunner::SaveFileHeader header;
	if (!BladeRunner::SaveFileManager::readHeader(*saveFile, header)) {
		error("Invalid savegame");
	}

	setTotalPlayTime(header._playTime);
	// this essentially does something similar with setTotalPlayTime
	// resetting and updating Blade Runner's _pauseStart and offset before starting a loaded game
	_time->resetPauseStart();

	loadGame(*saveFile, header._version);

	delete saveFile;

	return Common::kNoError;
}

bool BladeRunnerEngine::canSaveGameStateCurrently(Common::U32String *msg) {
	return
		playerHasControl() &&
		_gameIsRunning &&
		!_isNonInteractiveDemo &&
		!_actorIsSpeaking &&
		!_vqaIsPlaying &&
		!_gameJustLaunched &&
		!_sceneScript->isInsideScript() &&
		!_aiScripts->isInsideScript() &&
		!_kia->isOpen() &&
		!_spinner->isOpen() &&
		!_vk->isOpen() &&
		!_elevator->isOpen();
}

Common::Error BladeRunnerEngine::saveGameState(int slot, const Common::String &desc, bool isAutosave) {
	Common::OutSaveFile *saveFile = BladeRunner::SaveFileManager::openForSaving(_targetName, slot);
	if (saveFile == nullptr || saveFile->err()) {
		delete saveFile;
		return Common::kReadingFailed;
	}

	BladeRunner::SaveFileHeader header;
	header._name = desc;
	header._playTime = getTotalPlayTime();

	BladeRunner::SaveFileManager::writeHeader(*saveFile, header);
	_time->pause();
	saveGame(*saveFile);
	_time->resume();

	saveFile->finalize();

	delete saveFile;

	return Common::kNoError;
}

void BladeRunnerEngine::pauseEngineIntern(bool pause) {
	_mixer->pauseAll(pause);
}

Common::Error BladeRunnerEngine::run() {
	Common::Array<Common::String> missingFiles;
	const Common::FSNode gameDataDir(ConfMan.getPath("path"));
	SearchMan.addSubDirectoryMatching(gameDataDir, "base");
	SearchMan.addSubDirectoryMatching(gameDataDir, "cd1");
	SearchMan.addSubDirectoryMatching(gameDataDir, "cd2");
	SearchMan.addSubDirectoryMatching(gameDataDir, "cd3");
	SearchMan.addSubDirectoryMatching(gameDataDir, "cd4");
	if (!_isNonInteractiveDemo && !checkFiles(missingFiles)) {
		Common::String missingFileStr = "";
		for (uint i = 0; i < missingFiles.size(); ++i) {
			if (i > 0) {
				missingFileStr += ", ";
			}
			missingFileStr += missingFiles[i];
		}
		// shutting down
		return Common::Error(Common::kNoGameDataFoundError, missingFileStr);
	}

	int16 gameBRWidth = kOriginalGameWidth;
	int16 gameBRHeight = kOriginalGameHeight;
	if (_isNonInteractiveDemo) {
		if (Common::File::exists("SIZZLE2.VQP")) {
			gameBRWidth = kDemoGameWidth * 2;
			gameBRHeight = kDemoGameHeight * 2;
		} else {
			gameBRWidth = kDemoGameWidth;
			gameBRHeight = kDemoGameHeight;
		}
	}

	initGraphics(gameBRWidth, gameBRHeight, nullptr);
	_screenPixelFormat = g_system->getScreenFormat();
	debug("Using pixel format: %s", _screenPixelFormat.toString().c_str());

	_system->showMouse(_isNonInteractiveDemo ? false : true);

	bool hasSavegames = !SaveFileManager::list(getMetaEngine(), _targetName).empty();

	if (!startup(hasSavegames)) {
		// shutting down
		return Common::Error(Common::kUnknownError, _("Failed to initialize resources"));
	}

	if (_isNonInteractiveDemo) {
		_gameOver         = false;
		_gameIsRunning    = true;
		_gameJustLaunched = true;

		if (getEventManager()->getKeymapper() != nullptr) {
			if (getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kGameplayKeymapId) != nullptr) {
				getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kGameplayKeymapId)->setEnabled(true);
				const Common::Keymap::ActionArray karr = getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kGameplayKeymapId)->getActions();
				for (uint8 i = 0; i < karr.size(); ++i) {
					if (karr[i]->description == "COMBAT"
					    || karr[i]->description == "SKIPDLG"
					    || karr[i]->description == "KIADB") {
						getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kGameplayKeymapId)->unregisterMapping(karr[i]);
					}
				}
			}
		}

		// Required for calls in OuttakePlayer::play() of playerLosesControl(), playerGainsControl()
		_mouse = new Mouse(this);
		_mouse->disable();

		// Speech Sound Type (kSpeechSoundType) is the volume of the outtake video,
		// so we don't mute that one
		_mixer->setVolumeForSoundType(_mixer->kMusicSoundType, 0);
		_mixer->setVolumeForSoundType(_mixer->kPlainSoundType, 0);
		_mixer->setVolumeForSoundType(_mixer->kSFXSoundType, 0);
		int vqpCompanionPresenceId = Common::File::exists("SIZZLE2.VQP")? -2 : -3;
		outtakePlay("SIZZLE2", true, vqpCompanionPresenceId);
		// shutting down
		return Common::kNoError;
	}

	// Improvement: Use a do-while() loop to handle the normal end-game state
	// so that the game won't exit abruptly after end credits
	do {
		// additional code for gracefully handling end-game after _endCredits->show()
		_gameOver         = false;
		_gameIsRunning    = true;
		_gameJustLaunched = true;
		// reset ammo amounts
		_settings->reset();
		// clear subtitles
		_subtitles->clear();
		// need to clear kFlagKIAPrivacyAddon to remove Bob's Privacy Addon for KIA
		// so it won't appear here after end credits
		_gameFlags->reset(kFlagKIAPrivacyAddon);
		if (!playerHasControl()) {
			// force a player gains control
			playerGainsControl(true);
		}
		if (_mouse->isDisabled()) {
			// force a mouse enable here since otherwise, after end-game,
			// we need extra call(s) to mouse->enable to get the _disabledCounter to 0
			_mouse->enable(true);
		}
		// end of additional code for gracefully handling end-game

		if (getEventManager()->getKeymapper() != nullptr) {
			if (getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kCommonKeymapId) != nullptr)
				getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kCommonKeymapId)->setEnabled(true);

			if (getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kGameplayKeymapId) != nullptr)
				getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kGameplayKeymapId)->setEnabled(true);

			if (getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kKiaKeymapId) != nullptr) {
				// When disabling a keymap, make sure all their active events in the _activeCustomEvents array
				// are cleared, because they won't get an explicit "EVENT_CUSTOM_ENGINE_ACTION_END" event.
				cleanupPendingRepeatingEvents(BladeRunnerEngine::kKiaKeymapId);
				getEventManager()->getKeymapper()->getKeymap(BladeRunnerEngine::kKiaKeymapId)->setEnabled(false);
			}
		}

		if (_validBootParam) {
			// clear the flag, so that after a possible game gameOver / end-game
			// it won't be true again; just to be safe and avoid potential side-effects
			_validBootParam = false;
		} else {
			if (ConfMan.hasKey("save_slot") && ConfMan.getInt("save_slot") != -1) {
				// when loading from ScummVM main menu, we should emulate
				// the Kia pause/resume in order to get a valid "current" time when the game
				// is actually loaded (assuming delays can be introduced by a popup warning dialogue)
				if (!_time->isLocked()) {
					_time->pause();
				}
				loadGameState(ConfMan.getInt("save_slot"));
				ConfMan.set("save_slot", "-1");
				if (_time->isLocked()) {
					_time->resume();
				}
			} else if (hasSavegames) {
				_kia->_forceOpen = true;
				_kia->open(kKIASectionLoad);
			} else {
				// Despite the redundancy (wrt initializations done in startup()),
				// newGame() also does some additional setting up explicitly,
				// so better to keep this here (helps with code readability too
				// and with using the proper seed for randomization).
				newGame(kGameDifficultyMedium);
			}
		}
		gameLoop();

		_mouse->disable();

		if (_gameOver) {
			// In the original game this created a single "END_GAME_STATE.END"
			// which had the a valid format of a save game but was never accessed
			// from the loading screen. (Due to the .END extension)
			// It was also a single file that was overwritten each time the player
			// finished the game.
			// Maybe its purpose was debugging (?) by renaming it to .SAV and also
			// for the game to "know" if the player has already finished the game at least once (?)
			// although that latter one seems not to be used for anything, or maybe it was planned
			// to be used for a sequel (?). We will never know.
			// Disabling as in current state it will only fill-up save slots
			// autoSaveGame(4, true);
			_endCredits->show();
		}
	} while (_gameOver); // if main game loop ended and _gameOver == false, then shutdown

	// shutting down
	return Common::kNoError;
}

bool BladeRunnerEngine::checkFiles(Common::Array<Common::String> &missingFiles) {
	missingFiles.clear();

	Common::Array<const char *> requiredFiles;

	if (_enhancedEdition) {
		requiredFiles.push_back("BladeRunner.kpf");
	} else {
		requiredFiles.push_back("1.TLK");
		requiredFiles.push_back("2.TLK");
		requiredFiles.push_back("3.TLK");
		requiredFiles.push_back("A.TLK");
		requiredFiles.push_back("MODE.MIX");
		requiredFiles.push_back("MUSIC.MIX");
		requiredFiles.push_back("OUTTAKE1.MIX");
		requiredFiles.push_back("OUTTAKE2.MIX");
		requiredFiles.push_back("OUTTAKE3.MIX");
		requiredFiles.push_back("OUTTAKE4.MIX");
		requiredFiles.push_back("SFX.MIX");
		requiredFiles.push_back("SPCHSFX.TLK");
		requiredFiles.push_back("STARTUP.MIX");
		requiredFiles.push_back("VQA1.MIX");
		requiredFiles.push_back("VQA2.MIX");
		requiredFiles.push_back("VQA3.MIX");
	}
	requiredFiles.push_back("COREANIM.DAT");

	for (uint i = 0; i < requiredFiles.size(); ++i) {
		if (!Common::File::exists(requiredFiles[i])) {
			missingFiles.push_back(requiredFiles[i]);
		}
	}

	bool hasHdFrames = Common::File::exists("HDFRAMES.DAT");

	if (!hasHdFrames) {
		for (uint i = 1; i <= 4; ++i) {
			if (!Common::File::exists(Common::Path(Common::String::format("CDFRAMES%d.DAT", i))) && !Common::File::exists(Common::Path(Common::String::format("CD%d/CDFRAMES.DAT", i)))) {
				missingFiles.push_back(Common::String::format("CD%d/CDFRAMES.DAT", i));
			}
		}
	}

	return missingFiles.empty();
}

bool BladeRunnerEngine::startup(bool hasSavegames) {
	// Assign default values to the ScummVM configuration manager, in case settings are missing
	ConfMan.registerDefault("sfx_volume", 192);
	ConfMan.registerDefault("music_volume", 192);
	ConfMan.registerDefault("speech_volume", 192);
	ConfMan.registerDefault("mute", "false");
	ConfMan.registerDefault("speech_mute", "false");
	ConfMan.registerDefault("nodelaymillisfl", "false");
	ConfMan.registerDefault("frames_per_secondfl", "false");

	_noDelayMillisFramelimiter = ConfMan.getBool("nodelaymillisfl");
	_framesPerSecondMax        = ConfMan.getBool("frames_per_secondfl");

	// This is the original startup in the game
	_surfaceFront.create(_system->getWidth(), _system->getHeight(), screenPixelFormat());
	_surfaceFrontCreated = true;
	_surfaceBack.create(_system->getWidth(), _system->getHeight(), screenPixelFormat());
	_surfaceBackCreated = true;

	_time = new Time(this);

//	debug("_framesPerSecondMax:: %s", _framesPerSecondMax? "true" : "false");
	_framelimiter = new Framelimiter(this, _framesPerSecondMax? 120 : 60);

	// Try to load the SUBTITLES.MIX first, before Startup.MIX
	// allows overriding any identically named resources (such as the original font files and as a bonus also the TRE files for the UI and dialogue menu)
	_subtitles = new Subtitles(this);
	if (!_isNonInteractiveDemo) {
		if (MIXArchive::exists("SUBTITLES.MIX")) {
			bool r = openArchive("SUBTITLES.MIX");
			if (!r)
				return false;

			_subtitles->init();
		} else {
			debug("Download SUBTITLES.MIX from ScummVM's website to enable subtitles");
		}
	}

	_audioMixer = new AudioMixer(this);

	_audioPlayer = new AudioPlayer(this);

	_music = new Music(this);

	_audioSpeech = new AudioSpeech(this);

	_ambientSounds = new AmbientSounds(this);

	// Query the selected music device (defaults to MT_AUTO device).
	Common::String selDevStr = ConfMan.hasKey("music_driver") ? ConfMan.get("music_driver") : Common::String("auto");
	MidiDriver::DeviceHandle dev = MidiDriver::getDeviceHandle(selDevStr.empty() ? Common::String("auto") : selDevStr);
	//
	// We just respect the "No Music" choice (or an invalid choice)
	//
	// We're lenient with all the invalid/ irrelevant choices in the Audio Driver dropdown
	// TODO Ideally these controls (OptionsDialog::addAudioControls()) ie. "Music Device" and "Adlib Emulator"
	//      should not appear in games like Blade Runner, since they are largely irrelevant
	//      and may cause confusion when combined/ conflicting with the global settings
	//      which are by default applied, if the user does not explicitly override them.
	_noMusicDriver = (MidiDriver::getMusicType(dev) == MT_NULL || MidiDriver::getMusicType(dev) == MT_INVALID);

	// BLADE.INI was read here, but it was replaced by ScummVM configuration
	//
	syncSoundSettings();

	if (!_isNonInteractiveDemo) {
		ConfMan.registerDefault("subtitles", "true");
		ConfMan.registerDefault("use_crawl_subs", "true");
		ConfMan.registerDefault("sitcom", "false");
		ConfMan.registerDefault("shorty", "false");
		ConfMan.registerDefault("disable_stamina_drain", "false");
		ConfMan.registerDefault("correct_spanish_credits", "false");

		_sitcomMode                = ConfMan.getBool("sitcom");
		_shortyMode                = ConfMan.getBool("shorty");
		_disableStaminaDrain       = ConfMan.getBool("disable_stamina_drain");
		if (_language == Common::ES_ESP) {
			_spanishCreditsCorrection  = ConfMan.getBool("correct_spanish_credits");
		}

		// These are static objects in original game
		_screenEffects = new ScreenEffects(this, 0x8000);

		_endCredits = new EndCredits(this);

		_actorDialogueQueue = new ActorDialogueQueue(this);

		_settings = new Settings(this);

		_itemPickup = new ItemPickup(this);

		_lights = new Lights(this);

		// outtake player was initialized here in the original game - but this is done bit differently

		_obstacles = new Obstacles(this);

		_sceneScript = new SceneScript(this);

		_debugger = new Debugger(this);
		setDebugger(_debugger);

		bool r = _enhancedEdition ? openArchiveEnhancedEdition() : openArchive("STARTUP.MIX");
		if (!r)
			return false;

		_gameInfo = new GameInfo(this);
		if (!_gameInfo)
			return false;

		r = _gameInfo->open("GAMEINFO.DAT");
		if (!r) {
			return false;
		}

		if (hasSavegames) {
			if (!loadSplash()) {
				return false;
			}
		}

		_waypoints = new Waypoints(this, _gameInfo->getWaypointCount());

		_combat = new Combat(this);

		_gameVars = new int[_gameInfo->getGlobalVarCount()]();

		// Seed rand

		_cosTable1024 = new Math::CosineTable(1024); // 10-bits = 1024 points for 2*PI;
		_sinTable1024 = new Math::SineTable(1024);

		_view = new View();

		_sceneObjects = new SceneObjects(this, _view);

		_gameFlags = new GameFlags();
		_gameFlags->setFlagCount(_gameInfo->getFlagCount());

		_items = new Items(this);

		_audioCache = new AudioCache();

		_chapters = new Chapters(this);
		if (!_chapters)
			return false;

		if (!openArchive("MUSIC.MIX"))
			return false;

		if (!openArchive("SFX.MIX"))
			return false;

		if (!openArchive("SPCHSFX.TLK"))
			return false;

		_overlays = new Overlays(this);
		_overlays->init();

		_zbuffer = new ZBuffer();
		_zbuffer->init(kOriginalGameWidth, kOriginalGameHeight);

		int actorCount = (int)_gameInfo->getActorCount();
		assert(actorCount < kActorCount);
		for (int i = 0; i != actorCount; ++i) {
			_actors[i] = new Actor(this, i);
		}
		_actors[kActorVoiceOver] = new Actor(this, kActorVoiceOver);
		_playerActor = _actors[_gameInfo->getPlayerId()];

		_playerActor->setFPS(15); // this seems redundant
#if BLADERUNNER_ORIGINAL_BUGS
		_playerActor->timerStart(kActorTimerRunningStaminaFPS, 200);
#else
		// Make code here similar to the bugfix in newGame in that
		// we only start the timer in vanilla game mode (not Restored Content mode)
		if (!_cutContent) {
			_playerActor->timerStart(kActorTimerRunningStaminaFPS, 200);
		}
#endif // BLADERUNNER_ORIGINAL_BUGS

		_policeMaze = new PoliceMaze(this);

		_textActorNames = new TextResource(this);
		if (!_textActorNames->open("ACTORS"))
			return false;

		_textCrimes = new TextResource(this);
		if (!_textCrimes->open("CRIMES"))
			return false;

		_textClueTypes = new TextResource(this);
		if (!_textClueTypes->open("CLUETYPE"))
			return false;

		_textKIA = new TextResource(this);
		if (!_textKIA->open("KIA"))
			return false;

		_textSpinnerDestinations = new TextResource(this);
		if (!_textSpinnerDestinations->open("SPINDEST"))
			return false;

		_textVK = new TextResource(this);
		if (!_textVK->open("VK"))
			return false;

		_textOptions = new TextResource(this);
		if (!_textOptions->open("OPTIONS"))
			return false;

		_russianCP1251 = ((uint8)_textOptions->getText(0)[0]) == 209;

		_dialogueMenu = new DialogueMenu(this);
		if (!_dialogueMenu->loadResources())
			return false;

		_suspectsDatabase = new SuspectsDatabase(this, _gameInfo->getSuspectCount());

		_kia = new KIA(this);

		_spinner = new Spinner(this);

		_elevator = new Elevator(this);

		_scores = new Scores(this);

		_mainFont = Font::load(this, "KIA6PT.FON", 1, false);

		_shapes = new Shapes(this);
		_shapes->load("SHAPES.SHP");

		_esper = new ESPER(this);

		_vk = new VK(this);

		_mouse = new Mouse(this);
		_mouse->setCursor(0);

		_sliceAnimations = new SliceAnimations(this);
		r = _sliceAnimations->open("INDEX.DAT");
		if (!r)
			return false;

		r = _sliceAnimations->openCoreAnim();
		if (!r) {
			return false;
		}

		_sliceRenderer = new SliceRenderer(this);
		_sliceRenderer->setScreenEffects(_screenEffects);

		_crimesDatabase = new CrimesDatabase(this, "CLUES", _gameInfo->getClueCount());

		_scene = new Scene(this);

		// Load INIT.DLL
		InitScript initScript(this);
		initScript.SCRIPT_Initialize_Game();

		// Load AI-ACT1.DLL
		_aiScripts = new AIScripts(this, actorCount);

		initChapterAndScene();

		// Handle Boot Params here:
		// If this process (loading an explicit set of Chapter, Set and Scene) fails,
		// then the game will keep with the default Chapter, Set and Scene for a New Game
		// as set in the initChapterAndScene() above.
		// If the process succeeds (_bootParam will be true), then in run()
		// we skip auto-starting a New Game proper or showing the KIA to load a saved game / start new game,
		// and go directly to gameLoop() to start the game with the custom settings for Chapter, Set and Scene.
		if (ConfMan.hasKey("boot_param")) {
			int param = ConfMan.getInt("boot_param"); // CTTTSSS
			if (param < 1000000 || param >= 6000000) {
				debug("Invalid boot parameter. Valid format is: CTTTSSS");
			} else {
				int chapter = param / 1000000;
				param -= chapter * 1000000;
				int set = param / 1000;
				param -= set * 1000;
				int scene = param;

				// init chapter to default first chapter (required by dbgAttemptToLoadChapterSetScene())
				_settings->setChapter(1);
				_validBootParam = _debugger->dbgAttemptToLoadChapterSetScene(chapter, set, scene);
				if (_validBootParam) {
					debug("Explicitly loading Chapter: %d Set: %d Scene: %d", chapter, set, scene);
				} else {
					debug("Invalid combination of Chapter Set and Scene ids as boot parameters");
				}
			}
		}

	}
	return true;
}

void BladeRunnerEngine::initChapterAndScene() {
	for (int i = 0, end = _gameInfo->getActorCount(); i != end; ++i) {
		_aiScripts->initialize(i);
	}

	for (int i = 0, end = _gameInfo->getActorCount(); i != end; ++i) {
		_actors[i]->changeAnimationMode(kAnimationModeIdle);
	}

	for (int i = 1, end = _gameInfo->getActorCount(); i != end; ++i) { // skip first actor, probably player
		_actors[i]->movementTrackNext(true);
	}

	_settings->setChapter(1);
	_settings->setNewSetAndScene(_gameInfo->getInitialSetId(), _gameInfo->getInitialSceneId());
}

void BladeRunnerEngine::shutdown() {
	_mixer->stopAll();

	// BLADE.INI as updated here

	delete _aiScripts;
	_aiScripts = nullptr;

	delete _scene;
	_scene = nullptr;

	delete _crimesDatabase;
	_crimesDatabase = nullptr;

	delete _sliceRenderer;
	_sliceRenderer = nullptr;

	delete _sliceAnimations;
	_sliceAnimations = nullptr;

	delete _mouse;
	_mouse = nullptr;

	delete _vk;
	_vk = nullptr;

	delete _esper;
	_esper = nullptr;

	delete _shapes;
	_shapes = nullptr;

	delete _mainFont;
	_mainFont = nullptr;

	delete _scores;
	_scores = nullptr;

	delete _elevator;
	_elevator = nullptr;

	delete _spinner;
	_spinner = nullptr;

	delete _kia;
	_kia = nullptr;

	delete _suspectsDatabase;
	_suspectsDatabase = nullptr;

	delete _dialogueMenu;
	_dialogueMenu = nullptr;

	delete _textOptions;
	_textOptions = nullptr;

	delete _textVK;
	_textVK = nullptr;

	delete _textSpinnerDestinations;
	_textSpinnerDestinations = nullptr;

	delete _textKIA;
	_textKIA = nullptr;

	delete _textClueTypes;
	_textClueTypes = nullptr;

	delete _textCrimes;
	_textCrimes = nullptr;

	delete _textActorNames;
	_textActorNames = nullptr;

	delete _policeMaze;
	_policeMaze = nullptr;

	// don't delete _playerActor since that is handled
	// in the loop over _actors below
	_playerActor = nullptr;

	delete _actors[kActorVoiceOver];
	_actors[kActorVoiceOver] = nullptr;

	int actorCount = kActorCount;
	if (_gameInfo) {
		actorCount = (int)_gameInfo->getActorCount();
	}

	for (int i = 0; i < actorCount; ++i) {
		delete _actors[i];
		_actors[i] = nullptr;
	}

	delete _zbuffer;
	_zbuffer = nullptr;

	delete _overlays;
	_overlays = nullptr;

	if (isArchiveOpen("SPCHSFX.TLK")) {
		closeArchive("SPCHSFX.TLK");
	}

#if BLADERUNNER_ORIGINAL_BUGS
#else
	if (isArchiveOpen("A.TLK")) {
		closeArchive("A.TLK");
	}
#endif // BLADERUNNER_ORIGINAL_BUGS

	if (isArchiveOpen("SFX.MIX")) {
		closeArchive("SFX.MIX");
	}

	if (isArchiveOpen("MUSIC.MIX")) {
		closeArchive("MUSIC.MIX");
	}

	// in case player closes the ScummVM window when in ESPER mode or similar
	if (isArchiveOpen("MODE.MIX")) {
		closeArchive("MODE.MIX");
	}

	if (_chapters && _chapters->hasOpenResources()) {
		_chapters->closeResources();
	}
	delete _chapters;
	_chapters = nullptr;

	delete _ambientSounds;
	_ambientSounds = nullptr;

	delete _audioSpeech;
	_audioSpeech = nullptr;

	delete _music;
	_music = nullptr;

	delete _audioPlayer;
	_audioPlayer = nullptr;

	delete _audioMixer;
	_audioMixer = nullptr;

	delete _audioCache;
	_audioCache = nullptr;

	delete _items;
	_items = nullptr;

	delete _gameFlags;
	_gameFlags = nullptr;

	delete _sceneObjects;
	_sceneObjects = nullptr;

	delete _view;
	_view = nullptr;

	delete _sinTable1024;
	_sinTable1024 = nullptr;
	delete _cosTable1024;
	_cosTable1024 = nullptr;

	delete[] _gameVars;
	_gameVars = nullptr;

	delete _combat;
	_combat = nullptr;

	delete _waypoints;
	_waypoints = nullptr;

	delete _gameInfo;
	_gameInfo = nullptr;

	if (isArchiveOpen("STARTUP.MIX")) {
		closeArchive("STARTUP.MIX");
	}

	delete _archive;
	_archive = nullptr;

	if (isArchiveOpen("SUBTITLES.MIX")) {
		closeArchive("SUBTITLES.MIX");
	}

	delete _subtitles;
	_subtitles = nullptr;

	delete _framelimiter;
	_framelimiter = nullptr;

	delete _time;
	_time = nullptr;

	// guard the free() call to Surface::free() will boolean flags
	// since according to free() documentation:
	// it should only be used, when "the Surface data was created via
	// create! Otherwise this function has undefined behavior."
	if (_surfaceBackCreated)
		_surfaceBack.free();

	if (_surfaceFrontCreated)
		_surfaceFront.free();

	// These are static objects in original game

	//delete _debugger;	Debugger deletion is handled by Engine
	_debugger = nullptr;

	delete _sceneScript;
	_sceneScript = nullptr;

	delete _obstacles;
	_obstacles = nullptr;

	delete _lights;
	_lights = nullptr;

	delete _itemPickup;
	_itemPickup = nullptr;

	delete _settings;
	_settings = nullptr;

	delete _actorDialogueQueue;
	_actorDialogueQueue = nullptr;

	delete _endCredits;
	_endCredits = nullptr;

	delete _screenEffects;
	_screenEffects = nullptr;
}

bool BladeRunnerEngine::loadSplash() {
	Image img(this);
	if (!img.open("SPLASH.IMG")) {
		return false;
	}

	img.copyToSurface(&_surfaceFront);

	blitToScreen(_surfaceFront);

	return true;
}

Common::Point BladeRunnerEngine::getMousePos() const {
	Common::Point p = _eventMan->getMousePos();
	p.x = CLIP(p.x, int16(0), int16(639));
	p.y = CLIP(p.y, int16(0), int16(479));
	return p;
}

bool BladeRunnerEngine::isMouseButtonDown() const {
	return _eventMan->getButtonState() != 0;
}

void BladeRunnerEngine::gameLoop() {
	_gameIsRunning = true;
	do {
		if (_playerDead) {
			playerDied();
			_playerDead = false;
		}
		gameTick();
	} while (_gameIsRunning);
}

void BladeRunnerEngine::gameTick() {
	handleEvents();

	if (!_gameIsRunning || !_windowIsActive) {
		return;
	}

	if (!_kia->isOpen() && !_sceneScript->isInsideScript() && !_aiScripts->isInsideScript()) {
		if (!_settings->openNewScene()) {
			Common::Error runtimeError = Common::Error(Common::kUnknownError, _("A required game resource was not found"));
			GUI::MessageDialog dialog(runtimeError.getDesc());
			dialog.runModal();
			return;
		}
	}

	if (_gameAutoSaveTextId >= 0) {
		autoSaveGame(_gameAutoSaveTextId, false);
		_gameAutoSaveTextId = -1;
	}

	//probably not needed, this version of tick is just loading data from buffer
	//_audioMixer->tick();

	if (_kia->isOpen()) {
		_kia->tick();
		return;
	}

	if (_spinner->isOpen()) {
		_spinner->tick();
		_ambientSounds->tick();
		return;
	}

	if (_esper->isOpen()) {
		_esper->tick();
		return;
	}

	if (_vk->isOpen()) {
		_vk->tick();
		_ambientSounds->tick();
		return;
	}

	if (_elevator->isOpen()) {
		_elevator->tick();
		_ambientSounds->tick();
		return;
	}

	if (_scores->isOpen()) {
		_scores->tick();
		_ambientSounds->tick();
		return;
	}

	_actorDialogueQueue->tick();
	if (_scene->didPlayerWalkIn()) {
		_sceneScript->playerWalkedIn();
	}

	bool inDialogueMenu = _dialogueMenu->isVisible();
	if  (!inDialogueMenu) {
		for (int i = 0; i < (int)_gameInfo->getActorCount(); ++i) {
			_actors[i]->tickCombat();
		}
	}

	_policeMaze->tick();

	_zbuffer->clean();

	_ambientSounds->tick();

	bool backgroundChanged = false;
	int frame = _scene->advanceFrame();
	if (frame >= 0) {
//		debug("_sceneScript->sceneFrameAdvanced(%d)", frame);
		_sceneScript->sceneFrameAdvanced(frame);
		backgroundChanged = true;
	}
	blit(_surfaceBack, _surfaceFront);

	_overlays->tick();

	if (!inDialogueMenu) {
		// TODO This is probably responsible for actors getting stuck in place
		// after reaching a waypoint when dialoge menu is open
		actorsUpdate();
	}

	if (_settings->getNewScene() != -1 && !_sceneScript->isInsideScript() && !_aiScripts->isInsideScript()) {
		return;
	}

	_sliceRenderer->setView(_view);

	// Tick and draw all actors in current set
	int setId = _scene->getSetId();
	for (int i = 0, end = _gameInfo->getActorCount(); i != end; ++i) {
		if (_actors[i]->getSetId() == setId) {
			Common::Rect screenRect;
			if (_actors[i]->tick(backgroundChanged, &screenRect)) {
				_zbuffer->mark(screenRect);
			}
		}
	}

	_items->tick();

	_itemPickup->tick();
	_itemPickup->draw();

	Common::Point p = getMousePos();

	if (_dialogueMenu->isVisible()) {
		_dialogueMenu->tick(p.x, p.y);
		_dialogueMenu->draw(_surfaceFront);
	}

	if (_debugger->_viewZBuffer) {
		// The surface front pixel format is 32 bit now,
		// but the _zbuffer->getData() still returns 16bit pixels
		// We need to copy pixel by pixel, converting each pixel from 16 to 32bit
		for (int y = 0; y < kOriginalGameHeight; ++y) {
			for (int x = 0; x < kOriginalGameWidth; ++x) {
				uint8 a, r, g, b;
				//getGameDataColor(_zbuffer->getData()[y*kOriginalGameWidth + x], a, r, g, b);
				a = 1;
				r = _zbuffer->getData()[y*kOriginalGameWidth + x] / 256;
				g = r;
				b = r;
				void   *dstPixel = _surfaceFront.getBasePtr(x, y);
				drawPixel(_surfaceFront, dstPixel, _surfaceFront.format.ARGBToColor(a, r, g, b));
			}
		}
	}

	if (_debugger->_dbgPendingOuttake.pending) {
		// NOTE a side-effect of forcibly playing back an outtake video with the debugger
		//      is that the ambient sounds of the scene may be removed
		//      and won't be restored after the scene has completed.
		//      Eg. this happens for outtake 29 (FLYTRU_E), which is the special muted outtake
		//      for which, in Restored Content mode, we add custom ambient sounds.
		//      Fixing this (storing ambient sounds and restoring them after the outtake has finished)
		//      is too cumbersome to be worth it.
		int ambientSoundsPreOuttakeVol = _mixer->getVolumeForSoundType(_mixer->kPlainSoundType);
		int musicPreOuttakeVol = _mixer->getVolumeForSoundType(_mixer->kMusicSoundType);
		int sfxPreOuttakeVol = _mixer->getVolumeForSoundType(_mixer->kSFXSoundType);
		// Speech Sound Type (kSpeechSoundType) is the volume of the outtake video,
		// so we don't mute that one
		_mixer->setVolumeForSoundType(_mixer->kMusicSoundType, 0);
		_mixer->setVolumeForSoundType(_mixer->kPlainSoundType, 0);
		_mixer->setVolumeForSoundType(_mixer->kSFXSoundType, 0);
		if (_debugger->_dbgPendingOuttake.outtakeId == -1 && _debugger->_dbgPendingOuttake.container < -1) {
			outtakePlay(_debugger->_dbgPendingOuttake.externalFilename, _debugger->_dbgPendingOuttake.notLocalized, _debugger->_dbgPendingOuttake.container);
		} else {
			outtakePlay(_debugger->_dbgPendingOuttake.outtakeId, _debugger->_dbgPendingOuttake.notLocalized, _debugger->_dbgPendingOuttake.container);
		}
		_mixer->setVolumeForSoundType(_mixer->kSFXSoundType, sfxPreOuttakeVol);
		_mixer->setVolumeForSoundType(_mixer->kPlainSoundType, ambientSoundsPreOuttakeVol);
		_mixer->setVolumeForSoundType(_mixer->kMusicSoundType, musicPreOuttakeVol);
		_debugger->resetPendingOuttake();
	}

	_mouse->tick(p.x, p.y);
	_mouse->draw(_surfaceFront, p.x, p.y);

	if (_walkSoundId >= 0) {
		_audioPlayer->playAud(_gameInfo->getSfxTrack(_walkSoundId), _walkSoundVolume, _walkSoundPan, _walkSoundPan, 50, 0);
		_walkSoundId = -1;
	}

	if (_debugger->_isDebuggerOverlay) {
		_debugger->drawDebuggerOverlay();
	}

	if (_debugger->_viewObstacles) {
		_obstacles->draw();
	}

	_subtitles->tick(_surfaceFront);

	 // Without this condition the game may flash back to the game screen
	 // between and ending outtake and the end credits.
	if (!_gameOver) {
		blitToScreen(_surfaceFront);
	}
}

void BladeRunnerEngine::actorsUpdate() {
#if BLADERUNNER_ORIGINAL_BUGS
#else
	uint32 timeNow = _time->current();
	// Don't update actors more than 60 or 120 times per second
	if (timeNow - _actorUpdateTimeLast < 1000u / ( _framesPerSecondMax? 120u : 60u)) {
		return;
	}
	_actorUpdateTimeLast = timeNow;
#endif // BLADERUNNER_ORIGINAL_BUGS

	int actorCount = (int)_gameInfo->getActorCount();
	int setId = _scene->getSetId();

	// what a "nice" last minute fix...
	if ( setId == kSetUG18
	 && _gameVars[kVariableChapter] == 4
	 && _gameFlags->query(kFlagCallWithGuzza)
	 && _aiScripts->isInsideScript()
	) {
		return;
	}

	for (int i = 0; i < actorCount; ++i) {
		Actor *actor = _actors[i];
		if (actor->getSetId() == setId || i == _actorUpdateCounter) {
			_aiScripts->update(i);
			actor->timersUpdate();
		}
	}
	++_actorUpdateCounter;
	if (_actorUpdateCounter >= actorCount) {
		_actorUpdateCounter = 0;
	}
}

void BladeRunnerEngine::walkingReset() {
	_mouseClickTimeLast   = 0;
	_mouseClickTimeDiff   = 0;
	_walkingToExitId      = -1;
	_isInsideScriptExit   = false;
	_walkingToRegionId    = -1;
	_isInsideScriptRegion = false;
	_walkingToObjectId    = -1;
	_isInsideScriptObject = false;
	_walkingToItemId      = -1;
	_isInsideScriptItem   = false;
	_walkingToEmpty       = false;
	_walkingToEmptyX      = 0;
	_walkingToEmptyY      = 0;
	_isInsideScriptEmpty  = false;
	_walkingToActorId     = -1;
	_isInsideScriptActor  = false;
}

bool BladeRunnerEngine::isAllowedRepeatedCustomEvent(const Common::Event &currevent) {
	switch (currevent.type) {
	case Common::EVENT_CUSTOM_ENGINE_ACTION_START:
		switch ((BladeRunnerEngineMappableAction)currevent.customType) {
		case kMpActionCutsceneSkip:
			// fall through
		case kMpActionDialogueSkip:
			// fall through
		case kMpActionToggleKiaOptions:
			// fall through
		case kMpConfirmDlg:
			return true;

		default:
			return false;
		}
		break;

	default:
		break;
	}
	return false;
}

// The original allowed a few keyboard keys to be repeated ("key spamming")
// namely, Esc, Return and Space during normal gameplay.
// "Spacebar" spamming would result in McCoy quickly switching between combat mode and normal mode,
// which is not very useful -- by introducing the keymapper with custom action events this behavior is no longer replicated.
// Spamming Space, backspace, latin letter keys and symbols is allowed in KIA mode,
// particularly in the Save Panel when typing in the name for a save game.
// For simplicity, we allow everything after the 0x20 (space ascii code) up to 0xFF.
// The UIInputBox::charIsValid() will filter out any unsupported characters.
// F-keys are not repeated.
bool BladeRunnerEngine::isAllowedRepeatedKey(const Common::KeyState &currKeyState) {
	// Return and KP_Enter keys are repeatable in KIA.
	// This is noticeable when choosing an already saved game to overwrite
	// and holding down Enter would cause the confirmation dialogue to pop up
	// and it would subsequently confirm it as well.
	return  currKeyState.keycode == Common::KEYCODE_BACKSPACE
	    ||  currKeyState.keycode == Common::KEYCODE_SPACE
	    ||  currKeyState.keycode == Common::KEYCODE_KP_MINUS
	    ||  currKeyState.keycode == Common::KEYCODE_KP_PLUS
	    ||  currKeyState.keycode == Common::KEYCODE_KP_EQUALS
	    || (currKeyState.keycode != Common::KEYCODE_INVALID
	        && (currKeyState.ascii > 0x20 && currKeyState.ascii <= 0xFF));
}

void BladeRunnerEngine::handleEvents() {
	if (shouldQuit()) {
		_gameIsRunning = false;
		return;
	}

	// This flag check is to skip the first call of handleEvents() in gameTick().
	// This prevents a "hack" whereby the player could press Esc quickly and enter the KIA screen,
	// even in the case when no save games for the game exist. In such case the game is supposed
	// to immediately play the intro video and subsequently start a new game of medium difficulty.
	// It does not expect the player to enter KIA beforehand, which causes side-effects and unforeseen behavior.
	// NOTE Eventually, we will support the option to launch into KIA in any case,
	// but not via the "hack" way that is fixed here.
	if (_gameJustLaunched) {
		_gameJustLaunched = false;
		return;
	}

	Common::Event event;
	Common::EventManager *eventMan = _system->getEventManager();
	while (eventMan->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_CUSTOM_ENGINE_ACTION_END:
			if (shouldDropRogueCustomEvent(event)) {
				return;
			}
			switch ((BladeRunnerEngineMappableAction)event.customType) {
			case kMpActionToggleCombat:
				// fall through
			case kMpActionToggleCluePrivacy:
				handleMouseAction(event.mouse.x, event.mouse.y, false, false);
				break;

			case kMpActionCutsceneSkip:
				// fall through
			case kMpActionDialogueSkip:
				// fall through
			case kMpActionToggleKiaOptions:
				// fall through
			case kMpActionOpenKiaDatabase:
				// fall through
			case kMpActionOpenKIATabHelp:
				// fall through
			case kMpActionOpenKIATabSaveGame:
				// fall through
			case kMpActionOpenKIATabLoadGame:
				// fall through
			case kMpActionOpenKIATabCrimeSceneDatabase:
				// fall through
			case kMpActionOpenKIATabSuspectDatabase:
				// fall through
			case kMpActionOpenKIATabClueDatabase:
				// fall through
			case kMpActionOpenKIATabQuitGame:
				// fall through
			case kMpConfirmDlg:
				// fall through
			case kMpDeleteSelectedSvdGame:
				handleCustomEventStop(event);
				break;

			default:
				break;
			}
			break;

		case Common::EVENT_CUSTOM_ENGINE_ACTION_START:
			if (shouldDropRogueCustomEvent(event)) {
				return;
			}
			// Process the initial/actual custom event only here, filter out repeats
			// TODO Does this actually filter repeats?
			if (!event.kbdRepeat) {
				switch ((BladeRunnerEngineMappableAction)event.customType) {
				case kMpActionToggleCombat:
					// fall through
				case kMpActionToggleCluePrivacy:
					handleMouseAction(event.mouse.x, event.mouse.y, false, true);
					break;

				case kMpActionCutsceneSkip:
					// fall through
				case kMpActionDialogueSkip:
					// fall through
				case kMpActionToggleKiaOptions:
					// fall through
				case kMpActionOpenKiaDatabase:
					// fall through
				case kMpActionOpenKIATabHelp:
					// fall through
				case kMpActionOpenKIATabSaveGame:
					// fall through
				case kMpActionOpenKIATabLoadGame:
					// fall through
				case kMpActionOpenKIATabCrimeSceneDatabase:
					// fall through
				case kMpActionOpenKIATabSuspectDatabase:
					// fall through
				case kMpActionOpenKIATabClueDatabase:
					// fall through
				case kMpActionOpenKIATabQuitGame:
					// fall through
				case kMpConfirmDlg:
					// fall through
				case kMpDeleteSelectedSvdGame:
					if (isAllowedRepeatedCustomEvent(event)
					    && _activeCustomEvents.size() < kMaxCustomConcurrentRepeatableEvents) {
						if (_activeCustomEvents.empty()) {
							_customEventRepeatTimeLast = _time->currentSystem();
							_customEventRepeatTimeDelay = kKeyRepeatInitialDelay;
						}
						_activeCustomEvents.push_back(event);
					}
					handleCustomEventStart(event);
					break;

				case kMpActionScrollUp:
					handleMouseAction(event.mouse.x, event.mouse.y, false, false, -1);
					break;

				case kMpActionScrollDown:
					handleMouseAction(event.mouse.x, event.mouse.y, false, false, 1);
					break;

				default:
					break;
				}
			}
			break;

		case Common::EVENT_KEYUP:
			handleKeyUp(event);
			break;

		case Common::EVENT_KEYDOWN:
			// Process the initial/actual key press only here, filter out repeats
			// TODO Does this actually filter repeats?
			if (!event.kbdRepeat) {
				// Only for some keys, allow repeated firing emulation
				// First hit (fire) has a bigger delay (kKeyRepeatInitialDelay) before repeated events are fired from the same key
				if (isAllowedRepeatedKey(event.kbd)) {
					_currentKeyDown = event.kbd;
					_keyRepeatTimeLast =  _time->currentSystem();
					_keyRepeatTimeDelay = kKeyRepeatInitialDelay;
				}
				handleKeyDown(event);
			}
			break;

		case Common::EVENT_LBUTTONUP:
			handleMouseAction(event.mouse.x, event.mouse.y, true, false);
			break;

		case Common::EVENT_LBUTTONDOWN:
			handleMouseAction(event.mouse.x, event.mouse.y, true, true);
			break;

		default:
			; // nothing to do
		}
	}

	// The switch clause above handles multiple events.
	// Some of those may lead to their own internal gameTick() loops (which will call handleEvents()).
	// Thus, we need to get a new timeNow value here to ensure we're not comparing with a stale version.
	uint32 timeNow = _time->currentSystem();
	if (!_activeCustomEvents.empty()
	    && (timeNow - _customEventRepeatTimeLast >= _customEventRepeatTimeDelay)) {
		_customEventRepeatTimeLast = timeNow;
		_customEventRepeatTimeDelay = kKeyRepeatSustainDelay;
		uint16 aceSize = _activeCustomEvents.size();
		for (ActiveCustomEventsArray::iterator it = _activeCustomEvents.begin(); it != _activeCustomEvents.end(); it++) {
			// kbdRepeat field will be unused here since we emulate the kbd repeat behavior anyway,
			// but maybe it's good to set it for consistency
			it->kbdRepeat = true;
			// reissue the custom start event
			handleCustomEventStart(*it);
			// This extra check is needed since it's possible that during this for loop
			// within the above handleCustomEventStart() execution,
			// cleanupPendingRepeatingEvents() is called
			// and elements from _activeCustomEvents are removed!
			// TODO This is probably an indication that this could be reworked
			//      as something cleaner and safer.
			//      Or event repetition could be handled by the keymapper code (outside the engine code)
			if (aceSize != _activeCustomEvents.size()) {
				break;
			}
		}
	} else if (isAllowedRepeatedKey(_currentKeyDown)
	           && (timeNow - _keyRepeatTimeLast >= _keyRepeatTimeDelay)) {
		// create a "new" keydown event
		event.type = Common::EVENT_KEYDOWN;
		// kbdRepeat field will be unused here since we emulate the kbd repeat behavior anyway,
		// but it's good to set it for consistency
		event.kbdRepeat = true;
		event.kbd = _currentKeyDown;
		_keyRepeatTimeLast = timeNow;
		_keyRepeatTimeDelay = kKeyRepeatSustainDelay;
		handleKeyDown(event);
	}
}

void BladeRunnerEngine::handleKeyUp(Common::Event &event) {
	if (event.kbd.keycode == _currentKeyDown.keycode) {
		// Only stop firing events if it's the current key
		_currentKeyDown.keycode = Common::KEYCODE_INVALID;
	}

	if (!playerHasControl() || _isWalkingInterruptible) {
		return;
	}

	if (_kia->isOpen()) {
		_kia->handleKeyUp(event.kbd);
		return;
	}
}

void BladeRunnerEngine::handleKeyDown(Common::Event &event) {
	if (!playerHasControl() || _isWalkingInterruptible || _actorIsSpeaking || _vqaIsPlaying) {
		return;
	}

	if (_kia->isOpen()) {
		_kia->handleKeyDown(event.kbd);
		return;
	}

	if (_spinner->isOpen()) {
		return;
	}

	if (_elevator->isOpen()) {
		return;
	}

	if (_esper->isOpen()) {
		return;
	}

	if (_vk->isOpen()) {
		return;
	}

	if (_dialogueMenu->isOpen()) {
		return;
	}

	if (_scores->isOpen()) {
		_scores->handleKeyDown(event.kbd);
		return;
	}

	if ((_scene->getSetId() == kSetMA02_MA04 || _scene->getSetId() == kSetMA04)
	    && _scene->getSceneId() == kSceneMA04
	    && _subtitles->isHDCPresent()
	    && _extraCNotify == 0) {
		if (toupper(event.kbd.ascii) != _subtitles->getGoVib()[_extraCPos]) {
			setExtraCNotify(0);
		}
		if (toupper(event.kbd.ascii) == _subtitles->getGoVib()[_extraCPos]) {
			++_extraCPos;
			if (!_subtitles->getGoVib()[_extraCPos]) {
				_subtitles->xcReload();
				playerLosesControl();
				setExtraCNotify(1);
				_extraCPos = 0;
			}
		}
	}
}

uint8 BladeRunnerEngine::getExtraCNotify() {
	return _extraCNotify;
}

void BladeRunnerEngine::setExtraCNotify(uint8 val) {
	if (val == 0) {
		_extraCPos = 0;
	}
	_extraCNotify = val;
}

// Check if a polled event belongs to a currently disabled keymap and, if so, drop it.
bool BladeRunnerEngine::shouldDropRogueCustomEvent(const Common::Event &evt) {
	if (getEventManager()->getKeymapper() != nullptr) {
		Common::KeymapArray kmpsArr = getEventManager()->getKeymapper()->getKeymaps();
		for (Common::KeymapArray::iterator kmpsIt = kmpsArr.begin(); kmpsIt != kmpsArr.end(); ++kmpsIt) {
			if (!(*kmpsIt)->isEnabled()) {
				Common::Keymap::ActionArray actionsInKm = (*kmpsIt)->getActions();
				for (Common::Keymap::ActionArray::iterator kmIt = actionsInKm.begin(); kmIt != actionsInKm.end(); ++kmIt) {
					if ((evt.type != Common::EVENT_INVALID) && (evt.customType == (*kmIt)->event.customType)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

void BladeRunnerEngine::cleanupPendingRepeatingEvents(const Common::String &keymapperId) {
	// Also clean up any currently repeating key down here.
	// This prevents a bug where holding down Enter key in save screen with a filled in save game
	// or a selected game to overwrite, would complete the save and then maintain Enter as a repeating key
	// in the main gameplay (without ever sending a key up event to stop it).
	_currentKeyDown.keycode = Common::KEYCODE_INVALID;

	if (getEventManager()->getKeymapper() != nullptr
	    && getEventManager()->getKeymapper()->getKeymap(keymapperId) != nullptr
		&& !_activeCustomEvents.empty()) {

		Common::Keymap::ActionArray actionsInKm = getEventManager()->getKeymapper()->getKeymap(keymapperId)->getActions();
		for (Common::Keymap::ActionArray::iterator kmIt = actionsInKm.begin(); kmIt != actionsInKm.end(); ++kmIt) {
			for (ActiveCustomEventsArray::iterator actIt = _activeCustomEvents.begin(); actIt != _activeCustomEvents.end(); ++actIt) {
				if ((actIt->type != Common::EVENT_INVALID) && (actIt->customType == (*kmIt)->event.customType)) {
					_activeCustomEvents.erase(actIt);
					// When erasing an element from an array, erase(iterator pos)
					// will return an iterator pointing to the next element in the array.
					// Thus, we should check if we reached the end() here, to avoid moving
					// the iterator in the next loop repetition to an invalid memory location.
					if (actIt == _activeCustomEvents.end()) {
						break;
					}
				}
			}
		}
	}
}

void BladeRunnerEngine::handleCustomEventStop(Common::Event &event) {
	if (!_activeCustomEvents.empty()) {
		for (ActiveCustomEventsArray::iterator it = _activeCustomEvents.begin(); it != _activeCustomEvents.end(); it++) {
			if ((it->type != Common::EVENT_INVALID) && (it->customType == event.customType)) {
				_activeCustomEvents.erase(it);
				break;
			}
		}
	}

	if (!playerHasControl() || _isWalkingInterruptible) {
		return;
	}

	if (_kia->isOpen()) {
		_kia->handleCustomEventStop(event);
		return;
	}
}

void BladeRunnerEngine::handleCustomEventStart(Common::Event &event) {
	if (_vqaIsPlaying && (BladeRunnerEngineMappableAction)event.customType == kMpActionCutsceneSkip) {
		_vqaStopIsRequested = true;
		_vqaIsPlaying = false;
		return;
	}

	if (_vqaStopIsRequested && (BladeRunnerEngineMappableAction)event.customType == kMpActionCutsceneSkip) {
		return;
	}

	if (_actorIsSpeaking && (BladeRunnerEngineMappableAction)event.customType == kMpActionDialogueSkip) {
		_actorSpeakStopIsRequested = true;
		_actorIsSpeaking = false;
	}

	if (_actorSpeakStopIsRequested && (BladeRunnerEngineMappableAction)event.customType == kMpActionDialogueSkip) {
		return;
	}

	if (!playerHasControl() || _isWalkingInterruptible || _actorIsSpeaking || _vqaIsPlaying) {
		return;
	}

	if (_kia->isOpen()) {
		_kia->handleCustomEventStart(event);
		return;
	}

	if (_spinner->isOpen()) {
		return;
	}

	if (_elevator->isOpen()) {
		return;
	}

	if (_esper->isOpen()) {
		return;
	}

	if (_vk->isOpen()) {
		return;
	}

	if (_dialogueMenu->isOpen()) {
		return;
	}

	if (_scores->isOpen()) {
		_scores->handleCustomEventStart(event);
		return;
	}

	switch ((BladeRunnerEngineMappableAction)event.customType) {
		case kMpActionOpenKIATabHelp:
			_kia->open(kKIASectionHelp);
			break;
		case kMpActionOpenKIATabSaveGame:
			_kia->open(kKIASectionSave);
			break;
		case kMpActionOpenKIATabLoadGame:
			_kia->open(kKIASectionLoad);
			break;
		case kMpActionOpenKIATabCrimeSceneDatabase:
			_kia->open(kKIASectionCrimes);
			break;
		case kMpActionOpenKIATabSuspectDatabase:
			_kia->open(kKIASectionSuspects);
			break;
		case kMpActionOpenKIATabClueDatabase:
			_kia->open(kKIASectionClues);
			break;
		case kMpActionOpenKIATabQuitGame:
			_kia->open(kKIASectionQuit);
			break;
		case kMpActionOpenKiaDatabase:
			_kia->openLastOpened();
			break;
		case kMpActionToggleKiaOptions:
			_kia->open(kKIASectionSettings);
			break;
		default:
			break;
	}
}

void BladeRunnerEngine::handleMouseAction(int x, int y, bool mainButton, bool buttonDown, int scrollDirection /* = 0 */) {
	x = CLIP(x, 0, 639);
	y = CLIP(y, 0, 479);

	uint32 timeNow = _time->current();

	if (buttonDown) {
		// unsigned difference is intentional
		_mouseClickTimeDiff = timeNow - _mouseClickTimeLast;
		_mouseClickTimeLast = timeNow;
	}

	if (!playerHasControl() || _mouse->isDisabled()) {
		return;
	}

	if (_kia->isOpen()) {
		if (scrollDirection != 0) {
			_kia->handleMouseScroll(x, y, scrollDirection);
		} else if (buttonDown) {
			_kia->handleMouseDown(x, y, mainButton);
		} else {
			_kia->handleMouseUp(x, y, mainButton);
		}
		return;
	}

	if (_spinner->isOpen()) {
		if (buttonDown) {
			_spinner->handleMouseDown(x, y);
		} else {
			_spinner->handleMouseUp(x, y);
		}
		return;
	}

	if (_esper->isOpen()) {
		if (buttonDown) {
			_esper->handleMouseDown(x, y, mainButton);
		} else {
			_esper->handleMouseUp(x, y, mainButton);
		}
		return;
	}

	if (_vk->isOpen()) {
		if (buttonDown) {
			_vk->handleMouseDown(x, y, mainButton);
		} else {
			_vk->handleMouseUp(x, y, mainButton);
		}
		return;
	}

	if (_elevator->isOpen()) {
		if (buttonDown) {
			_elevator->handleMouseDown(x, y);
		} else {
			_elevator->handleMouseUp(x, y);
		}
		return;
	}

	if (_scores->isOpen()) {
		if (buttonDown) {
			_scores->handleMouseDown(x, y);
		} else {
			_scores->handleMouseUp(x, y);
		}
		return;
	}

	if (_dialogueMenu->waitingForInput()) {
		if (mainButton && !buttonDown) {
			_dialogueMenu->mouseUp();
		}
		return;
	}

	if (mainButton) {
		Vector3 scenePosition = _mouse->getXYZ(x, y);

		bool isClickable;
		bool isObstacle;
		bool isTarget;

		int sceneObjectId = _sceneObjects->findByXYZ(&isClickable, &isObstacle, &isTarget, scenePosition, true, false, true);
		int exitIndex = _scene->_exits->getRegionAtXY(x, y);
		int regionIndex = _scene->_regions->getRegionAtXY(x, y);

		if (_debugger->_showMouseClickInfo) {
			// Region has highest priority when overlapping
			debug("Mouse: %02.2f, %02.2f, %02.2f at ScreenX: %d ScreenY: %d", scenePosition.x, scenePosition.y, scenePosition.z, x, y);
			if ((sceneObjectId < kSceneObjectOffsetActors || sceneObjectId >= kSceneObjectOffsetItems) && exitIndex >= 0) {
				debug("Clicked on Region-Exit=%d", exitIndex);
			} else if (regionIndex >= 0) {
				debug("Clicked on Region-Regular=%d", regionIndex);
			}
			// In debug mode we're interested in *all* object/actors/items under mouse click
			if (sceneObjectId >= kSceneObjectOffsetActors && sceneObjectId < kSceneObjectOffsetItems) {
				debug("Clicked on Actor: %d", sceneObjectId  - kSceneObjectOffsetActors);
			}
			if (sceneObjectId >= kSceneObjectOffsetItems && sceneObjectId < kSceneObjectOffsetObjects) {
				debug("Clicked on Item: %d", sceneObjectId  - kSceneObjectOffsetItems);
			}
			if (sceneObjectId >= kSceneObjectOffsetObjects && sceneObjectId <= (95 + kSceneObjectOffsetObjects) ) {
				debug("Clicked on Object: %d", sceneObjectId - kSceneObjectOffsetObjects);
			}
		}

		if ((sceneObjectId < kSceneObjectOffsetActors || sceneObjectId >= kSceneObjectOffsetItems) && exitIndex >= 0) {
			handleMouseClickExit(exitIndex, x, y, buttonDown);
		} else if (regionIndex >= 0) {
			handleMouseClickRegion(regionIndex, x, y, buttonDown);
		} else if (sceneObjectId == -1) {
			handleMouseClickEmpty(x, y, scenePosition, buttonDown);
		} else if (sceneObjectId >= kSceneObjectOffsetActors && sceneObjectId < kSceneObjectOffsetItems) {
			handleMouseClickActor(sceneObjectId - kSceneObjectOffsetActors, mainButton, buttonDown, scenePosition, x, y);
		} else if (sceneObjectId >= kSceneObjectOffsetItems && sceneObjectId < kSceneObjectOffsetObjects) {
			handleMouseClickItem(sceneObjectId - kSceneObjectOffsetItems, buttonDown);
		} else if (sceneObjectId >= kSceneObjectOffsetObjects && sceneObjectId <= (95 + kSceneObjectOffsetObjects)) {
			handleMouseClick3DObject(sceneObjectId - kSceneObjectOffsetObjects, buttonDown, isClickable, isTarget);
		}
	} else if (buttonDown) {
		if (_playerActor->mustReachWalkDestination()) {
			if (!_isWalkingInterruptible) {
				return;
			}
			_playerActor->stopWalking(false);
			_interruptWalking = true;
		}
		_combat->change();
	}
}

void BladeRunnerEngine::handleMouseClickExit(int exitId, int x, int y, bool buttonDown) {
	if (_isWalkingInterruptible && exitId != _walkingToExitId) {
		_isWalkingInterruptible = false;
		_interruptWalking = true;
		walkingReset();
		_walkingToExitId = exitId;
		return;
	}

	if (buttonDown) {
		return;
	}

	if (_isInsideScriptExit && exitId == _walkingToExitId) {
		_playerActor->run();
		if (_mouseClickTimeDiff <= 10000) {
			_playerActor->increaseFPS();
		}
	} else {
		_walkingToExitId   = exitId;
		_walkingToRegionId = -1;
		_walkingToObjectId = -1;
		_walkingToItemId   = -1;
		_walkingToEmpty    = false;
		_walkingToActorId  = -1;

		_isInsideScriptExit = true;
		_sceneScript->clickedOnExit(exitId);
		_isInsideScriptExit = false;
	}
}

void BladeRunnerEngine::handleMouseClickRegion(int regionId, int x, int y, bool buttonDown) {
	if (_isWalkingInterruptible && regionId != _walkingToRegionId) {
		_isWalkingInterruptible = false;
		_interruptWalking = true;
		walkingReset();
		_walkingToRegionId = regionId;
		return;
	}

	if (buttonDown || _mouse->isInactive()) {
		return;
	}

	if (_isInsideScriptRegion && regionId == _walkingToRegionId) {
		_playerActor->run();
		if (_mouseClickTimeDiff <= 10000) {
			_playerActor->increaseFPS();
		}
	} else {
		_walkingToExitId   = -1;
		_walkingToRegionId = regionId;
		_walkingToObjectId = -1;
		_walkingToItemId   = -1;
		_walkingToEmpty    = false;
		_walkingToActorId  = -1;

		_isInsideScriptRegion = true;
		_sceneScript->clickedOn2DRegion(regionId);
		_isInsideScriptRegion = false;
	}
}

void BladeRunnerEngine::handleMouseClick3DObject(int objectId, bool buttonDown, bool isClickable, bool isTarget) {
	const Common::String &objectName = _scene->objectGetName(objectId);

	if (_isWalkingInterruptible && objectId != _walkingToObjectId) {
		_isWalkingInterruptible = false;
		_interruptWalking = true;
		walkingReset();
		_walkingToObjectId = objectId;
		return;
	}

	if (_mouse->isInactive()) {
		return;
	}

	if (!_combat->isActive()) {
		if (buttonDown || !isClickable) {
			return;
		}

		if (_isInsideScriptObject && objectId == _walkingToObjectId) {
			_playerActor->run();
			if (_mouseClickTimeDiff <= 10000) {
				_playerActor->increaseFPS();
			}
		} else {
			_walkingToExitId   = -1;
			_walkingToRegionId = -1;
			_walkingToObjectId = objectId;
			_walkingToItemId   = -1;
			_walkingToEmpty    = false;
			_walkingToActorId  = -1;

			_isInsideScriptObject = true;
			_sceneScript->clickedOn3DObject(objectName.c_str(), false);
			_isInsideScriptObject = false;
		}
	} else {
		if (!buttonDown || !isTarget) {
			return;
		}
		_playerActor->stopWalking(false);
		_playerActor->faceObject(objectName, false);
		_playerActor->changeAnimationMode(kAnimationModeCombatAttack, false);
		_settings->decreaseAmmo();
		_audioPlayer->playAud(_gameInfo->getSfxTrack(_combat->getHitSound()), 100, 0, 0, 90, 0);

		_mouse->setMouseJitterUp();

		_isInsideScriptObject = true;
		_sceneScript->clickedOn3DObject(objectName.c_str(), true);
		_isInsideScriptObject = false;
	}
}

void BladeRunnerEngine::handleMouseClickEmpty(int x, int y, Vector3 &scenePosition, bool buttonDown) {
	if (_isWalkingInterruptible) {
		_isWalkingInterruptible = false;
		_interruptWalking = true;
		walkingReset();
		_walkingToEmpty = false;
		return;
	}

	_isInsideScriptEmpty = true;
	bool sceneMouseClick = _sceneScript->mouseClick(x, y);
	_isInsideScriptEmpty = false;

	if (sceneMouseClick) {
		return;
	}

	int actorId = Actor::findTargetUnderMouse(this, x, y);
	int itemId = _items->findTargetUnderMouse(x, y);

	if (_combat->isActive() && buttonDown && (actorId >= 0 || itemId >= 0)) {
		_playerActor->stopWalking(false);
		if (actorId >= 0) {
			_playerActor->faceActor(actorId, false);
		} else {
			_playerActor->faceItem(itemId, false);
		}
		_playerActor->changeAnimationMode(kAnimationModeCombatAttack, false);
		_settings->decreaseAmmo();
		_audioPlayer->playAud(_gameInfo->getSfxTrack(_combat->getMissSound()), 100, 0, 0, 90, 0);

		_mouse->setMouseJitterUp();

		if (actorId > 0) {
			_aiScripts->shotAtAndMissed(actorId);
		}
	} else {
		if (buttonDown) {
			return;
		}

		_walkingToExitId   = -1;
		_walkingToRegionId = -1;
		_walkingToObjectId = -1;
		_walkingToItemId   = -1;
		_walkingToEmpty    = true;
		_walkingToActorId  = -1;

		if (_combat->isActive() && (actorId > 0 || itemId > 0)) {
			return;
		}

		int xDist = abs(_walkingToEmptyX - x);
		int yDist = abs(_walkingToEmptyY - y);

		_walkingToEmptyX = x;
		_walkingToEmptyY = y;

		bool inWalkbox = false;
		float altitude = _scene->_set->getAltitudeAtXZ(scenePosition.x, scenePosition.z, &inWalkbox);

		if (!inWalkbox || scenePosition.y >= altitude + 6.0f) {
			return;
		}

		bool shouldRun = _playerActor->isRunning();
		if (_mouseClickTimeDiff <= 10000 && xDist <= 10 && yDist <= 10) {
			shouldRun = true;
		}

		_playerActor->walkTo(shouldRun, scenePosition, false);

		if (shouldRun && _playerActor->isWalking()) {
			_playerActor->increaseFPS();
		}
	}
}

void BladeRunnerEngine::handleMouseClickItem(int itemId, bool buttonDown) {
	if (_isWalkingInterruptible && itemId != _walkingToItemId) {
		_isWalkingInterruptible = false;
		_interruptWalking = true;
		walkingReset();
		_walkingToItemId = itemId;
		return;
	}

	if (_mouse->isInactive()) {
		return;
	}

	if (!_combat->isActive()) {
		if (buttonDown) {
			return;
		}

		if (_isInsideScriptItem && itemId == _walkingToItemId) {
			_playerActor->run();
			if (_mouseClickTimeDiff <= 10000) {
				_playerActor->increaseFPS();
			}
		} else {
			_walkingToExitId = -1;
			_walkingToRegionId = -1;
			_walkingToObjectId = -1;
			_walkingToItemId = itemId;
			_walkingToEmpty = false;
			_walkingToActorId = -1;

			_isInsideScriptItem = true;
			_sceneScript->clickedOnItem(itemId, false);
			_isInsideScriptItem = false;
		}
	} else {
		if (!buttonDown || !_items->isTarget(itemId) || _mouse->isRandomized()) {
			return;
		}

		_playerActor->stopWalking(false);
		_playerActor->faceItem(itemId, false);
		_playerActor->changeAnimationMode(kAnimationModeCombatAttack, false);
		_settings->decreaseAmmo();
		_audioPlayer->playAud(_gameInfo->getSfxTrack(_combat->getHitSound()), 100, 0, 0, 90, 0);

		_mouse->setMouseJitterUp();

		_isInsideScriptItem = true;
		_sceneScript->clickedOnItem(itemId, true);
		_isInsideScriptItem = false;
	}
}

void BladeRunnerEngine::handleMouseClickActor(int actorId, bool mainButton, bool buttonDown, Vector3 &scenePosition, int x, int y) {
	if (_isWalkingInterruptible && actorId != _walkingToActorId) {
		_isWalkingInterruptible = false;
		_interruptWalking = true;
		walkingReset();
		_walkingToActorId = actorId;
		return;
	}

	if (_mouse->isInactive()) {
		return;
	}

	if (!buttonDown) {
		if (actorId == kActorMcCoy) {
			// While McCoy is moving, clicking on him does not bring up KIA (this is also the original behavior)
			if (!_playerActor->isMoving()) {
				if (mainButton) {
					if (!_combat->isActive()) {
						_kia->openLastOpened();
					}
				} else if (!_playerActor->mustReachWalkDestination()) {
					_combat->change();
				}
			}
			return;
		}
		if (_isInsideScriptActor && actorId == _walkingToActorId) {
			_playerActor->run();
			if (_mouseClickTimeDiff <= 10000) {
				_playerActor->increaseFPS();
			}
		} else {
			_walkingToExitId = -1;
			_walkingToRegionId = -1;
			_walkingToObjectId = -1;
			_walkingToItemId = -1;
			_walkingToEmpty = false;
			_walkingToActorId = actorId;

			_isInsideScriptActor = true;
			bool processedBySceneScript = _sceneScript->clickedOnActor(actorId);
			_isInsideScriptActor = false;

			if (!_combat->isActive() && !processedBySceneScript) {
				_aiScripts->clickedByPlayer(actorId);
			}
		}
	} else {
		Actor *actor = _actors[actorId];
		if (!_combat->isActive() || actorId == kActorMcCoy || !actor->isTarget() || actor->isRetired() || _mouse->isRandomized()) {
			return;
		}
		_playerActor->stopWalking(false);
		_playerActor->faceActor(actorId, false);
		_playerActor->changeAnimationMode(kAnimationModeCombatAttack, false);
		_settings->decreaseAmmo();

		bool missed = _playerActor->isObstacleBetween(actor->getXYZ());

		_audioPlayer->playAud(_gameInfo->getSfxTrack(missed ? _combat->getMissSound() : _combat->getHitSound()), 100, 0, 0, 90, 0);

		_mouse->setMouseJitterUp();

		if (missed) {
			_aiScripts->shotAtAndMissed(actorId);
		} else {
			_isInsideScriptActor = true;
			bool canShoot = _aiScripts->shotAtAndHit(actorId);
			_isInsideScriptActor = false;
			if (!canShoot) {
				_combat->shoot(actorId, scenePosition, x);
			}
		}
	}
}

void BladeRunnerEngine::gameWaitForActive() {
	while (!_windowIsActive) {
		handleEvents();
	}
}

void BladeRunnerEngine::loopActorSpeaking() {
	if (!_audioSpeech->isPlaying()) {
		return;
	}

	playerLosesControl();

	do {
		gameTick();
	} while (_gameIsRunning && _audioSpeech->isPlaying());

	playerGainsControl();
}

/**
* To be used only for when there is a chance an ongoing dialogue in a dialogue queue
* might be interrupted AND that is unwanted behavior (sometimes, it's intended that the dialogue
* can be interrupted without necessarily being finished).
*/
void BladeRunnerEngine::loopQueuedDialogueStillPlaying() {
	if (_actorDialogueQueue->isEmpty()) {
		return;
	}

	do {
		gameTick();
	} while (_gameIsRunning && !_actorDialogueQueue->isEmpty());

}

void BladeRunnerEngine::outtakePlay(int id, bool noLocalization, int container) {
	Common::String name = _gameInfo->getOuttake(id);

	outtakePlay(name, noLocalization, container);
}

void BladeRunnerEngine::outtakePlay(const Common::String &basenameNoExt, bool noLocalization, int container) {
	OuttakePlayer player(this);

	player.play(basenameNoExt, noLocalization, container);
}

bool BladeRunnerEngine::openArchive(const Common::String &name) {
	if (_enhancedEdition) {
		return true;
	}

	int i;

	// If archive is already open, return true
	for (i = 0; i != kArchiveCount; ++i) {
		if (_archives[i].isOpen() && _archives[i].getName() == name) {
			return true;
		}
	}

	// Find first available slot
	for (i = 0; i != kArchiveCount; ++i) {
		if (!_archives[i].isOpen()) {
			break;
		}
	}
	if (i == kArchiveCount) {
		/* TODO: BLADE.EXE retires the least recently used
		 * archive when it runs out of slots. */

		error("openArchive: No more archive slots");
	}

	_archives[i].open(Common::Path(name));
	return _archives[i].isOpen();
}

bool BladeRunnerEngine::closeArchive(const Common::String &name) {
	if (_enhancedEdition) {
		return true;
	}

	for (int i = 0; i != kArchiveCount; ++i) {
		if (_archives[i].isOpen() && _archives[i].getName() == name) {
			_archives[i].close();
			return true;
		}
	}

	warning("closeArchive: Archive %s not open.", name.c_str());
	return false;
}

bool BladeRunnerEngine::isArchiveOpen(const Common::String &name) const {
	if (_enhancedEdition) {
		return false;
	}
	for (int i = 0; i != kArchiveCount; ++i) {
		if (_archives[i].isOpen() && _archives[i].getName() == name)
			return true;
	}

	return false;
}

bool BladeRunnerEngine::openArchiveEnhancedEdition() {
	_archive = Common::makeZipArchive("BladeRunner.kpf");
	return _archive != nullptr;
}

void BladeRunnerEngine::syncSoundSettings() {
	Engine::syncSoundSettings();

	_subtitlesEnabled = ConfMan.getBool("subtitles");
	_showSubtitlesForTextCrawl = ConfMan.getBool("use_crawl_subs");

	_mixer->setVolumeForSoundType(_mixer->kMusicSoundType, ConfMan.getInt("music_volume"));
	_mixer->setVolumeForSoundType(_mixer->kSFXSoundType, ConfMan.getInt("sfx_volume"));
	_mixer->setVolumeForSoundType(_mixer->kSpeechSoundType, ConfMan.getInt("speech_volume"));
	// By default, if no ambient_volume is found in configuration manager, set ambient volume from sfx volume
	int configAmbientVolume = _mixer->getVolumeForSoundType(_mixer->kSFXSoundType);
	if (ConfMan.hasKey("ambient_volume")) {
		configAmbientVolume = ConfMan.getInt("ambient_volume");
	} else {
		ConfMan.setInt("ambient_volume", configAmbientVolume);
	}
	_mixer->setVolumeForSoundType(_mixer->kPlainSoundType, configAmbientVolume);
	// debug("syncSoundSettings: Volumes synced as Music: %d, Sfx: %d, Speech: %d", ConfMan.getInt("music_volume"), ConfMan.getInt("sfx_volume"), ConfMan.getInt("speech_volume"));

	if (_noMusicDriver) {
		// This affects *only* the music muting.
		_mixer->muteSoundType(_mixer->kMusicSoundType, true);
	}

	bool allSoundIsMuted = false;
	if (ConfMan.hasKey("mute")) {
		allSoundIsMuted = ConfMan.getBool("mute");
		if (!_noMusicDriver) {
			_mixer->muteSoundType(_mixer->kMusicSoundType, allSoundIsMuted);
		}
		_mixer->muteSoundType(_mixer->kSFXSoundType, allSoundIsMuted);
		_mixer->muteSoundType(_mixer->kSpeechSoundType, allSoundIsMuted);
		_mixer->muteSoundType(_mixer->kPlainSoundType, allSoundIsMuted);
	}

	if (ConfMan.hasKey("speech_mute") && !allSoundIsMuted) {
		// if true it means show only subtitles
		// "subtitles" key will already be set appropriately by Engine::syncSoundSettings();
		// but we need to mute the speech
		_mixer->muteSoundType(_mixer->kSpeechSoundType, ConfMan.getBool("speech_mute"));
	}

	// write-back to ini file for persistence
	ConfMan.flushToDisk(); // TODO Or maybe call this only when game is shut down?
}

bool BladeRunnerEngine::isSubtitlesEnabled() {
	return _subtitlesEnabled;
}

void BladeRunnerEngine::setSubtitlesEnabled(bool newVal) {
	ConfMan.setBool("subtitles", newVal);
	syncSoundSettings();
}

Common::SeekableReadStream *BladeRunnerEngine::getResourceStream(const Common::String &name) {
	Common::Path path(name);
	// If the file is extracted from MIX files use it directly, it is used by Russian translation patched by Siberian Studio
	if (Common::File::exists(path)) {
		Common::File directFile;
		if (directFile.open(path)) {
			Common::SeekableReadStream *stream = directFile.readStream(directFile.size());
			directFile.close();
			return stream;
		}
	}

	if (_enhancedEdition) {
		assert(_archive != nullptr);
		return _archive->createReadStreamForMember(path);
	}

	for (int i = 0; i != kArchiveCount; ++i) {
		if (!_archives[i].isOpen()) {
			continue;
		}

		// debug("getResource: Searching archive %s for %s.", _archives[i].getName().c_str(), name.c_str());
		Common::SeekableReadStream *stream = _archives[i].createReadStreamForMember(path);
		if (stream) {
			return stream;
		}
	}

	warning("getResource: Resource %s not found", name.c_str());
	return nullptr;
}

bool BladeRunnerEngine::playerHasControl() {
	return _playerLosesControlCounter == 0;
}

void BladeRunnerEngine::playerLosesControl() {
	if (++_playerLosesControlCounter == 1) {
		_mouse->disable();
	}
}

void BladeRunnerEngine::playerGainsControl(bool force) {
	if (!force && _playerLosesControlCounter == 0) {
		warning("Unbalanced call to BladeRunnerEngine::playerGainsControl");
	}

	if (force) {
		_playerLosesControlCounter = 0;
		_mouse->enable(force);
	} else {
		if (_playerLosesControlCounter > 0) {
			--_playerLosesControlCounter;
		}
		if (_playerLosesControlCounter == 0) {
			_mouse->enable();
		}
	}
}

void BladeRunnerEngine::playerDied() {
	playerLosesControl();

#if BLADERUNNER_ORIGINAL_BUGS
#else
	// reset ammo amounts
	_settings->reset();
	// need to clear kFlagKIAPrivacyAddon to remove Bob's Privacy Addon for KIA
	// so it won't appear here after end credits
	_gameFlags->reset(kFlagKIAPrivacyAddon);

	_ambientSounds->removeAllNonLoopingSounds(true);
	_ambientSounds->removeAllLoopingSounds(4u);
	_music->stop(4u);
	_audioSpeech->stopSpeech();
	// clear subtitles
	_subtitles->clear();

#endif // BLADERUNNER_ORIGINAL_BUGS

	uint32 timeWaitStart = _time->current();
	// unsigned difference is intentional
	while (_time->current() - timeWaitStart < 5000u) {
		gameTick();
	}

	_actorDialogueQueue->flush(1, false);

	while (_playerLosesControlCounter > 0) {
		playerGainsControl();
	}

	_kia->_forceOpen = true;
	_kia->open(kKIASectionLoad);
}

bool BladeRunnerEngine::saveGame(Common::WriteStream &stream, Graphics::Surface *thumb, bool origformat) {
	if ( !_gameIsAutoSaving
	     && ( !playerHasControl() || _sceneScript->isInsideScript() || _aiScripts->isInsideScript())
	) {
		return false;
	}

	Common::MemoryWriteStreamDynamic memoryStream(DisposeAfterUse::YES);
	SaveFileWriteStream s(memoryStream);

	if (!origformat) {
		if (thumb)
			Graphics::saveThumbnail(s, *thumb);
		else
			Graphics::saveThumbnail(s);
	} else {
		thumb->convertToInPlace(gameDataPixelFormat());

		uint16 *thumbnailData = (uint16*)thumb->getPixels();
		for (uint i = 0; i < SaveFileManager::kThumbnailSize / 2; ++i) {
			s.writeUint16LE(thumbnailData[i]);
		}
	}

	s.writeFloat(1.0f);
	_settings->save(s);
	_scene->save(s);
	_scene->_exits->save(s);
	_scene->_regions->save(s);
	_scene->_set->save(s);
	for (uint i = 0; i != _gameInfo->getGlobalVarCount(); ++i) {
		s.writeInt(_gameVars[i]);
	}
	_music->save(s);
	// _audioPlayer->save(s) // zero func
	// _audioSpeech->save(s) // zero func
	_combat->save(s);
	_gameFlags->save(s);
	_items->save(s);
	_sceneObjects->save(s);
	_ambientSounds->save(s);
	_overlays->save(s);
	_spinner->save(s);
	_scores->save(s);
	_dialogueMenu->save(s);
	_obstacles->save(s);
	_actorDialogueQueue->save(s);
	_waypoints->save(s);

	for (uint i = 0; i != _gameInfo->getActorCount(); ++i) {
		_actors[i]->save(s);

		int animationState, animationFrame, animationStateNext, nextAnimation;
		_aiScripts->queryAnimationState(i, &animationState, &animationFrame, &animationStateNext, &nextAnimation);
		s.writeInt(animationState);
		s.writeInt(animationFrame);
		s.writeInt(animationStateNext);
		s.writeInt(nextAnimation);
	}
	_actors[kActorVoiceOver]->save(s);
	_policeMaze->save(s);
	_crimesDatabase->save(s);

	s.finalize();

	stream.writeUint32LE(memoryStream.size() + 4);
	stream.write(memoryStream.getData(), memoryStream.size());
	stream.flush();

	return true;
}

bool BladeRunnerEngine::loadGame(Common::SeekableReadStream &stream, int version) {
	if (!playerHasControl() || _sceneScript->isInsideScript() || _aiScripts->isInsideScript()) {
		return false;
	}

	SaveFileReadStream s(stream);

	_ambientSounds->removeAllNonLoopingSounds(true);
#if BLADERUNNER_ORIGINAL_BUGS
	_ambientSounds->removeAllLoopingSounds(1);
	_music->stop(2);
#else
	// loading into another game that also has music would
	// cause two music tracks to overlap and none was stopped
	_ambientSounds->removeAllLoopingSounds(0u);
	_music->stop(0u);
#endif // BLADERUNNER_ORIGINAL_BUGS
	_audioSpeech->stopSpeech();
	_actorDialogueQueue->flush(true, false);
	// clear subtitles
	_subtitles->clear();

#if BLADERUNNER_ORIGINAL_BUGS
#else
	_screenEffects->toggleEntry(-1, false); // clear the skip list
#endif
	_screenEffects->_entries.clear();

	int size = s.readInt();

	if (size != s.size() - s.pos() + 4) {
		_gameIsLoading = false;
		return false;
	}

	_gameIsLoading = true;
	_settings->setLoadingGame();

	if (version >= 4)
		Graphics::skipThumbnail(s);
	else
		s.skip(SaveFileManager::kThumbnailSize); // skip the thumbnail

	s.skip(4);// always float 1.0, but never used, assuming it's the game version
	_settings->load(s);
	_scene->load(s);
	_scene->_exits->load(s);
	_scene->_regions->load(s);
	_scene->_set->load(s);
	for (uint i = 0; i != _gameInfo->getGlobalVarCount(); ++i) {
		_gameVars[i] = s.readInt();
		if (i == 3 && _gameVars[i] != kBladeRunnerScummVMVersion) {
			warning("This game was saved using an older version of the engine (v%d), currently the engine is at v%d", _gameVars[i], kBladeRunnerScummVMVersion);
		}
	}
	_music->load(s);
	// _audioPlayer->load(s) // zero func
	// _audioSpeech->load(s) // zero func
	_combat->load(s);
	_gameFlags->load(s);

	if ((_gameFlags->query(kFlagGamePlayedInRestoredContentMode) && !_cutContent)
	    || (!_gameFlags->query(kFlagGamePlayedInRestoredContentMode) && _cutContent)
	) {
		Common::U32String warningMsg;
		if (!_cutContent) {
			warningMsg = _("WARNING: This game was saved in Restored Cut Content mode, but you are playing in Original Content mode. The mode will be adjusted to Restored Cut Content for this session until you completely Quit the game.");
		} else {
			warningMsg = _("WARNING: This game was saved in Original Content mode, but you are playing in Restored Cut Content mode. The mode will be adjusted to Original Content mode for this session until you completely Quit the game.");
		}
		GUI::MessageDialog dialog(warningMsg, _("Continue"));
		dialog.runModal();
		_cutContent = !_cutContent;
		// force a Key Down event, since we need it to remove the KIA
		// but it's lost due to the modal dialogue
		Common::EventManager *eventMan = _system->getEventManager();
		Common::Event event;
		event.type = Common::EVENT_KEYDOWN;
		eventMan->pushEvent(event);
	}

	_items->load(s);
	_sceneObjects->load(s);
	_ambientSounds->load(s);
	_overlays->load(s);
	_spinner->load(s);
	_scores->load(s);
	_dialogueMenu->load(s);
	_obstacles->load(s);
	_actorDialogueQueue->load(s);
	_waypoints->load(s);
	for (uint i = 0; i != _gameInfo->getActorCount(); ++i) {
		_actors[i]->load(s);

		int animationState = s.readInt();
		int animationFrame = s.readInt();
		int animationStateNext = s.readInt();
		int nextAnimation = s.readInt();
		_aiScripts->setAnimationState(i, animationState, animationFrame, animationStateNext, nextAnimation);
	}
	_actors[kActorVoiceOver]->load(s);
	_policeMaze->load(s);
	_crimesDatabase->load(s);

	_actorUpdateCounter = 0;
	_actorUpdateTimeLast = 0;
	_gameIsLoading = false;

	_settings->setNewSetAndScene(_settings->getSet(), _settings->getScene());
	_settings->setChapter(_settings->getChapter());
	return true;
}

void BladeRunnerEngine::newGame(int difficulty) {
	// Set a (new) seed for randomness when starting a new game.
	// This also makes sure that if there's a custom random seed set in ScummVM's configuration,
	// that's the one that will be used.
	_newGameRandomSeed = Common::RandomSource::generateNewSeed();
	_rnd.setSeed(_newGameRandomSeed );
	//debug("Random seed for the New Game is: %u", _newGameRandomSeed );

	_settings->reset();
	_combat->reset();

	// clear subtitles
	_subtitles->clear();

	for (uint i = 0; i < _gameInfo->getActorCount(); ++i) {
		_actors[i]->setup(i);
	}
	_actors[kActorVoiceOver]->setup(kActorVoiceOver);

#if BLADERUNNER_ORIGINAL_BUGS
#else
	// Special settings for McCoy that are in BladeRunnerEngine::startup()
	// but are overridden here, resulting to the stamina counter
	// never being initialized in the original game
	_playerActor->setFPS(15); // this seems redundant
	if (!_cutContent) {
		_playerActor->timerStart(kActorTimerRunningStaminaFPS, 200);
	}
#endif // BLADERUNNER_ORIGINAL_BUGS

	for (uint i = 0; i < _gameInfo->getSuspectCount(); ++i) {
		_suspectsDatabase->get(i)->reset();
	}

#if !BLADERUNNER_ORIGINAL_BUGS
	// Fix for Designers Cut setting in a New Game
	// The original game would always clear the setting for Designers Cut when starting a New Game,
	// even if it was set beforehand from the KIA Menu. It would, however, maintain the KIA setting for McCoy's Mood.
	// By not maintaining the value for Designers Cut when starting a New Game, it was impossible to skip a line
	// of dialogue for McCoy which plays at the end of the intro of the game, before the player gains control,
	// and which is actually marked for skipping in Designers Cut mode (see SceneScriptRC01::SceneLoaded())
	//
	// Part 1 of fix for Designers Cut setting
	// Before clearing the _gameFlags check if kFlagDirectorsCut was already set
	bool isDirectorsCut = _gameFlags->query(kFlagDirectorsCut);
#endif // BLADERUNNER_ORIGINAL_BUGS
	_gameFlags->clear();

	for (uint i = 0; i < _gameInfo->getGlobalVarCount(); ++i) {
		_gameVars[i] = 0;
	}

	_items->reset();
	_scores->reset();
	_kia->reset();
	_dialogueMenu->clear();
	_scene->_exits->enable();

	if (difficulty >= 0 && difficulty < 3) {
		_settings->setDifficulty(difficulty);
	}

	InitScript initScript(this);
	initScript.SCRIPT_Initialize_Game();

#if !BLADERUNNER_ORIGINAL_BUGS
	// Part 2 of fix for Designers Cut setting
	// Maintain the flag (if it was set) for the New Game
	// Note: This is done here, since SCRIPT_Initialize_Game() also clears the game flags
	if (isDirectorsCut) {
		_gameFlags->set(kFlagDirectorsCut);
	}
#endif // BLADERUNNER_ORIGINAL_BUGS

	_actorUpdateCounter = 0;
	_actorUpdateTimeLast = 0;
	initChapterAndScene();

	_settings->setStartingGame();
}

void BladeRunnerEngine::autoSaveGame(int textId, bool endgame) {
	TextResource textAutoSave(this);
	if (!textAutoSave.open("AUTOSAVE")) {
		return;
	}
	_gameIsAutoSaving = true;

	SaveStateList saveList = BladeRunner::SaveFileManager::list(getMetaEngine(), getTargetName());

	// Find first available save slot
	int slot = -1;
	int maxSlot = -1;
	for (int i = 0; i < (int)saveList.size(); ++i) {
		maxSlot = MAX(maxSlot, saveList[i].getSaveSlot());
		if (saveList[i].getSaveSlot() != i) {
			slot = i;
			break;
		}
	}

	if (slot == -1) {
		slot = maxSlot + 1;
	}
	if (endgame) {
		saveGameState(slot, "END_GAME_STATE");
	} else {
		saveGameState(slot,  textAutoSave.getText(textId));
	}
	_gameIsAutoSaving = false;
}

void BladeRunnerEngine::ISez(const Common::String &str) {
	debug("\t%s", str.c_str());
}

void BladeRunnerEngine::blitToScreen(const Graphics::Surface &src) const {
	_framelimiter->wait();
	_system->copyRectToScreen(src.getPixels(), src.pitch, 0, 0, src.w, src.h);
	_system->updateScreen();
}

Graphics::Surface BladeRunnerEngine::generateThumbnail() const {
	Graphics::Surface thumbnail;
	thumbnail.create(kOriginalGameWidth / 8, kOriginalGameHeight / 8, gameDataPixelFormat());

	for (int y = 0; y < thumbnail.h; ++y) {
		for (int x = 0; x < thumbnail.w; ++x) {
			uint8 r, g, b;

			uint32  srcPixel = READ_UINT32(_surfaceFront.getBasePtr(CLIP(x * 8, 0, _surfaceFront.w - 1), CLIP(y * 8, 0, _surfaceFront.h - 1)));
			void   *dstPixel = thumbnail.getBasePtr(CLIP(x, 0, thumbnail.w - 1), CLIP(y, 0, thumbnail.h - 1));

			// Throw away alpha channel as it is not needed
			_surfaceFront.format.colorToRGB(srcPixel, r, g, b);
			drawPixel(thumbnail, dstPixel, thumbnail.format.RGBToColor(r, g, b));
		}
	}

	return thumbnail;
}

Common::String BladeRunnerEngine::getTargetName() const {
	return _targetName;
}

void blit(const Graphics::Surface &src, Graphics::Surface &dst) {
	dst.copyRectToSurface(src.getPixels(), src.pitch, 0, 0, src.w, src.h);
}

} // End of namespace BladeRunner
