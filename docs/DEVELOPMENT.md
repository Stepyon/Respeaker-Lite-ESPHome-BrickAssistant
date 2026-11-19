# Development, validation and release

The source of truth is `config/respeaker-lite-brick-assistant.yaml`. The tested
CI and local development ESPHome version is **2026.11.0**, pinned in
`requirements-ci.txt`. The YAML's `min_version: 2026.11.0` is a lower-bound
compatibility declaration, not the CI build version. Update the minimum only
when a required ESPHome feature makes older versions incompatible.

## ESPHome 2026.11 I2S compatibility

ESPHome 2026.11 changed the I2S speaker DAC interface from a single
`audio_dac_` member to multiple `audio_dacs_`. The old pinned
`formatBCE/esphome` I2S override fails C++ compilation because it references
the removed member. No change to compiler settings can make that fork safe
without updating it.

The ReSpeaker XMOS still clocks its microphones and speaker at **48 kHz**.
The old custom fork also decimated both microphone channels by keeping one
of every three samples for the **16 kHz** voice-assistant and micro-wake-word
paths. Removing the fork alone would discard that adaptation. The new
configuration uses ESPHome's built-in I2S implementation together with the
standalone `decimator` component pinned from the upstream ReSpeaker
integration at commit `919b4aceae49f64d0af874dce7d3b1750e7734ef`.

The same upstream revision also supplies the `respeaker_lite` XMOS component.
The older pinned ReSpeaker integration called `str_sprintf()`, removed from
ESPHome 2026.11, and failed compilation in `dfu_get_version_()`. The updated
component uses standard `std::snprintf()` and retains the existing XMOS
firmware configuration.

`i2s_mics_raw` is the existing 48 kHz stereo I2S microphone. The decimator
exposes the original `i2s_mics` ID at 16 kHz with channel 0 for Assist and
channel 1 for wake words. No change to the existing media speaker settings
or volume limits is intended. Verify speaker output, both microphone channels,
wake word sensitivity, music/radio streaming and full-duplex behaviour on
hardware after upgrading.

## Local development

1. Install Python 3.12 and run `pip install -r requirements-ci.txt`.
2. Create `config/secrets.yaml` with Wi-Fi/OTA values appropriate for the device
   (do not use a public sample device's settings).
3. Run `esphome config config/respeaker-lite-brick-assistant.yaml`.
4. Run `esphome compile config/respeaker-lite-brick-assistant.yaml`.
5. Flash a test device; verify hardware behaviour against the checklist below.

The CI workflow validates configuration and compiles an ESP32-S3 firmware; it does
**not** prove audio timing, device safety, or hardware compatibility.
It uses a shared pip dependency file for both reproducible installation and
GitHub Actions pip-cache keying.

## Hardware regression checklist

- Test media, FLAC announcements, wake-word interruption and recovery.
- Test *each* wake word and all three sensitivity levels, including Kenobi.
- Ring a timer while media is playing, say Stop, and verify the timer audio
  stops; repeat when media is paused or disconnected.
- Follow the dedicated [Timer Snooze checklist](SNOOZE.md), including reboot
  persistence, no-clock fallback, cancellation, and siren precedence.
- Ring the siren and disable it halfway through the sound: audio must stop.
- During a siren, verify the saved user volume is restored after cancellation.
- Trigger a notification during music and a voice assistant response.
- Make a TTS URL fail or disconnect Home Assistant during playback. Confirm
  the next voice session can start without a reboot.
- Repeat with rapid consecutive wake words and back-to-back announcements.
- Reboot during an active timer or siren and check the defined OFF recovery.
- Observe heap, PSRAM and loop-time stability after an hour of mixed playback.

## Investigating a sudden volume change

BrickAssistant's `external_media_player` uses one persistent media-player volume
for music and announcements in ESPHome 2026.11.0. Its `volume_min: 0.3` also
remaps nonzero volume to a fairly strong output: a 30% Home Assistant volume
setting corresponds to 51% at the audio output. This scaling alone is not
a change to the Home Assistant volume slider.

The only BrickAssistant path intentionally setting 100% volume is Siren Ring.
Its previous 500 ms delayed ON action could leave the player at 100% if the
siren was stopped before the delay completed, or overwrite its saved level if
an ON action ran again. The guarded siren implementation captures volume once,
before stopping audio or waiting, and only applies the siren volume when the
siren remains active. Its OFF action restores the saved level once.

The firmware now logs volume changes through `speaker_source.on_volume`:
- `audio_volume` WARN: volume rose by more than 15 percentage points outside
  a siren. These warnings are visible with the default WARN logging level.
- `audio_volume` INFO: all other player-volume changes; enable INFO logging
  through the Logger Level select if needed.

If normal TTS causes a jump, check the Home Assistant `Media Player`
`volume_level` attribute immediately before and after. If the attribute
actually changes, inspect `media_player.volume_set` automations, Music
Assistant/Sendspin grouping, and any announcement scripts. ESPHome's Sendspin
media source can request global player-volume changes. The `on_volume` event
indicates a volume setter ran, but cannot by itself identify which caller.

If the level remains the same while the sound is louder, check audio content
loudness, mixer ducking and the configured `volume_min` remapping.

### Hardware regression tests

- Start with a player volume of 20%, activate Siren Ring, and stop it after
  100 ms (before the 500 ms delay). Verify the volume stays at 20%.
- With 20% volume, let the siren play and stop normally; verify restoration
  to 20%.
- Reissue Siren Ring ON while it is already ringing; stop it and confirm it
  restores the initial level rather than 100%.
- Run ordinary Home Assistant TTS announcements repeatedly, with and without
  Sendspin media active; watch the player volume level and `audio_volume` logs.
- Verify normal volume-up/down, timer/Snooze audio, notifications and media
  ducking are otherwise unchanged.

## Release process

The firmware installer manifest currently targets the **2026.04.25** tested
binary. This is **not** the same as the latest YAML source. Do not silently
change the manifest to claim the newer firmware is available.

For a release:
1. Merge reviewed changes and pass the build workflow.
2. Download the compiled artifact and **flash and test it on ReSpeaker Lite**.
3. Publish that exact tested factory binary under `firmware/` with a versioned
   filename. Confirm whether the artifact is a factory image suitable for
   flashing at offset 0 before referencing it.
4. Update `firmware/manifest.json` to match that file and its version.
5. Test installation, recovery, voice controls and both physical buttons using
   the published manifest URL.

## Dependency management

External ESPHome components, custom Kenobi model, XMOS image and bundled sound
assets refer to specific Git revisions. Update these revisions intentionally,
compile, then test the hardware before publishing. The declared XMOS firmware
MD5 is upstream-defined and should be rechecked when updating the XMOS binary.

## Scope exclusions

This development pass deliberately does not modify access credentials, API
encryption, the fallback access point or credential history. They require
their own security changes.
