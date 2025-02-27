
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

#include "Settings.hxx"

#include "ControllerDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerDetector::detectType(const uInt8* image, uInt32 size,
                                      const string& controller, const Controller::Jack port,
                                      const Settings& settings)
{
  string type(controller);

  if(type == "AUTO" || settings.getBool("rominfo"))
  {
    string detectedType = autodetectPort(image, size, port, settings);

    if(type != "AUTO" && type != detectedType)
    {
      cerr << "Controller auto-detection not consistent: "
        << type << ", " << detectedType << endl;
    }
    type = detectedType;
  }

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerDetector::detectName(const uInt8* image, uInt32 size,
                                      const string& controller, const Controller::Jack port,
                                      const Settings& settings)
{
  return getControllerName(detectType(image, size, controller, port, settings));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerDetector::autodetectPort(const uInt8* image, uInt32 size,
                                          Controller::Jack port, const Settings& settings)
{
  // default type joystick
  string type = "JOYSTICK"; // TODO: remove magic strings

  if(isProbablySaveKey(image, size, port))
    type = "SAVEKEY";
  else if(usesJoystickButton(image, size, port))
  {
    if(isProbablyTrakBall(image, size))
      type = "TRAKBALL";
    else if(isProbablyAtariMouse(image, size))
      type = "ATARIMOUSE";
    else if(isProbablyAmigaMouse(image, size))
      type = "AMIGAMOUSE";
    else if(usesKeyboard(image, size, port))
      type = "KEYBOARD";
    else if(usesGenesisButton(image, size, port))
      type = "GENESIS";
  }
  else
  {
    if(usesPaddle(image, size, port, settings))
      type = "PADDLES";
  }
  // TODO: BOOSTERGRIP, DRIVING, MINDLINK, ATARIVOX, KIDVID
  // not detectable: PADDLES_IAXIS, PADDLES_IAXDR
  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::searchForBytes(const uInt8* image, uInt32 imagesize,
                                        const uInt8* signature, uInt32 sigsize)
{
  if (imagesize >= sigsize)
    for(uInt32 i = 0; i < imagesize - sigsize; ++i)
    {
      uInt32 matches = 0;
      for(uInt32 j = 0; j < sigsize; ++j)
      {
        if(image[i + j] == signature[j])
          ++matches;
        else
          break;
      }
      if(matches == sigsize)
      {
        return true;
      }
    }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::usesJoystickButton(const uInt8* image, uInt32 size, Controller::Jack port)
{
  if(port == Controller::Jack::Left)
  {
    // check for INPT4 access
    const int NUM_SIGS_0 = 23;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x0c, 0x10 }, // bit INPT4; bpl (joystick games only)
      { 0x24, 0x0c, 0x30 }, // bit INPT4; bmi (joystick games only)
      { 0xa5, 0x0c, 0x10 }, // lda INPT4; bpl (joystick games only)
      { 0xa5, 0x0c, 0x30 }, // lda INPT4; bmi (joystick games only)
      { 0xb5, 0x0c, 0x10 }, // lda INPT4,x; bpl (joystick games only)
      { 0xb5, 0x0c, 0x30 }, // lda INPT4,x; bmi (joystick games only)
      { 0x24, 0x3c, 0x10 }, // bit INPT4|$30; bpl (joystick games + Compumate)
      { 0x24, 0x3c, 0x30 }, // bit INPT4|$30; bmi (joystick, keyboard and mindlink games)
      { 0xa5, 0x3c, 0x10 }, // lda INPT4|$30; bpl (joystick and keyboard games)
      { 0xa5, 0x3c, 0x30 }, // lda INPT4|$30; bmi (joystick, keyboard and mindlink games)
      { 0xb5, 0x3c, 0x10 }, // lda INPT4|$30,x; bpl (joystick, keyboard and driving games)
      { 0xb5, 0x3c, 0x30 }, // lda INPT4|$30,x; bmi (joystick and keyboard games)
      { 0xb4, 0x0c, 0x30 }, // ldy INPT4|$30,x; bmi (joystick games only)
      { 0xa5, 0x3c, 0x2a }, // ldy INPT4|$30; rol (joystick games only)
      { 0xa6, 0x3c, 0x8e }, // ldx INPT4|$30; stx (joystick games only)
      { 0xa4, 0x3c, 0x8c }, // ldy INPT4; sty (joystick games only, Scramble)
      { 0xa5, 0x0c, 0x8d }, // lda INPT4; sta (joystick games only, Super Cobra Arcade)
      { 0xa4, 0x0c, 0x30 }, // ldy INPT4|; bmi (only Game of Concentration)
      { 0xa4, 0x3c, 0x30 }, // ldy INPT4|$30; bmi (only Game of Concentration)
      { 0xa5, 0x0c, 0x25 }, // lda INPT4; and (joystick games only)
      { 0xa6, 0x3c, 0x30 }, // ldx INPT4|$30; bmi (joystick games only)
      { 0xa6, 0x0c, 0x30 }, // ldx INPT4; bmi
      { 0xa5, 0x0c, 0x0a }, // lda INPT4; asl (joystick games only)
    };
    const int NUM_SIGS_1 = 9;
    const int SIG_SIZE_1 = 4;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb9, 0x0c, 0x00, 0x10 }, // lda INPT4,y; bpl (joystick games only)
      { 0xb9, 0x0c, 0x00, 0x30 }, // lda INPT4,y; bmi (joystick games only)
      { 0xb9, 0x3c, 0x00, 0x10 }, // lda INPT4,y; bpl (joystick games only)
      { 0xb9, 0x3c, 0x00, 0x30 }, // lda INPT4,y; bmi (joystick games only)
      { 0xa5, 0x0c, 0x0a, 0xb0 }, // lda INPT4; asl; bcs (joystick games only)
      { 0xb5, 0x0c, 0x29, 0x80 }, // lda INPT4,x; and #$80 (joystick games only)
      { 0xb5, 0x3c, 0x29, 0x80 }, // lda INPT4|$30,x; and #$80 (joystick games only)
      { 0xa5, 0x0c, 0x29, 0x80 }, // lda INPT4; and #$80 (joystick games only)
      { 0xa5, 0x3c, 0x29, 0x80 }, // lda INPT4|$30; and #$80 (joystick games only)
    };
    const int NUM_SIGS_2 = 9;
    const int SIG_SIZE_2 = 5;
    uInt8 signature_2[NUM_SIGS_2][SIG_SIZE_2] = {
      { 0xa5, 0x0c, 0x25, 0x0d, 0x10 }, // lda INPT4; and INPT5; bpl (joystick games only)
      { 0xa5, 0x0c, 0x25, 0x0d, 0x30 }, // lda INPT4; and INPT5; bmi (joystick games only)
      { 0xa5, 0x3c, 0x25, 0x3d, 0x10 }, // lda INPT4|$30; and INPT5|$30; bpl (joystick games only)
      { 0xa5, 0x3c, 0x25, 0x3d, 0x30 }, // lda INPT4|$30; and INPT5|$30; bmi (joystick games only)
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,y; and #$80; bne (Basic Programming)
      { 0xa9, 0x80, 0x24, 0x0c, 0xd0 }, // lda #$80; bit INPT4; bne (bBasic)
      { 0xa5, 0x0c, 0x29, 0x80, 0xd0 }, // lda INPT4; and #$80; bne (joystick games only)
      { 0xa5, 0x3c, 0x29, 0x80, 0xd0 }, // lda INPT4|$30; and #$80; bne (joystick games only)
      { 0xad, 0x0c, 0x00, 0x29, 0x80 }, // lda.w INPT4|$30; and #$80 (joystick games only)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image, size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image, size, signature_1[i], SIG_SIZE_1))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_2; ++i)
      if(searchForBytes(image, size, signature_2[i], SIG_SIZE_2))
        return true;
  }
  else if(port == Controller::Jack::Right)
  {
    // check for INPT5 and indexed INPT4 access
    const int NUM_SIGS_0 = 16;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x0d, 0x10 }, // bit INPT5; bpl (joystick games only)
      { 0x24, 0x0d, 0x30 }, // bit INPT5; bmi (joystick games only)
      { 0xa5, 0x0d, 0x10 }, // lda INPT5; bpl (joystick games only)
      { 0xa5, 0x0d, 0x30 }, // lda INPT5; bmi (joystick games only)
      { 0xb5, 0x0c, 0x10 }, // lda INPT4,x; bpl (joystick games only)
      { 0xb5, 0x0c, 0x30 }, // lda INPT4,x; bmi (joystick games only)
      { 0x24, 0x3d, 0x10 }, // bit INPT5|$30; bpl (joystick games, Compumate)
      { 0x24, 0x3d, 0x30 }, // bit INPT5|$30; bmi (joystick and keyboard games)
      { 0xa5, 0x3d, 0x10 }, // lda INPT5|$30; bpl (joystick games only)
      { 0xa5, 0x3d, 0x30 }, // lda INPT5|$30; bmi (joystick and keyboard games)
      { 0xb5, 0x3c, 0x10 }, // lda INPT4|$30,x; bpl (joystick, keyboard and driving games)
      { 0xb5, 0x3c, 0x30 }, // lda INPT4|$30,x; bmi (joystick and keyboard games)
      { 0xa4, 0x3d, 0x30 }, // ldy INPT5; bmi (only Game of Concentration)
      { 0xa5, 0x0d, 0x25 }, // lda INPT5; and (joystick games only)
      { 0xa6, 0x3d, 0x30 }, // ldx INPT5|$30; bmi (joystick games only)
      { 0xa6, 0x0d, 0x30 }, // ldx INPT5; bmi
    };
    const int NUM_SIGS_1 = 7;
    const int SIG_SIZE_1 = 4;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb9, 0x0c, 0x00, 0x10 }, // lda INPT4,y; bpl (joystick games only)
      { 0xb9, 0x0c, 0x00, 0x30 }, // lda INPT4,y; bmi (joystick games only)
      { 0xb9, 0x3c, 0x00, 0x10 }, // lda INPT4,y; bpl (joystick games only)
      { 0xb9, 0x3c, 0x00, 0x30 }, // lda INPT4,y; bmi (joystick games only)
      { 0xb5, 0x0c, 0x29, 0x80 }, // lda INPT4,x; and #$80 (joystick games only)
      { 0xb5, 0x3c, 0x29, 0x80 }, // lda INPT4|$30,x; and #$80 (joystick games only)
      { 0xa5, 0x3d, 0x29, 0x80 }, // lda INPT5|$30; and #$80 (joystick games only)
    };
    const int NUM_SIGS_2 = 3;
    const int SIG_SIZE_2 = 5;
    uInt8 signature_2[NUM_SIGS_2][SIG_SIZE_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,y; and #$80; bne (Basic Programming)
      { 0xa9, 0x80, 0x24, 0x0d, 0xd0 }, // lda #$80; bit INPT5; bne (bBasic)
      { 0xad, 0x0d, 0x00, 0x29, 0x80 }, // lda.w INPT5|$30; and #$80 (joystick games only)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image, size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image, size, signature_1[i], SIG_SIZE_1))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_2; ++i)
      if(searchForBytes(image, size, signature_2[i], SIG_SIZE_2))
        return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::usesKeyboard(const uInt8* image, uInt32 size, Controller::Jack port)
{
  if(port == Controller::Jack::Left)
  {
    // check for INPT0 *AND* INPT1 access
    const int NUM_SIGS_0_0 = 6;
    const int SIG_SIZE_0_0 = 3;
    uInt8 signature_0_0[NUM_SIGS_0_0][SIG_SIZE_0_0] = {
      { 0x24, 0x38, 0x30 }, // bit INPT0|$30; bmi
      { 0xa5, 0x38, 0x10 }, // lda INPT0|$30; bpl
      { 0xa4, 0x38, 0x30 }, // ldy INPT0|$30; bmi
      { 0xb5, 0x38, 0x30 }, // lda INPT0|$30,x; bmi
      { 0x24, 0x08, 0x30 }, // bit INPT0; bmi
      { 0xa6, 0x08, 0x30 }, // ldx INPT0; bmi
    };
    const int NUM_SIGS_0_2 = 1;
    const int SIG_SIZE_0_2 = 5;
    uInt8 signature_0_2[NUM_SIGS_0_2][SIG_SIZE_0_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0,x; and #80; bne
    };

    const int NUM_SIGS_1_0 = 7;
    const int SIG_SIZE_1_0 = 3;
    uInt8 signature_1_0[NUM_SIGS_1_0][SIG_SIZE_1_0] = {
      { 0x24, 0x39, 0x10 }, // bit INPT1|$30; bpl
      { 0x24, 0x39, 0x30 }, // bit INPT1|$30; bmi
      { 0xa5, 0x39, 0x10 }, // lda INPT1|$30; bpl
      { 0xa4, 0x39, 0x30 }, // ldy INPT1|$30; bmi
      { 0xb5, 0x38, 0x30 }, // lda INPT0|$30,x; bmi
      { 0x24, 0x09, 0x30 }, // bit INPT1; bmi
      { 0xa6, 0x09, 0x30 }, // ldx INPT1; bmi
    };
    const int NUM_SIGS_1_2 = 1;
    const int SIG_SIZE_1_2 = 5;
    uInt8 signature_1_2[NUM_SIGS_1_2][SIG_SIZE_1_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0,x; and #80; bne
    };

    bool found = false;

    for(uInt32 i = 0; i < NUM_SIGS_0_0; ++i)
      if(searchForBytes(image, size, signature_0_0[i], SIG_SIZE_0_0))
      {
        found = true;
        break;
      }
    if(!found)
      for(uInt32 i = 0; i < NUM_SIGS_0_2; ++i)
        if(searchForBytes(image, size, signature_0_2[i], SIG_SIZE_0_2))
        {
          found = true;
          break;
        }
    if(found)
    {
      for(uInt32 j = 0; j < NUM_SIGS_1_0; ++j)
        if(searchForBytes(image, size, signature_1_0[j], SIG_SIZE_1_0))
        {
          return true;
        }

      for(uInt32 j = 0; j < NUM_SIGS_1_2; ++j)
        if(searchForBytes(image, size, signature_1_2[j], SIG_SIZE_1_2))
        {
          return true;
        }
    }
  }
  else if(port == Controller::Jack::Right)
  {
    // check for INPT2 *AND* INPT3 access
    const int NUM_SIGS_0_0 = 5;
    const int SIG_SIZE_0_0 = 3;
    uInt8 signature_0_0[NUM_SIGS_0_0][SIG_SIZE_0_0] = {
      { 0x24, 0x3a, 0x30 }, // bit INPT2|$30; bmi
      { 0xa5, 0x3a, 0x10 }, // lda INPT2|$30; bpl
      { 0xa4, 0x3a, 0x30 }, // ldy INPT2|$30; bmi
      { 0x24, 0x0a, 0x30 }, // bit INPT2; bmi
      { 0xa6, 0x0a, 0x30 }, // ldx INPT2; bmi
    };
    const int NUM_SIGS_0_2 = 1;
    const int SIG_SIZE_0_2 = 5;
    uInt8 signature_0_2[NUM_SIGS_0_2][SIG_SIZE_0_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT2,x; and #80; bne
    };

    const int NUM_SIGS_1_0 = 5;
    const int SIG_SIZE_1_0 = 3;
    uInt8 signature_1_0[NUM_SIGS_1_0][SIG_SIZE_1_0] = {
      { 0x24, 0x3b, 0x30 }, // bit INPT3|$30; bmi
      { 0xa5, 0x3b, 0x10 }, // lda INPT3|$30; bpl
      { 0xa4, 0x3b, 0x30 }, // ldy INPT3|$30; bmi
      { 0x24, 0x0b, 0x30 }, // bit INPT3; bmi
      { 0xa6, 0x0b, 0x30 }, // ldx INPT3; bmi
    };
    const int NUM_SIGS_1_2 = 1;
    const int SIG_SIZE_1_2 = 5;
    uInt8 signature_1_2[NUM_SIGS_1_2][SIG_SIZE_1_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT2,x; and #80; bne
    };

    bool found = false;

    for(uInt32 i = 0; i < NUM_SIGS_0_0; ++i)
      if(searchForBytes(image, size, signature_0_0[i], SIG_SIZE_0_0))
      {
        found = true;
        break;
      }

    if(!found)
      for(uInt32 i = 0; i < NUM_SIGS_0_2; ++i)
        if(searchForBytes(image, size, signature_0_2[i], SIG_SIZE_0_2))
        {
          found = true;
          break;
        }

    if(found)
    {
      for(uInt32 j = 0; j < NUM_SIGS_1_0; ++j)
        if(searchForBytes(image, size, signature_1_0[j], SIG_SIZE_1_0))
        {
          return true;
        }

      for(uInt32 j = 0; j < NUM_SIGS_1_2; ++j)
        if(searchForBytes(image, size, signature_1_2[j], SIG_SIZE_1_2))
        {
          return true;
        }
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::usesGenesisButton(const uInt8* image, uInt32 size, Controller::Jack port)
{
  if(port == Controller::Jack::Left)
  {
    // check for INPT1 access
    const int NUM_SIGS_0 = 18;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x09, 0x10 }, // bit INPT1; bpl (Genesis only)
      { 0x24, 0x09, 0x30 }, // bit INPT1; bmi (paddle ROMS too)
      { 0xa5, 0x09, 0x10 }, // lda INPT1; bpl (paddle ROMS too)
      { 0xa5, 0x09, 0x30 }, // lda INPT1; bmi (paddle ROMS too)
      { 0xa4, 0x09, 0x30 }, // ldy INPT1; bmi (Genesis only)
      { 0xa6, 0x09, 0x30 }, // ldx INPT1; bmi (Genesis only)
      { 0x24, 0x39, 0x10 }, // bit INPT1|$30; bpl (keyboard and paddle ROMS too)
      { 0x24, 0x39, 0x30 }, // bit INPT1|$30; bmi (keyboard and paddle ROMS too)
      { 0xa5, 0x39, 0x10 }, // lda INPT1|$30; bpl (keyboard ROMS too)
      { 0xa5, 0x39, 0x30 }, // lda INPT1|$30; bmi (keyboard and paddle ROMS too)
      { 0xa4, 0x39, 0x30 }, // ldy INPT1|$30; bmi (keyboard ROMS too)
      { 0xa5, 0x39, 0x6a }, // lda INPT1|$30; ror (Genesis only)
      { 0xa6, 0x39, 0x8e }, // ldx INPT1|$30; stx (Genesis only)
      { 0xa4, 0x39, 0x8c }, // ldy INPT1|$30; sty (Genesis only, Scramble)
      { 0xa5, 0x09, 0x8d }, // lda INPT1; sta (Genesis only, Super Cobra Arcade)
      { 0xa5, 0x09, 0x29 }, // lda INPT1; and (Genesis only)
      { 0x25, 0x39, 0x30 }, // and INPT1|$30; bmi (Genesis only)
      { 0x25, 0x09, 0x10 }, // and INPT1; bpl (Genesis only)
    };
    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image, size, signature_0[i], SIG_SIZE_0))
        return true;
  }
  else if(port == Controller::Jack::Right)
  {
    // check for INPT3 access
    const int NUM_SIGS_0 = 10;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x0b, 0x10 }, // bit INPT3; bpl
      { 0x24, 0x0b, 0x30 }, // bit INPT3; bmi
      { 0xa5, 0x0b, 0x10 }, // lda INPT3; bpl
      { 0xa5, 0x0b, 0x30 }, // lda INPT3; bmi
      { 0x24, 0x3b, 0x10 }, // bit INPT3|$30; bpl
      { 0x24, 0x3b, 0x30 }, // bit INPT3|$30; bmi
      { 0xa5, 0x3b, 0x10 }, // lda INPT3|$30; bpl
      { 0xa5, 0x3b, 0x30 }, // lda INPT3|$30; bmi
      { 0xa6, 0x3b, 0x8e }, // ldx INPT3|$30; stx
      { 0x25, 0x0b, 0x10 }, // and INPT3; bpl (Genesis only)
    };
    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image, size, signature_0[i], SIG_SIZE_0))
        return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::usesPaddle(const uInt8* image, uInt32 size,
                                    Controller::Jack port, const Settings& settings)
{
  if(port == Controller::Jack::Left)
  {
    // check for INPT0 access
    const int NUM_SIGS_0 = 12;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      //{ 0x24, 0x08, 0x10 }, // bit INPT0; bpl (many joystick games too!)
      //{ 0x24, 0x08, 0x30 }, // bit INPT0; bmi (joystick games: Spike's Peak, Sweat, Turbo!)
      { 0xa5, 0x08, 0x10 }, // lda INPT0; bpl (no joystick games)
      { 0xa5, 0x08, 0x30 }, // lda INPT0; bmi (no joystick games)
      //{ 0xb5, 0x08, 0x10 }, // lda INPT0,x; bpl (Duck Attack (graphics)!, Toyshop Trouble (Easter Egg))
      { 0xb5, 0x08, 0x30 }, // lda INPT0,x; bmi (no joystick games)
      { 0x24, 0x38, 0x10 }, // bit INPT0|$30; bpl (no joystick games)
      { 0x24, 0x38, 0x30 }, // bit INPT0|$30; bmi (no joystick games)
      { 0xa5, 0x38, 0x10 }, // lda INPT0|$30; bpl (no joystick games)
      { 0xa5, 0x38, 0x30 }, // lda INPT0|$30; bmi (no joystick games)
      { 0xb5, 0x38, 0x10 }, // lda INPT0|$30,x; bpl (Circus Atari, old code!)
      { 0xb5, 0x38, 0x30 }, // lda INPT0|$30,x; bmi (no joystick games)
      { 0x68, 0x48, 0x10 }, // pla; pha; bpl (i.a. Bachelor Party)
      { 0xa5, 0x08, 0x4c }, // lda INPT0; jmp (only Backgammon)
      { 0xa4, 0x38, 0x30 }, // ldy INPT0; bmi (no joystick games)
    };
    const int NUM_SIGS_1 = 3;
    const int SIG_SIZE_1 = 4;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb9, 0x08, 0x00, 0x30 }, // lda INPT0,y; bmi (i.a. Encounter at L-5)
      { 0xb9, 0x38, 0x00, 0x30 }, // lda INPT0|$30,y; bmi (i.a. SW-Jedi Arena, Video Olympics)
      { 0x24, 0x08, 0x30, 0x02 }, // bit INPT0; bmi +2 (Picnic)
    };
    const int NUM_SIGS_2 = 4;
    const int SIG_SIZE_2 = 5;
    uInt8 signature_2[NUM_SIGS_2][SIG_SIZE_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,x; and #$80; bne (Basic Programming)
      { 0x24, 0x38, 0x85, 0x08, 0x10 }, // bit INPT0|$30; sta COLUPF, bpl (Fireball)
      { 0xb5, 0x38, 0x49, 0xff, 0x0a }, // lda INPT0|$30,x; eor #$ff; asl (Blackjack)
      { 0xb1, 0xf2, 0x30, 0x02, 0xe6 }  // lda ($f2),y; bmi...; inc (Warplock)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image, size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image, size, signature_1[i], SIG_SIZE_1))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_2; ++i)
      if(searchForBytes(image, size, signature_2[i], SIG_SIZE_2))
        return true;
  }
  else if(port == Controller::Jack::Right)
  {
    // check for INPT2 and indexed INPT0 access
    const int NUM_SIGS_0 = 18;
    const int SIG_SIZE_0 = 3;
    uInt8 signature_0[NUM_SIGS_0][SIG_SIZE_0] = {
      { 0x24, 0x0a, 0x10 }, // bit INPT2; bpl (no joystick games)
      { 0x24, 0x0a, 0x30 }, // bit INPT2; bmi (no joystick games)
      { 0xa5, 0x0a, 0x10 }, // lda INPT2; bpl (no joystick games)
      { 0xa5, 0x0a, 0x30 }, // lda INPT2; bmi
      { 0xb5, 0x0a, 0x10 }, // lda INPT2,x; bpl
      { 0xb5, 0x0a, 0x30 }, // lda INPT2,x; bmi
      { 0xb5, 0x08, 0x10 }, // lda INPT0,x; bpl (no joystick games)
      { 0xb5, 0x08, 0x30 }, // lda INPT0,x; bmi (no joystick games)
      { 0x24, 0x3a, 0x10 }, // bit INPT2|$30; bpl
      { 0x24, 0x3a, 0x30 }, // bit INPT2|$30; bmi
      { 0xa5, 0x3a, 0x10 }, // lda INPT2|$30; bpl
      { 0xa5, 0x3a, 0x30 }, // lda INPT2|$30; bmi
      { 0xb5, 0x3a, 0x10 }, // lda INPT2|$30,x; bpl
      { 0xb5, 0x3a, 0x30 }, // lda INPT2|$30,x; bmi
      { 0xb5, 0x38, 0x10 }, // lda INPT0|$30,x; bpl  (Circus Atari, old code!)
      { 0xb5, 0x38, 0x30 }, // lda INPT0|$30,x; bmi (no joystick games)
      { 0xa4, 0x3a, 0x30 }, // ldy INPT2|$30; bmi (no joystick games)
      { 0xa5, 0x3b, 0x30 }, // lda INPT3|$30; bmi (only Tac Scan, ports and paddles swapped)
    };
    const int NUM_SIGS_1 = 1;
    const int SIG_SIZE_1 = 4;
    uInt8 signature_1[NUM_SIGS_1][SIG_SIZE_1] = {
      { 0xb9, 0x38, 0x00, 0x30 }, // lda INPT0|$30,y; bmi (Video Olympics)
    };
    const int NUM_SIGS_2 = 3;
    const int SIG_SIZE_2 = 5;
    uInt8 signature_2[NUM_SIGS_2][SIG_SIZE_2] = {
      { 0xb5, 0x38, 0x29, 0x80, 0xd0 }, // lda INPT0|$30,x; and #$80; bne (Basic Programming)
      { 0x24, 0x38, 0x85, 0x08, 0x10 }, // bit INPT2|$30; sta COLUPF, bpl (Fireball, patched at runtime!)
      { 0xb5, 0x38, 0x49, 0xff, 0x0a }  // lda INPT0|$30,x; eor #$ff; asl (Blackjack)
    };

    for(uInt32 i = 0; i < NUM_SIGS_0; ++i)
      if(searchForBytes(image, size, signature_0[i], SIG_SIZE_0))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_1; ++i)
      if(searchForBytes(image, size, signature_1[i], SIG_SIZE_1))
        return true;

    for(uInt32 i = 0; i < NUM_SIGS_2; ++i)
      if(searchForBytes(image, size, signature_2[i], SIG_SIZE_2))
        return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyTrakBall(const uInt8* image, uInt32 size)
{
  // check for TrakBall tables
  const int NUM_SIGS = 3;
  const int SIG_SIZE = 6;
  uInt8 signature[NUM_SIGS][SIG_SIZE] = {
    { 0b1010, 0b1000, 0b1000, 0b1010, 0b0010, 0b0000/*, 0b0000, 0b0010*/ }, // NextTrackTbl (T. Jentzsch)
    { 0x00, 0x07, 0x87, 0x07, 0x88, 0x01/*, 0xff, 0x01*/ }, // .MovementTab_1 (Omegamatrix, SMX7)
    { 0x00, 0x01, 0x81, 0x01, 0x82, 0x03 }, // .MovementTab_1 (Omegamatrix)
  }; // all pattern checked, only TrakBall matches

  for(uInt32 i = 0; i < NUM_SIGS; ++i)
    if(searchForBytes(image, size, signature[i], SIG_SIZE))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyAtariMouse(const uInt8* image, uInt32 size)
{
  // check for Atari Mouse tables
  const int NUM_SIGS = 3;
  const int SIG_SIZE = 6;
  uInt8 signature[NUM_SIGS][SIG_SIZE] = {
    { 0b0101, 0b0111, 0b0100, 0b0110, 0b1101, 0b1111/*, 0b1100, 0b1110*/ }, // NextTrackTbl (T. Jentzsch)
    { 0x00, 0x87, 0x07, 0x00, 0x08, 0x81/*, 0x7f, 0x08*/ }, // .MovementTab_1 (Omegamatrix, SMX7)
    { 0x00, 0x81, 0x01, 0x00, 0x02, 0x83 }, // .MovementTab_1 (Omegamatrix)
  }; // all pattern checked, only Atari Mouse matches

  for(uInt32 i = 0; i < NUM_SIGS; ++i)
    if(searchForBytes(image, size, signature[i], SIG_SIZE))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablyAmigaMouse(const uInt8* image, uInt32 size)
{
  // check for Amiga Mouse tables
  const int NUM_SIGS = 4;
  const int SIG_SIZE = 6;
  uInt8 signature[NUM_SIGS][SIG_SIZE] = {
    { 0b1100, 0b1000, 0b0100, 0b0000, 0b1101, 0b1001/*, 0b0101, 0b0001*/ }, // NextTrackTbl (T. Jentzsch)
    { 0x00, 0x88, 0x07, 0x01, 0x08, 0x00/*, 0x7f, 0x07*/ }, // .MovementTab_1 (Omegamatrix, SMX7)
    { 0x00, 0x82, 0x01, 0x03, 0x02, 0x00 }, // .MovementTab_1 (Omegamatrix)
    { 0b100, 0b000, 0b000, 0b000, 0b101, 0b001} // NextTrackTbl (T. Jentzsch, MCTB)
  }; // all pattern checked, only Amiga Mouse matches

  for(uInt32 i = 0; i < NUM_SIGS; ++i)
    if(searchForBytes(image, size, signature[i], SIG_SIZE))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerDetector::isProbablySaveKey(const uInt8* image, uInt32 size, Controller::Jack port)
{
  // check for known SaveKey code, only supports right port
  if(port == Controller::Jack::Right)
  {
    const int NUM_SIGS = 4;
    const int SIG_SIZE = 9;
    uInt8 signature[NUM_SIGS][SIG_SIZE] = {
      { // from I2C_START (i2c.inc)
        0xa9, 0x08,       // lda #I2C_SCL_MASK
        0x8d, 0x80, 0x02, // sta SWCHA
        0xa9, 0x0c,       // lda #I2C_SCL_MASK|I2C_SDA_MASK
        0x8d, 0x81        // sta SWACNT
      },
      { // from I2C_START (i2c_v2.1..3.inc)
        0xa9, 0x18,       // #(I2C_SCL_MASK|I2C_SDA_MASK)*2
        0x8d, 0x80, 0x02, // sta SWCHA
        0x4a,             // lsr
        0x8d, 0x81, 0x02  // sta SWACNT
      },
      { // from I2C_START (Strat-O-Gems)
        0xa2, 0x08,       // ldx #I2C_SCL_MASK
        0x8e, 0x80, 0x02, // stx SWCHA
        0xa2, 0x0c,       // ldx #I2C_SCL_MASK|I2C_SDA_MASK
        0x8e, 0x81        // stx SWACNT
      },
      { // from I2C_START (AStar, Fall Down, Go Fish!)
        0xa9, 0x08,       // lda #I2C_SCL_MASK
        0x8d, 0x80, 0x02, // sta SWCHA
        0xea,             // nop
        0xa9, 0x0c,       // lda #I2C_SCL_MASK|I2C_SDA_MASK
        0x8d              // sta SWACNT
      }
    };

    for(uInt32 i = 0; i < NUM_SIGS; ++i)
      if(searchForBytes(image, size, signature[i], SIG_SIZE))
        return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string ControllerDetector::getControllerName(const string& controller)
{
  // auto detected:
  if(BSPF::equalsIgnoreCase(controller, "JOYSTICK"))
    return "Joystick";
  if(BSPF::equalsIgnoreCase(controller, "SAVEKEY"))
    return "SaveKey";
  if(BSPF::equalsIgnoreCase(controller, "TRAKBALL"))
    return "TrakBall";
  if(BSPF::equalsIgnoreCase(controller, "ATARIMOUSE"))
    return "AtariMouse";
  if(BSPF::equalsIgnoreCase(controller, "AMIGAMOUSE"))
    return "AmigaMouse";
  if(BSPF::equalsIgnoreCase(controller, "KEYBOARD"))
    return "Keyboard";
  if(BSPF::equalsIgnoreCase(controller, "GENESIS"))
    return "Sega Genesis";
  if(BSPF::equalsIgnoreCase(controller, "PADDLES"))
    return "Paddles";
  // not auto detected:
  if(BSPF::equalsIgnoreCase(controller, "BOOSTERGRIP"))
    return "BoosterGrip";
  if(BSPF::equalsIgnoreCase(controller, "DRIVING"))
    return "Driving";
  if(BSPF::equalsIgnoreCase(controller, "MINDLINK"))
    return "MindLink";
  if(BSPF::equalsIgnoreCase(controller, "ATARIVOX"))
    return "AtariVox";
  if(BSPF::equalsIgnoreCase(controller, "PADDLES_IAXIS"))
    return "Paddles IAxis";
  if(BSPF::equalsIgnoreCase(controller, "PADDLES_IAXDR"))
    return "Paddles IAxDr";
  if(BSPF::equalsIgnoreCase(controller, "COMPUMATE"))
    return "CompuMate";
  if(BSPF::equalsIgnoreCase(controller, "KIDVID"))
    return "KidVid";

  return controller;
}
