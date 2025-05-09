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

#include "common/archive.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/textconsole.h"
#include "common/system.h"
#include "backends/fs/fs-factory.h"

namespace Common {

File::File()
	: _handle(nullptr) {
}

File::~File() {
	close();
}

bool File::open(const Path &filename) {
	return open(filename, SearchMan);
}

bool File::open(const Path &filename, Archive &archive) {
	assert(!filename.empty());
	assert(!_handle);

	SeekableReadStream *stream = nullptr;

	if ((stream = archive.createReadStreamForMember(filename))) {
		debug(8, "Opening hashed: %s", filename.toString().c_str());
	} else if ((stream = archive.createReadStreamForMember(filename.append(".")))) {
		// WORKAROUND: Bug #2548: "SIMON1: Game Detection fails"
		// sometimes instead of "GAMEPC" we get "GAMEPC." (note trailing dot)
		debug(8, "Opening hashed: %s.", filename.toString().c_str());
	}

	return open(stream, filename.toString());
}

bool File::open(const FSNode &node) {
	assert(!_handle);

	if (!node.exists()) {
		warning("File::open: node does not exist");
		return false;
	} else if (node.isDirectory()) {
		warning("File::open: '%s' is a directory", node.getPath().toString(Common::Path::kNativeSeparator).c_str());
		return false;
	}

	SeekableReadStream *stream = node.createReadStream();
	return open(stream, node.getPath().toString(Common::Path::kNativeSeparator));
}

bool File::open(SeekableReadStream *stream, const String &name) {
	assert(!_handle);

	if (stream) {
		_handle = stream;
		_name = name;
	} else {
		debug(8, "File::open: opening '%s' failed", name.c_str());
	}
	return _handle != nullptr;
}


bool File::exists(const Path &filename) {
	if (SearchMan.hasFile(filename)) {
		return true;
	} else if (SearchMan.hasFile(filename.append("."))) {
		// WORKAROUND: Bug #2548: "SIMON1: Game Detection fails"
		// sometimes instead of "GAMEPC" we get "GAMEPC." (note trailing dot)
		return true;
	}

	return false;
}

void File::close() {
	delete _handle;
	_handle = nullptr;
}

bool File::isOpen() const {
	return _handle != nullptr;
}

bool File::err() const {
	assert(_handle);
	return _handle->err();
}

void File::clearErr() {
	assert(_handle);
	_handle->clearErr();
}

bool File::eos() const {
	assert(_handle);
	return _handle->eos();
}

int64 File::pos() const {
	assert(_handle);
	return _handle->pos();
}

int64 File::size() const {
	assert(_handle);
	return _handle->size();
}

bool File::seek(int64 offs, int whence) {
	assert(_handle);
	return _handle->seek(offs, whence);
}

uint32 File::read(void *ptr, uint32 len) {
	assert(_handle);
	return _handle->read(ptr, len);
}


DumpFile::DumpFile() : _handle(nullptr) {
}

DumpFile::~DumpFile() {
	close();
}

bool DumpFile::open(const Path &filename, bool createPath) {
	assert(!filename.empty());
	assert(!_handle);

	if (createPath) {
		Common::Path dirname(filename.getParent());

		// If the parent directory already exists keep it simple
		AbstractFSNode *node = g_system->getFilesystemFactory()->makeFileNodePath(dirname.toString(Common::Path::kNativeSeparator));
		if (!node->exists()) {
			delete node;

			dirname = dirname.normalize();
			StringArray components = dirname.splitComponents();

			Common::Path subpath;
			for (auto &component : components) {
				subpath.appendInPlace(component, Common::Path::kNoSeparator);
				// Add a trailing path separator
				subpath.appendInPlace("/");
				node = g_system->getFilesystemFactory()->makeFileNodePath(subpath.toString(Common::Path::kNativeSeparator));
				if (node->exists()) {
					delete node;
					continue;
				}
				if (!node->createDirectory()) {
					warning("DumpFile: unable to create directories from path prefix (%s)", subpath.toString(Common::Path::kNativeSeparator).c_str());
				}
				delete node;
			}
		} else {
			delete node;
		}
	}

	FSNode node(filename);
	return open(node);
}

bool DumpFile::open(const FSNode &node) {
	assert(!_handle);

	if (node.isDirectory()) {
		warning("DumpFile::open: FSNode is a directory");
		return false;
	}

	_handle = node.createWriteStream();

	if (_handle == nullptr)
		debug(2, "File %s not found", node.getName().c_str());

	return _handle != nullptr;
}

void DumpFile::close() {
	delete _handle;
	_handle = nullptr;
}

bool DumpFile::isOpen() const {
	return _handle != nullptr;
}

bool DumpFile::err() const {
	assert(_handle);
	return _handle->err();
}

void DumpFile::clearErr() {
	assert(_handle);
	_handle->clearErr();
}

uint32 DumpFile::write(const void *ptr, uint32 len) {
	assert(_handle);
	return _handle->write(ptr, len);
}

bool DumpFile::flush() {
	assert(_handle);
	return _handle->flush();
}

int64 DumpFile::pos() const { return _handle->pos(); }

bool DumpFile::seek(int64 offset, int whence) {
	SeekableWriteStream *ws = dynamic_cast<SeekableWriteStream *>(_handle);
	return ws ? ws->seek(offset, whence) : false;
}

int64 DumpFile::size() const {
	SeekableWriteStream *ws = dynamic_cast<SeekableWriteStream *>(_handle);
	return ws ? ws->size() : -1;
}

} // End of namespace Common
