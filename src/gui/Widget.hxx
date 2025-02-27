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

#ifndef WIDGET_HXX
#define WIDGET_HXX

class Dialog;

namespace GUI {
  class Font;
}

#include <cassert>

#include "bspf.hxx"
#include "Event.hxx"
#include "GuiObject.hxx"
#include "Font.hxx"

/**
  This is the base class for all widgets.

  @author  Stephen Anthony
*/
class Widget : public GuiObject
{
  friend class Dialog;

  public:
    enum : uInt32 {
      FLAG_ENABLED       = 1 << 0,
      FLAG_INVISIBLE     = 1 << 1,
      FLAG_HILITED       = 1 << 2,
      FLAG_BORDER        = 1 << 3,
      FLAG_CLEARBG       = 1 << 4,
      FLAG_TRACK_MOUSE   = 1 << 5,
      FLAG_RETAIN_FOCUS  = 1 << 6,
      FLAG_WANTS_TAB     = 1 << 7,
      FLAG_WANTS_RAWDATA = 1 << 8
    };

  public:
    Widget(GuiObject* boss, const GUI::Font& font, int x, int y, int w, int h);
    virtual ~Widget();

    virtual int getAbsX() const override { return _x + _boss->getChildX(); }
    virtual int getAbsY() const override { return _y + _boss->getChildY(); }
    virtual int getLeft() const { return _x; }
    virtual int getTop() const { return _y; }
    virtual int getRight() const { return _x + getWidth(); }
    virtual int getBottom() const { return _y + getHeight(); }

    virtual bool handleText(char text)                        { return false; }
    virtual bool handleKeyDown(StellaKey key, StellaMod mod)  { return false; }
    virtual bool handleKeyUp(StellaKey key, StellaMod mod)    { return false; }
    virtual void handleMouseDown(int x, int y, MouseButton b, int clickCount) { }
    virtual void handleMouseUp(int x, int y, MouseButton b, int clickCount) { }
    virtual void handleMouseEntered() { }
    virtual void handleMouseLeft() { }
    virtual void handleMouseMoved(int x, int y) { }
    virtual void handleMouseWheel(int x, int y, int direction) { }
    virtual bool handleMouseClicks(int x, int y, MouseButton b) { return false; }
    virtual void handleJoyDown(int stick, int button) { }
    virtual void handleJoyUp(int stick, int button) { }
    virtual void handleJoyAxis(int stick, int axis, int value, int button = JOY_CTRL_NONE) { }
    virtual bool handleJoyHat(int stick, int hat, JoyHat value, int button = JOY_CTRL_NONE) { return false; }
    virtual bool handleEvent(Event::Type event) { return false; }

    void setDirty() override;
    void draw() override;
    void receivedFocus();
    void lostFocus();
    void addFocusWidget(Widget* w) override { _focusList.push_back(w); }
    void addToFocusList(WidgetArray& list) override {
      Vec::append(_focusList, list);
    }

    /** Set/clear FLAG_ENABLED */
    void setEnabled(bool e);

    void setFlags(uInt32 flags)    { _flags |= flags;  setDirty(); }
    void clearFlags(uInt32 flags)  { _flags &= ~flags; setDirty(); }
    uInt32 getFlags() const        { return _flags; }

    bool isEnabled() const          { return _flags & FLAG_ENABLED;         }
    bool isVisible() const override { return !(_flags & FLAG_INVISIBLE);    }
    virtual bool wantsFocus() const { return _flags & FLAG_RETAIN_FOCUS;    }
    bool wantsTab() const           { return _flags & FLAG_WANTS_TAB;       }
    bool wantsRaw() const           { return _flags & FLAG_WANTS_RAWDATA;   }

    void setID(uInt32 id) { _id = id;   }
    uInt32 getID() const  { return _id; }

    virtual const GUI::Font& font() const { return _font; }

    void setTextColor(ColorId color)   { _textcolor = color;   setDirty(); }
    void setTextColorHi(ColorId color) { _textcolorhi = color; setDirty(); }
    void setBGColor(ColorId color)     { _bgcolor = color;     setDirty(); }
    void setBGColorHi(ColorId color)   { _bgcolorhi = color;   setDirty(); }
    void setShadowColor(ColorId color) { _shadowcolor = color; setDirty(); }

    virtual void loadConfig() { }

  protected:
    virtual void drawWidget(bool hilite) { }

    virtual void receivedFocusWidget() { }
    virtual void lostFocusWidget() { }

    virtual Widget* findWidget(int x, int y) { return this; }

    void releaseFocus() override { assert(_boss); _boss->releaseFocus(); }

    // By default, delegate unhandled commands to the boss
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override
         { assert(_boss); _boss->handleCommand(sender, cmd, data, id); }

  protected:
    GuiObject* _boss;
    const GUI::Font& _font;
    Widget*    _next;
    uInt32     _id;
    uInt32     _flags;
    bool       _hasFocus;
    int        _fontWidth;
    int        _fontHeight;
    ColorId    _bgcolor;
    ColorId    _bgcolorhi;
    ColorId    _bgcolorlo;
    ColorId    _textcolor;
    ColorId    _textcolorhi;
    ColorId    _textcolorlo;
    ColorId    _shadowcolor;

  public:
    static Widget* findWidgetInChain(Widget* start, int x, int y);

    /** Determine if 'find' is in the chain pointed to by 'start' */
    static bool isWidgetInChain(Widget* start, Widget* find);

    /** Determine if 'find' is in the widget array */
    static bool isWidgetInChain(WidgetArray& list, Widget* find);

    /** Select either previous, current, or next widget in chain to have
        focus, and deselects all others */
    static Widget* setFocusForChain(GuiObject* boss, WidgetArray& arr,
                                    Widget* w, int direction,
                                    bool emitFocusEvents = true);

    /** Sets all widgets in this chain to be dirty (must be redrawn) */
    static void setDirtyInChain(Widget* start);

  private:
    // Following constructors and assignment operators not supported
    Widget() = delete;
    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;
};


/* StaticTextWidget */
class StaticTextWidget : public Widget
{
  public:
    StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h,
                     const string& text = "", TextAlign align = TextAlign::Left,
                     ColorId shadowColor = kNone);
    StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y,
                     const string& text = "", TextAlign align = TextAlign::Left,
                     ColorId shadowColor = kNone);
    void setValue(int value);
    void setLabel(const string& label);
    void setAlign(TextAlign align) { _align = align; setDirty(); }
    const string& getLabel() const { return _label; }
    bool isEditable() const { return _editable; }

  protected:
    void drawWidget(bool hilite) override;

  protected:
    string    _label;
    bool      _editable;
    TextAlign _align;

  private:
    // Following constructors and assignment operators not supported
    StaticTextWidget() = delete;
    StaticTextWidget(const StaticTextWidget&) = delete;
    StaticTextWidget(StaticTextWidget&&) = delete;
    StaticTextWidget& operator=(const StaticTextWidget&) = delete;
    StaticTextWidget& operator=(StaticTextWidget&&) = delete;
};


/* ButtonWidget */
class ButtonWidget : public StaticTextWidget, public CommandSender
{
  public:
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h,
                 const string& label, int cmd = 0, bool repeat = false);
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int dw,
                 const string& label, int cmd = 0, bool repeat = false);
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y,
                 const string& label, int cmd = 0, bool repeat = false);
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int dw, int dh,
                 uInt32* bitmap, int bmw, int bmh,
                 int cmd = 0, bool repeat = false);

    void setCmd(int cmd)  { _cmd = cmd; }
    int getCmd() const    { return _cmd; }
    /* Sets/changes the button's bitmap **/
    void setBitmap(uInt32* bitmap, int bmw, int bmh);

  protected:
    bool handleMouseClicks(int x, int y, MouseButton b) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseEntered() override;
    void handleMouseLeft() override;
    bool handleEvent(Event::Type event) override;

    void drawWidget(bool hilite) override;

  protected:
    int     _cmd;
    bool    _repeat; // button repeats
    bool    _useBitmap;
    uInt32* _bitmap;
    int     _bmw, _bmh;

  private:
    // Following constructors and assignment operators not supported
    ButtonWidget() = delete;
    ButtonWidget(const ButtonWidget&) = delete;
    ButtonWidget(ButtonWidget&&) = delete;
    ButtonWidget& operator=(const ButtonWidget&) = delete;
    ButtonWidget& operator=(ButtonWidget&&) = delete;
};


/* CheckboxWidget */
class CheckboxWidget : public ButtonWidget
{
  public:
    enum { kCheckActionCmd  = 'CBAC' };
    enum class FillType { Normal, Inactive, Circle };

  public:
    CheckboxWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                   const string& label, int cmd = 0);

    void setEditable(bool editable);
    void setFill(FillType type);

    void setState(bool state, bool changed = false);
    void toggleState()     { setState(!_state); }
    bool getState() const  { return _state;     }

    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseEntered() override;
    void handleMouseLeft() override;

    static int boxSize() { return 14; }  // box is square

  protected:
    void drawWidget(bool hilite) override;

  protected:
    bool _state;
    bool _holdFocus;
    bool _drawBox;
    bool _changed;

    uInt32* _img;
    ColorId _fillColor;
    int _boxY;
    int _textY;

  private:
    // Following constructors and assignment operators not supported
    CheckboxWidget() = delete;
    CheckboxWidget(const CheckboxWidget&) = delete;
    CheckboxWidget(CheckboxWidget&&) = delete;
    CheckboxWidget& operator=(const CheckboxWidget&) = delete;
    CheckboxWidget& operator=(CheckboxWidget&&) = delete;
};

/* SliderWidget */
class SliderWidget : public ButtonWidget
{
  public:
    SliderWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h,
                 const string& label = "", int labelWidth = 0, int cmd = 0,
                 int valueLabelWidth = 0, const string& valueUnit = "", int valueLabelGap = 4);
    SliderWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y,
                 const string& label = "", int labelWidth = 0, int cmd = 0,
                 int valueLabelWidth = 0, const string& valueUnit = "", int valueLabelGap = 4);

    void setValue(int value);
    int getValue() const { return _value; }

    void setMinValue(int value);
    int  getMinValue() const { return _valueMin; }
    void setMaxValue(int value);
    int  getMaxValue() const { return _valueMax; }
    void setStepValue(int value);
    int  getStepValue() const { return _stepValue; }
    void setValueLabel(const string& valueLabel);
    void setValueLabel(int value);
    const string& getValueLabel() const { return _valueLabel; }
    void setValueUnit(const string& valueUnit);

    void setTickmarkIntervals(int numIntervals);

  protected:
    void handleMouseMoved(int x, int y) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleEvent(Event::Type event) override;

    void drawWidget(bool hilite) override;

    int valueToPos(int value) const;
    int posToValue(int pos) const;

  protected:
    int    _value, _stepValue;
    int    _valueMin, _valueMax;
    bool   _isDragging;
    int    _labelWidth;
    string _valueLabel;
    string _valueUnit;
    int    _valueLabelGap;
    int    _valueLabelWidth;
    int    _numIntervals;

  private:
    // Following constructors and assignment operators not supported
    SliderWidget() = delete;
    SliderWidget(const SliderWidget&) = delete;
    SliderWidget(SliderWidget&&) = delete;
    SliderWidget& operator=(const SliderWidget&) = delete;
    SliderWidget& operator=(SliderWidget&&) = delete;
};

#endif
