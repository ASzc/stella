//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "FBSurface.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "EditTextWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditTextWidget::EditTextWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h, const string& text)
  : EditableWidget(boss, font, x, y, w, h + 2, text),
    _changed(false)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG | Widget::FLAG_RETAIN_FOCUS;

  startEditMode();  // We're always in edit mode
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::setText(const string& str, bool changed)
{
  EditableWidget::setText(str, changed);
  _backupString = str;
  _changed = changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::handleMouseEntered()
{
  setFlags(Widget::FLAG_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::handleMouseLeft()
{
  clearFlags(Widget::FLAG_HILITED);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(!isEditable())
    return;

  x += _editScrollOffset;

  int width = 0;
  uInt32 i;

  for (i = 0; i < editString().size(); ++i)
  {
    width += _font.getCharWidth(editString()[i]);
    if (width >= x)
      break;
  }

  if (setCaretPos(i))
    setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  bool onTop = _boss->dialog().isOnTop();

  // Highlight changes
  if(_changed && onTop)
    s.fillRect(_x, _y, _w, _h, kDbgChangedColor);
  else if(!isEditable() || !isEnabled())
    s.fillRect(_x, _y, _w, _h, onTop ? kDlgColor : kBGColorLo);

  // Draw a thin frame around us.
  s.frameRect(_x, _y, _w, _h, hilite && isEditable() && isEnabled() ? kWidColorHi : kColor);

  // Draw the text
  adjustOffset();
  s.drawString(_font, editString(), _x + 2, _y + 2, getEditRect().w(),
               _changed && onTop && isEnabled()
               ? kDbgChangedTextColor
               : onTop && isEnabled() ? _textcolor : kColor,
               TextAlign::Left, isEditable() ? -_editScrollOffset : 0, !isEditable());

  // Draw the caret
  drawCaret();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect EditTextWidget::getEditRect() const
{
  return Common::Rect(2, 1, _w - 2, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::lostFocusWidget()
{
  // If we loose focus, 'commit' the user changes
  _backupString = editString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::startEditMode()
{
  EditableWidget::startEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::endEditMode()
{
  // Editing is always enabled
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::abortEditMode()
{
  // Editing is always enabled
  setText(_backupString);
}
