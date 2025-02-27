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
#include "Dialog.hxx"
#include "Stack.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Joystick.hxx"
#include "bspf.hxx"
#include "DialogContainer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DialogContainer::DialogContainer(OSystem& osystem)
  : myOSystem(osystem),
    myTime(0),
    myClickRepeatTime(0),
    myButtonRepeatTime(0),
    myAxisRepeatTime(0),
    myHatRepeatTime(0)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::updateTime(uInt64 time)
{
  if(myDialogStack.empty())
    return;

  // We only need millisecond precision
  myTime = time / 1000;

  // Check for pending continuous events and send them to the active dialog box
  Dialog* activeDialog = myDialogStack.top();

  // Mouse button still pressed
  if(myCurrentMouseDown.b != MouseButton::NONE && myClickRepeatTime < myTime)
  {
    activeDialog->handleMouseDown(myCurrentMouseDown.x - activeDialog->_x,
                                  myCurrentMouseDown.y - activeDialog->_y,
                                  myCurrentMouseDown.b, 1);
    myClickRepeatTime = myTime + kRepeatSustainDelay;
  }

  // Joystick button still pressed
  if(myCurrentButtonDown.stick != -1 && myButtonRepeatTime < myTime)
  {
    activeDialog->handleJoyDown(myCurrentButtonDown.stick, myCurrentButtonDown.button);
    myButtonRepeatTime = myTime + kRepeatSustainDelay;
  }

  // Joystick axis still pressed
  if(myCurrentAxisDown.stick != -1 && myAxisRepeatTime < myTime)
  {
    activeDialog->handleJoyAxis(myCurrentAxisDown.stick, myCurrentAxisDown.axis,
                                myCurrentAxisDown.value);
    myAxisRepeatTime = myTime + kRepeatSustainDelay;
  }

  // Joystick hat still pressed
  if(myCurrentHatDown.stick != -1 && myHatRepeatTime < myTime)
  {
    activeDialog->handleJoyHat(myCurrentHatDown.stick, myCurrentHatDown.hat,
                                myCurrentHatDown.value);
    myHatRepeatTime = myTime + kRepeatSustainDelay;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::draw(bool full)
{
  if(myDialogStack.empty())
    return false;

  // Make the top dialog dirty if a full redraw is requested
  if(full)
    myDialogStack.top()->setDirty();

  // If the top dialog is dirty, then all below it must be redrawn too
  const bool dirty = needsRedraw();

  myDialogStack.applyAll([&](Dialog*& d){
    if(dirty)
      d->setDirty();
    full |= d->render();
  });

  return full;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::needsRedraw() const
{
  return !myDialogStack.empty() ? myDialogStack.top()->isDirty() : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DialogContainer::baseDialogIsActive() const
{
  return myDialogStack.size() == 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DialogContainer::addDialog(Dialog* d)
{
  const Common::Rect& r = myOSystem.frameBuffer().imageRect();
  const uInt32 scale = myOSystem.frameBuffer().hidpiScaleFactor();

  if(uInt32(d->getWidth() * scale) > r.w() || uInt32(d->getHeight() * scale) > r.h())
    myOSystem.frameBuffer().showMessage(
        "Unable to show dialog box; FIX THE CODE");
  else
  {
    d->setDirty();
    myDialogStack.push(d);
  }
  return myDialogStack.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::removeDialog()
{
  if(!myDialogStack.empty())
  {
    myDialogStack.pop();
    if(!myDialogStack.empty())
      myDialogStack.top()->setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reStack()
{
  // Pop all items from the stack, and then add the base menu
  while(!myDialogStack.empty())
    myDialogStack.top()->close();

  baseDialog()->open();

  // Reset all continuous events
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleTextEvent(char text)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->handleText(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleKeyEvent(StellaKey key, StellaMod mod, bool pressed, bool repeated)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  if(pressed)
    activeDialog->handleKeyDown(key, mod);
  else
    activeDialog->handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseMotionEvent(int x, int y)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->surface().translateCoords(x, y);
  activeDialog->handleMouseMoved(x - activeDialog->_x, y - activeDialog->_y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleMouseButtonEvent(MouseButton b, bool pressed,
                                             int x, int y)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();
  activeDialog->surface().translateCoords(x, y);

  switch(b)
  {
    case MouseButton::LEFT:
    case MouseButton::RIGHT:
      if(pressed)
      {
        // If more than two clicks have been recorded, we start over
        if(myLastClick.count == 2)
        {
          myLastClick.x = myLastClick.y = 0;
          myLastClick.time = 0;
          myLastClick.count = 0;
        }

        if(myLastClick.count && (myTime < myLastClick.time + kDoubleClickDelay)
           && std::abs(myLastClick.x - x) < 3
           && std::abs(myLastClick.y - y) < 3)
        {
          myLastClick.count++;
        }
        else
        {
          myLastClick.x = x;
          myLastClick.y = y;
          myLastClick.count = 1;
        }
        myLastClick.time = myTime;

        // Now account for repeated mouse events (click and hold), but only
        // if the dialog wants them
        if(activeDialog->handleMouseClicks(x - activeDialog->_x, y - activeDialog->_y, b))
        {
          myCurrentMouseDown.x = x;
          myCurrentMouseDown.y = y;
          myCurrentMouseDown.b = b;
          myClickRepeatTime = myTime + kRepeatInitialDelay;
        }
        else
          myCurrentMouseDown.b = MouseButton::NONE;

        activeDialog->handleMouseDown(x - activeDialog->_x, y - activeDialog->_y,
                                      b, myLastClick.count);
      }
      else
      {
        activeDialog->handleMouseUp(x - activeDialog->_x, y - activeDialog->_y,
                                    b, myLastClick.count);

        if(b == myCurrentMouseDown.b)
          myCurrentMouseDown.b = MouseButton::NONE;
      }
      break;

    case MouseButton::WHEELUP:
      activeDialog->handleMouseWheel(x - activeDialog->_x, y - activeDialog->_y, -1);
      break;

    case MouseButton::WHEELDOWN:
      activeDialog->handleMouseWheel(x - activeDialog->_x, y - activeDialog->_y, 1);
      break;

    case MouseButton::NONE:  // should never get here
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyBtnEvent(int stick, int button, bool pressed)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  if(pressed)
  {
    myCurrentButtonDown.stick  = stick;
    myCurrentButtonDown.button = button;
    myButtonRepeatTime = myTime + kRepeatInitialDelay;

    activeDialog->handleJoyDown(stick, button);
  }
  else
  {
    // Only stop firing events if it's the current button
    if(stick == myCurrentButtonDown.stick)
      myCurrentButtonDown.stick = myCurrentButtonDown.button = -1;

    activeDialog->handleJoyUp(stick, button);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyAxisEvent(int stick, int axis, int value, int button)
{
  if(myDialogStack.empty())
    return;

  // Only stop firing events if it's the current stick
  if(myCurrentAxisDown.stick == stick && value == 0)
  {
    cerr << "handleJoyAxisEvent 0" << endl;
    myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
  }
  else if(value != 0)  // never repeat the 'off' event
  {
    cerr << "handleJoyAxisEvent repeat" << endl;
    // Now account for repeated axis events (press and hold)
    myCurrentAxisDown.stick = stick;
    myCurrentAxisDown.axis  = axis;
    myCurrentAxisDown.value = value;
    myAxisRepeatTime = myTime + kRepeatInitialDelay;
  }
  myDialogStack.top()->handleJoyAxis(stick, axis, value, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::handleJoyHatEvent(int stick, int hat, JoyHat value, int button)
{
  if(myDialogStack.empty())
    return;

  // Only stop firing events if it's the current stick
  if(myCurrentHatDown.stick == stick && value == JoyHat::CENTER)
  {
    myCurrentHatDown.stick = myCurrentHatDown.hat = -1;
  }
  else if(value != JoyHat::CENTER)  // never repeat the 'center' direction
  {
    // Now account for repeated hat events (press and hold)
    myCurrentHatDown.stick = stick;
    myCurrentHatDown.hat  = hat;
    myCurrentHatDown.value = value;
    myHatRepeatTime = myTime + kRepeatInitialDelay;
  }
  myDialogStack.top()->handleJoyHat(stick, hat, value, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DialogContainer::reset()
{
  myCurrentMouseDown.b = MouseButton::NONE;
  myLastClick.x = myLastClick.y = 0;
  myLastClick.time = 0;
  myLastClick.count = 0;

  myCurrentButtonDown.stick = myCurrentButtonDown.button = -1;
  myCurrentAxisDown.stick = myCurrentAxisDown.axis = -1;
  myCurrentHatDown.stick = myCurrentHatDown.hat = -1;
}
