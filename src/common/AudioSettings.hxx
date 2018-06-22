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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef AUDIO_PARAMTERS_HXX
#define AUDIO_PARAMTERS_HXX

#include "bspf.hxx"

class Settings;

class AudioSettings
{
  public:

    enum class Preset {
      custom                    = 1,
      lowQualityMediumLag       = 2,
      highQualityMediumLag      = 3,
      highQualityLowLag         = 4,
      veryHighQualityVeryLowLag = 5
    };

    enum class ResamplingQuality {
      nearestNeightbour   = 1,
      lanczos_2           = 2,
      lanczos_3           = 3
    };

    static constexpr const char* SETTING_PRESET              = "audio.preset";
    static constexpr const char* SETTING_SAMPLE_RATE         = "audio.sample_rate";
    static constexpr const char* SETTING_FRAGMENT_SIZE       = "audio.fragment_size";
    static constexpr const char* SETTING_BUFFER_SIZE         = "audio.buffer_size";
    static constexpr const char* SETTING_HEADROOM            = "audio.headroom";
    static constexpr const char* SETTING_RESAMPLING_QUALITY  = "audio.resampling_quality";
    static constexpr const char* SETTING_VOLUME              = "audio.volume";
    static constexpr const char* SETTING_ENABLED             = "audio.enabled";

    static constexpr Preset DEFAULT_PRESET                          = Preset::highQualityMediumLag;
    static constexpr uInt32 DEFAULT_SAMPLE_RATE                     = 44100;
    static constexpr uInt32 DEFAULT_FRAGMENT_SIZE                   = 512;
    static constexpr uInt32 DEFAULT_BUFFER_SIZE                     = 3;
    static constexpr uInt32 DEFAULT_HEADROOM                        = 2;
    static constexpr ResamplingQuality DEFAULT_RESAMPLING_QUALITY   = ResamplingQuality::lanczos_2;
    static constexpr uInt32 DEFAULT_VOLUME                          = 80;
    static constexpr bool DEFAULT_ENABLED                           = true;

  public:

    AudioSettings() = default;

    AudioSettings(Settings* mySettings);

    static void initialize(Settings& settings);

    static void normalize(Settings& settings);

    Preset preset();

    uInt32 sampleRate();

    uInt32 fragmentSize();

    uInt32 bufferSize();

    uInt32 headroom();

    ResamplingQuality resamplingQuality();

    uInt32 volume() const;

    bool enabled() const;

    void setPreset(Preset preset);

    void setSampleRate(uInt32 sampleRate);

    void setFragmentSize(uInt32 fragmentSize);

    void setBufferSize(uInt32 bufferSize);

    void setHeadroom(uInt32 headroom);

    void setResamplingQuality(ResamplingQuality resamplingQuality);

    void setVolume(uInt32 volume);

    void setEnabled(bool isEnabled);

  private:

    bool customSettings() const;

    void updatePresetFromSettings();

  private:

    Settings* mySettings;

    Preset myPreset;

    uInt32 myPresetSampleRate;
    uInt32 myPresetFragmentSize;
    uInt32 myPresetBufferSize;
    uInt32 myPresetHeadroom;
    ResamplingQuality myPresetResamplingQuality;
};

#endif // AUDIO_PARAMTERS_HXX
