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

#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/debug.h"
#include "common/events.h"
#include "common/file.h"
#include "common/random.h"
#include "common/fs.h"
#include "common/keyboard.h"
#include "common/substream.h"
#include "common/str.h"

#include "graphics/surface.h"
#include "graphics/pixelformat.h"

#include "engines/util.h"

#include "prince/prince.h"
#include "prince/graphics.h"
#include "prince/script.h"
#include "prince/debugger.h"
#include "prince/object.h"
#include "prince/mob.h"
#include "prince/music.h"
#include "prince/variatxt.h"
#include "prince/font.h"
#include "prince/mhwanh.h"
#include "prince/cursor.h"
#include "prince/archive.h"
#include "prince/hero.h"
#include "prince/animation.h"
#include "prince/curve_values.h"

namespace Prince {

#ifdef USE_TTS

// Mazovia encoding
static const uint16 polishEncodingTable[] = {
	0x86, 0xc485, 0x8d, 0xc487, 0x8f, 0xc484, 0x90, 0xc498,	// ą, ć, Ą, Ę
	0x91, 0xc499, 0x92, 0xc582, 0x95, 0xc486, 0x98, 0xc59a,	// ę, ł, Ć, Ś
	0x9c, 0xc581, 0x9e, 0xc59b, 0xa0, 0xc5b9, 0xa1, 0xc5bb,	// Ł, ś, Ź, Ż
	0xa2, 0xc3b3, 0xa3, 0xc393, 0xa4, 0xc584, 0xa5, 0xc583,	// ó, Ó, ń, Ń
	0xa6, 0xc5ba, 0xa7, 0xc5bc,								// ź, ż
	0
};

// Custom encoding
static const uint16 russianEncodingTable[] = {
	0x46, 0xd090, 0x66, 0xd0b0, 0x83, 0xd091, 0x92, 0xd0b1,	// А, а, Б, б
	0x44, 0xd092, 0x64, 0xd0b2, 0x55, 0xd093, 0x75, 0xd0b3,	// В, в, Г, г
	0x4c, 0xd094, 0x6c, 0xd0b4, 0x54, 0xd095, 0x74, 0xd0b5,	// Д, д, Е, е
	0x81, 0xd096, 0x8f, 0xd0b6, 0x50, 0xd097, 0x70, 0xd0b7,	// Ж, ж, З, з
	0x42, 0xd098, 0x62, 0xd0b8, 0x51, 0xd099, 0x71, 0xd0b9,	// И, и, Й, й
	0x52, 0xd09a, 0x72, 0xd0ba, 0x4b, 0xd09b, 0x6b, 0xd0bb,	// К, к, Л, л
	0x56, 0xd09c, 0x76, 0xd0bc, 0x59, 0xd09d, 0x79, 0xd0bd,	// М, м, Н, н
	0x4a, 0xd09e, 0x6a, 0xd0be, 0x47, 0xd09f, 0x67, 0xd0bf,	// О, о, П, п
	0x48, 0xd0a0, 0x68, 0xd180, 0x43, 0xd0a1, 0x63, 0xd181,	// Р, р, С, с
	0x4e, 0xd0a2, 0x6e, 0xd182, 0x45, 0xd0a3, 0x65, 0xd183,	// Т, т, У, у
	0x41, 0xd0a4, 0x61, 0xd184, 0x7f, 0xd0a5, 0x85, 0xd185,	// Ф, ф, Х, х
	0x57, 0xd0a6, 0x77, 0xd186, 0x58, 0xd0a7, 0x78, 0xd187,	// Ц, ц, Ч, ч
	0x49, 0xd0a8, 0x69, 0xd188, 0x4f, 0xd0a9, 0x6f, 0xd189,	// Ш, ш, Щ, щ
	0x8d, 0xd18a, 0x53, 0xd0ab, 0x73, 0xd18b, 0x4d, 0xd0ac,	// ъ, Ы, ы, Ь
	0x6d, 0xd18c, 0x82, 0xd0ad, 0x90, 0xd18d, 0x92, 0xd18e,	// ь, Э, э, ю
	0x5a, 0xd0af, 0x7a, 0xd18f,								// Я, я
	0
};

// Custom encoding
static const uint16 spanishEncodingTable[] = {
	0x25, 0xc3ad, 0x26, 0xc3ba, 0x35, 0xc3a1, 0x36, 0xc3a9, // í, ú, á, é
	0x37, 0xc2bf, 0x38, 0xc3b1, 0x3b, 0xc2a1, 0x5f, 0xc3b3, // ¿, ñ, ¡, ó
	0
};

// Custom encoding
static const uint16 germanEncodingTable[] = {
	0x83, 0xc384, 0x84, 0xc396, 0x85, 0xc39c, 0x7f, 0xc39f,  // Ä, Ö, Ü, ß
	0x80, 0xc3a4, 0x81, 0xc3b6, 0x82, 0xc3bc,				 // ä, ö, ü
	0
};

struct CharacterVoiceData {
	uint8 textColor;
	uint8 voiceID;
	uint8 locationNumber;
	int8 mobIndex;
	bool male;
};

static const CharacterVoiceData characterVoiceData[] = {
	{ 220, 0, 0, -1, true },	// Hero
	{ 216, 0, 0, -1, true },	// Hover text
	{ 211, 1, 7, -1, true },	// Bard
	{ 211, 1, 5, 11, true },	// Bard (tavern)
	{ 211, 2, 6, -1, true },	// Witch
	{ 211, 3, 0, -1, true },	// Arivald (all other cases of text color 211)
	{ 202, 4, 1, -1, true },	// Grave digger
	{ 202, 0, 13, -1, false },	// Sheila
	{ 253, 5, 4, -1, true },	// Tall merchant
	{ 253, 6, 54, -1, true },	// Butcher
	{ 225, 7, 4, -1, true },	// Thief
	{ 225, 8, 6, -1, true },	// Alchemist
	{ 225, 1, 7, -1, false },	// Bard's wife
	{ 236, 9, 4, 19, true },	// Fat merchant
	{ 236, 9, 4, 2, true },		// Fat merchant (initial town cutscene)
	{ 236, 10, 25, 4, true },	// Dragon
	{ 236, 2, 0, -1, false },	// Shandria (all other cases of text color 236)
	{ 246, 11, 5, -1, true },	// Monk
	{ 246, 12, 31, -1, true },	// Priest
	{ 195, 13, 0, -1, true },	// Zandahan
	{ 195, 14, 3, -1, true },	// Hermit
	{ 252, 15, 10, -1, true },	// Gate guard
	{ 252, 16, 12, -1, true },	// Courtyard guard
	{ 252, 17, 30, -1, true },	// Passerby
	{ 196, 18, 30, -1, true },	// Modern merchant
	{ 196, 3, 5, -1, false },	// Stranger
	{ 244, 19, 43, -1, true },	// Devil
	{ 244, 20, 0, -1, true },	// Devil
	{ 203, 4, 0, -1, false },	// Witch
	{ 197, 21, 0, -1, true },	// Short merchant
	{ 212, 22, 0, -1, true },	// Merchant cooking soup
	{ 200, 23, 0, -1, true },	// Homunculus
	{ 205, 24, 0, -1, true },	// Beggar
	{ 232, 25, 0, -1, true },	// Dwarf
	{ 208, 26, 0, -1, true },	// Barkeeper
	{ 235, 27, 42, -1, true },	// Devil
	{ 235, 28, 0, -1, true },	// Lord Sun
	{ 201, 5, 0, -1, false },	// Elegant lady
	{ 245, 29, 0, -1, true },	// Devil
	{ 219, 30, 0, -1, true },	// Lucifer
	{ 217, 31, 0, -1, true }	// Narrator
};

static const int kCharacterVoiceDataCount = ARRAYSIZE(characterVoiceData);

#endif

static const uint8 kNarratorTextColor = 217;

void PrinceEngine::debugEngine(const char *s, ...) {
	char buf[STRINGBUFLEN];
	va_list va;

	va_start(va, s);
	vsnprintf(buf, STRINGBUFLEN, s, va);
	va_end(va);

	debug("Prince::Engine %s", buf);
}

PrinceEngine::PrinceEngine(OSystem *syst, const PrinceGameDescription *gameDesc) :
	Engine(syst), _gameDescription(gameDesc), _graph(nullptr), _script(nullptr), _interpreter(nullptr), _flags(nullptr),
	_locationNr(0), _debugger(nullptr), _midiPlayer(nullptr), _room(nullptr),
	_cursor1(nullptr), _cursor2(nullptr), _cursor3(nullptr), _font(nullptr),
	_suitcaseBmp(nullptr), _roomBmp(nullptr), _cursorNr(0), _picWindowX(0), _picWindowY(0), _randomSource("prince"),
	_invLineX(134), _invLineY(176), _invLine(5), _invLines(3), _invLineW(70), _invLineH(76), _maxInvW(72), _maxInvH(76),
	_printMapNotification(false), _invLineSkipX(2), _invLineSkipY(3), _showInventoryFlag(false), _inventoryBackgroundRemember(false),
	_mst_shadow(0), _mst_shadow2(0), _candleCounter(0), _invX1(53), _invY1(18), _invWidth(536), _invHeight(438),
	_invCurInside(false), _optionsFlag(false), _optionEnabled(0), _invExamY(120), _invMaxCount(2), _invCounter(0),
	_optionsMob(-1), _currentPointerNumber(1), _selectedMob(-1), _previousMob(-1), _dialogMob(-1), _selectedItem(0), _selectedMode(0),
	_optionsWidth(210), _optionsHeight(170), _invOptionsWidth(210), _invOptionsHeight(130), _optionsStep(20),
	_invOptionsStep(20), _optionsNumber(7), _invOptionsNumber(5), _optionsColor1(236), _optionsColor2(252),
	_dialogWidth(600), _dialogHeight(0), _dialogLineSpace(10), _dialogColor1(220), _dialogColor2(223),
	_dialogFlag(false), _dialogLines(0), _dialogText(nullptr), _previousSelectedDialog(-1), _isConversing(false), _mouseFlag(1),
	_roomPathBitmap(nullptr), _roomPathBitmapTemp(nullptr), _coordsBufEnd(nullptr), _coordsBuf(nullptr), _coords(nullptr),
	_traceLineLen(0), _rembBitmapTemp(nullptr), _rembBitmap(nullptr), _rembMask(0), _rembX(0), _rembY(0), _fpX(0), _fpY(0),
	_checkBitmapTemp(nullptr), _checkBitmap(nullptr), _checkMask(0), _checkX(0), _checkY(0), _traceLineFirstPointFlag(false),
	_tracePointFirstPointFlag(false), _coordsBuf2(nullptr), _coords2(nullptr), _coordsBuf3(nullptr), _coords3(nullptr),
	_shanLen(0), _directionTable(nullptr), _currentMidi(0), _lightX(0), _lightY(0), _curveData(nullptr), _curvPos(0),
	_creditsData(nullptr), _creditsDataSize(0), _currentTime(0), _zoomBitmap(nullptr), _shadowBitmap(nullptr), _transTable(nullptr),
	_flcFrameSurface(nullptr), _shadScaleValue(0), _shadLineLen(0), _scaleValue(0), _dialogImage(nullptr), _mobTranslationData(nullptr),
	_mobTranslationSize(0), _missingVoice(false), _intro(false), _credits(false) {

	DebugMan.enableDebugChannel("script");

	memset(_audioStream, 0, sizeof(_audioStream));
}

PrinceEngine::~PrinceEngine() {
	delete _rnd;
	delete _cursor1;
	delete _cursor3;
	delete _midiPlayer;
	delete _script;
	delete _flags;
	delete _interpreter;
	delete _font;
	delete _roomBmp;
	delete _suitcaseBmp;
	delete _variaTxt;
	free(_talkTxt);
	free(_invTxt);
	free(_dialogDat);
	delete _graph;
	delete _room;
	//_debugger is deleted by Engine

	if (_cursor2 != nullptr) {
		_cursor2->free();
		delete _cursor2;
	}

	for (uint i = 0; i < _objList.size(); i++) {
		delete _objList[i];
	}
	_objList.clear();

	free(_objSlot);

	for (uint32 i = 0; i < _pscrList.size(); i++) {
		delete _pscrList[i];
	}
	_pscrList.clear();

	for (uint i = 0; i < _maskList.size(); i++) {
		free(_maskList[i]._data);
	}
	_maskList.clear();

	_drawNodeList.clear();

	clearBackAnimList();
	_backAnimList.clear();

	freeAllNormAnims();
	_normAnimList.clear();

	for (uint i = 0; i < _allInvList.size(); i++) {
		_allInvList[i]._surface->free();
		delete _allInvList[i]._surface;
	}
	_allInvList.clear();

	_optionsPic->free();
	delete _optionsPic;

	_optionsPicInInventory->free();
	delete _optionsPicInInventory;

	for (uint i = 0; i < _mainHero->_moveSet.size(); i++) {
		delete _mainHero->_moveSet[i];
	}

	for (uint i = 0; i < _secondHero->_moveSet.size(); i++) {
		delete _secondHero->_moveSet[i];
	}

	delete _mainHero;
	delete _secondHero;

	free(_roomPathBitmap);
	free(_roomPathBitmapTemp);
	free(_coordsBuf);

	_mobPriorityList.clear();

	freeAllSamples();

	free(_zoomBitmap);
	free(_shadowBitmap);
	free(_transTable);

	free(_curveData);

	free(_shadowLine);

	free(_creditsData);

	if (_dialogImage != nullptr) {
		_dialogImage->free();
		delete _dialogImage;
	}

	free(_mobTranslationData);
}

void PrinceEngine::init() {

	const Common::FSNode gameDataDir(ConfMan.getPath("path"));

	debugEngine("Adding all path: %s", gameDataDir.getPath().toString(Common::Path::kNativeSeparator).c_str());

	if (!(getFeatures() & GF_EXTRACTED)) {
		PtcArchive *all = new PtcArchive();
		if (!all->open("all/databank.ptc"))
			error("Can't open all/databank.ptc");

		PtcArchive *voices = new PtcArchive();

		if (!(getFeatures() & GF_NOVOICES)) {
			if (!voices->open("voices/databank.ptc"))
				error("Can't open voices/databank.ptc");
		}

		PtcArchive *sound = new PtcArchive();
		if (!sound->open("sound/databank.ptc"))
			error("Can't open sound/databank.ptc");

		SearchMan.addSubDirectoryMatching(gameDataDir, "all");

		// Prefix the archive names, so that "all" doesn't conflict with the
		// "all" directory, if that happens to be named in all lower case.
		// It isn't on the CD, but we should try to stay case-insensitive.
		SearchMan.add("_all", all);
		SearchMan.add("_voices", voices);
		SearchMan.add("_sound", sound);
	} else {
		SearchMan.addSubDirectoryMatching(gameDataDir, "all");
		SearchMan.addSubDirectoryMatching(gameDataDir, "voices");
		SearchMan.addSubDirectoryMatching(gameDataDir, "sound");
	}

	if (getFeatures() & GF_TRANSLATED) {
		PtcArchive *translation = new PtcArchive();
		if (getFeatures() & GF_TRANSLATED) {
			if (!translation->openTranslation("prince_translation.dat"))
				error("Can't open prince_translation.dat");
		}

		SearchMan.add("translation", translation);
	}

	_graph = new GraphicsMan(this);

	_rnd = new Common::RandomSource("prince");

	_midiPlayer = new MusicPlayer(this);

	if (getLanguage() == Common::DE_DEU) {
		_font = new Font();
		Resource::loadResource(_font, "font3.raw", true);
	} else {
		_font = new Font();
		Resource::loadResource(_font, "font1.raw", true);
	}

	_suitcaseBmp = new MhwanhDecoder();
	Resource::loadResource(_suitcaseBmp, "walizka", true);

	_script = new Script(this);
	Resource::loadResource(_script, "skrypt.dat", true);

	_flags = new InterpreterFlags();
	_interpreter = new Interpreter(this, _script, _flags);

	_debugger = new Debugger(this, _flags);
	setDebugger(_debugger);

	_variaTxt = new VariaTxt();
	if (getFeatures() & GF_TRANSLATED) {
		Resource::loadResource(_variaTxt, "variatxt_translate.dat", true);
	} else {
		Resource::loadResource(_variaTxt, "variatxt.dat", true);
	}

	_cursor1 = new Cursor();
	Resource::loadResource(_cursor1, "mouse1.cur", true);

	_cursor3 = new Cursor();
	Resource::loadResource(_cursor3, "mouse2.cur", true);

	Common::SeekableReadStream *talkTxtStream;
	if (getFeatures() & GF_TRANSLATED) {
		talkTxtStream = SearchMan.createReadStreamForMember("talktxt_translate.dat");
	} else {
		talkTxtStream = SearchMan.createReadStreamForMember("talktxt.dat");
	}
	if (!talkTxtStream) {
		error("Can't load talkTxtStream");
		return;
	}
	_talkTxtSize = talkTxtStream->size();
	_talkTxt = (byte *)malloc(_talkTxtSize);
	talkTxtStream->read(_talkTxt, _talkTxtSize);

	delete talkTxtStream;

	Common::SeekableReadStream *invTxtStream;
	if (getFeatures() & GF_TRANSLATED) {
		invTxtStream = SearchMan.createReadStreamForMember("invtxt_translate.dat");
	} else {
		invTxtStream = SearchMan.createReadStreamForMember("invtxt.dat");
	}
	if (!invTxtStream) {
		error("Can't load invTxtStream");
		return;
	}
	_invTxtSize = invTxtStream->size();
	_invTxt = (byte *)malloc(_invTxtSize);
	invTxtStream->read(_invTxt, _invTxtSize);

	delete invTxtStream;

	loadAllInv();

	Common::SeekableReadStream *dialogDatStream = SearchMan.createReadStreamForMember("dialog.dat");
	if (!dialogDatStream) {
		error("Can't load dialogDatStream");
		return;
	}

	dialogDatStream = Resource::getDecompressedStream(dialogDatStream);

	_dialogDatSize = dialogDatStream->size();
	_dialogDat = (byte *)malloc(_dialogDatSize);
	dialogDatStream->read(_dialogDat, _dialogDatSize);

	delete dialogDatStream;

	_optionsPic = new Graphics::Surface();
	_optionsPic->create(_optionsWidth, _optionsHeight, Graphics::PixelFormat::createFormatCLUT8());
	Common::Rect picRect(0, 0, _optionsWidth, _optionsHeight);
	_optionsPic->fillRect(picRect, _graph->kShadowColor);

	_optionsPicInInventory = new Graphics::Surface();
	_optionsPicInInventory->create(_invOptionsWidth, _invOptionsHeight, Graphics::PixelFormat::createFormatCLUT8());
	Common::Rect invPicRect(0, 0, _invOptionsWidth, _invOptionsHeight);
	_optionsPicInInventory->fillRect(invPicRect, _graph->kShadowColor);

	_roomBmp = new Image::BitmapDecoder();

	_room = new Room();

	_mainHero = new Hero(this, _graph);
	_secondHero = new Hero(this, _graph);
	_secondHero->_maxBoredom = 140;
	_secondHero->loadAnimSet(3);

	_roomPathBitmap = (byte *)malloc(kPathBitmapLen);
	_roomPathBitmapTemp = (byte *)malloc(kPathBitmapLen);
	_coordsBuf = (byte *)malloc(kTracePts * 4);
	_coords = _coordsBuf;
	_coordsBufEnd = _coordsBuf + kTracePts * 4 - 4;

	BackgroundAnim tempBackAnim;
	tempBackAnim._seq._currRelative = 0;
	for (int i = 0; i < kMaxBackAnims; i++) {
		_backAnimList.push_back(tempBackAnim);
	}

	Anim tempAnim;
	tempAnim._animData = nullptr;
	tempAnim._shadowData = nullptr;
	for (int i = 0; i < kMaxNormAnims; i++) {
		_normAnimList.push_back(tempAnim);
	}

	_objSlot = (uint16 *)malloc(kMaxObjects * sizeof(uint16));
	for (int i = 0; i < kMaxObjects; i++) {
		_objSlot[i] = 0xFF;
	}

	_zoomBitmap = (byte *)malloc(kZoomBitmapLen);
	_shadowBitmap = (byte *)malloc(2 * kShadowBitmapSize);
	_transTable = (byte *)malloc(kTransTableSize);

	_curveData = (int16 *)malloc(2 * kCurveLen * sizeof(int16));

	_shadowLine = (byte *)malloc(kShadowLineArraySize);

	Common::SeekableReadStream *creditsDataStream;
	if (getFeatures() & GF_TRANSLATED) {
		creditsDataStream = SearchMan.createReadStreamForMember("credits_translate.dat");
	} else {
		creditsDataStream = SearchMan.createReadStreamForMember("credits.dat");
	}
	if (!creditsDataStream) {
		error("Can't load creditsDataStream");
		return;
	}
	_creditsDataSize = creditsDataStream->size();
	_creditsData = (byte *)malloc(_creditsDataSize);
	creditsDataStream->read(_creditsData, _creditsDataSize);
	delete creditsDataStream;

	if (getFeatures() & GF_TRANSLATED) {
		loadMobTranslationTexts();
	}

	Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();
	if (ttsMan != nullptr) {
		ttsMan->enable(ConfMan.getBool("tts_enabled_objects") || ConfMan.getBool("tts_enabled_speech") || ConfMan.getBool("tts_enabled_missing_voice"));
		ttsMan->setLanguage(ConfMan.get("language"));
	}
}

void PrinceEngine::showLogo() {
	MhwanhDecoder logo;
	if (Resource::loadResource(&logo, "logo.raw", true)) {
		loadSample(0, "LOGO.WAV");
		playSample(0, 0);
		_graph->draw(_graph->_frontScreen, logo.getSurface());
		_graph->change();
		_graph->update(_graph->_frontScreen);
		setPalette(logo.getPalette().data());

		uint32 logoStart = _system->getMillis();
		while (_system->getMillis() < logoStart + 5000) {
			Common::Event event;
			Common::EventManager *eventMan = _system->getEventManager();
			while (eventMan->pollEvent(event)) {
				switch (event.type) {
				case Common::EVENT_KEYDOWN:
					if (event.kbd.keycode == Common::KEYCODE_ESCAPE) {
						stopSample(0);
						return;
					}
					break;
				case Common::EVENT_LBUTTONDOWN:
					stopSample(0);
					return;
				default:
					break;
				}
			}

			if (shouldQuit()) {
				return;
			}
		}
	}
}

Common::Error PrinceEngine::run() {
	syncSoundSettings();
	int startGameSlot = ConfMan.hasKey("save_slot") ? ConfMan.getInt("save_slot") : -1;
	init();
	if (startGameSlot == -1) {
		_intro = true;
		playVideo("topware.avi");
		showLogo();
	} else {
		loadLocation(59); // load intro location - easiest way to set everything up
		loadGame(startGameSlot);
	}
	mainLoop();
	return Common::kNoError;
}

void PrinceEngine::pauseEngineIntern(bool pause) {
	Engine::pauseEngineIntern(pause);

	if (_midiPlayer) {
		if (pause) {
			_midiPlayer->pause();
		} else {
			_midiPlayer->resume();
		}
	}
}

void PrinceEngine::setShadowScale(int32 shadowScale) {
	shadowScale = 100 - shadowScale;
	if (!shadowScale) {
		_shadScaleValue = 10000;
	} else {
		_shadScaleValue = 10000 / shadowScale;
	}
}

bool PrinceEngine::playNextFLCFrame() {
	if (!_flicPlayer.isVideoLoaded())
		return false;

	const Graphics::Surface *s = _flicPlayer.decodeNextFrame();
	if (s) {
		_graph->drawTransparentSurface(_graph->_frontScreen, 0, 0, s, 255);
		_graph->change();
		_flcFrameSurface = s;
	} else if (_flicLooped) {
		_flicPlayer.rewind();
		playNextFLCFrame();
	} else if (_flcFrameSurface) {
		_graph->drawTransparentSurface(_graph->_frontScreen, 0, 0, _flcFrameSurface, 255);
		_graph->change();
	}

	return true;
}

void PrinceEngine::loadMobTranslationTexts() {
	Common::SeekableReadStream *mobTranslationStream = SearchMan.createReadStreamForMember("mob_translate.dat");
	if (!mobTranslationStream) {
		error("Can't load mob_translate.dat");
	}
	_mobTranslationSize = mobTranslationStream->size();
	_mobTranslationData = (byte *)malloc(_mobTranslationSize);
	mobTranslationStream->read(_mobTranslationData, _mobTranslationSize);
	delete mobTranslationStream;
}

void PrinceEngine::setMobTranslationTexts() {
	int locationOffset = READ_LE_UINT16(_mobTranslationData + (_locationNr - 1) * 2);
	if (locationOffset) {
		byte *locationText = _mobTranslationData + locationOffset;
		for (uint i = 0; i < _mobList.size(); i++) {
			byte c;
			locationText++;
			_mobList[i]._name.clear();
			while ((c = *locationText)) {
				_mobList[i]._name += c;
				locationText++;
			}
			locationText++;
			_mobList[i]._examText.clear();
			c = *locationText;
			locationText++;
			if (c) {
				_mobList[i]._examText += c;
				do {
					c = *locationText;
					_mobList[i]._examText += c;
					locationText++;
				} while (c != 255);
			}
		}
	}
}

void PrinceEngine::keyHandler(Common::Event event) {
	uint16 nChar = event.kbd.keycode;
	switch (nChar) {
	case Common::KEYCODE_F1:
		if (canLoadGameStateCurrently())
			scummVMSaveLoadDialog(false);
		break;
	case Common::KEYCODE_F2:
		if (canSaveGameStateCurrently())
			scummVMSaveLoadDialog(true);
		break;
	case Common::KEYCODE_z:
		if (_flags->getFlagValue(Flags::POWERENABLED)) {
			_flags->setFlagValue(Flags::MBFLAG, 1);
		}
		break;
	case Common::KEYCODE_x:
		if (_flags->getFlagValue(Flags::POWERENABLED)) {
			_flags->setFlagValue(Flags::MBFLAG, 2);
		}
		break;
	case Common::KEYCODE_ESCAPE:
		if (_intro) {
			stopTextToSpeech();
			_intro = false;
		}

		_flags->setFlagValue(Flags::ESCAPED2, 1);
		break;
	default:
		break;
	}
}

void PrinceEngine::printAt(uint32 slot, uint8 color, char *s, uint16 x, uint16 y) {
	debugC(1, DebugChannel::kEngine, "PrinceEngine::printAt slot %d, color %d, x %02d, y %02d, str %s", slot, color, x, y, s);

	if (getLanguage() == Common::DE_DEU)
		correctStringDEU(s);

	// Cutscene
	if (slot == 9) {
		setTTSVoice(color);
		sayText(s, true, Common::TextToSpeechManager::QUEUE);
	} else {
		bool printText = true;
		bool isSpeech = false;

		if (slot == 10) {
			if (_locationNr == 50) {	// Map
				if (_printMapNotification) {
					_printMapNotification = false;
				} else {
					printText = false;
				}
			} else {
				isSpeech = true;
			}
		} else if (slot == 0) {
			isSpeech = true;
		}

		if (printText) {
			setTTSVoice(color);
			sayText(s, isSpeech);
		}
	}

	Text &text = _textSlots[slot];
	text._str = s;
	text._x = x;
	text._y = y;
	text._color = color;
	int lines = calcTextLines(s);
	text._time = calcTextTime(lines);
}

int PrinceEngine::calcTextLines(const char *s) {
	int lines = 1;
	while (*s) {
		if (*s == '\n') {
			lines++;
		}
		s++;
	}
	return lines;
}

int PrinceEngine::calcTextTime(int numberOfLines) {
	return numberOfLines * 30;
}

void PrinceEngine::correctStringDEU(char *s) {
	while (*s) {
		switch (*s) {
		case '\xc4':
			*s = '\x83';
			break;
		case '\xd6':
			*s = '\x84';
			break;
		case '\xdc':
			*s = '\x85';
			break;
		case '\xdf':
			*s = '\x7f';
			break;
		case '\xe4':
			*s = '\x80';
			break;
		case '\xf6':
			*s = '\x81';
			break;
		case '\xfc':
			*s = '\x82';
			break;
		default:
			break;
		}
		s++;
	}
}

uint32 PrinceEngine::getTextWidth(const char *s) {
	uint16 textW = 0;
	while (*s) {
		textW += _font->getCharWidth(*s) + _font->getKerningOffset(0, 0);
		s++;
	}
	return textW;
}

void PrinceEngine::showTexts(Graphics::Surface *screen) {
	Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();

	for (uint32 slot = 0; slot < kMaxTexts; slot++) {

		if (_showInventoryFlag && slot) {
			// only slot 0 for inventory
			break;
		}

		Text& text = _textSlots[slot];
		if (!text._str && !text._time) {
			continue;
		}

		int x = text._x;
		int y = text._y;

		if (!_showInventoryFlag) {
			x -= _picWindowX;
			y -= _picWindowY;
		}

		Common::Array<Common::String> lines;
		_font->wordWrapText(text._str, _graph->_frontScreen->w, lines);

		int wideLine = 0;
		for (uint i = 0; i < lines.size(); i++) {
			int textLen = getTextWidth(lines[i].c_str());
			if (textLen > wideLine) {
				wideLine = textLen;
			}
		}

		int leftBorderText = 6;
		if (x + wideLine / 2 >  kNormalWidth - leftBorderText) {
			x = kNormalWidth - leftBorderText - wideLine / 2;
		}

		if (x - wideLine / 2 < leftBorderText) {
			x = leftBorderText + wideLine / 2;
		}

		int textSkip = 2;
		for (uint i = 0; i < lines.size(); i++) {
			int drawX = x - getTextWidth(lines[i].c_str()) / 2;
			int drawY = y - 10 - (lines.size() - i) * (_font->getFontHeight() - textSkip);
			if (drawX < 0) {
				drawX = 0;
			}
			if (drawY < 0) {
				drawY = 0;
			}
			_font->drawString(screen, lines[i], drawX, drawY, screen->w, text._color);
		}

		text._time--;
		if (!text._time) {
			if (ttsMan != nullptr && (ConfMan.getBool("tts_enabled_speech") || 
				ConfMan.getBool("tts_enabled_objects") || ConfMan.getBool("tts_enabled_missing_voice")) && ttsMan->isSpeaking()) {
				text._time = 1;
				continue;
			}
			text._str = nullptr;
		}
	}
}

void PrinceEngine::sayText(const Common::String &text, bool isSpeech, Common::TextToSpeechManager::Action action) {
	Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();
	// Only voice subtitles if either this is a version with no voices or the speech volume is muted (the English/Spanish
	// translations still have dubs in different languages, so don't voice the subtitles unless the dub is muted)
	bool speak = (!isSpeech && ConfMan.getBool("tts_enabled_objects")) || 
				 (isSpeech && ConfMan.getBool("tts_enabled_speech") && 
				 (getFeatures() & GF_NOVOICES || ConfMan.getInt("speech_volume") == 0 || ConfMan.getBool("subtitles"))); 
	if (ttsMan != nullptr && speak) {
		Common::String ttsText(text);
		// Some emotive text has a < at the front, which causes the entire text to not be voiced by the TTS system
		// Text with quotation marks also contains \ as an escape character, which is awkwardly voiced by TTS if not
		// removed
		ttsText.replace('\n', ' ');
		ttsText.replace('<', ' ');
		ttsText.replace('\\', ' ');
#ifdef USE_TTS
		ttsMan->say(convertText(ttsText), action);
#endif
	}
}

#ifdef USE_TTS

Common::U32String PrinceEngine::convertText(const Common::String &text) const {
	const uint16 *conversionTable;

	switch (getLanguage()) {
	case Common::EN_ANY:	// Some of the English text has a few Polish characters
	case Common::PL_POL:
		conversionTable = polishEncodingTable;
		break;
	case Common::RU_RUS:
		if (getFeatures() & GF_RUSPROJEDITION) {
			return Common::U32String(text, Common::CodePage::kDos866);
		}

		conversionTable = russianEncodingTable;
		break;
	case Common::DE_DEU:
		conversionTable = germanEncodingTable;
		break;
	case Common::ES_ESP:
		conversionTable = spanishEncodingTable;
		break;
	default:
		conversionTable = polishEncodingTable;
	}

	const byte *bytes = (const byte *)text.c_str();
	byte *convertedBytes = new byte[text.size() * 2 + 1];

	int i = 0;
	for (const byte *b = bytes; *b; ++b) {
		bool inTable = checkConversionTable(b, i, convertedBytes, conversionTable);
		
		if (_credits && !inTable) {
			if (*b == 0x2a) {	// * in credits
				convertedBytes[i] = 0x20;
				i++;
				continue;
			}

			if (*b == 0x23) {
				i++;
				break;
			}

			// Credits in other languages may have some Polish characters
			inTable = checkConversionTable(b, i, convertedBytes, polishEncodingTable);
		}

		if (!inTable) {
			convertedBytes[i] = *b;
			i++;
		}
	}

	convertedBytes[i] = 0;

	Common::U32String result((char *)convertedBytes);
	delete[] convertedBytes;

	return result;
}

bool PrinceEngine::checkConversionTable(const byte *character, int &index, byte *convertedBytes, const uint16 *table) const {
	for (int i = 0; table[i]; i += 2) {
		if (*character == table[i]) {
			convertedBytes[index] = (table[i + 1] >> 8) & 0xff;
			convertedBytes[index + 1] = table[i + 1] & 0xff;
			index += 2;
			return true;
		}
	}

	return false;
}

#endif

void PrinceEngine::setTTSVoice(uint8 textColor) const {
#ifdef USE_TTS
	Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();
	if (ttsMan != nullptr && (ConfMan.getBool("tts_enabled_speech") || ConfMan.getBool("tts_enabled_objects") || ConfMan.getBool("tts_enabled_missing_voice"))) {
		int id = 0;

		for (int i = 0; i < kCharacterVoiceDataCount; ++i) {
			// In many cases, characters can be differentiated by just the text color, but sometimes
			// there may be different characters with the same text colors in different locations, and rarely
			// different characters with the same text colors in the same location. Using the location number and/or
			// mob index differentiates characters in these cases
			if (characterVoiceData[i].textColor == textColor && 
				(characterVoiceData[i].locationNumber == 0 || characterVoiceData[i].locationNumber == _locationNr) &&
				(characterVoiceData[i].mobIndex == -1 || characterVoiceData[i].mobIndex == _dialogMob)) {
				id = i;
				break;
			}
		}

		Common::Array<int> voices;
		int pitch = 0;
		Common::TTSVoice::Gender gender;

		if (characterVoiceData[id].male) {
			voices = ttsMan->getVoiceIndicesByGender(Common::TTSVoice::MALE);
			gender = Common::TTSVoice::MALE;
		} else {
			voices = ttsMan->getVoiceIndicesByGender(Common::TTSVoice::FEMALE);
			gender = Common::TTSVoice::FEMALE;
		}

		// If no voice is available for the necessary gender, set the voice to default
		if (voices.empty()) {
			ttsMan->setVoice(0);
		} else {
			int voiceIndex = characterVoiceData[id].voiceID % voices.size();
			ttsMan->setVoice(voices[voiceIndex]);
		}

		// If no voices are available for this gender, alter the pitch to mimic a voice
		// of the other gender
		if (ttsMan->getVoice().getGender() != gender) {
			if (gender == Common::TTSVoice::MALE) {
				pitch -= 50;
			} else {
				pitch += 50;
			}
		}

		ttsMan->setPitch(pitch);
	}
#endif
}

void PrinceEngine::stopTextToSpeech() const {
	Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();
	if (ttsMan != nullptr && (ConfMan.getBool("tts_enabled_objects") || ConfMan.getBool("tts_enabled_speech") || ConfMan.getBool("tts_enabled_missing_voice")) &&
		ttsMan->isSpeaking()) {
		ttsMan->stop();
	}
}

void PrinceEngine::pausePrinceEngine(int fps) {
	int delay = 1000 / fps - int32(_system->getMillis() - _currentTime);
	delay = delay < 0 ? 0 : delay;
	_system->delayMillis(delay);
	_currentTime = _system->getMillis();
}

void PrinceEngine::leftMouseButton() {
	_flags->setFlagValue(Flags::ESCAPED2, 1); // skip intro animation
	_flags->setFlagValue(Flags::LMOUSE, 1);
	if (_flags->getFlagValue(Flags::POWERENABLED)) {
		_flags->setFlagValue(Flags::MBFLAG, 1);
	}
	if (_mouseFlag) {
		int option = 0;
		int optionEvent = -1;

		if (_optionsFlag) {
			if (_optionEnabled < _optionsNumber && _optionEnabled != -1) {
				option = _optionEnabled;
				_optionsFlag = 0;
			} else {
				return;
			}
		} else {
			_optionsMob = _selectedMob;
			if (_optionsMob == -1) {
				walkTo();
				return;
			}
			option = 0;
		}
		//do_option
		if (_currentPointerNumber != 2) {
			//skip_use_code
			int optionScriptOffset = _room->getOptionOffset(option);
			if (optionScriptOffset != 0) {
				optionEvent = _script->scanMobEvents(_optionsMob, optionScriptOffset);
			}
			if (optionEvent == -1) {
				if (!option) {
					walkTo();
					return;
				} else {
					optionEvent = _script->getOptionStandardOffset(option);
				}
			}
		} else if (_selectedMode) {
			//give_item
			if (_room->_itemGive) {
				optionEvent = _script->scanMobEventsWithItem(_optionsMob, _room->_itemGive, _selectedItem);
			}
			if (optionEvent == -1) {
				//standard_giveitem
				optionEvent = _script->_scriptInfo.stdGiveItem;
			}
		} else {
			if (_room->_itemUse) {
				optionEvent = _script->scanMobEventsWithItem(_optionsMob, _room->_itemUse, _selectedItem);
				_flags->setFlagValue(Flags::SELITEM, _selectedItem);
			}
			if (optionEvent == -1) {
				//standard_useitem
				optionEvent = _script->_scriptInfo.stdUseItem;
			}
		}
		_interpreter->storeNewPC(optionEvent);
		_flags->setFlagValue(Flags::CURRMOB, _selectedMob);
		_dialogMob = _selectedMob;
		_selectedMob = -1;
		_optionsMob = -1;
	} else {
		if (_intro) {
			stopTextToSpeech();
			_intro = false;
		}

		if (!_flags->getFlagValue(Flags::POWERENABLED)) {
			if (!_flags->getFlagValue(Flags::NOCLSTEXT)) {
				for (int slot = 0; slot < kMaxTexts; slot++) {
					if (slot != 9) {
						Text& text = _textSlots[slot];
						if (!text._str) {
							continue;
						}
						stopTextToSpeech();
						text._str = nullptr;
						text._time = 0;
					}
				}
				_mainHero->_talkTime = 0;
				_secondHero->_talkTime = 0;
			}
		}
	}
}

void PrinceEngine::rightMouseButton() {
	if (_flags->getFlagValue(Flags::POWERENABLED)) {
		_flags->setFlagValue(Flags::MBFLAG, 2);
	}
	if (_mouseFlag && _mouseFlag != 3) {
		_mainHero->freeOldMove();
		_secondHero->freeOldMove();
		_interpreter->storeNewPC(0);
		if (_currentPointerNumber < 2) {
			enableOptions(true);
		} else {
			_currentPointerNumber = 1;
			changeCursor(1);
		}
	}
}

void PrinceEngine::createDialogBox(int dialogBoxNr) {
	_dialogLines = 0;
	int amountOfDialogOptions = 0;
	int dialogDataValue = (int)READ_LE_UINT32(_dialogData);

	byte c;
	int sentenceNumber;
	_dialogText = _dialogBoxAddr[dialogBoxNr];
	byte *dialogText = _dialogText;

	while ((sentenceNumber = *dialogText) != 0xFF) {
		dialogText++;
		if (!(dialogDataValue & (1 << sentenceNumber))) {
			_dialogLines += calcTextLines((const char *)dialogText);
			amountOfDialogOptions++;
		}
		do {
			c = *dialogText;
			dialogText++;
		} while (c);
	}

	_dialogHeight = _font->getFontHeight() * _dialogLines + _dialogLineSpace * (amountOfDialogOptions + 1);
	_dialogImage = new Graphics::Surface();
	_dialogImage->create(_dialogWidth, _dialogHeight, Graphics::PixelFormat::createFormatCLUT8());
	Common::Rect dBoxRect(0, 0, _dialogWidth, _dialogHeight);
	_dialogImage->fillRect(dBoxRect, _graph->kShadowColor);
}

void PrinceEngine::dialogRun() {

	_dialogFlag = true;
	setTTSVoice(kHeroTextColor);

	while (!shouldQuit()) {

		_interpreter->stepBg();
		drawScreen();

		int dialogX = (640 - _dialogWidth) / 2;
		int dialogY = 460 - _dialogHeight;
		_graph->drawAsShadowSurface(_graph->_frontScreen, dialogX, dialogY, _dialogImage, _graph->_shadowTable50);

		int dialogSkipLeft = 14;
		int dialogSkipUp = 10;

		int dialogTextX = dialogX + dialogSkipLeft;
		int dialogTextY = dialogY + dialogSkipUp;

		Common::Point mousePos = _system->getEventManager()->getMousePos();

		byte c;
		int sentenceNumber;
		byte *dialogText = _dialogText;
		byte *dialogCurrentText = nullptr;
		int dialogSelected = -1;
		int dialogDataValue = (int)READ_LE_UINT32(_dialogData);

		while ((sentenceNumber = *dialogText) != 0xFF) {
			dialogText++;
			int actualColor = _dialogColor1;

			if (!(dialogDataValue & (1 << sentenceNumber))) {
				if (getLanguage() == Common::DE_DEU) {
					correctStringDEU((char *)dialogText);
				}
				Common::Array<Common::String> lines;
				_font->wordWrapText((const char *)dialogText, _graph->_frontScreen->w, lines);

				Common::Rect dialogOption(dialogTextX, dialogTextY - dialogSkipUp / 2, dialogX + _dialogWidth - dialogSkipLeft, dialogTextY + lines.size() * _font->getFontHeight() + dialogSkipUp / 2 - 1);
				if (dialogOption.contains(mousePos)) {
					actualColor = _dialogColor2;
					dialogSelected = sentenceNumber;
					dialogCurrentText = dialogText;

					if (_previousSelectedDialog != dialogSelected) {
						sayText((const char *)dialogCurrentText, false);
						_previousSelectedDialog = dialogSelected;
					}
				}

				for (uint j = 0; j < lines.size(); j++) {
					_font->drawString(_graph->_frontScreen, lines[j], dialogTextX, dialogTextY, _graph->_frontScreen->w, actualColor);
					dialogTextY += _font->getFontHeight();
				}
				dialogTextY += _dialogLineSpace;
			}
			do {
				c = *dialogText;
				dialogText++;
			} while (c);
		}

		if (dialogSelected == -1) {
			_previousSelectedDialog = -1;
		}

		Common::Event event;
		Common::EventManager *eventMan = _system->getEventManager();
		while (eventMan->pollEvent(event)) {
			switch (event.type) {
			case Common::EVENT_KEYDOWN:
				keyHandler(event);
				break;
			case Common::EVENT_LBUTTONDOWN:
				if (dialogSelected != -1) {
					dialogLeftMouseButton(dialogCurrentText, dialogSelected);
					_dialogFlag = false;
				}
				break;
			default:
				break;
			}
		}

		if (shouldQuit()) {
			return;
		}

		if (!_dialogFlag) {
			break;
		}


		_graph->update(_graph->_frontScreen);
		pausePrinceEngine();
	}
	_dialogImage->free();
	delete _dialogImage;
	_dialogImage = nullptr;
	_dialogFlag = false;
}

void PrinceEngine::dialogLeftMouseButton(byte *string, int dialogSelected) {
	_interpreter->setString(string);
	talkHero(0);

	int dialogDataValue = (int)READ_LE_UINT32(_dialogData);
	dialogDataValue |= (1u << dialogSelected);
	WRITE_LE_UINT32(_dialogData, dialogDataValue);

	_flags->setFlagValue(Flags::BOXSEL, dialogSelected + 1);
	setVoice(0, 28, dialogSelected + 1);

	_flags->setFlagValue(Flags::VOICE_H_LINE, _dialogOptLines[dialogSelected * 4]);
	_flags->setFlagValue(Flags::VOICE_A_LINE, _dialogOptLines[dialogSelected * 4 + 1]);
	_flags->setFlagValue(Flags::VOICE_B_LINE, _dialogOptLines[dialogSelected * 4 + 2]);

	_interpreter->setString(_dialogOptAddr[dialogSelected]);
}

void PrinceEngine::talkHero(int slot) {
	// heroSlot = textSlot (slot 0 or 1)
	Text &text = _textSlots[slot];
	int lines = calcTextLines((const char *)_interpreter->getString());
	int time = lines * 30;

	if (slot == 0) {
		text._color = 220; // TODO - test this
		_mainHero->_state = Hero::kHeroStateTalk;
		_mainHero->_talkTime = time;
		text._x = _mainHero->_middleX;
		text._y = _mainHero->_middleY - _mainHero->_scaledFrameYSize;
	} else {
		text._color = _flags->getFlagValue(Flags::KOLOR); // TODO - test this
		_secondHero->_state = Hero::kHeroStateTalk;
		_secondHero->_talkTime = time;
		text._x = _secondHero->_middleX;
		text._y = _secondHero->_middleY - _secondHero->_scaledFrameYSize;
	}
	text._time = time;
	if (getLanguage() == Common::DE_DEU) {
		correctStringDEU((char *)_interpreter->getString());
	}
	text._str = (const char *)_interpreter->getString();

	setTTSVoice(text._color);
	sayText(text._str, true);

	_interpreter->increaseString();
}

void PrinceEngine::getCurve() {
	_flags->setFlagValue(Flags::TORX1, _curveData[_curvPos]);
	_flags->setFlagValue(Flags::TORY1, _curveData[_curvPos + 1]);
	_curvPos += 2;
}

void PrinceEngine::makeCurve() {
	_curvPos = 0;
	int x1 = _flags->getFlagValue(Flags::TORX1);
	int y1 = _flags->getFlagValue(Flags::TORY1);
	int x2 = _flags->getFlagValue(Flags::TORX2);
	int y2 = _flags->getFlagValue(Flags::TORY2);

	for (int i = 0; i < kCurveLen; i++) {
		int sum1 = x1 * curveValues[i][0];
		sum1 += (x2 + (x1 - x2) / 2) * curveValues[i][1];
		sum1 += x2 * curveValues[i][2];
		sum1 += x2 * curveValues[i][3];

		int sum2 = y1 * curveValues[i][0];
		sum2 += (y2 - 20) * curveValues[i][1];
		sum2 += (y2 - 10) * curveValues[i][2];
		sum2 += y2 * curveValues[i][3];

		_curveData[i * 2] = (sum1 >> 15);
		_curveData[i * 2 + 1] = (sum2 >> 15);
	}
}

void PrinceEngine::mouseWeirdo() {
	if (_mouseFlag == 3) {
		int weirdDir = _randomSource.getRandomNumber(3);
		Common::Point mousePos = _system->getEventManager()->getMousePos();
		switch (weirdDir) {
		case 0:
			mousePos.x += kCelStep;
			break;
		case 1:
			mousePos.x -= kCelStep;
			break;
		case 2:
			mousePos.y += kCelStep;
			break;
		case 3:
			mousePos.y -= kCelStep;
			break;
		default:
			break;
		}
		mousePos.x = CLIP(mousePos.x, (int16) 315, (int16) 639);
		_flags->setFlagValue(Flags::MXFLAG, mousePos.x);
		mousePos.y = CLIP(mousePos.y, (int16) 0, (int16) 170);
		_flags->setFlagValue(Flags::MYFLAG, mousePos.y);
		_system->warpMouse(mousePos.x, mousePos.y);
	}
}

void PrinceEngine::showPower() {
	if (_flags->getFlagValue(Flags::POWERENABLED)) {
		int power = _flags->getFlagValue(Flags::POWER);

		byte *dst = (byte *)_graph->_frontScreen->getBasePtr(kPowerBarPosX, kPowerBarPosY);
		for (int y = 0; y < kPowerBarHeight; y++) {
			byte *dst2 = dst;
			for (int x = 0; x < kPowerBarWidth; x++, dst2++) {
				*dst2 = kPowerBarBackgroundColor;
			}
			dst += _graph->_frontScreen->pitch;
		}

		if (power) {
			dst = (byte *)_graph->_frontScreen->getBasePtr(kPowerBarPosX, kPowerBarGreenPosY);
			for (int y = 0; y < kPowerBarGreenHeight; y++) {
				byte *dst2 = dst;
				for (int x = 0; x < power + 1; x++, dst2++) {
					if (x < 58) {
						*dst2 = kPowerBarGreenColor1;
					} else {
						*dst2 = kPowerBarGreenColor2;
					}
				}
				dst += _graph->_frontScreen->pitch;
			}
		}

		_graph->change();
	}
}

void PrinceEngine::scrollCredits() {
	byte *scrollAdress = _creditsData;

	_credits = true;
	setTTSVoice(kNarratorTextColor);
	if (getLanguage() == Common::DE_DEU) {
		correctStringDEU((char *)scrollAdress);
	}
	sayText((char *)scrollAdress, false, Common::TextToSpeechManager::INTERRUPT);

	while (!shouldQuit()) {
		for (int scrollPos = 0; scrollPos > -23; scrollPos--) {
			const Graphics::Surface *roomSurface = _roomBmp->getSurface();
			if (roomSurface) {
				_graph->draw(_graph->_frontScreen, roomSurface);
			}
			char *s = (char *)scrollAdress;
			int drawY = scrollPos;
			for (int i = 0; i < 22; i++) {
				Common::String line;
				char *linePos = s;
				while ((*linePos != 13)) {
					line += *linePos;
					linePos++;
				}
				if (!line.empty()) {
					int drawX = (kNormalWidth - getTextWidth(line.c_str())) / 2;
					_font->drawString(_graph->_frontScreen, line, drawX, drawY, _graph->_frontScreen->w, 217);
				}

				char letter1;
				bool gotIt1 = false;
				do {
					letter1 = *s;
					s++;
					if (letter1 == 13) {
						if (*s == 10) {
							s++;
						}
						if (*s != 35) {
							gotIt1 = true;
						}
						break;
					}
				} while (letter1 != 35);

				if (gotIt1) {
					drawY += 23;
				} else {
					break;
				}
			}
			Common::Event event;
			Common::EventManager *eventMan = _system->getEventManager();
			while (eventMan->pollEvent(event)) {
				if (event.type == Common::EVENT_KEYDOWN) {
					if (event.kbd.keycode == Common::KEYCODE_ESCAPE) {
						blackPalette();
						return;
					}
				}
			}
			if (shouldQuit()) {
				return;
			}
			_graph->change();
			_graph->update(_graph->_frontScreen);
			pausePrinceEngine(kFPS * 2);
		}
		char letter2;
		byte *scan2 = scrollAdress;
		bool gotIt2 = false;
		do {
			letter2 = *scan2;
			scan2++;
			if (letter2 == 13) {
				if (*scan2 == 10) {
					scan2++;
				}
				if (*scan2 != 35) {
					gotIt2 = true;
				}
				break;
			}
		} while (letter2 != 35);
		if (gotIt2) {
			scrollAdress = scan2;
		} else {
			break;
		}
	}
	blackPalette();
}

void PrinceEngine::mainLoop() {
	changeCursor(0);
	_currentTime = _system->getMillis();

	while (!shouldQuit()) {
		Common::Event event;
		Common::EventManager *eventMan = _system->getEventManager();
		while (eventMan->pollEvent(event)) {
			switch (event.type) {
			case Common::EVENT_KEYDOWN:
				keyHandler(event);
				break;
			case Common::EVENT_LBUTTONDOWN:
				leftMouseButton();
				break;
			case Common::EVENT_RBUTTONDOWN:
				rightMouseButton();
				break;
			default:
				break;
			}
		}

		if (shouldQuit()) {
			return;
		}

		// for "throw a rock" mini-game
		mouseWeirdo();

		_interpreter->stepBg();
		_interpreter->stepFg();

		drawScreen();

		_graph->update(_graph->_frontScreen);

		openInventoryCheck();

		pausePrinceEngine();
	}
}

} // End of namespace Prince
