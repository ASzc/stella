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

#ifndef PHYSICAL_JOYSTICK_HANDLER_HXX
#define PHYSICAL_JOYSTICK_HANDLER_HXX

#include <map>

class OSystem;
class EventHandler;
class Event;

#include "bspf.hxx"
#include "EventHandlerConstants.hxx"
#include "PhysicalJoystick.hxx"
#include "Variant.hxx"

using PhysicalJoystickPtr = shared_ptr<PhysicalJoystick>;

/**
  This class handles all physical joystick-related operations in Stella.

  It is responsible for adding/accessing/removing PhysicalJoystick objects,
  and getting/setting events associated with joystick actions (button presses,
  axis/hat actions, etc).

  Essentially, this class is an extension of the EventHandler class, but
  handling only joystick-specific functionality.

  @author  Stephen Anthony, Thomas Jentzsch
*/
class PhysicalJoystickHandler
{
  private:
    struct StickInfo
    {
      StickInfo(const string& map = EmptyString, PhysicalJoystickPtr stick = nullptr)
        : mapping(map), joy(stick) {}

      string mapping;
      PhysicalJoystickPtr joy;

      friend ostream& operator<<(ostream& os, const StickInfo& si) {
        os << "  joy: " << si.joy << endl << "  map: " << si.mapping;
        return os;
      }
    };

  public:
    PhysicalJoystickHandler(OSystem& system, EventHandler& handler, Event& event);

    /** Return stick ID on success, -1 on failure. */
    int add(PhysicalJoystickPtr stick);
    bool remove(int id);
    bool remove(const string& name);
    void mapStelladaptors(const string& saport);
    void setDefaultMapping(Event::Type type, EventMode mode);

    /** define mappings for current controllers */
    void defineControllerMappings(const string& controllerName, Controller::Jack port);
    /** enable mappings for emulation mode */
    void enableEmulationMappings();

    void eraseMapping(Event::Type event, EventMode mode);
    void saveMapping();
    string getMappingDesc(Event::Type, EventMode mode) const;

    /** Bind a physical joystick event to a virtual event/action. */
    bool addJoyMapping(Event::Type event, EventMode mode, int stick,
                       int button, JoyAxis axis, int value);
    bool addJoyHatMapping(Event::Type event, EventMode mode, int stick,
                          int button, int hat, JoyHat hdir);

    /** Handle a physical joystick event. */
    void handleAxisEvent(int stick, int axis, int value);
    void handleBtnEvent(int stick, int button, bool pressed);
    void handleHatEvent(int stick, int hat, int value);

    Event::Type eventForAxis(EventMode mode, int stick, int axis, int value, int button) const {
      const PhysicalJoystickPtr j = joy(stick);
      return j->joyMap.get(mode, button, JoyAxis(axis), convertAxisValue(value));
    }
    Event::Type eventForButton(EventMode mode, int stick, int button) const {
      const PhysicalJoystickPtr j = joy(stick);
      return j->joyMap.get(mode, button);
    }
    Event::Type eventForHat(EventMode mode, int stick, int hat, JoyHat hatDir, int button) const {
      const PhysicalJoystickPtr j = joy(stick);
      return j->joyMap.get(mode, button, hat, hatDir);
    }

    /** Returns a list of pairs consisting of joystick name and associated ID. */
    VariantList database() const;

  private:
    using StickDatabase = std::map<string,StickInfo>;
    using StickList = std::map<int, PhysicalJoystickPtr>;

    OSystem& myOSystem;
    EventHandler& myHandler;
    Event& myEvent;

    // Contains all joysticks that Stella knows about, indexed by name
    StickDatabase myDatabase;

    // Contains only joysticks that are currently available, indexed by id
    StickList mySticks;

    // Get joystick corresponding to given id (or nullptr if it doesn't exist)
    // Make this inline so it's as fast as possible
    const PhysicalJoystickPtr joy(int id) const {
      const auto& i = mySticks.find(id);
      return i != mySticks.cend() ? i->second : nullptr;
    }

    // Set default mapping for given joystick when no mappings already exist
    void setStickDefaultMapping(int stick, Event::Type type, EventMode mode,
                                bool updateDefaults = false);

    friend ostream& operator<<(ostream& os, const PhysicalJoystickHandler& jh);

    JoyDir convertAxisValue(int value) const {
      return value == int(JoyDir::NONE) ? JoyDir::NONE : value > 0 ? JoyDir::POS : JoyDir::NEG;
    }

    // Structures used for action menu items
    struct EventMapping {
      Event::Type event;
      int button;
      JoyAxis axis = JoyAxis::NONE;
      JoyDir adir = JoyDir::NONE;
      int hat = JOY_CTRL_NONE;
      JoyHat hdir = JoyHat::CENTER;
    };
    using EventMappingArray = std::vector<EventMapping>;

    void setDefaultAction(const PhysicalJoystickPtr& j,
                          EventMapping map, Event::Type event = Event::NoType,
                          EventMode mode = kEmulationMode, bool updateDefaults = false);

    /** returns the event's controller mode */
    EventMode getEventMode(const Event::Type event, const EventMode mode) const;
    /** Checks event type. */
    bool isJoystickEvent(const Event::Type event) const;
    bool isPaddleEvent(const Event::Type event) const;
    bool isKeypadEvent(const Event::Type event) const;
    bool isCommonEvent(const Event::Type event) const;

    void enableCommonMappings();

    void enableMappings(const Event::EventSet events, EventMode mode);
    void enableMapping(const Event::Type event, EventMode mode);

  private:
    EventMode myLeftMode;
    EventMode myRightMode;

    // Controller menu and common emulation mappings
    static EventMappingArray DefaultMenuMapping;
    // Controller specific mappings
    static EventMappingArray DefaultLeftJoystickMapping;
    static EventMappingArray DefaultRightJoystickMapping;
    static EventMappingArray DefaultLeftPaddlesMapping;
    static EventMappingArray DefaultRightPaddlesMapping;
    static EventMappingArray DefaultLeftKeypadMapping;
    static EventMappingArray DefaultRightKeypadMapping;
};

#endif
