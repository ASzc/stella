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

#ifndef DIALOG_CONTAINER_HXX
#define DIALOG_CONTAINER_HXX

class Dialog;
class OSystem;
class EventHandler;

#include "EventHandlerConstants.hxx"
#include "StellaKeys.hxx"
#include "Stack.hxx"
#include "bspf.hxx"

/**
  The base class for groups of dialog boxes.  Each dialog box has a
  parent.  In most cases, the parent is itself a dialog box, but in the
  case of the lower-most dialog box, this class is its parent.

  This class keeps track of its children (dialog boxes), organizes them into
  a stack, and handles their events.

  @author  Stephen Anthony
*/
class DialogContainer
{
  friend class EventHandler;
  friend class Dialog;

  public:
    /**
      Create a new DialogContainer stack
    */
    explicit DialogContainer(OSystem& osystem);
    virtual ~DialogContainer() = default;

  public:
    /**
      Update the dialog container with the current time.
      This is useful if we want to trigger events at some specified time.

      @param time  The current time in microseconds
    */
    void updateTime(uInt64 time);

    /**
      Handle a keyboard Unicode text event.

      @param text   Unicode character string
    */
    void handleTextEvent(char text);

    /**
      Handle a keyboard single-key event.

      @param key      Actual key symbol
      @param mod      Modifiers
      @param pressed  Pressed (true) or released (false)
    */
    void handleKeyEvent(StellaKey key, StellaMod mod, bool pressed, bool repeat);

    /**
      Handle a mouse motion event.

      @param x  The x location
      @param y  The y location
    */
    void handleMouseMotionEvent(int x, int y);

    /**
      Handle a mouse button event.

      @param b        The mouse button
      @param pressed  Whether the button was pressed (true) or released (false)
      @param x        The x location
      @param y        The y location
    */
    void handleMouseButtonEvent(MouseButton b, bool pressed, int x, int y);

    /**
      Handle a joystick button event.

      @param stick   The joystick number
      @param button  The joystick button
      @param pressed Pressed (true) or released (false)
    */
    void handleJoyBtnEvent(int stick, int button, bool pressed);

    /**
      Handle a joystick axis event.

      @param stick  The joystick number
      @param axis   The joystick axis
      @param value  Value associated with given axis
    */
    void handleJoyAxisEvent(int stick, int axis, int value, int button);

    /**
      Handle a joystick hat event.

      @param stick  The joystick number
      @param hat    The joystick hat
      @param value  Value associated with given hat
    */
    void handleJoyHatEvent(int stick, int hat, JoyHat value, int button);

    /**
      Draw the stack of menus (full indicates to redraw all items).

      @return  Answers whether any drawing actually occurred.
    */
    bool draw(bool full = false);

    /**
      Answers whether a full redraw is required.
    */
    bool needsRedraw() const;

    /**
      Answers whether the base dialog is currently active
      (ie, there are no overlaid dialogs other than the bottom one)
    */
    bool baseDialogIsActive() const;

    /**
      Reset dialog stack to the main configuration menu.
    */
    void reStack();

    /**
      Inform the container that it should resize according to the current
      screen dimensions.  We make this virtual, since the container may or
      may not choose to do a resize, and even if it does, *how* it does it
      is determined by the specific container.
    */
    virtual void requestResize() { }

    /**
      Return (and possibly create) the bottom-most dialog of this container.
    */
    virtual Dialog* baseDialog() = 0;

  private:
    void reset();

    /**
      Add a dialog box to the stack.
    */
    int addDialog(Dialog* d);

    /**
      Remove the topmost dialog box from the stack.
    */
    void removeDialog();

  protected:
    OSystem& myOSystem;
    Common::FixedStack<Dialog*> myDialogStack;

  private:
    enum {
      kDoubleClickDelay   = 500,
      kRepeatInitialDelay = 400,
      kRepeatSustainDelay = 50
    };

    // Indicates the most current time (in milliseconds) as set by updateTime()
    uInt64 myTime;

    // For continuous 'mouse down' events
    struct {
      int x;
      int y;
      MouseButton b;
    } myCurrentMouseDown;
    uInt64 myClickRepeatTime;

    // For continuous 'joy button down' events
    struct {
      int stick;
      int button;
    } myCurrentButtonDown;
    uInt64 myButtonRepeatTime;

    // For continuous 'joy axis down' events
    struct {
      int stick;
      int axis;
      int value;
    } myCurrentAxisDown;
    uInt64 myAxisRepeatTime;

    // For continuous 'joy hat' events
    struct {
      int stick;
      int hat;
      JoyHat value;
    } myCurrentHatDown;
    uInt64 myHatRepeatTime;

    // Position and time of last mouse click (used to detect double clicks)
    struct {
      int x, y;    // Position of mouse when the click occurred
      int count;   // How often was it already pressed?
      uInt64 time; // Time
    } myLastClick;

  private:
    // Following constructors and assignment operators not supported
    DialogContainer() = delete;
    DialogContainer(const DialogContainer&) = delete;
    DialogContainer(DialogContainer&&) = delete;
    DialogContainer& operator=(const DialogContainer&) = delete;
    DialogContainer& operator=(DialogContainer&&) = delete;
};

#endif
