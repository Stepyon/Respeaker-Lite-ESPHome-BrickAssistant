# Timer Snooze in BrickAssistant

This feature is adapted from the ReSpeaker Lite upstream 2026.10.0 Snooze
implementation. It applies to **voice-assistant timers**, not to BrickAssistant's
`Siren Ring` security/emergency siren. BrickAssistant has no separate scheduled
daily-alarm feature; the upstream daily-alarm Snooze behaviour is therefore not
ported.

## Using Snooze

1. Create a voice-assistant timer in Home Assistant.
2. When BrickAssistant rings, say **"Snooze"**. Ringing stops and the same
   ringtone resumes after the configured interval (default **9 minutes**).
3. Say **"Stop"** during ringing to dismiss it rather than Snooze.
4. Use these Home Assistant entities:
   - `switch.snooze_enabled`: enable/disable listening for Snooze (default on).
   - `number.snooze_duration`: 1–60 minutes, restored after a reboot.
   - `binary_sensor.snooze_pending`: whether a snoozed ring is scheduled.
   - `button.cancel_snooze`: cancel a pending Snooze without another ring.

Disabling Snooze also cancels an existing pending Snooze. The Snooze model is
only enabled when a timer rings and Snooze is enabled; it is disabled when the
timer stops. A Snooze recognition outside an actively ringing timer is ignored.

## LED indication

BrickAssistant now uses an explicit **amber flash** while a Snooze is pending:
750 ms on, 750 ms off, at 65% brightness. This is deliberately different
from the cyan timer-ringing animation. The LED returns to the normal state
when Snooze is cancelled, disabled, expires or resumes ringing.

The flashing controller is clocked independently of the Snooze deadline check;
it does not rely on the light effect's internal selection state. This prevents
an interrupted/disabled effect from silently suppressing the indicator.

Active voice interactions, notifications, ringing timers and the security siren
take precedence over the Snooze indication. Once they finish, the pending
Snooze indicator returns automatically. Timer-tick flashes are suppressed while
Snooze is pending to prevent them from interrupting the amber flash.

## Reliability and limitations

- **Clock needed:** Home Assistant must provide valid device time. If the clock
  is unavailable, Snooze is ignored and the timer continues ringing; it must
  not silently disappear.
- **After reboot:** the next ringing deadline is stored and rechecked every
  second after the Home Assistant clock becomes valid. The underlying Home
  Assistant timer itself is not recreated; only the deferred ringtone resumes.
- **Late recovery:** if the deadline is more than 10 minutes past (e.g. the
  device was offline for hours), the pending ring is discarded, matching the
  existing 10-minute maximum ring duration.
- **Audio priority:** a deferred ring will not interrupt a currently ringing
  timer or active security siren. It will retry until the interruption ends or
  the recovery grace period expires.
- **New timer:** a newly finished voice timer supersedes and cancels any older
  pending Snooze.
- **During Snooze:** the Stop wake word is intentionally not active. Cancel via
  Home Assistant instead.
- **False activations:** the Snooze model's upstream cutoff (0.95) is unchanged
  by the normal wake-word sensitivity selector. The extra model uses additional
  RAM and CPU while loaded; validate runtime stability on the device.
- **Security:** the security siren is never snoozed by this feature.

## Device acceptance tests

- [ ] Create a 1-minute timer; say Snooze during ringing. Check that the audio
      stops immediately, Snooze pending becomes on, and it re-rings after the
      configured duration.
- [ ] Say Stop on the resumed ring. Verify no later re-ring occurs.
- [ ] Say Stop on a normal ring; confirm no pending Snooze.
- [ ] Press the physical volume/stop button while ringing; check normal stop.
- [ ] Change Snooze duration, reboot, and confirm the duration is restored.
- [ ] Reboot during a pending Snooze. After time sync, check the ring resumes.
- [ ] Disconnect HA so the device has no valid wall clock, trigger a ring and
      say Snooze. Confirm it keeps ringing instead of losing the alarm.
- [ ] Turn off Snooze while ringing. Confirm the model is disabled. Turn it on
      again and verify Snooze is available.
- [ ] Disable Snooze during a pending snooze; confirm it is cancelled.
- [ ] Use Cancel snooze while pending; confirm it does not re-ring.
- [ ] Trigger a new timer completion while a Snooze is pending. Only one
      ringtone should persist.
- [ ] During a pending Snooze, verify the LED flashes amber (not cyan),
      approximately 750 ms on / 750 ms off, without interruption from ticks.
- [ ] Confirm `binary_sensor.snooze_pending` is ON whenever this flash is expected.
- [ ] Cancel Snooze, disable it, or let it ring again; verify the LED indication
      is removed and normal timer-ringing LED resumes when the timer returns.
- [ ] Start a voice response or notification while Snooze is pending and verify
      its LED takes priority; the amber pulse should return afterward.
- [ ] Ring the siren during pending Snooze; verify the siren continues
      uninterrupted and deferred ringing honours the recovery limit.
- [ ] Say "Snooze" outside timer ringing and while a security siren rings.
      Confirm neither voice assistant nor security siren is affected.
- [ ] Test music ducking, STT/TTS, all wake words, button controls, and
      microphone mute; monitor heap and audio-task stability.

## Reference

- Upstream release: https://github.com/formatBCE/Respeaker-Lite-ESPHome-integration/releases/tag/2026.10.0
- Model: `microwakeword/models/v2/snooze.json` at pinned revision
  `00131ff518b878413b5de7023b956766747ee600`.
