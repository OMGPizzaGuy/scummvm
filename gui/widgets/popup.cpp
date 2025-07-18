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

#include "common/system.h"
#include "gui/gui-manager.h"
#include "gui/widgets/popup.h"

#include "gui/ThemeEval.h"

namespace GUI {

//
// PopUpDialog
//

PopUpDialog::PopUpDialog(Widget *boss, const Common::String &name, int clickX, int clickY):
		Dialog(name),
		_boss(boss),
		// Remember original mouse position
		_clickX(clickX),
		_clickY(clickY),
		_selection(-1),
		_initialSelection(-1),
		_openTime(0),
		_twoColumns(false),
		_entriesPerColumn(1),
		_leftPadding(0),
		_rightPadding(0),
		_lineHeight(kLineHeight),
		_lastRead(-1) {
	_backgroundType = ThemeEngine::kDialogBackgroundNone;
	_w = _boss->getWidth();
}

void PopUpDialog::open() {
	// Time the popup was opened
	_openTime = g_system->getMillis();

	_initialSelection = _selection;

	// Calculate real popup dimensions
	_h = _entries.size() * _lineHeight + 2;
	_w = 0;

	for (uint i = 0; i < _entries.size(); i++) {
		int width = g_gui.getStringWidth(_entries[i]);

		if (width > _w)
			_w = width;
	}


	_entriesPerColumn = 1;

	// Perform clipping / switch to scrolling mode if we don't fit on the screen
	// FIXME - OSystem should send out notification messages when the screen
	// resolution changes... we could generalize CommandReceiver and CommandSender.

	Common::Rect safeArea = g_system->getSafeOverlayArea();

	// HACK: For now, we do not do scrolling. Instead, we draw the dialog
	// in two columns if it's too tall.

	if (_h >= safeArea.height()) {
		_twoColumns = true;
		_entriesPerColumn = _entries.size() / 2;

		if (_entries.size() & 1)
			_entriesPerColumn++;

		_h = _entriesPerColumn * _lineHeight + 2;
		_w = 2 * _w + 10;

		if (!(_w & 1))
			_w++;

		if (_selection >= _entriesPerColumn) {
			_x -= _w / 2;
			_y = _boss->getAbsY() - (_selection - _entriesPerColumn) * _lineHeight;
		}

	} else {
		_twoColumns = false;

		_w = MAX<uint16>(_boss->getWidth(), _w + 20);
	}

	if (_w >= safeArea.width())
		_w = safeArea.width() - 1;
	if (_x < safeArea.left)
		_x = safeArea.left;
	if (_x + _w >= safeArea.right)
		_x = safeArea.right - 1 - _w;

	if (_h >= safeArea.height())
		_h = safeArea.height() - 1;
	if (_y < safeArea.top)
		_y = safeArea.top;
	else if (_y + _h >= safeArea.bottom)
		_y = safeArea.bottom - 1 - _h;

	// TODO - implement scrolling if we had to move the menu, or if there are too many entries

	_lastRead = -1;

	Dialog::open();
}

void PopUpDialog::reflowLayout() {
}

void PopUpDialog::drawDialog(DrawLayer layerToDraw) {
	Dialog::drawDialog(layerToDraw);

	int16 x = _x;
	if (g_gui.useRTL()) {
		x = g_system->getOverlayWidth() - _x - _w;
	}

	// Draw the menu border
	g_gui.theme()->drawWidgetBackground(Common::Rect(x, _y, x + _w, _y + _h), ThemeEngine::kWidgetBackgroundPlain);

	/*if (_twoColumns)
		g_gui.vLine(_x + _w / 2, _y, _y + _h - 2, g_gui._color);*/

	// Draw the entries
	int count = _entries.size();
	for (int i = 0; i < count; i++) {
		drawMenuEntry(i, i == _selection);
	}

	// The last entry may be empty. Fill it with black.
	/*if (_twoColumns && (count & 1)) {
		g_gui.fillRect(_x + 1 + _w / 2, _y + 1 + kLineHeight * (_entriesPerColumn - 1), _w / 2 - 1, kLineHeight, g_gui._bgcolor);
	}*/
}

void PopUpDialog::handleMouseUp(int x, int y, int button, int clickCount) {
	int absX = x + getAbsX();
	int absY = y + getAbsY();

	// Mouse was released. If it wasn't moved much since the original mouse down,
	// let the popup stay open. If it did move, assume the user made his selection.
	int dist = (_clickX - absX) * (_clickX - absX) + (_clickY - absY) * (_clickY - absY);
	if (dist > 3 * 3 || g_system->getMillis() - _openTime > 300) {
		int item = findItem(x, y);

		// treat separator item as if no item was clicked
		if (item >= 0 && _entries[item].size() == 0) {
			item = -1;
		}

		setResult(item);
		close();
	}
	_clickX = -1;
	_clickY = -1;
	_openTime = (uint32)-1;
}

void PopUpDialog::handleMouseWheel(int x, int y, int direction) {
	if (direction < 0)
		moveUp();
	else if (direction > 0)
		moveDown();
}

void PopUpDialog::handleMouseMoved(int x, int y, int button) {
	// Compute over which item the mouse is...
	int item = findItem(x, y);

	// treat separator item as if no item was moused over
	if (item >= 0 && _entries[item].size() == 0)
		item = -1;

	if (item == -1 && !isMouseDown()) {
		setSelection(_initialSelection);
		return;
	}

	// ...and update the selection accordingly
	setSelection(item);
	if (_lastRead != item && _entries.size() > 0 && item != -1) {
		read(_entries[item]);
		_lastRead = item;
	}
}

void PopUpDialog::handleMouseLeft(int button) {
	_lastRead = -1;
}

void PopUpDialog::read(const Common::U32String &str) {
	if (ConfMan.hasKey("tts_enabled", "scummvm") &&
			ConfMan.getBool("tts_enabled", "scummvm")) {
		Common::TextToSpeechManager *ttsMan = g_system->getTextToSpeechManager();
		if (ttsMan != nullptr)
			ttsMan->say(str);
	}
}

void PopUpDialog::handleKeyDown(Common::KeyState state) {
	if (state.keycode == Common::KEYCODE_ESCAPE) {
		// Don't change the previous selection
		setResult(-1);
		close();
		return;
	}

	if (isMouseDown())
		return;

	switch (state.keycode) {

	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		setResult(_selection);
		close();
		break;

	// Keypad & special keys
	//   - if num lock is set, we ignore the keypress
	//   - if num lock is not set, we fall down to the special key case

	case Common::KEYCODE_KP1:
		if (state.flags & Common::KBD_NUM)
			break;
		// fall through
	case Common::KEYCODE_END:
		setSelection(_entries.size()-1);
		break;

	case Common::KEYCODE_KP2:
		if (state.flags & Common::KBD_NUM)
			break;
		// fall through
	case Common::KEYCODE_DOWN:
		moveDown();
		break;

	case Common::KEYCODE_KP7:
		if (state.flags & Common::KBD_NUM)
			break;
		// fall through
	case Common::KEYCODE_HOME:
		setSelection(0);
		break;

	case Common::KEYCODE_KP8:
		if (state.flags & Common::KBD_NUM)
			break;
		// fall through
	case Common::KEYCODE_UP:
		moveUp();
		break;

	default:
		break;
	}
}

void PopUpDialog::setPosition(int x, int y) {
	_x = x;
	_y = y;
}

void PopUpDialog::setPadding(int left, int right) {
	_leftPadding  = left;
	_rightPadding = right;
}

void PopUpDialog::setLineHeight(int lineHeight) {
	_lineHeight = lineHeight;
}

void PopUpDialog::setWidth(uint16 width) {
	_w = width;
}

void PopUpDialog::appendEntry(const Common::U32String &entry) {
	_entries.push_back(entry);
}

void PopUpDialog::clearEntries() {
	_entries.clear();
}

int PopUpDialog::findItem(int x, int y) const {
	if (x >= 0 && x < _w && y >= 0 && y < _h) {
		if (_twoColumns) {
			uint entry = (y - 2) / _lineHeight;
			if (x > _w / 2) {
				entry += _entriesPerColumn;

				if (entry >= _entries.size())
					return -1;
			}
			return entry;
		}
		return (y - 2) / _lineHeight;
	}
	return -1;
}

void PopUpDialog::setSelection(int item) {
	if (item != _selection) {
		// Undraw old selection
		if (_selection >= 0)
			drawMenuEntry(_selection, false);

		// Change selection
		_selection = item;

		// Draw new selection
		if (item >= 0)
			drawMenuEntry(item, true);
	}
}

bool PopUpDialog::isMouseDown() {
	// TODO/FIXME - need a way to determine whether any mouse buttons are pressed or not.
	// Sure, we could just count mouse button up/down events, but that is cumbersome and
	// error prone. Would be much nicer to add an API to OSystem for this...

	return false;
}

void PopUpDialog::moveUp() {
	if (_selection < 0) {
		setSelection(_entries.size() - 1);
	} else if (_selection > 0) {
		int item = _selection;
		do {
			item--;
		} while (item >= 0 && _entries[item].size() == 0);
		if (item >= 0)
			setSelection(item);
	}
}

void PopUpDialog::moveDown() {
	int lastItem = _entries.size() - 1;

	if (_selection < 0) {
		setSelection(0);
	} else if (_selection < lastItem) {
		int item = _selection;
		do {
			item++;
		} while (item <= lastItem && _entries[item].size() == 0);
		if (item <= lastItem)
			setSelection(item);
	}
}

void PopUpDialog::drawMenuEntry(int entry, bool hilite) {
	// Draw one entry of the popup menu, including selection
	assert(entry >= 0);
	int x, y, w;

	if (_twoColumns) {
		int n = _entries.size() / 2;

		if (_entries.size() & 1)
			n++;

		if (entry >= n) {
			x = _x + 1 + _w / 2;
			y = _y + 1 + _lineHeight * (entry - n);
		} else {
			x = _x + 1;
			y = _y + 1 + _lineHeight * entry;
		}

		w = _w / 2 - 1;
	} else {
		x = _x + 1;
		y = _y + 1 + _lineHeight * entry;
		w = _w - 2;
	}

	Common::U32String &name(_entries[entry]);

	Common::Rect r1(x, y, x + w, y + _lineHeight);
	Common::Rect r2(x + 1, y + 2, x + w, y + 2 + _lineHeight);
	Graphics::TextAlign alignment = Graphics::kTextAlignLeft;
	int pad = _leftPadding;

	if (g_gui.useRTL()) {
		const int16 screenW = g_system->getOverlayWidth();
		r1.left = screenW - r1.left - w;
		r1.right = r1.left + w;
		r2.left = screenW - r2.left - w;
		r2.right = r2.left + w;

		alignment = Graphics::kTextAlignRight;
		pad = _rightPadding;
	}

	if (name.size() == 0) {
		// Draw a separator
		g_gui.theme()->drawLineSeparator(r1);
	} else {
		g_gui.theme()->drawText(
			r2,
			name, hilite ? ThemeEngine::kStateHighlight : ThemeEngine::kStateEnabled,
			alignment, ThemeEngine::kTextInversionNone, pad, false
		);
	}
}


#pragma mark -

//
// PopUpWidget
//

PopUpWidget::PopUpWidget(GuiObject *boss, const Common::String &name, const Common::U32String &tooltip, uint32 cmd)
	: Widget(boss, name, tooltip), CommandSender(boss) {
	setFlags(WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS | WIDGET_IGNORE_DRAG);
	_type = kPopUpWidget;
	_cmd = cmd;

	_selectedItem = -1;
	_leftPadding = _rightPadding = 0;
}

PopUpWidget::PopUpWidget(GuiObject *boss, int x, int y, int w, int h, const Common::U32String &tooltip, uint32 cmd)
	: Widget(boss, x, y, w, h, tooltip), CommandSender(boss) {
	setFlags(WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS | WIDGET_IGNORE_DRAG);
	_type = kPopUpWidget;
	_cmd = cmd;

	_selectedItem = -1;

	_leftPadding = _rightPadding = 0;
}

void PopUpWidget::handleMouseDown(int x, int y, int button, int clickCount) {
	if (isEnabled()) {
		PopUpDialog popupDialog(this, "", x + getAbsX(), y + getAbsY());
		popupDialog.setPosition(getAbsX(), getAbsY() - _selectedItem * kLineHeight);
		popupDialog.setPadding(_leftPadding, _rightPadding);
		popupDialog.setWidth(getWidth() - kLineHeight + 2);


		for (uint i = 0; i < _entries.size(); i++) {
			popupDialog.appendEntry(_entries[i].name);
		}
		popupDialog.setSelection(_selectedItem);

		int newSel = popupDialog.runModal();
		if (newSel != -1 && _selectedItem != newSel) {
			_selectedItem = newSel;
			sendCommand(_cmd, _entries[_selectedItem].tag);
			markAsDirty();
		}
	}
}

void PopUpWidget::handleMouseWheel(int x, int y, int direction) {
	if (isEnabled()) {
		int newSelection = _selectedItem + direction;

		// Skip separator entries
		while ((newSelection >= 0) && (newSelection < (int)_entries.size()) &&
		       _entries[newSelection].name.empty()) {
			newSelection += direction;
		}

		// Just update the selected item when we're in range
		if ((newSelection >= 0) && (newSelection < (int)_entries.size()) &&
			(newSelection != _selectedItem)) {
			_selectedItem = newSelection;
			sendCommand(_cmd, _entries[_selectedItem].tag);
			markAsDirty();
		}
	}
}

void PopUpWidget::reflowLayout() {
	_leftPadding = g_gui.xmlEval()->getVar("Globals.PopUpWidget.Padding.Left", 0);
	_rightPadding = g_gui.xmlEval()->getVar("Globals.PopUpWidget.Padding.Right", 0);

	Widget::reflowLayout();
}

void PopUpWidget::appendEntry(const Common::U32String &entry, uint32 tag) {
	Entry e;
	e.name = entry;
	e.tag = tag;
	_entries.push_back(e);
}

void PopUpWidget::appendEntry(const Common::String &entry, uint32 tag) {
	appendEntry(Common::U32String(entry), tag);
}

void PopUpWidget::clearEntries() {
	_entries.clear();
	_selectedItem = -1;
}

void PopUpWidget::setSelected(int item) {
	if (item != _selectedItem) {
		if (item >= 0 && item < (int)_entries.size()) {
			_selectedItem = item;
		} else {
			_selectedItem = -1;
		}
	}
}

void PopUpWidget::setSelectedTag(uint32 tag) {
	uint item;
	for (item = 0; item < _entries.size(); ++item) {
		if (_entries[item].tag == tag) {
			setSelected(item);
			return;
		}
	}
}

void PopUpWidget::drawWidget() {
	Common::U32String sel;
	if (_selectedItem >= 0)
		sel = _entries[_selectedItem].name;

	int pad = _leftPadding;

	if (g_gui.useRTL() && _useRTL)
		pad = _rightPadding;

	g_gui.theme()->drawPopUpWidget(Common::Rect(_x, _y, _x + _w, _y + _h), sel, pad, _state, (g_gui.useRTL() && _useRTL));
}

} // End of namespace GUI
