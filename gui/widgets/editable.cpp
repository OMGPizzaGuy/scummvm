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

#include "common/rect.h"
#include "common/system.h"
#include "common/unicode-bidi.h"
#include "gui/widgets/editable.h"
#include "gui/gui-manager.h"
#include "graphics/font.h"

namespace GUI {

EditableWidget::EditableWidget(GuiObject *boss, int x, int y, int w, int h, bool scale, const Common::U32String &tooltip, uint32 cmd)
	: Widget(boss, x, y, w, h, scale, tooltip), CommandSender(boss), _cmd(cmd) {
	setFlags(WIDGET_TRACK_MOUSE);
	init();
}

EditableWidget::EditableWidget(GuiObject *boss, int x, int y, int w, int h, const Common::U32String &tooltip, uint32 cmd)
	: EditableWidget(boss, x, y, w, h, false, tooltip, cmd) {
}

EditableWidget::EditableWidget(GuiObject *boss, const Common::String &name, const Common::U32String &tooltip, uint32 cmd)
	: Widget(boss, name, tooltip), CommandSender(boss), _cmd(cmd) {
	setFlags(WIDGET_TRACK_MOUSE);
	init();
}

void EditableWidget::init() {
	_caretVisible = false;
	_caretTime = 0;
	_caretPos = 0;

	_caretInverse = false;

	_editScrollOffset = 0;

	_selCaretPos = -1;
	_selOffset = 0;

	_shiftPressed = _isDragging = false;
	_disableSelection = g_gui.useRTL();

	_align = g_gui.useRTL() ? Graphics::kTextAlignRight : Graphics::kTextAlignLeft;
	_drawAlign = _align;

	_font = ThemeEngine::kFontStyleBold;
	_inversion = ThemeEngine::kTextInversionNone;
}

EditableWidget::~EditableWidget() {
}

void EditableWidget::drawWidget() {
	if (_caretVisible) {
		drawCaret(false, true);
	}
}

void EditableWidget::reflowLayout() {
	Widget::reflowLayout();

	_editScrollOffset = g_gui.getStringWidth(_editString, _font) - getEditRect().width();
	if (_editScrollOffset < 0) {
		_editScrollOffset = 0;
		_drawAlign = _align;
	} else {
		_drawAlign = Graphics::kTextAlignLeft;
	}
}

void EditableWidget::setEditString(const Common::U32String &str) {
	// TODO: We probably should filter the input string here,
	// e.g. using tryInsertChar.
	_editString = str;
	clearSelection();
	setCaretPos(caretVisualPos(str.size()));
	markAsDirty();
}

bool EditableWidget::isCharAllowed(Common::u32char_type_t c) const {
	return (c >= 32 && c <= 127) || c >= 160;
}

bool EditableWidget::tryInsertChar(Common::u32char_type_t c, int pos) {
	if (!isCharAllowed(c))
		return false;
	_editString.insertChar(c, pos);
	return true;
}

int EditableWidget::caretVisualPos(int logicalPos) const {
	return Common::convertBiDiU32String(_editString + " ").getVisualPosition(logicalPos);
}

int EditableWidget::caretLogicalPos() const {
	return Common::convertBiDiU32String(_editString + " ").getLogicalPosition(_caretPos);
}

void EditableWidget::handleTickle() {
	uint32 time = g_system->getMillis();
	if (_caretTime < time && isEnabled()) {
		_caretTime = time + kCaretBlinkTime;
		drawCaret(_caretVisible);
	}
}

void EditableWidget::handleMouseDown(int x, int y, int button, int clickCount) {
	if (!isEnabled())
		return;

	_isDragging = true;
	// Select all text incase of double press
	if (clickCount > 1) {
		_selCaretPos = 0;
		setCaretPos(caretVisualPos(_editString.size()));
		setSelectionOffset(_editString.size() - _selCaretPos);
		markAsDirty();
		return;
	}

	// Clear any selection
	if (_selOffset != 0 && !_shiftPressed)
		clearSelection();
	else if (_shiftPressed && _selCaretPos < 0 && !_disableSelection)
		_selCaretPos = _caretPos;

	if (g_gui.useRTL()) {
		x = _w - x;
	}

	x += _editScrollOffset;
	int width = 0;
	if (_drawAlign == Graphics::kTextAlignRight)
		width = _editScrollOffset + getEditRect().width() - g_gui.getStringWidth(_editString, _font);

	uint i, last = 0;
	for (i = 0; i < _editString.size(); ++i) {
		const uint cur = _editString[i];
		width += g_gui.getCharWidth(cur, _font) + g_gui.getKerningOffset(last, cur, _font);
		if (width >= x && width > _editScrollOffset)
			break;
		last = cur;
	}
	setCaretPos(i);
	if (_selCaretPos >= 0 && !_disableSelection)
		setSelectionOffset(i - _selCaretPos);
	markAsDirty();
}

void EditableWidget::handleMouseUp(int x, int y, int button, int clickCount) {
	if(isEnabled())
		_isDragging = false;
}

void EditableWidget::handleMouseMoved(int x, int y, int button) {
	if (_isDragging && isEnabled() && !_disableSelection) {
		if (_selCaretPos < 0)
			_selCaretPos = _caretPos;

		if (g_gui.useRTL()) {
			x = _w - x;
		}

		if (x < 0 && _editScrollOffset > 0) {
			_editScrollOffset += x;
			if(_editScrollOffset < 0)
				_editScrollOffset = 0;
		}

		x += _editScrollOffset;
		int width = 0;
		if (_drawAlign == Graphics::kTextAlignRight)
			width = _editScrollOffset + getEditRect().width() - g_gui.getStringWidth(_editString, _font);
		uint i, last = 0;
		for (i = 0; i < _editString.size(); ++i) {
			const uint cur = _editString[i];
			width += g_gui.getCharWidth(cur, _font) + g_gui.getKerningOffset(last, cur, _font);
			if (width >= x && width > _editScrollOffset)
				break;
			last = cur;
		}

		setCaretPos(i);
		if(_selCaretPos >= 0)
			setSelectionOffset(i - _selCaretPos);
		markAsDirty();
	}
}

bool EditableWidget::handleKeyUp(Common::KeyState state) {
	_shiftPressed = state.hasFlags(Common::KBD_SHIFT);
	return false;
}

bool EditableWidget::handleKeyDown(Common::KeyState state) {
	bool handled = true;
	bool dirty = false;
	bool forcecaret = false;
	int deleteIndex;

	if (!isEnabled())
		return false;

	// First remove caret
	if (_caretVisible)
		drawCaret(true);

	_shiftPressed = state.hasFlags(Common::KBD_SHIFT);

	// Remap numeric keypad if NUM lock is *not* active.
	// This code relies on the fact that the various KEYCODE_KP* values are
	// consecutive.
	if (0 == (state.flags & Common::KBD_NUM)
		&& Common::KEYCODE_KP0 <= state.keycode
		&& state.keycode <= Common::KEYCODE_KP_PERIOD) {
		const Common::KeyCode remap[11] = {
			Common::KEYCODE_INSERT, 	// KEYCODE_KP0
			Common::KEYCODE_END,	 	// KEYCODE_KP1
			Common::KEYCODE_DOWN, 		// KEYCODE_KP2
			Common::KEYCODE_PAGEDOWN, 	// KEYCODE_KP3
			Common::KEYCODE_LEFT, 		// KEYCODE_KP4
			Common::KEYCODE_INVALID, 	// KEYCODE_KP5
			Common::KEYCODE_RIGHT,	 	// KEYCODE_KP6
			Common::KEYCODE_HOME,	 	// KEYCODE_KP7
			Common::KEYCODE_UP, 		// KEYCODE_KP8
			Common::KEYCODE_PAGEUP, 	// KEYCODE_KP9
			Common::KEYCODE_DELETE,	 	// KEYCODE_KP_PERIOD
		};
		state.keycode = remap[state.keycode - Common::KEYCODE_KP0];
	}

	switch (state.keycode) {
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_KP_ENTER:
		// confirm edit and exit editmode
		endEditMode();
		dirty = true;
		break;

	case Common::KEYCODE_ESCAPE:
		abortEditMode();
		dirty = true;
		break;

	case Common::KEYCODE_BACKSPACE:
		deleteIndex = caretLogicalPos();
		if (deleteIndex > 0 && _selOffset == 0) {
			deleteIndex--;
			_editString.deleteChar(deleteIndex);
			setCaretPos(caretVisualPos(deleteIndex));
			_selCaretPos = -1;
			dirty = true;

			sendCommand(_cmd, 0);
		} else if (deleteIndex >= 0 && _selOffset != 0) {
			int selBegin = _selCaretPos;
			int selEnd = _selCaretPos + _selOffset;
			if (selBegin > selEnd)
				SWAP(selBegin, selEnd);
			_editString.erase(selBegin, selEnd - selBegin);
			setCaretPos(caretVisualPos(selBegin));
			_selCaretPos = -1;
			_selOffset = 0;
			dirty = true;

			sendCommand(_cmd, 0);
		}
		forcecaret = true;
		break;

	case Common::KEYCODE_DELETE:
		deleteIndex = caretLogicalPos();
		if (deleteIndex < (int)_editString.size()) {
			_editString.deleteChar(deleteIndex);
			setCaretPos(caretVisualPos(deleteIndex));
			_selCaretPos = -1;
			_selOffset = 0;
			dirty = true;

			sendCommand(_cmd, 0);
		}
		forcecaret = true;
		break;

	case Common::KEYCODE_DOWN:
	case Common::KEYCODE_END:
		moveCaretToEnd(state.hasFlags(Common::KBD_SHIFT));
		forcecaret = true;
		dirty = true;
		break;

	case Common::KEYCODE_LEFT:
		if (state.hasFlags(Common::KBD_SHIFT)) {
			if (_disableSelection)
				break;
			if (_selCaretPos < 0)
				_selCaretPos = _caretPos;
			if (_caretPos > 0)
				_selOffset--;
		} else {
			clearSelection();
		}
		// Move caret one left (if possible)
		if (_caretPos > 0) {
			dirty = setCaretPos(_caretPos - 1);
		}
		forcecaret = true;
		dirty = true;
		break;

	case Common::KEYCODE_RIGHT:
		if (state.hasFlags(Common::KBD_SHIFT)) {
			if (_disableSelection)
				break;
			if (_selCaretPos < 0)
				_selCaretPos = _caretPos;
			if (_selOffset + _selCaretPos < (int)_editString.size())
				_selOffset++;
		} else {
			clearSelection();
		}
		// Move caret one right (if possible)
		if (_caretPos < (int)_editString.size()) {
			dirty = setCaretPos(_caretPos + 1);
		}
		forcecaret = true;
		dirty = true;
		break;

	case Common::KEYCODE_UP:
	case Common::KEYCODE_HOME:
		moveCaretToStart(state.hasFlags(Common::KBD_SHIFT));
		forcecaret = true;
		dirty = true;
		break;

	default:
		defaultKeyDownHandler(state, dirty, forcecaret, handled);
	}

	if (dirty)
		markAsDirty();

	if (forcecaret)
		makeCaretVisible();

	return handled;
}

void EditableWidget::defaultKeyDownHandler(Common::KeyState &state, bool &dirty, bool &forcecaret, bool &handled) {
	if (isCharAllowed(state.ascii)) {
		// Incase of a selection, replace the selection with the character
		if (_selCaretPos >= 0) {
			int selBegin = _selCaretPos;
			int selEnd = _selCaretPos + _selOffset;
			if (selBegin > selEnd)
				SWAP(selBegin, selEnd);
			_editString.replace(selBegin, selEnd - selBegin, Common::U32String(state.ascii));
			if (_editString.size() > 0)
				selBegin++;
			setCaretPos(caretVisualPos(selBegin));
			_selCaretPos = -1;
			_selOffset = 0;
		} else {
			// Insert char normally at caretPos
			const int logicalPosition = caretLogicalPos();
			_editString.insertChar(state.ascii, logicalPosition);
			setCaretPos(caretVisualPos(logicalPosition + 1));
		}
		dirty = true;
		forcecaret = true;
		sendCommand(_cmd, 0);
	} else {
		handled = false;
	}
}

void EditableWidget::moveCaretToStart(bool shiftPressed) {
	if (shiftPressed) {
		if (_selCaretPos < 0)
			_selCaretPos = _caretPos;

		setSelectionOffset(0 - _selCaretPos);
	} else {
		clearSelection();
	}
	// Move caret to start
	setCaretPos(caretVisualPos(0));
}

void EditableWidget::moveCaretToEnd(bool shiftPressed) {
	if (_shiftPressed) {
		if (_selCaretPos < 0)
			_selCaretPos = _caretPos;

		setSelectionOffset(_editString.size() - _selCaretPos);
	} else {
		clearSelection();
	}

	// Move caret to end
	setCaretPos(caretVisualPos(_editString.size()));
}

void EditableWidget::handleOtherEvent(const Common::Event &evt) {
	bool dirty = false;
	bool forcecaret = false;

	if (!isEnabled())
		return;

	// First remove caret
	if (_caretVisible)
		drawCaret(true);

	switch (evt.type) {
	case Common::EVENT_CUSTOM_ENGINE_ACTION_START:
		switch (evt.customType) {
		case kActionHome:
			moveCaretToStart(false);
			forcecaret = true;
			dirty = true;
			break;
		case kActionShiftHome:
			moveCaretToStart(true);
			forcecaret = true;
			dirty = true;
			break;
		case kActionEnd:
			moveCaretToEnd(false);
			forcecaret = true;
			dirty = true;
			break;
		case kActionShiftEnd:
			moveCaretToEnd(true);
			forcecaret = true;
			dirty = true;
			break;
		case kActionCut:
			if (!getEditString().empty() && _selOffset != 0) {
				int selBegin = _selCaretPos;
				int selEnd = _selCaretPos + _selOffset;
				if (selBegin > selEnd)
					SWAP(selBegin, selEnd);
				const Common::U32String selected(getEditString().begin() + selBegin, getEditString().begin() + selEnd);
				g_system->setTextInClipboard(selected);

				_editString.erase(selBegin, selEnd - selBegin);
				setCaretPos(caretVisualPos(selBegin));
				_selCaretPos = -1;
				_selOffset = 0;
				dirty = true;
			}
			break;

		case kActionCopy:
			if (!getEditString().empty()) {
				int selBegin = _selCaretPos;
				int selEnd = _selCaretPos + _selOffset;
				if (selBegin > selEnd)
					SWAP(selBegin, selEnd);
				const Common::U32String selected(getEditString().begin() + selBegin, getEditString().begin() + selEnd);
				g_system->setTextInClipboard(selected);
			}
			break;

		case kActionPaste:
			if (g_system->hasTextInClipboard()) {
				Common::U32String text = g_system->getTextFromClipboard();
				if (_selOffset != 0) {
					int selBegin = _selCaretPos;
					int selEnd = _selCaretPos + _selOffset;
					if (selBegin > selEnd)
						SWAP(selBegin, selEnd);
					_editString.replace(selBegin, selEnd - selBegin, text);
					setCaretPos(caretVisualPos(selBegin));
					const int logicalPosition = caretLogicalPos();
					setCaretPos(caretVisualPos(logicalPosition + text.size()));
					clearSelection();
				} else {
					for (uint32 i = 0; i < text.size(); ++i) {
						const int logicalPosition = caretLogicalPos();
						if (tryInsertChar(text[i], logicalPosition))
								setCaretPos(caretVisualPos(logicalPosition + 1));
					}
				}
				dirty = true;
			}
			break;
		default:
			break;
	}
	default:
		break;
	}

	if (dirty)
		markAsDirty();

	if (forcecaret)
		makeCaretVisible();
}

int EditableWidget::getCaretOffset() const {
	Common::UnicodeBiDiText utxt(_editString);
	Common::U32String substr(utxt.visual.begin(), utxt.visual.begin() + _caretPos);
	return g_gui.getStringWidth(substr, _font) - _editScrollOffset;
}

int EditableWidget::getSelectionCarretOffset() const {
	Common::UnicodeBiDiText utxt(_editString);
	Common::U32String substr(utxt.visual.begin(), utxt.visual.begin() + _selCaretPos);
	return g_gui.getStringWidth(substr, _font) - _editScrollOffset;
}

  void EditableWidget::drawCaret(bool erase, bool useRelativeCoordinates) {
	// Only draw if item is visible
	if (!isVisible() || !_boss->isVisible())
		return;

	Common::Rect editRect = getEditRect();

	int xOff;
	int yOff;

	if (useRelativeCoordinates) {
		xOff = getRelX();
		yOff = getRelY();
	} else {
		xOff = getAbsX();
		yOff = getAbsY();
	}

	int x = editRect.left;
	int y = editRect.top;

	if (_align == Graphics::kTextAlignRight) {
		int strVisibleWidth = g_gui.getStringWidth(_editString, _font) - _editScrollOffset;
		if (strVisibleWidth > editRect.width()) {
			_drawAlign = Graphics::kTextAlignLeft;
			strVisibleWidth = editRect.width();
		} else {
			_drawAlign = _align;
		}
		x = editRect.right - strVisibleWidth;
	}

	const int caretOffset = getCaretOffset();
	x += caretOffset;

	if (y < 0 || y + editRect.height() > _h)
		return;

	if (g_gui.useRTL())
		x += g_system->getOverlayWidth() - _w - xOff;
	else
		x += xOff;
	y += yOff;

	g_gui.theme()->drawCaret(Common::Rect(x, y, x + 1, y + editRect.height()), erase);

	if (erase) {
		Common::U32String character;
		int width;

		if ((uint)_caretPos < _editString.size()) {
			Common::UnicodeBiDiText utxt(_editString);
			const Common::u32char_type_t chr = utxt.visual[_caretPos];
			width = g_gui.getCharWidth(chr, _font);
			character = Common::U32String(chr);

			const uint32 last = (_caretPos > 0) ?  utxt.visual[_caretPos - 1] : 0;
			x += g_gui.getKerningOffset(last, chr, _font);
		} else {
			// We draw a fake space here to assure that removing the caret
			// does not result in color glitches in case the edit rect is
			// drawn with an inversion.
			width = g_gui.getCharWidth(' ', _font);
			character = " ";
		}

		// TODO: Right now we manually prevent text from being drawn outside
		// the edit area here. We might want to consider to use
		// setTextDrawableArea for this. However, it seems that only
		// EditTextWidget uses that but not ListWidget. Thus, one should check
		// whether we can unify the drawing in the text area first to avoid
		// possible glitches due to different methods used.

		ThemeEngine::TextInversionState inversion = _inversion;

		if (!_disableSelection)
			inversion = (_selOffset < 0) ? ThemeEngine::kTextInversionFocus : ThemeEngine::kTextInversionNone;

		width = MIN(editRect.width() - caretOffset, width);
		if (width > 0) {
			g_gui.theme()->drawText(Common::Rect(x, y, x + width, y + editRect.height()), character,
			                        _state, _drawAlign, inversion, 0, false, _font,
			                        ThemeEngine::kFontColorNormal, true, _textDrawableArea);
		}
	}

	_caretVisible = !erase;
}

bool EditableWidget::setCaretPos(int newPos) {
	assert(newPos >= 0 && newPos <= (int)_editString.size());
	_caretPos = newPos;
	return adjustOffset();
}

bool EditableWidget::adjustOffset() {
	// check if the caret is still within the textbox; if it isn't,
	// adjust _editScrollOffset

	int caretpos = getCaretOffset();
	const int editWidth = getEditRect().width();

	if (caretpos < 0) {
		// scroll left
		_editScrollOffset += caretpos;
		return true;
	} else if (caretpos >= editWidth) {
		// scroll right
		_editScrollOffset -= (editWidth - caretpos);
		return true;
	} else if (_editScrollOffset > 0) {
		const int strWidth = g_gui.getStringWidth(_editString, _font);
		if (strWidth - _editScrollOffset < editWidth) {
			// scroll right
			_editScrollOffset = (strWidth - editWidth);
			if (_editScrollOffset < 0)
				_editScrollOffset = 0;
		}
	}

	return false;
}

void EditableWidget::makeCaretVisible() {
	_caretTime = g_system->getMillis() + kCaretBlinkTime;
	_caretVisible = true;
	drawCaret(false);
}

void EditableWidget::clearSelection() {
	_selCaretPos = -1;
	_selOffset = 0;
	markAsDirty();
}

void EditableWidget::setSelectionOffset(int newOffset) {
	_selOffset = newOffset;
}

} // End of namespace GUI
