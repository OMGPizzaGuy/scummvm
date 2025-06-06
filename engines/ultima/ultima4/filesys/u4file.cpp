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

#include "ultima/ultima4/filesys/u4file.h"
#include "common/file.h"

namespace Ultima {
namespace Ultima4 {

Std::vector<Common::String> u4read_stringtable(const Common::String &filename) {
	Common::File f;
	if (!f.open(Common::Path(Common::String::format("data/text/%s.dat", filename.c_str()))))
		error("Could not open string table '%s'", filename.c_str());

	Std::vector<Common::String> strs;
	Common::String line;
	int64 filesize = f.size();

	while (f.pos() < filesize)
		strs.push_back(f.readString());

	return strs;
}

} // End of namespace Ultima4
} // End of namespace Ultima
