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

#include <cmath>

#include "Event.hxx"
#include "Paddles.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Paddles::Paddles(Jack jack, const Event& event, const System& system,
                 bool swappaddle, bool swapaxis, bool swapdir)
  : Controller(jack, event, system, Controller::Type::Paddles),
    myMPaddleID(-1),
    myMPaddleIDX(-1),
    myMPaddleIDY(-1)
{
  // We must start with minimum resistance; see commit
  // 38b452e1a047a0dca38c5bcce7c271d40f76736e for more information
  setPin(AnalogPin::Five, MIN_RESISTANCE);
  setPin(AnalogPin::Nine, MIN_RESISTANCE);

  // The following logic reflects that mapping paddles to different
  // devices can be extremely complex
  // As well, while many paddle games have horizontal movement of
  // objects (which maps nicely to horizontal movement of the joystick
  // or mouse), others have vertical movement
  // This vertical handling is taken care of by swapping the axes
  // On the other hand, some games treat paddle resistance differently,
  // (ie, increasing resistance can move an object right instead of left)
  // This is taken care of by swapping the direction of movement
  // Arrgh, did I mention that paddles are complex ...

  // As much as possible, precompute which events we care about for
  // a given port; this will speed up processing in update()

  // Consider whether this is the left or right port
  if(myJack == Jack::Left)
  {
    if(!swappaddle)  // First paddle is 0, second is 1
    {
      // These aren't affected by changes in axis orientation
      myP0AxisValue  = Event::PaddleZeroAnalog;
      myP1AxisValue  = Event::PaddleOneAnalog;
      myP0FireEvent1 = Event::PaddleZeroFire;
      myP0FireEvent2 = Event::JoystickZeroFire;
      myP1FireEvent1 = Event::PaddleOneFire;
      myP1FireEvent2 = Event::JoystickZeroFire9;

      // Direction of movement is swapped
      // That is, moving in a certain direction on an axis can
      // result in either increasing or decreasing paddle movement
      if(!swapdir)
      {
        myP0DecEvent1 = Event::PaddleZeroDecrease;
        myP0DecEvent2 = Event::JoystickZeroRight;
        myP0IncEvent1 = Event::PaddleZeroIncrease;
        myP0IncEvent2 = Event::JoystickZeroLeft;
        myP1DecEvent1 = Event::PaddleOneDecrease;
        myP1DecEvent2 = Event::JoystickZeroDown;
        myP1IncEvent1 = Event::PaddleOneIncrease;
        myP1IncEvent2 = Event::JoystickZeroUp;
      }
      else
      {
        myP0DecEvent1 = Event::PaddleZeroIncrease;
        myP0DecEvent2 = Event::JoystickZeroLeft;
        myP0IncEvent1 = Event::PaddleZeroDecrease;
        myP0IncEvent2 = Event::JoystickZeroRight;
        myP1DecEvent1 = Event::PaddleOneIncrease;
        myP1DecEvent2 = Event::JoystickZeroUp;
        myP1IncEvent1 = Event::PaddleOneDecrease;
        myP1IncEvent2 = Event::JoystickZeroDown;
      }
    }
    else           // First paddle is 1, second is 0
    {
      // These aren't affected by changes in axis orientation
      myP0AxisValue  = Event::PaddleOneAnalog;
      myP1AxisValue  = Event::PaddleZeroAnalog;
      myP0FireEvent1 = Event::PaddleOneFire;
      myP0FireEvent2 = Event::JoystickZeroFire9;
      myP1FireEvent1 = Event::PaddleZeroFire;
      myP1FireEvent2 = Event::JoystickZeroFire;

      // Direction of movement is swapped
      // That is, moving in a certain direction on an axis can
      // result in either increasing or decreasing paddle movement
      if(!swapdir)
      {
        myP0DecEvent1 = Event::PaddleOneDecrease;
        myP0DecEvent2 = Event::JoystickZeroDown;
        myP0IncEvent1 = Event::PaddleOneIncrease;
        myP0IncEvent2 = Event::JoystickZeroUp;
        myP1DecEvent1 = Event::PaddleZeroDecrease;
        myP1DecEvent2 = Event::JoystickZeroRight;
        myP1IncEvent1 = Event::PaddleZeroIncrease;
        myP1IncEvent2 = Event::JoystickZeroLeft;
      }
      else
      {
        myP0DecEvent1 = Event::PaddleOneIncrease;
        myP0DecEvent2 = Event::JoystickZeroUp;
        myP0IncEvent1 = Event::PaddleOneDecrease;
        myP0IncEvent2 = Event::JoystickZeroDown;
        myP1DecEvent1 = Event::PaddleZeroIncrease;
        myP1DecEvent2 = Event::JoystickZeroLeft;
        myP1IncEvent1 = Event::PaddleZeroDecrease;
        myP1IncEvent2 = Event::JoystickZeroRight;
      }
    }
  }
  else    // Jack is right port
  {
    if(!swappaddle)  // First paddle is 2, second is 3
    {
      // These aren't affected by changes in axis orientation
      myP0AxisValue  = Event::PaddleTwoAnalog;
      myP1AxisValue  = Event::PaddleThreeAnalog;
      myP0FireEvent1 = Event::PaddleTwoFire;
      myP0FireEvent2 = Event::JoystickOneFire;
      myP1FireEvent1 = Event::PaddleThreeFire;
      myP1FireEvent2 = Event::JoystickOneFire9;

      // Direction of movement is swapped
      // That is, moving in a certain direction on an axis can
      // result in either increasing or decreasing paddle movement
      if(!swapdir)
      {
        myP0DecEvent1 = Event::PaddleTwoDecrease;
        myP0DecEvent2 = Event::JoystickOneRight;
        myP0IncEvent1 = Event::PaddleTwoIncrease;
        myP0IncEvent2 = Event::JoystickOneLeft;
        myP1DecEvent1 = Event::PaddleThreeDecrease;
        myP1DecEvent2 = Event::JoystickOneDown;
        myP1IncEvent1 = Event::PaddleThreeIncrease;
        myP1IncEvent2 = Event::JoystickOneUp;
      }
      else
      {
        myP0DecEvent1 = Event::PaddleTwoIncrease;
        myP0DecEvent2 = Event::JoystickOneLeft;
        myP0IncEvent1 = Event::PaddleTwoDecrease;
        myP0IncEvent2 = Event::JoystickOneRight;
        myP1DecEvent1 = Event::PaddleThreeIncrease;
        myP1DecEvent2 = Event::JoystickOneUp;
        myP1IncEvent1 = Event::PaddleThreeDecrease;
        myP1IncEvent2 = Event::JoystickOneDown;
      }
    }
    else           // First paddle is 3, second is 2
    {
      // These aren't affected by changes in axis orientation
      myP0AxisValue  = Event::PaddleThreeAnalog;
      myP1AxisValue  = Event::PaddleTwoAnalog;
      myP0FireEvent1 = Event::PaddleThreeFire;
      myP0FireEvent2 = Event::JoystickOneFire9;
      myP1FireEvent1 = Event::PaddleTwoFire;
      myP1FireEvent2 = Event::JoystickOneFire;

      // Direction of movement is swapped
      // That is, moving in a certain direction on an axis can
      // result in either increasing or decreasing paddle movement
      if(!swapdir)
      {
        myP0DecEvent1 = Event::PaddleThreeDecrease;
        myP0DecEvent2 = Event::JoystickOneDown;
        myP0IncEvent1 = Event::PaddleThreeIncrease;
        myP0IncEvent2 = Event::JoystickOneUp;
        myP1DecEvent1 = Event::PaddleTwoDecrease;
        myP1DecEvent2 = Event::JoystickOneRight;
        myP1IncEvent1 = Event::PaddleTwoIncrease;
        myP1IncEvent2 = Event::JoystickOneLeft;
      }
      else
      {
        myP0DecEvent1 = Event::PaddleThreeIncrease;
        myP0DecEvent2 = Event::JoystickOneUp;
        myP0IncEvent1 = Event::PaddleThreeDecrease;
        myP0IncEvent2 = Event::JoystickOneDown;
        myP1DecEvent1 = Event::PaddleTwoIncrease;
        myP1DecEvent2 = Event::JoystickOneLeft;
        myP1IncEvent1 = Event::PaddleTwoDecrease;
        myP1IncEvent2 = Event::JoystickOneRight;
      }
    }
  }

  // The following are independent of whether or not the port
  // is left or right
  MOUSE_SENSITIVITY = swapdir ? -abs(MOUSE_SENSITIVITY) :
                                 abs(MOUSE_SENSITIVITY);
  if(!swapaxis)
  {
    myAxisMouseMotion = Event::MouseAxisXValue;
    myAxisDigitalZero = 0;
    myAxisDigitalOne  = 1;
  }
  else
  {
    myAxisMouseMotion = Event::MouseAxisYValue;
    myAxisDigitalZero = 1;
    myAxisDigitalOne  = 0;
  }

  // Digital pins 1, 2 and 6 are not connected
  setPin(DigitalPin::One, true);
  setPin(DigitalPin::Two, true);
  setPin(DigitalPin::Six, true);

  // Digital emulation of analog paddle movement
  myKeyRepeat0 = myKeyRepeat1 = false;
  myPaddleRepeat0 = myPaddleRepeat1 = myLastAxisX = myLastAxisY = 0;

  myCharge[0] = myCharge[1] = TRIGRANGE / 2;
  myLastCharge[0] = myLastCharge[1] = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::update()
{
  setPin(DigitalPin::Three, true);
  setPin(DigitalPin::Four, true);

  // Digital events (from keyboard or joystick hats & buttons)
  setPin(DigitalPin::Three,
      myEvent.get(myP1FireEvent1) == 0 && myEvent.get(myP1FireEvent2) == 0);
  setPin(DigitalPin::Four,
      myEvent.get(myP0FireEvent1) == 0 && myEvent.get(myP0FireEvent2) == 0);

  // Paddle movement is a very difficult thing to accurately emulate,
  // since it originally came from an analog device that had very
  // peculiar behaviour
  // Compounding the problem is the fact that we'd like to emulate
  // movement with 'digital' data (like from a keyboard or a digital
  // joystick axis), but also from a mouse (relative values)
  // and Stelladaptor-like devices (absolute analog values clamped to
  // a certain range)
  // And to top it all off, we don't want one devices input to conflict
  // with the others ...

  // Analog axis events from Stelladaptor-like devices
  // These devices generate data in the range -32768 to 32767,
  // so we have to scale appropriately
  // Since these events are generated and stored indefinitely,
  // we only process the first one we see (when it differs from
  // previous values by a pre-defined amount)
  // Otherwise, it would always override input from digital and mouse
  bool sa_changed = false;
  int sa_xaxis = myEvent.get(myP0AxisValue);
  int sa_yaxis = myEvent.get(myP1AxisValue);
  int new_val;

  const double bFac[MAX_DEJITTER - MIN_DEJITTER + 1] = {
    // higher values mean more dejitter strength
    0, // off
    0.50, 0.59, 0.67, 0.74, 0.80,
    0.85, 0.89, 0.92, 0.94, 0.95
  };
  const double dFac[MAX_DEJITTER - MIN_DEJITTER + 1] = {
    // lower values mean more dejitter strength
    1, // off
    1.0 /  181, 1.0 /  256, 1.0 /  362, 1.0 /  512, 1.0 /  724,
    1.0 / 1024, 1.0 / 1448, 1.0 / 2048, 1.0 / 2896, 1.0 / 4096
  };
  const double baseFactor = bFac[DEJITTER_BASE];
  const double diffFactor = dFac[DEJITTER_DIFF];

  if(abs(myLastAxisX - sa_xaxis) > 10)
  {
    // dejitter, suppress small changes only
    double dejitter = std::pow(baseFactor, abs(sa_xaxis - myLastAxisX) * diffFactor);
    new_val = sa_xaxis * (1 - dejitter) + myLastAxisX * dejitter;

    // only use new dejittered value for larger differences
    if (abs(new_val - sa_xaxis) > 10)
      sa_xaxis = new_val;

    setPin(AnalogPin::Nine, Int32(MAX_RESISTANCE * ((32767 - Int16(sa_xaxis)) / 65536.0)));
    sa_changed = true;
  }
  if(abs(myLastAxisY - sa_yaxis) > 10)
  {
    // dejitter, suppress small changes only
    double dejitter = std::pow(baseFactor, abs(sa_yaxis - myLastAxisY) * diffFactor);
    new_val = sa_yaxis * (1 - dejitter) + myLastAxisY * dejitter;

    // only use new dejittered value for larger differences
    if (abs(new_val - sa_yaxis) > 10)
      sa_yaxis = new_val;

    setPin(AnalogPin::Five, Int32(MAX_RESISTANCE * ((32767 - Int16(sa_yaxis)) / 65536.0)));
    sa_changed = true;
  }
  myLastAxisX = sa_xaxis;
  myLastAxisY = sa_yaxis;
  if(sa_changed)
    return;

  // Mouse motion events give relative movement
  // That is, they're only relevant if they're non-zero
  if(myMPaddleID > -1)
  {
    // We're in auto mode, where a single axis is used for one paddle only
    myCharge[myMPaddleID] = BSPF::clamp(myCharge[myMPaddleID] -
        (myEvent.get(myAxisMouseMotion) * MOUSE_SENSITIVITY),
        TRIGMIN, TRIGRANGE);
    if(myEvent.get(Event::MouseButtonLeftValue) ||
       myEvent.get(Event::MouseButtonRightValue))
      setPin(ourButtonPin[myMPaddleID], false);
  }
  else
  {
    // Test for 'untied' mouse axis mode, where each axis is potentially
    // mapped to a separate paddle
    if(myMPaddleIDX > -1)
    {
      myCharge[myMPaddleIDX] = BSPF::clamp(myCharge[myMPaddleIDX] -
          (myEvent.get(Event::MouseAxisXValue) * MOUSE_SENSITIVITY),
          TRIGMIN, TRIGRANGE);
      if(myEvent.get(Event::MouseButtonLeftValue))
        setPin(ourButtonPin[myMPaddleIDX], false);
    }
    if(myMPaddleIDY > -1)
    {
      myCharge[myMPaddleIDY] = BSPF::clamp(myCharge[myMPaddleIDY] -
          (myEvent.get(Event::MouseAxisYValue) * MOUSE_SENSITIVITY),
          TRIGMIN, TRIGRANGE);
      if(myEvent.get(Event::MouseButtonRightValue))
        setPin(ourButtonPin[myMPaddleIDY], false);
    }
  }

  // Finally, consider digital input, where movement happens
  // until a digital event is released
  if(myKeyRepeat0)
  {
    myPaddleRepeat0++;
    if(myPaddleRepeat0 > DIGITAL_SENSITIVITY)
      myPaddleRepeat0 = DIGITAL_DISTANCE;
  }
  if(myKeyRepeat1)
  {
    myPaddleRepeat1++;
    if(myPaddleRepeat1 > DIGITAL_SENSITIVITY)
      myPaddleRepeat1 = DIGITAL_DISTANCE;
  }

  myKeyRepeat0 = false;
  myKeyRepeat1 = false;

  if(myEvent.get(myP0DecEvent1) || myEvent.get(myP0DecEvent2))
  {
    myKeyRepeat0 = true;
    if(myCharge[myAxisDigitalZero] > myPaddleRepeat0)
      myCharge[myAxisDigitalZero] -= myPaddleRepeat0;
  }
  if(myEvent.get(myP0IncEvent1) || myEvent.get(myP0IncEvent2))
  {
    myKeyRepeat0 = true;
    if((myCharge[myAxisDigitalZero] + myPaddleRepeat0) < TRIGRANGE)
      myCharge[myAxisDigitalZero] += myPaddleRepeat0;
  }
  if(myEvent.get(myP1DecEvent1) || myEvent.get(myP1DecEvent2))
  {
    myKeyRepeat1 = true;
    if(myCharge[myAxisDigitalOne] > myPaddleRepeat1)
      myCharge[myAxisDigitalOne] -= myPaddleRepeat1;
  }
  if(myEvent.get(myP1IncEvent1) || myEvent.get(myP1IncEvent2))
  {
    myKeyRepeat1 = true;
    if((myCharge[myAxisDigitalOne] + myPaddleRepeat1) < TRIGRANGE)
      myCharge[myAxisDigitalOne] += myPaddleRepeat1;
  }

  // Only change state if the charge has actually changed
  if(myCharge[1] != myLastCharge[1])
    setPin(AnalogPin::Five, Int32(MAX_RESISTANCE * (myCharge[1] / double(TRIGMAX))));
  if(myCharge[0] != myLastCharge[0])
    setPin(AnalogPin::Nine, Int32(MAX_RESISTANCE * (myCharge[0] / double(TRIGMAX))));

  myLastCharge[1] = myCharge[1];
  myLastCharge[0] = myCharge[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Paddles::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // In 'automatic' mode, both axes on the mouse map to a single paddle,
  // and the paddle axis and direction settings are taken into account
  // This overrides any other mode
  if(xtype == Controller::Type::Paddles && ytype == Controller::Type::Paddles && xid == yid)
  {
    myMPaddleID = ((myJack == Jack::Left && (xid == 0 || xid == 1)) ||
                   (myJack == Jack::Right && (xid == 2 || xid == 3))
                  ) ? xid & 0x01 : -1;
    myMPaddleIDX = myMPaddleIDY = -1;
  }
  else
  {
    // The following is somewhat complex, but we need to pre-process as much
    // as possible, so that ::update() can run quickly
    myMPaddleID = -1;
    if(myJack == Jack::Left && xtype == Controller::Type::Paddles)
    {
      myMPaddleIDX = (xid == 0 || xid == 1) ? xid & 0x01 : -1;
      myMPaddleIDY = (yid == 0 || yid == 1) ? yid & 0x01 : -1;
    }
    else if(myJack == Jack::Right && ytype == Controller::Type::Paddles)
    {
      myMPaddleIDX = (xid == 2 || xid == 3) ? xid & 0x01 : -1;
      myMPaddleIDY = (yid == 2 || yid == 3) ? yid & 0x01 : -1;
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDejitterBase(int strength)
{
  DEJITTER_BASE = BSPF::clamp(strength, MIN_DEJITTER, MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDejitterDiff(int strength)
{
  DEJITTER_DIFF = BSPF::clamp(strength, MIN_DEJITTER, MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDigitalSensitivity(int sensitivity)
{
  DIGITAL_SENSITIVITY = BSPF::clamp(sensitivity, 1, MAX_DIGITAL_SENSE);
  DIGITAL_DISTANCE = 20 + (DIGITAL_SENSITIVITY << 3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setMouseSensitivity(int sensitivity)
{
  MOUSE_SENSITIVITY = BSPF::clamp(sensitivity, 1, MAX_MOUSE_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setPaddleRange(int range)
{
  range = BSPF::clamp(range, 1, 100);
  TRIGRANGE = int(TRIGMAX * (range / 100.0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Paddles::TRIGRANGE = Paddles::TRIGMAX;
int Paddles::DIGITAL_SENSITIVITY = -1;
int Paddles::DIGITAL_DISTANCE = -1;
int Paddles::MOUSE_SENSITIVITY = -1;
int Paddles::DEJITTER_BASE = 0;
int Paddles::DEJITTER_DIFF = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Controller::DigitalPin Paddles::ourButtonPin[2] = {
  DigitalPin::Four, DigitalPin::Three
};
