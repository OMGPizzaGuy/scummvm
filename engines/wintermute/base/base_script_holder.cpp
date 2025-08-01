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

#include "engines/wintermute/ad/ad_game.h"
#include "engines/wintermute/base/base_script_holder.h"
#include "engines/wintermute/base/base_parser.h"
#include "engines/wintermute/base/base_engine.h"
#include "engines/wintermute/base/scriptables/script_value.h"
#include "engines/wintermute/base/scriptables/script_engine.h"
#include "engines/wintermute/base/scriptables/script.h"
#include "engines/wintermute/base/scriptables/script_stack.h"

namespace Wintermute {

IMPLEMENT_PERSISTENT(BaseScriptHolder, false)

//////////////////////////////////////////////////////////////////////
BaseScriptHolder::BaseScriptHolder(BaseGame *inGame) : BaseScriptable(inGame) {
	setName("<unnamed>");
	_ready = false;
	_freezable = true;
	_filename = nullptr;
}


//////////////////////////////////////////////////////////////////////
BaseScriptHolder::~BaseScriptHolder() {
	cleanup();
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::cleanup() {
	delete[] _filename;
	_filename = nullptr;

	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		_scripts[i]->finish(true);
		_scripts[i]->_owner = nullptr;
	}
	_scripts.removeAll();

	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////
void BaseScriptHolder::setFilename(const char *filename) {
	if (_filename != nullptr) {
		delete[] _filename;
		_filename = nullptr;
	}
	if (filename == nullptr) {
		return;
	}
	size_t filenameSize = strlen(filename) + 1;
	_filename = new char [filenameSize];
	Common::strcpy_s(_filename, filenameSize, filename);
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::applyEvent(const char *eventName, bool unbreakable) {
	int numHandlers = 0;

	bool ret = STATUS_FAILED;
	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		if (!_scripts[i]->_thread) {
			ScScript *handler = _scripts[i]->invokeEventHandler(eventName, unbreakable);
			if (handler) {
				//_scripts.add(handler);
				numHandlers++;
				ret = STATUS_OK;
			}
		}
	}
	if (numHandlers > 0 && unbreakable) {
		_gameRef->_scEngine->tickUnbreakable();
	}

	return ret;
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::listen(BaseScriptHolder *param1, uint32 param2) {
	return STATUS_FAILED;
}


//////////////////////////////////////////////////////////////////////////
// high level scripting interface
//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::scCallMethod(ScScript *script, ScStack *stack, ScStack *thisStack, const char *name) {
	//////////////////////////////////////////////////////////////////////////
	// DEBUG_CrashMe
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "DEBUG_CrashMe") == 0) {
		stack->correctParams(0);
		byte *p = 0;
		*p = 10;
		stack->pushNULL();

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// ApplyEvent
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "ApplyEvent") == 0) {
		stack->correctParams(1);
		ScValue *val = stack->pop();
		bool ret;
		ret = applyEvent(val->getString());

		if (DID_SUCCEED(ret)) {
			stack->pushBool(true);
		} else {
			stack->pushBool(false);
		}

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// CanHandleEvent
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CanHandleEvent") == 0) {
		stack->correctParams(1);
		stack->pushBool(canHandleEvent(stack->pop()->getString()));

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// CanHandleMethod
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "CanHandleMethod") == 0) {
		stack->correctParams(1);
		stack->pushBool(canHandleMethod(stack->pop()->getString()));

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AttachScript
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AttachScript") == 0) {
		stack->correctParams(1);
		stack->pushBool(DID_SUCCEED(addScript(stack->pop()->getString())));

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// DetachScript
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "DetachScript") == 0) {
		stack->correctParams(2);
		const char *filename = stack->pop()->getString();
		bool killThreads = stack->pop()->getBool(false);
		bool ret = false;
		for (uint32 i = 0; i < _scripts.getSize(); i++) {
			if (scumm_stricmp(_scripts[i]->_filename, filename) == 0) {
				_scripts[i]->finish(killThreads);
				ret = true;
				break;
			}
		}
		stack->pushBool(ret);

		return STATUS_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// IsScriptRunning
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "IsScriptRunning") == 0) {
		stack->correctParams(1);
		const char *filename = stack->pop()->getString();
		bool ret = false;
		for (uint32 i = 0; i < _scripts.getSize(); i++) {
			if (scumm_stricmp(_scripts[i]->_filename, filename) == 0 && _scripts[i]->_state != SCRIPT_FINISHED && _scripts[i]->_state != SCRIPT_ERROR) {
				ret = true;
				break;
			}
		}
		stack->pushBool(ret);

		return STATUS_OK;
	} else {
		return BaseScriptable::scCallMethod(script, stack, thisStack, name);
	}
}


//////////////////////////////////////////////////////////////////////////
ScValue *BaseScriptHolder::scGetProperty(const Common::String &name) {
	_scValue->setNULL();

	//////////////////////////////////////////////////////////////////////////
	// Type
	//////////////////////////////////////////////////////////////////////////
	if (name == "Type") {
		_scValue->setString("script_holder");
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Name
	//////////////////////////////////////////////////////////////////////////
	else if (name == "Name") {
		_scValue->setString(getName());
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Filename (RO)
	//////////////////////////////////////////////////////////////////////////
	else if (name == "Filename") {
		_scValue->setString(_filename);
		return _scValue;
	} else {
		return BaseScriptable::scGetProperty(name);
	}
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::scSetProperty(const char *name, ScValue *value) {
	//////////////////////////////////////////////////////////////////////////
	// Name
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Name") == 0) {
		setName(value->getString());
		return STATUS_OK;
	} else {
		return BaseScriptable::scSetProperty(name, value);
	}
}


//////////////////////////////////////////////////////////////////////////
const char *BaseScriptHolder::scToString() {
	return "[script_holder]";
}

//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::saveAsText(BaseDynamicBuffer *buffer, int indent) {
	return BaseClass::saveAsText(buffer, indent);
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::persist(BasePersistenceManager *persistMgr) {
	BaseScriptable::persist(persistMgr);

	persistMgr->transferCharPtr(TMEMBER(_filename));
	persistMgr->transferBool(TMEMBER(_freezable));
	if (persistMgr->getIsSaving()) {
		const char *name = getName();
		persistMgr->transferConstChar(TMEMBER(name));
	} else {
		char *name;
		persistMgr->transferCharPtr(TMEMBER(name));
		setName(name);
		delete[] name;
	}
	_scripts.persist(persistMgr);

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::addScript(const char *filename) {
	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		if (scumm_stricmp(_scripts[i]->_filename, filename) == 0) {
			if (_scripts[i]->_state != SCRIPT_FINISHED) {
				BaseEngine::LOG(0, "BaseScriptHolder::AddScript - trying to add script '%s' multiple times (obj: '%s')", filename, getName());
				return STATUS_OK;
			}
		}
	}

	ScScript *scr =  _gameRef->_scEngine->runScript(filename, this);
	if (!scr) {
		if (_gameRef->_editorForceScripts) {
			// editor hack
#if EXTENDED_DEBUGGER_ENABLED
			scr = new DebuggableScript(_gameRef,  _gameRef->_scEngine);
#else
			scr = new ScScript(_gameRef,  _gameRef->_scEngine);
#endif
			size_t filenameSize = strlen(filename) + 1;
			scr->_filename = new char[filenameSize];
			Common::strcpy_s(scr->_filename, filenameSize, filename);
			scr->_state = SCRIPT_ERROR;
			scr->_owner = this;
			_scripts.add(scr);
			_gameRef->_scEngine->_scripts.add(scr);

			return STATUS_OK;
		}
		return STATUS_FAILED;
	} else {
		scr->_freezable = _freezable;
		_scripts.add(scr);
		return STATUS_OK;
	}
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::removeScript(ScScript *script) {
	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		if (_scripts[i] == script) {
			_scripts.removeAt(i);
			break;
		}
	}
	return STATUS_OK;
}

//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::canHandleEvent(const char *EventName) const {
	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		if (!_scripts[i]->_thread && _scripts[i]->canHandleEvent(EventName)) {
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::canHandleMethod(const char *MethodName) const {
	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		if (!_scripts[i]->_thread && _scripts[i]->canHandleMethod(MethodName)) {
			return true;
		}
	}
	return false;
}


TOKEN_DEF_START
TOKEN_DEF(PROPERTY)
TOKEN_DEF(NAME)
TOKEN_DEF(VALUE)
TOKEN_DEF_END
//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::parseProperty(char *buffer, bool complete) {
	TOKEN_TABLE_START(commands)
	TOKEN_TABLE(PROPERTY)
	TOKEN_TABLE(NAME)
	TOKEN_TABLE(VALUE)
	TOKEN_TABLE_END

	char *params;
	int cmd;
	BaseParser parser;

	if (complete) {
		if (parser.getCommand(&buffer, commands, &params) != TOKEN_PROPERTY) {
			BaseEngine::LOG(0, "'PROPERTY' keyword expected.");
			return STATUS_FAILED;
		}
		buffer = params;
	}

	char *propName = nullptr;
	char *propValue = nullptr;

	while ((cmd = parser.getCommand(&buffer, commands, &params)) > 0) {
		switch (cmd) {
		case TOKEN_NAME: {
			delete[] propName;
			size_t propNameSize = strlen(params) + 1;
			propName = new char[propNameSize];
			Common::strcpy_s(propName, propNameSize, params);
			break;
		}
		case TOKEN_VALUE: {
			delete[] propValue;
			size_t propValueSize = strlen(params) + 1;
			propValue = new char[propValueSize];
			Common::strcpy_s(propValue, propValueSize, params);
			break;
		}
		default:
			break;
		}

	}
	if (cmd == PARSERR_TOKENNOTFOUND) {
		delete[] propName;
		delete[] propValue;
		BaseEngine::LOG(0, "Syntax error in PROPERTY definition");
		return STATUS_FAILED;
	}
	if (cmd == PARSERR_GENERIC || propName == nullptr || propValue == nullptr) {
		delete[] propName;
		delete[] propValue;
		BaseEngine::LOG(0, "Error loading PROPERTY definition");
		return STATUS_FAILED;
	}


	ScValue *val = new ScValue(_gameRef);
	val->setString(propValue);
	scSetProperty(propName, val);

	delete val;
	delete[] propName;
	delete[] propValue;

	return STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////
void BaseScriptHolder::makeFreezable(bool freezable) {
	_freezable = freezable;
	for (uint32 i = 0; i < _scripts.getSize(); i++) {
		_scripts[i]->_freezable = freezable;
	}

}


//////////////////////////////////////////////////////////////////////////
ScScript *BaseScriptHolder::invokeMethodThread(const char *methodName) {
	for (int i = _scripts.getSize() - 1; i >= 0; i--) {
		if (_scripts[i]->canHandleMethod(methodName)) {
#if EXTENDED_DEBUGGER_ENABLED
			DebuggableScEngine* debuggableEngine;
			debuggableEngine = dynamic_cast<DebuggableScEngine*>(_scripts[i]->_engine);
			// TODO: Not pretty
			assert(debuggableEngine);
			ScScript *thread = new DebuggableScript(_gameRef,  debuggableEngine);
#else
			ScScript *thread = new ScScript(_gameRef,  _scripts[i]->_engine);
#endif
			if (thread) {
				bool ret = thread->createMethodThread(_scripts[i], methodName);
				if (DID_SUCCEED(ret)) {
					_scripts[i]->_engine->_scripts.add(thread);
					return thread;
				} else {
					delete thread;
				}
			}
		}
	}
	return nullptr;
}


//////////////////////////////////////////////////////////////////////////
void BaseScriptHolder::scDebuggerDesc(char *buf, int bufSize) {
	Common::strcpy_s(buf, bufSize, scToString());
	if (getName() && strcmp(getName(), "<unnamed>") != 0) {
		Common::strcat_s(buf, bufSize, "  Name: ");
		Common::strcat_s(buf, bufSize, getName());
	}
	if (_filename) {
		Common::strcat_s(buf, bufSize, "  File: ");
		Common::strcat_s(buf, bufSize, _filename);
	}
}


//////////////////////////////////////////////////////////////////////////
// IWmeObject
//////////////////////////////////////////////////////////////////////////
bool BaseScriptHolder::sendEvent(const char *eventName) {
	return DID_SUCCEED(applyEvent(eventName));
}

} // End of namespace Wintermute
