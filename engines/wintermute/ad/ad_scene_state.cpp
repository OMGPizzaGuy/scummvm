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

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/ad/ad_scene_state.h"
#include "engines/wintermute/ad/ad_node_state.h"
#include "engines/wintermute/persistent.h"
#include "engines/wintermute/platform_osystem.h"
#include "common/str.h"

namespace Wintermute {

IMPLEMENT_PERSISTENT(AdSceneState, false)

//////////////////////////////////////////////////////////////////////////
AdSceneState::AdSceneState(BaseGame *inGame) : BaseClass(inGame) {
	_filename = nullptr;
}


//////////////////////////////////////////////////////////////////////////
AdSceneState::~AdSceneState() {
	delete[] _filename;
	_filename = nullptr;

	for (uint32 i = 0; i < _nodeStates.getSize(); i++) {
		delete _nodeStates[i];
	}
	_nodeStates.removeAll();
}


//////////////////////////////////////////////////////////////////////////
bool AdSceneState::persist(BasePersistenceManager *persistMgr) {
	persistMgr->transferCharPtr(TMEMBER(_filename));
	_nodeStates.persist(persistMgr);

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
void AdSceneState::setFilename(const char *filename) {
	delete[] _filename;
	size_t filenameSize = strlen(filename) + 1;
	_filename = new char [filenameSize];
	Common::strcpy_s(_filename, filenameSize, filename);
}

const char *AdSceneState::getFilename() const {
	return _filename;
}

//////////////////////////////////////////////////////////////////////////
AdNodeState *AdSceneState::getNodeState(const char *name, bool saving) {
	for (uint32 i = 0; i < _nodeStates.getSize(); i++) {
		if (scumm_stricmp(_nodeStates[i]->getName(), name) == 0) {
			return _nodeStates[i];
		}
	}

	if (saving) {
		AdNodeState *ret = new AdNodeState(_gameRef);
		ret->setName(name);
		_nodeStates.add(ret);

		return ret;
	} else {
		return nullptr;
	}
}

} // End of namespace Wintermute
