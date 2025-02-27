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

#ifndef KEYMAP_HXX
#define KEYMAP_HXX

#include <unordered_map>
#include "Event.hxx"
#include "EventHandlerConstants.hxx"

/**
  This class handles keyboard mappings in Stella.

  @author  Thomas Jentzsch
*/
class KeyMap
{
  public:

    struct Mapping
    {
      EventMode mode;
      StellaKey key;
      StellaMod mod;

      Mapping() : mode(EventMode(0)), key(StellaKey(0)), mod(StellaMod(0)) { }
      Mapping(const Mapping& m) : mode(m.mode), key(m.key), mod(m.mod) { }
      explicit Mapping(EventMode c_mode, StellaKey c_key, StellaMod c_mod)
        : mode(c_mode), key(c_key), mod(c_mod) { }
      explicit Mapping(int c_mode, int c_key, int c_mod)
        : mode(EventMode(c_mode)), key(StellaKey(c_key)), mod(StellaMod(c_mod)) { }

      bool operator==(const Mapping& other) const
      {
        return (key == other.key
          && mode == other.mode
          && (((mod | other.mod) & KBDM_SHIFT) ? (mod & other.mod & KBDM_SHIFT) : true)
          && (((mod | other.mod) & KBDM_CTRL ) ? (mod & other.mod & KBDM_CTRL ) : true)
          && (((mod | other.mod) & KBDM_ALT  ) ? (mod & other.mod & KBDM_ALT  ) : true)
          && (((mod | other.mod) & KBDM_GUI  ) ? (mod & other.mod & KBDM_GUI  ) : true)
          );
      }
    };
    using MappingArray = std::vector<Mapping>;

    KeyMap();
    virtual ~KeyMap() = default;

    /** Add new mapping for given event */
    void add(const Event::Type event, const Mapping& mapping);
    void add(const Event::Type event, const int mode, const int key, const int mod);

    /** Erase mapping */
    void erase(const Mapping& mapping);
    void erase(const int mode, const int key, const int mod);

    /** Get event for mapping */
    Event::Type get(const Mapping& mapping) const;
    Event::Type get(const int mode, const int key, const int mod) const;

    /** Check if a mapping exists */
    bool check(const Mapping& mapping) const;
    bool check(const int mode, const int key, const int mod) const;

    /** Get mapping description */
    string getDesc(const Mapping& mapping) const;
    string getDesc(const int mode, const int key, const int mod) const;

    /** Get the mapping description(s) for given event and mode */
    string getEventMappingDesc(const Event::Type event, const int mode) const;

    MappingArray getEventMapping(const Event::Type event, const int mode) const;

    string saveMapping(const int mode) const;
    int loadMapping(string& list, const int mode);

    /** Erase all mappings for given mode */
    void eraseMode(const int mode);
    /** Erase given event's mapping for given mode */
    void eraseEvent(const Event::Type event, const int mode);
    /** clear all mappings for a modes */
    // void clear() { myMap.clear(); }
    size_t size() { return myMap.size(); }

    bool& enableMod() { return myModEnabled;  }

  private:
    //** Convert modifiers */
    Mapping convertMod(const Mapping& mapping) const;

    struct KeyHash {
      size_t operator()(const Mapping& m) const {
        return std::hash<uInt64>()((uInt64(m.mode))               // 3 bits
          + ((uInt64(m.key)) * 7)                                 // 8 bits
          + (((uInt64((m.mod & KBDM_SHIFT) != 0) << 0))           // 1 bit
           | ((uInt64((m.mod & KBDM_ALT  ) != 0) << 1))           // 1 bit
           | ((uInt64((m.mod & KBDM_GUI  ) != 0) << 2))           // 1 bit
           | ((uInt64((m.mod & KBDM_CTRL ) != 0) << 3))) * 2047); // 1 bit
      }
    };

    std::unordered_map<Mapping, Event::Type, KeyHash> myMap;

    // Indicates whether the key-combos tied to a modifier key are
    // being used or not (e.g. Ctrl by default is the fire button,
    // pressing it with a movement key could inadvertantly activate
    // a Ctrl combo when it isn't wanted)
    bool myModEnabled;
};

#endif
