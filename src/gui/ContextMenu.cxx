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
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "ScrollBarWidget.hxx"
#include "ContextMenu.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu::ContextMenu(GuiObject* boss, const GUI::Font& font,
                         const VariantList& items, int cmd, int width)
  : Dialog(boss->instance(), boss->parent(), font),
    CommandSender(boss),
    _rowHeight(font.getLineHeight()),
    _firstEntry(0),
    _numEntries(0),
    _selectedOffset(0),
    _selectedItem(-1),
    _showScroll(false),
    _isScrolling(false),
    _scrollUpColor(kColor),
    _scrollDnColor(kColor),
    _cmd(cmd),
    _xorig(0),
    _yorig(0),
    _maxWidth(width)
{
  addItems(items);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::addItems(const VariantList& items)
{
  _entries.clear();
  _entries = items;

  // Resize to largest string
  int maxwidth = _maxWidth;
  for(const auto& e: _entries)
    maxwidth = std::max(maxwidth, _font.getStringWidth(e.first));

  _x = _y = 0;
  _w = maxwidth + 23;
  _h = 1;  // recalculate this in ::recalc()

  _scrollUpColor = _firstEntry > 0 ? kScrollColor : kColor;
  _scrollDnColor = (_firstEntry + _numEntries < int(_entries.size())) ?
      kScrollColor : kColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::show(uInt32 x, uInt32 y, const Common::Rect& bossRect, int item)
{
  uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
  _xorig = bossRect.x() + x * scale;
  _yorig = bossRect.y() + y * scale;

  // Only show menu if we're inside the visible area
  if(!bossRect.contains(_xorig, _yorig))
    return;

  recalc(instance().frameBuffer().imageRect());
  open();
  setSelectedIndex(item);
  moveToSelected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::center()
{
  // First set position according to original coordinates
  surface().setDstPos(_xorig, _yorig);

  // Now make sure that the entire menu can fit inside the screen bounds
  // If not, we reset its position
  if(!instance().frameBuffer().screenRect().contains(
      _xorig, _yorig, surface().dstRect()))
    surface().setDstPos(_xorig, _yorig);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::recalc(const Common::Rect& image)
{
  // Now is the time to adjust the height
  // If it's higher than the screen, we need to scroll through
  uInt32 maxentries = std::min(18u, (image.h() - 2) / _rowHeight);
  if(_entries.size() > maxentries)
  {
    // We show two less than the max, so we have room for two scroll buttons
    _numEntries = maxentries - 2;
    _h = maxentries * _rowHeight + 2;
    _showScroll = true;
  }
  else
  {
    _numEntries = int(_entries.size());
    _h = int(_entries.size()) * _rowHeight + 2;
    _showScroll = false;
  }
  _isScrolling = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelectedIndex(int idx)
{
  if(idx >= 0 && idx < int(_entries.size()))
    _selectedItem = idx;
  else
    _selectedItem = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelected(const Variant& tag, const Variant& defaultTag)
{
  if(tag != "")  // indicates that the defaultTag should be used instead
  {
    for(uInt32 item = 0; item < _entries.size(); ++item)
    {
      if(BSPF::equalsIgnoreCase(_entries[item].second.toString(), tag.toString()))
      {
        setSelectedIndex(item);
        return;
      }
    }
  }

  // If we get this far, the value wasn't found; use the default value
  for(uInt32 item = 0; item < _entries.size(); ++item)
  {
    if(BSPF::equalsIgnoreCase(_entries[item].second.toString(), defaultTag.toString()))
    {
      setSelectedIndex(item);
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::setSelectedMax()
{
  setSelectedIndex(int(_entries.size()) - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::clearSelection()
{
  _selectedItem = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ContextMenu::getSelected() const
{
  return _selectedItem;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& ContextMenu::getSelectedName() const
{
  return (_selectedItem >= 0) ? _entries[_selectedItem].first : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Variant& ContextMenu::getSelectedTag() const
{
  return (_selectedItem >= 0) ? _entries[_selectedItem].second : EmptyVariant;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::sendSelectionUp()
{
  if(isVisible() || _selectedItem <= 0)
    return false;

  _selectedItem--;
  sendCommand(_cmd ? _cmd : ContextMenu::kItemSelectedCmd, _selectedItem, -1);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::sendSelectionDown()
{
  if(isVisible() || _selectedItem >= int(_entries.size()) - 1)
    return false;

  _selectedItem++;
  sendCommand(_cmd ? _cmd : ContextMenu::kItemSelectedCmd, _selectedItem, -1);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::sendSelectionFirst()
{
  if(isVisible())
    return false;

  _selectedItem = 0;
  sendCommand(_cmd ? _cmd : ContextMenu::kItemSelectedCmd, _selectedItem, -1);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::sendSelectionLast()
{
  if(isVisible())
    return false;

  _selectedItem = int(_entries.size()) - 1;
  sendCommand(_cmd ? _cmd : ContextMenu::kItemSelectedCmd, _selectedItem, -1);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  // Compute over which item the mouse is...
  int item = findItem(x, y);

  // Only do a selection when the left button is in the dialog
  if(b == MouseButton::LEFT)
  {
    if(item != -1)
    {
      _isScrolling = _showScroll && ((item == 0) || (item == _numEntries+1));
      sendSelection();
    }
    else
      close();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleMouseMoved(int x, int y)
{
  // Compute over which item the mouse is...
  int item = findItem(x, y);
  if(item == -1)
    return;

  // ...and update the selection accordingly
  drawCurrentSelection(item);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::handleMouseClicks(int x, int y, MouseButton b)
{
  // Let continuous mouse clicks come through, as the scroll buttons need them
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleMouseWheel(int x, int y, int direction)
{
  // Wheel events are only relevant in scroll mode
  if(_showScroll)
  {
    if(direction < 0)
      scrollUp(ScrollBarWidget::getWheelLines());
    else if(direction > 0)
      scrollDown(ScrollBarWidget::getWheelLines());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleKeyDown(StellaKey key, StellaMod mod)
{
  handleEvent(instance().eventHandler().eventForKey(kMenuMode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleJoyDown(int stick, int button)
{
  handleEvent(instance().eventHandler().eventForJoyButton(kMenuMode, stick, button));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleJoyAxis(int stick, int axis, int value, int button)
{
  if(value != 0)  // we don't care about 'axis up' events
    handleEvent(instance().eventHandler().eventForJoyAxis(kMenuMode, stick, axis, value, button));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ContextMenu::handleJoyHat(int stick, int hat, JoyHat value, int button)
{
  handleEvent(instance().eventHandler().eventForJoyHat(kMenuMode, stick, hat, value, button));
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::handleEvent(Event::Type e)
{
  switch(e)
  {
    case Event::UISelect:
      sendSelection();
      break;
    case Event::UIUp:
    case Event::UILeft:
      moveUp();
      break;
    case Event::UIDown:
    case Event::UIRight:
      moveDown();
      break;
    case Event::UIPgUp:
      movePgUp();
      break;
    case Event::UIPgDown:
      movePgDown();
      break;
    case Event::UIHome:
      moveToFirst();
      break;
    case Event::UIEnd:
      moveToLast();
      break;
    case Event::UICancel:
      close();
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ContextMenu::findItem(int x, int y) const
{
  if(x >= 0 && x < _w && y >= 0 && y < _h)
    return (y - 4) / _rowHeight;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawCurrentSelection(int item)
{
  // Change selection
  _selectedOffset = item;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::sendSelection()
{
  // Select the correct item when scrolling; we have to take into account
  // that the viewable items are no longer 1-to-1 with the entries
  int item = _firstEntry + _selectedOffset;

  if(_showScroll)
  {
    if(_selectedOffset == 0)  // scroll up
      return scrollUp();
    else if(_selectedOffset == _numEntries+1) // scroll down
      return scrollDown();
    else if(_isScrolling)
      return;
    else
      item--;
  }

  // We remove the dialog when the user has selected an item
  // Make sure the dialog is removed before sending any commands,
  // since one consequence of sending a command may be to add another
  // dialog/menu
  close();

  // Send any command associated with the selection
  _selectedItem = item;
  sendCommand(_cmd ? _cmd : ContextMenu::kItemSelectedCmd, _selectedItem, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveUp()
{
  if(_showScroll)
  {
    // Reaching the top of the list means we have to scroll up, but keep the
    // current item offset
    // Otherwise, the offset should decrease by 1
    if(_selectedOffset == 1)
      scrollUp();
    else if(_selectedOffset > 1)
      drawCurrentSelection(_selectedOffset-1);
  }
  else
  {
    if(_selectedOffset > 0)
      drawCurrentSelection(_selectedOffset-1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveDown()
{
  if(_showScroll)
  {
    // Reaching the bottom of the list means we have to scroll down, but keep the
    // current item offset
    // Otherwise, the offset should increase by 1
    if(_selectedOffset == _numEntries)
      scrollDown();
    else if(_selectedOffset < int(_entries.size()))
      drawCurrentSelection(_selectedOffset+1);
  }
  else
  {
    if(_selectedOffset < int(_entries.size()) - 1)
      drawCurrentSelection(_selectedOffset+1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::movePgUp()
{
  if(_firstEntry == 0)
    moveToFirst();
  else
    scrollUp(_numEntries);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::movePgDown()
{
  if(_firstEntry == int(_entries.size() - _numEntries))
    moveToLast();
  else
    scrollDown(_numEntries);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveToFirst()
{
  _firstEntry = 0;
  _scrollUpColor = kColor;
  _scrollDnColor = kScrollColor;

  drawCurrentSelection(_firstEntry + (_showScroll ? 1 : 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveToLast()
{
  _firstEntry = int(_entries.size()) - _numEntries;
  _scrollUpColor = kScrollColor;
  _scrollDnColor = kColor;

  drawCurrentSelection(_numEntries - (_showScroll ? 0 : 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::moveToSelected()
{
  if(_selectedItem < 0 || _selectedItem >= int(_entries.size()))
    return;

  // First jump immediately to the item
  _firstEntry = _selectedItem;
  int offset = 0;

  // Now check if we've gone past the current 'window' size, and scale
  // back accordingly
  int max_offset = int(_entries.size()) - _numEntries;
  if(_firstEntry > max_offset)
  {
    offset = _firstEntry - max_offset;
    _firstEntry -= offset;
  }

  _scrollUpColor = _firstEntry > 0 ? kScrollColor : kColor;
  _scrollDnColor = _firstEntry < max_offset ? kScrollColor : kColor;

  drawCurrentSelection(offset + (_showScroll ? 1 : 0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::scrollUp(int distance)
{
  if(_firstEntry == 0)
    return;

  _firstEntry = std::max(_firstEntry - distance, 0);
  _scrollUpColor = _firstEntry > 0 ? kScrollColor : kColor;
  _scrollDnColor = kScrollColor;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::scrollDown(int distance)
{
  int max_offset = int(_entries.size()) - _numEntries;
  if(_firstEntry == max_offset)
    return;

  _firstEntry = std::min(_firstEntry + distance, max_offset);
  _scrollUpColor = kScrollColor;
  _scrollDnColor = _firstEntry < max_offset ? kScrollColor : kColor;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ContextMenu::drawDialog()
{
  static uInt32 up_arrow[8] = {
    0b00011000,
    0b00011000,
    0b00111100,
    0b00111100,
    0b01111110,
    0b01111110,
    0b11111111,
    0b11111111
  };
  static uInt32 down_arrow[8] = {
    0b11111111,
    0b11111111,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00111100,
    0b00011000,
    0b00011000
  };

  // Normally we add widgets and let Dialog::draw() take care of this
  // logic.  But for some reason, this Dialog was written differently
  // by the ScummVM guys, so I'm not going to mess with it.
  FBSurface& s = surface();

  // Draw menu border and background
  s.fillRect(_x+1, _y+1, _w-2, _h-2, kWidColor);
  s.frameRect(_x, _y, _w, _h, kTextColor);

  // Draw the entries, taking scroll buttons into account
  int x = _x + 1, y = _y + 1, w = _w - 2;

  // Show top scroll area
  int offset = _selectedOffset;
  if(_showScroll)
  {
    s.hLine(x, y+_rowHeight-1, w+2, kColor);
    s.drawBitmap(up_arrow, ((_w-_x)>>1)-4, (_rowHeight>>1)+y-4, _scrollUpColor, 8);
    y += _rowHeight;
    offset--;
  }

  for(int i = _firstEntry, current = 0; i < _firstEntry + _numEntries; ++i, ++current)
  {
    bool hilite = offset == current;
    if(hilite) s.fillRect(x, y, w, _rowHeight, kTextColorHi);
    s.drawString(_font, _entries[i].first, x + 1, y + 2, w,
                 !hilite ? kTextColor : kTextColorInv);
    y += _rowHeight;
  }

  // Show bottom scroll area
  if(_showScroll)
  {
    s.hLine(x, y, w+2, kColor);
    s.drawBitmap(down_arrow, ((_w-_x)>>1)-4, (_rowHeight>>1)+y-4, _scrollDnColor, 8);
  }

  setDirty();
}
