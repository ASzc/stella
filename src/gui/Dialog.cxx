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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "Menu.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"

#include "ContextMenu.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Console.hxx"

#include "Vec.hxx"
#include "TIA.hxx"

/*
 * TODO list
 * - add some sense of the window being "active" (i.e. in front) or not. If it
 *   was inactive and just became active, reset certain vars (like who is focused).
 *   Maybe we should just add lostFocus and receivedFocus methods to Dialog, just
 *   like we have for class Widget?
 * ...
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Dialog(OSystem& instance, DialogContainer& parent, const GUI::Font& font,
               const string& title, int x, int y, int w, int h)
  : GuiObject(instance, parent, *this, x, y, w, h),
    _font(font),
    _mouseWidget(nullptr),
    _focusedWidget(nullptr),
    _dragWidget(nullptr),
    _defaultWidget(nullptr),
    _okWidget(nullptr),
    _cancelWidget(nullptr),
    _visible(false),
    _onTop(true),
    _processCancel(false),
    _title(title),
    _th(0),
    _layer(0),
    _surface(nullptr),
    _tabID(0),
    _flags(Widget::FLAG_ENABLED | Widget::FLAG_BORDER | Widget::FLAG_CLEARBG),
    _max_w(0),
    _max_h(0)
{
  setTitle(title);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Dialog(OSystem& instance, DialogContainer& parent,
               int x, int y, int w, int h)
  : Dialog(instance, parent, instance.frameBuffer().font(), "", x, y, w, h)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::~Dialog()
{
  _myFocus.list.clear();
  _myTabList.clear();

  delete _firstWidget;
  _firstWidget = nullptr;

  _buttonGroup.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::open()
{
  // Make sure we have a valid surface to draw into
  // Technically, this shouldn't be needed until drawDialog(), but some
  // dialogs cause drawing to occur within loadConfig()
  if (_surface == nullptr)
    _surface = instance().frameBuffer().allocateSurface(_w, _h);
  else if (uInt32(_w) > _surface->width() || uInt32(_h) > _surface->height())
    _surface->resize(_w, _h);
  _surface->setSrcSize(_w, _h);
  _layer = parent().addDialog(this);

  // Take hidpi scaling into account
  const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
  _surface->setDstSize(_w * scale, _h * scale);

  center();

  if(_myTabList.size())
    // (Re)-build the focus list to use for all widgets of all tabs
    for(auto& tabfocus : _myTabList)
      buildCurrentFocusList(tabfocus.widget->getID());
  else
    buildCurrentFocusList();

  /*if (!_surface->attributes().blending)
  {
    _surface->attributes().blending = true;
    _surface->attributes().blendalpha = 90;
    _surface->applyAttributes();
  }*/

  loadConfig(); // has to be done AFTER (re)building the focus list

  _visible = true;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::close()
{
  if(_mouseWidget)
  {
    _mouseWidget->handleMouseLeft();
    _mouseWidget = nullptr;
  }

  releaseFocus();

  _visible = false;

  parent().removeDialog();
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::setTitle(const string& title)
{
  _title = title;
  _h -= _th;
  if(title.empty())
    _th = 0;
  else
    _th = _font.getLineHeight() + 4;
  _h += _th;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::center()
{
  positionAt(instance().settings().getInt("dialogpos"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::positionAt(uInt32 pos)
{
  const bool fullscreen = instance().settings().getBool("fullscreen");
  const double overscan = fullscreen ? instance().settings().getInt("tia.fs_overscan") / 200.0 : 0.0;
  const Common::Size& screen = instance().frameBuffer().screenSize();
  const Common::Rect& dst = _surface->dstRect();
  // shift stacked dialogs
  Int32 hgap = (screen.w >> 6) * _layer + screen.w * overscan;
  Int32 vgap = (screen.w >> 6) * _layer + screen.h * overscan;
  int top = std::min(std::max(0, Int32(screen.h - dst.h())), vgap);
  int btm = std::max(0, Int32(screen.h - dst.h() - vgap));
  int left = std::min(std::max(0, Int32(screen.w - dst.w())), hgap);
  int right = std::max(0, Int32(screen.w - dst.w() - hgap));

  switch (pos)
  {
    case 1:
      _surface->setDstPos(left, top);
      break;

    case 2:
      _surface->setDstPos(right, top);
      break;

    case 3:
      _surface->setDstPos(right, btm);
      break;

    case 4:
      _surface->setDstPos(left, btm);
      break;

    default:
      // center
      _surface->setDstPos((screen.w - dst.w()) >> 1, (screen.h - dst.h()) >> 1);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::render()
{
  if(!_dirty || !isVisible())
    return false;

  // Draw this dialog
  center();
  drawDialog();

  // Update dialog surface; also render any extra surfaces
  // Extra surfaces must be rendered afterwards, so they are drawn on top
  if(_surface->render())
  {
    mySurfaceStack.applyAll([](shared_ptr<FBSurface>& surface){
      surface->render();
    });
  }
  _dirty = false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::releaseFocus()
{
  if(_focusedWidget)
  {
    // remember focus of all tabs for when dialog is reopened again
    for(auto& tabfocus : _myTabList)
      tabfocus.saveCurrentFocus(_focusedWidget);

    //_focusedWidget->lostFocus();
    //_focusedWidget = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addFocusWidget(Widget* w)
{
  if(!w)
    return;

  // All focusable widgets should retain focus
  w->setFlags(Widget::FLAG_RETAIN_FOCUS);

  _myFocus.widget = w;
  _myFocus.list.push_back(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addToFocusList(WidgetArray& list)
{
  // All focusable widgets should retain focus
  for(const auto& w: list)
    w->setFlags(Widget::FLAG_RETAIN_FOCUS);

  Vec::append(_myFocus.list, list);
  _focusList = _myFocus.list;

  if(list.size() > 0)
    _myFocus.widget = list[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addToFocusList(WidgetArray& list, TabWidget* w, int tabId)
{
  // Only add the list if the tab actually exists
  if(!w || w->getID() >= _myTabList.size())
    return;

  assert(w == _myTabList[w->getID()].widget);

  // All focusable widgets should retain focus
  for(const auto& fw: list)
    fw->setFlags(Widget::FLAG_RETAIN_FOCUS);

  // First get the appropriate focus list
  FocusList& focus = _myTabList[w->getID()].focus;

  // Now insert in the correct place in that focus list
  uInt32 id = tabId;
  if(id < focus.size())
    Vec::append(focus[id].list, list);
  else
  {
    // Make sure the array is large enough
    while(focus.size() <= id)
      focus.push_back(Focus());

    Vec::append(focus[id].list, list);
  }

  if(list.size() > 0)
    focus[id].widget = list[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addTabWidget(TabWidget* w)
{
  if(!w)
    return;

  // Make sure the array is large enough
  uInt32 id = w->getID();
  while(_myTabList.size() < id)
    _myTabList.push_back(TabFocus());

  _myTabList.push_back(TabFocus(w));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::setFocus(Widget* w)
{
  // If the click occured inside a widget which is not the currently
  // focused one, change the focus to that widget.
  if(w && w != _focusedWidget && w->wantsFocus())
  {
    // Redraw widgets for new focus
    _focusedWidget = Widget::setFocusForChain(this, getFocusList(), w, 0);

    // Update current tab based on new focused widget
    getTabIdForWidget(_focusedWidget);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::buildCurrentFocusList(int tabID)
{
  // Yes, this is hideously complex.  That's the price we pay for
  // tab navigation ...
  _focusList.clear();

  // Remember which tab item previously had focus, if applicable
  // This only applies if this method was called for a tab change
  Widget* tabFocusWidget = nullptr;
  if(tabID >= 0 && tabID < int(_myTabList.size()))
  {
    // Save focus in previously selected tab column,
    // and get focus for new tab column
    TabFocus& tabfocus = _myTabList[tabID];
    tabfocus.saveCurrentFocus(_focusedWidget);
    tabFocusWidget = tabfocus.getNewFocus();

    _tabID = tabID;
  }

  // Add appropriate items from tablist (if present)
  for(auto& tabfocus: _myTabList)
    tabfocus.appendFocusList(_focusList);

  // Add remaining items from main focus list
  Vec::append(_focusList, _myFocus.list);

  // Add button group at end of current focus list
  // We do it this way for TabWidget, so that buttons are scanned
  // *after* the widgets in the current tab
  if(_buttonGroup.size() > 0)
    Vec::append(_focusList, _buttonGroup);

  // Finally, the moment we've all been waiting for :)
  // Set the actual focus widget
  if(tabFocusWidget)
    _focusedWidget = tabFocusWidget;
  else if(!_focusedWidget && _focusList.size() > 0)
    _focusedWidget = _focusList[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addSurface(shared_ptr<FBSurface> surface)
{
  mySurfaceStack.push(surface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::drawDialog()
{
  if(!isVisible())
    return;

  FBSurface& s = surface();

  // Dialog is still on top if e.g a ContextMenu is opened
  _onTop = parent().myDialogStack.top() == this
    || (parent().myDialogStack.get(parent().myDialogStack.size() - 2) == this
    && !parent().myDialogStack.top()->hasTitle());

  if(_flags & Widget::FLAG_CLEARBG)
  {
    //    cerr << "Dialog::drawDialog(): w = " << _w << ", h = " << _h << " @ " << &s << endl << endl;
    s.fillRect(_x, _y + _th, _w, _h - _th, _onTop ? kDlgColor : kBGColorLo);
    if(_th)
    {
      s.fillRect(_x, _y, _w, _th, _onTop ? kColorTitleBar : kColorTitleBarLo);
      s.drawString(_font, _title, _x + 10, _y + 2 + 1, _font.getStringWidth(_title),
                   _onTop ? kColorTitleText : kColorTitleTextLo);
    }
  }
  else
    s.invalidate();
  if(_flags & Widget::FLAG_BORDER) // currently only used by Dialog itself
    s.frameRect(_x, _y, _w, _h, _onTop ? kColor : kShadowColor);

  // Make all child widget dirty
  Widget* w = _firstWidget;
  Widget::setDirtyInChain(w);

  // Draw all children
  w = _firstWidget;
  while(w)
  {
    w->draw();
    w = w->_next;
  }

  // Draw outlines for focused widgets
  // Don't change focus, since this will trigger lost and received
  // focus events
  if(_focusedWidget)
  {
    _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
      _focusedWidget, 0, false);
    if(_focusedWidget)
      _focusedWidget->draw(); // make sure the highlight color is drawn initially
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleText(char text)
{
  // Focused widget receives text events
  if(_focusedWidget)
    _focusedWidget->handleText(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  Event::Type e = Event::NoType;

// FIXME - I don't think this will compile!
#if defined(RETRON77)
  // special keys used for R77
  if (key == KBDK_F13)
    e = Event::UITabPrev;
  else if (key == KBDK_BACKSPACE)
    e = Event::UITabNext;
#endif

  // Check the keytable now, since we might get one of the above events,
  // which must always be processed before any widget sees it.
  if(e == Event::NoType)
    e = instance().eventHandler().eventForKey(kMenuMode, key, mod);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleKeyDown(key, mod);
    else
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyUp(StellaKey key, StellaMod mod)
{
  // Focused widget receives keyup events
  if(_focusedWidget)
    _focusedWidget->handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  Widget* w = findWidget(x, y);

  _dragWidget = w;
  setFocus(w);

  if(w)
    w->handleMouseDown(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y),
                       b, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(_focusedWidget)
  {
    // Lose focus on mouseup unless the widget requested to retain the focus
    if(! (_focusedWidget->getFlags() & Widget::FLAG_RETAIN_FOCUS ))
      releaseFocus();
  }

  Widget* w = _dragWidget;
  if(w)
    w->handleMouseUp(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y),
                     b, clickCount);

  _dragWidget = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseWheel(int x, int y, int direction)
{
  // This may look a bit backwards, but I think it makes more sense for
  // the mouse wheel to primarily affect the widget the mouse is at than
  // the widget that happens to be focused.

  Widget* w = findWidget(x, y);
  if(!w)
    w = _focusedWidget;
  if(w)
    w->handleMouseWheel(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseMoved(int x, int y)
{
  Widget* w;

  if(_focusedWidget && !_dragWidget)
  {
    w = _focusedWidget;
    int wx = w->getAbsX() - _x;
    int wy = w->getAbsY() - _y;

    // We still send mouseEntered/Left messages to the focused item
    // (but to no other items).
    bool mouseInFocusedWidget = (x >= wx && x < wx + w->_w && y >= wy && y < wy + w->_h);
    if(mouseInFocusedWidget && _mouseWidget != w)
    {
      if(_mouseWidget)
        _mouseWidget->handleMouseLeft();
      _mouseWidget = w;
      w->handleMouseEntered();
    }
    else if (!mouseInFocusedWidget && _mouseWidget == w)
    {
      _mouseWidget = nullptr;
      w->handleMouseLeft();
    }

    w->handleMouseMoved(x - wx, y - wy);
  }

  // While a "drag" is in process (i.e. mouse is moved while a button is pressed),
  // only deal with the widget in which the click originated.
  if (_dragWidget)
    w = _dragWidget;
  else
    w = findWidget(x, y);

  if (_mouseWidget != w)
  {
    if (_mouseWidget)
      _mouseWidget->handleMouseLeft();
    if (w)
      w->handleMouseEntered();
    _mouseWidget = w;
  }

  if (w && (w->getFlags() & Widget::FLAG_TRACK_MOUSE))
    w->handleMouseMoved(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleMouseClicks(int x, int y, MouseButton b)
{
  Widget* w = findWidget(x, y);

  if(w)
    return w->handleMouseClicks(x - (w->getAbsX() - _x),
                                y - (w->getAbsY() - _y), b);
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleJoyDown(int stick, int button)
{
  Event::Type e =
    instance().eventHandler().eventForJoyButton(kMenuMode, stick, button);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleJoyDown(stick, button);
    else
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleJoyUp(int stick, int button)
{
  // Focused widget receives joystick events
  if(_focusedWidget)
    _focusedWidget->handleJoyUp(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type Dialog::getJoyAxisEvent(int stick, int axis, int value, int button)
{
  return instance().eventHandler().eventForJoyAxis(kMenuMode, stick, axis, value, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleJoyAxis(int stick, int axis, int value, int button)
{
  Event::Type e = getJoyAxisEvent(stick, axis, value, button);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleJoyAxis(stick, axis, value, button);
    else if(value != 0)
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleJoyHat(int stick, int hat, JoyHat value, int button)
{
  Event::Type e =
    instance().eventHandler().eventForJoyHat(kMenuMode, stick, hat, value, button);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      return _focusedWidget->handleJoyHat(stick, hat, value, button);
    else
      return _focusedWidget->handleEvent(e);
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleNavEvent(Event::Type e)
{
  switch(e)
  {
    case Event::UITabPrev:
      if (cycleTab(-1))
        return true;
      break;

    case Event::UITabNext:
      if (cycleTab(1))
        return true;
      break;

    case Event::UINavPrev:
      if(_focusedWidget && !_focusedWidget->wantsTab())
      {
        _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                                                  _focusedWidget, -1);
        // Update current tab based on new focused widget
        getTabIdForWidget(_focusedWidget);

        return true;
      }
      break;

    case Event::UINavNext:
      if(_focusedWidget && !_focusedWidget->wantsTab())
      {
        _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                                                  _focusedWidget, +1);
        // Update current tab based on new focused widget
        getTabIdForWidget(_focusedWidget);

        return true;
      }
      break;

    case Event::UIOK:
      if(_okWidget && _okWidget->isEnabled())
      {
        // Receiving 'OK' is the same as getting the 'Select' event
        _okWidget->handleEvent(Event::UISelect);
        return true;
      }
      break;

    case Event::UICancel:
      if(_cancelWidget && _cancelWidget->isEnabled())
      {
        // Receiving 'Cancel' is the same as getting the 'Select' event
        _cancelWidget->handleEvent(Event::UISelect);
        return true;
      }
      else if(_processCancel)
      {
        // Some dialogs want the ability to cancel without actually having
        // a corresponding cancel button
        processCancel();
        return true;
      }
      break;

    default:
      return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::getTabIdForWidget(Widget* w)
{
  if(_myTabList.size() == 0 || !w)
    return;

  for(uInt32 id = 0; id < _myTabList.size(); ++id)
  {
    if(w->_boss == _myTabList[id].widget)
    {
      _tabID = id;
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::cycleTab(int direction)
{
  if(_tabID >= 0 && _tabID < int(_myTabList.size()))
  {
    _myTabList[_tabID].widget->cycleTab(direction);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case TabWidget::kTabChangedCmd:
      if(_visible)
        buildCurrentFocusList(id);
      break;

    case GuiObject::kCloseCmd:
      close();
      break;
  }
}

/*
 * Determine the widget at location (x,y) if any. Assumes the coordinates are
 * in the local coordinate system, i.e. relative to the top left of the dialog.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Dialog::findWidget(int x, int y) const
{
  return Widget::findWidgetInChain(_firstWidget, x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                               const string& okText, const string& cancelText,
                               bool focusOKButton, int buttonWidth)
{
  const int HBORDER = 10;
  const int VBORDER = 10;
  const int BTN_BORDER = 20;
  const int BUTTON_GAP = 8;
  buttonWidth = std::max(buttonWidth,
                         std::max(font.getStringWidth("Defaults"),
                         std::max(font.getStringWidth(okText),
                         font.getStringWidth(cancelText))) + BTN_BORDER);
  int buttonHeight = font.getLineHeight() + 4;

  _w = std::max(HBORDER * 2 + buttonWidth * 2 + BUTTON_GAP, _w);

#ifndef BSPF_MACOS
  addOKWidget(new ButtonWidget(this, font, _w - 2 * buttonWidth - HBORDER - BUTTON_GAP,
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, okText, GuiObject::kOKCmd));
  addCancelWidget(new ButtonWidget(this, font, _w - (buttonWidth + HBORDER),
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, cancelText, GuiObject::kCloseCmd));
#else
  addCancelWidget(new ButtonWidget(this, font, _w - 2 * buttonWidth - HBORDER - BUTTON_GAP,
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, cancelText, GuiObject::kCloseCmd));
  addOKWidget(new ButtonWidget(this, font, _w - (buttonWidth + HBORDER),
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, okText, GuiObject::kOKCmd));
#endif

  // Note that 'focusOKButton' only takes effect when there are no other UI
  // elements in the dialog; otherwise, the first widget of the dialog is always
  // automatically focused first
  // Changing this behaviour would require a fairly major refactoring of the UI code
  if(focusOKButton)
  {
    wid.push_back(_okWidget);
    wid.push_back(_cancelWidget);
  }
  else
  {
    wid.push_back(_cancelWidget);
    wid.push_back(_okWidget);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addDefaultsOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                                       const string& okText, const string& cancelText,
                                       const string& defaultsText,
                                       bool focusOKButton)
{
  const int HBORDER = 10;
  const int VBORDER = 10;
  const int BTN_BORDER = 20;
  int buttonWidth = font.getStringWidth(defaultsText) + BTN_BORDER;
  int buttonHeight = font.getLineHeight() + 4;

  addDefaultWidget(new ButtonWidget(this, font, HBORDER, _h - buttonHeight - VBORDER,
                   buttonWidth, buttonHeight, defaultsText, GuiObject::kDefaultsCmd));
  wid.push_back(_defaultWidget);

  addOKCancelBGroup(wid, font, okText, cancelText, focusOKButton, buttonWidth);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::TabFocus::appendFocusList(WidgetArray& list)
{
  int active = widget->getActiveTab();

  if(active >= 0 && active < int(focus.size()))
    Vec::append(list, focus[active].list);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::TabFocus::saveCurrentFocus(Widget* w)
{
  if(currentTab < focus.size() &&
      Widget::isWidgetInChain(focus[currentTab].list, w))
    focus[currentTab].widget = w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Dialog::TabFocus::getNewFocus()
{
  currentTab = widget->getActiveTab();

  return (currentTab < focus.size()) ? focus[currentTab].widget : nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::getDynamicBounds(uInt32& w, uInt32& h) const
{
  const Common::Rect& r = instance().frameBuffer().imageRect();
  const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();

  if(r.w() <= FBMinimum::Width || r.h() <= FBMinimum::Height)
  {
    w = r.w() / scale;
    h = r.h() / scale;
    return false;
  }
  else
  {
    w = uInt32(0.95 * r.w() / scale);
    h = uInt32(0.95 * r.h() / scale);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::setSize(uInt32 w, uInt32 h, uInt32 max_w, uInt32 max_h)
{
  _w = std::min(w, max_w);
  _max_w = w;
  _h = std::min(h, max_h);
  _max_h = h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::shouldResize(uInt32& w, uInt32& h) const
{
  getDynamicBounds(w, h);

  // returns true if the current size is larger than the allowed size or
  //  if the current size is smaller than the allowed and wanted size
  return (uInt32(_w) > w || uInt32(_h) > h ||
          (uInt32(_w) < w && uInt32(_w) < _max_w) ||
          (uInt32(_h) < h && uInt32(_h) < _max_h));
}
