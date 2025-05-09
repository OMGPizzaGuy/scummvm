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

#include "backends/audiocd/default/default-audiocd.h"
#include "audio/audiostream.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/system.h"
#include "common/util.h"
#include "common/formats/cue.h"

DefaultAudioCDManager::DefaultAudioCDManager() {
	_cd.playing = false;
	_cd.track = 0;
	_cd.start = 0;
	_cd.duration = 0;
	_cd.numLoops = 0;
	_cd.volume = Audio::Mixer::kMaxChannelVolume;
	_cd.balance = 0;
	_mixer = g_system->getMixer();
	_emulating = false;
	assert(_mixer);
}

DefaultAudioCDManager::~DefaultAudioCDManager() {
	// Subclasses should call close as well
	close();
}

bool DefaultAudioCDManager::open() {
	// For emulation, opening is always valid
	close();
	return true;
}

void DefaultAudioCDManager::close() {
	// Only need to stop for emulation
	stop();
}

void DefaultAudioCDManager::fillPotentialTrackNames(Common::Array<Common::String> &trackNames, int track) const {
	trackNames.reserve(4);
	trackNames.push_back(Common::String::format("track%d", track));
	trackNames.push_back(Common::String::format("track%02d", track));
	trackNames.push_back(Common::String::format("track_%d", track));
	trackNames.push_back(Common::String::format("track_%02d", track));
}

bool DefaultAudioCDManager::existExtractedCDAudioFiles(uint track) {
	// keep this in sync with STREAM_FILEFORMATS
	const char *extensions[] = {
#ifdef USE_VORBIS
		"ogg",
#endif
#ifdef USE_FLAC
		"fla", "flac",
#endif
#ifdef USE_MAD
		"mp3",
#endif
		"m4a",
		"wav",
		nullptr
	};

	Common::Array<Common::String> trackNames;
	fillPotentialTrackNames(trackNames, track);

	for (auto &trackName : trackNames) {
		for (const char **ext = extensions; *ext; ++ext) {
			const Common::String &filename = Common::String::format("%s.%s", trackName.c_str(), *ext);
			if (Common::File::exists(Common::Path(filename, '/'))) {
				return true;
			}
		}
	}
	return false;
}

bool DefaultAudioCDManager::play(int track, int numLoops, int startFrame, int duration, bool onlyEmulate,
		Audio::Mixer::SoundType soundType) {
	stop();

	if (numLoops != 0 || startFrame != 0) {
		_cd.track = track;
		_cd.numLoops = numLoops;
		_cd.start = startFrame;
		_cd.duration = duration;

		// Try to load the track from a compressed data file, and if found, use
		// that. If not found, attempt to start regular Audio CD playback of
		// the requested track.
		Common::Array<Common::String> trackNames;
		fillPotentialTrackNames(trackNames, track);
		Audio::SeekableAudioStream *stream = nullptr;

		for (auto &trackName : trackNames) {
			stream = Audio::SeekableAudioStream::openStreamFile(Common::Path(trackName, '/'));
			if (stream)
				break;
		}

		if (stream != nullptr) {
			Audio::Timestamp start = Audio::Timestamp(0, startFrame, 75);
			Audio::Timestamp end = duration ? Audio::Timestamp(0, startFrame + duration, 75) : stream->getLength();

			/*
			FIXME: Seems numLoops == 0 and numLoops == 1 both indicate a single repetition,
			while all other positive numbers indicate precisely the number of desired
			repetitions. Finally, -1 means infinitely many
			*/
			_emulating = true;
			_mixer->playStream(soundType, &_handle,
			                        Audio::makeLoopingAudioStream(stream, start, end, (numLoops < 1) ? numLoops + 1 : numLoops), -1, _cd.volume, _cd.balance);
			return true;
		}
	}

	return false;
}

bool DefaultAudioCDManager::playAbsolute(int startFrame, int numLoops, int duration, bool onlyEmulate,
		Audio::Mixer::SoundType soundType, const char *cuesheet) {

	Common::File cuefile;
	if (!cuefile.open(cuesheet)) {
		return false;
	}
	Common::String cuestring = cuefile.readString(0, cuefile.size());
	Common::CueSheet cue(cuestring.c_str());

	Common::CueSheet::CueTrack *track = cue.getTrackAtFrame(startFrame);
	if (track == nullptr) {
		warning("Unable to locate track for frame %i", startFrame);
		return false;
	} else {
		warning("Playing from frame %i", startFrame);
	}
	int firstFrame = track->indices[0] == -1 ? track->indices[1] : track->indices[0];

	return play(track->number, numLoops, startFrame - firstFrame, duration, onlyEmulate);
}

void DefaultAudioCDManager::stop() {
	if (_emulating) {
		// Audio CD emulation
		_mixer->stopHandle(_handle);
		_emulating = false;
	}
}

bool DefaultAudioCDManager::isPlaying() const {
	// Audio CD emulation
	if (_emulating)
		return _mixer->isSoundHandleActive(_handle);

	// The default class only handles emulation
	return false;
}

void DefaultAudioCDManager::setVolume(byte volume) {
	_cd.volume = volume;

	// Audio CD emulation
	if (_emulating && isPlaying())
		_mixer->setChannelVolume(_handle, _cd.volume);
}

void DefaultAudioCDManager::setBalance(int8 balance) {
	_cd.balance = balance;

	// Audio CD emulation
	if (_emulating && isPlaying())
		_mixer->setChannelBalance(_handle, _cd.balance);
}

void DefaultAudioCDManager::update() {
	if (_emulating) {
		// Check whether the audio track stopped playback
		if (!_mixer->isSoundHandleActive(_handle)) {
			// FIXME: We do not update the numLoops parameter here (and in fact,
			// currently can't do that). Luckily, only one engine ever checks
			// this part of the AudioCD status, namely the SCUMM engine; and it
			// only checks whether the track is currently set to infinite looping
			// or not.
			_emulating = false;
		}
	}
}

DefaultAudioCDManager::Status DefaultAudioCDManager::getStatus() const {
	Status info = _cd;
	info.playing = isPlaying();
	return info;
}

bool DefaultAudioCDManager::openRealCD() {
	Common::String cdrom = ConfMan.get("cdrom");

	// Try to parse it as an int
	char *endPos;
	int drive = strtol(cdrom.c_str(), &endPos, 0);

	// If not an integer, treat as a drive path
	if (endPos == cdrom.c_str())
		return openCD(Common::Path::fromConfig(cdrom));

	if (drive < 0)
		return false;

	return openCD(drive);
}

