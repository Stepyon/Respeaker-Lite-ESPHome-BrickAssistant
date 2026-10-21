# Development, validation and release

The source of truth is `config/respeaker-lite-brick-assistant.yaml`. The ESPHome
baseline declared by this project is **2026.5.0**. Pin updates deliberately.

## Local development

1. Install Python 3.12 and `pip install esphome==2026.5.0`.
2. Create `config/secrets.yaml` with Wi-Fi/OTA values appropriate for the device
   (do not use a public sample device's settings).
3. Run `esphome config config/respeaker-lite-brick-assistant.yaml`.
4. Run `esphome compile config/respeaker-lite-brick-assistant.yaml`.
5. Flash a test device; verify hardware behaviour against the checklist below.

The CI workflow validates configuration and compiles an ESP32-S3 firmware; it does
**not** prove audio timing, device safety, or hardware compatibility.

## Hardware regression checklist

- Test media, FLAC announcements, wake-word interruption and recovery.
- Test *each* wake word and all three sensitivity levels, including Kenobi.
- Ring a timer while media is playing, say Stop, and verify the timer audio
  stops; repeat when media is paused or disconnected.
- Ring the siren and disable it halfway through the sound: audio must stop.
- During a siren, verify the saved user volume is restored after cancellation.
- Trigger a notification during music and a voice assistant response.
- Make a TTS URL fail or disconnect Home Assistant during playback. Confirm
  the next voice session can start without a reboot.
- Repeat with rapid consecutive wake words and back-to-back announcements.
- Reboot during an active timer or siren and check the defined OFF recovery.
- Observe heap, PSRAM and loop-time stability after an hour of mixed playback.

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
