# Intermittent Wi-Fi latency during music and radio playback

## Observed symptom

Ping to ReSpeaker Lite (e.g. `192.168.1.237`) occasionally reaches
1.2–1.9 seconds while playback stutters even though the device is close to
the router. This is not normal LAN latency, but ping alone cannot determine
whether the delay is Wi-Fi retransmission, modem sleep, overloaded tasks,
network congestion or power supply instability.

## Firmware inspection

The BrickAssistant configuration uses ESP32-S3 (240 MHz) with 48 kHz
stereo microphone input, 48 kHz mono speaker output, wake word detection,
two resamplers, a mixer, HTTP audio streaming and optional Sendspin.
Both pipelines, plus the Wi-Fi stack, compete for CPU and memory.

The original Wi-Fi config omits `power_save_mode`, selecting ESPHome's
ESP32 default, `LIGHT`. In ESP-IDF this maps to `WIFI_PS_MIN_MODEM`.
AP-dependent modem sleep can delay packets; it is a plausible contributor
but is not proven to explain multi-second stalls.

The firmware additionally uses `output_power: 12.5` and
`fast_connect: true`. We deliberately leave both unchanged for the
first experiment. A weak uplink or a suboptimal AP choice may still be
responsible even with the device near the router.

## Experimental change

- Set `wifi.power_save_mode: NONE` on a **diagnostic branch only**.
  This uses more power but makes reception independent of modem sleep.
- Expose four ESPHome debug sensors and Wi-Fi RSSI in Home Assistant:
  `WiFi RSSI` (dBm), `Free Heap` (bytes),
  `Largest Free Heap Block` (bytes), `Longest Loop Time` (ms),
  `Free PSRAM` (bytes). All refresh at 5-second intervals.
- Preserve the prior codec, 48 kHz audio, buffer sizes, CPU speed,
  transmit power, secrets, and published firmware for a controlled test.

The diagnostics cannot directly measure Wi-Fi retransmissions, UDP/TCP
loss or background FreeRTOS task CPU utilization. A low main-loop time
does not rule out contention in the audio or Wi-Fi tasks.

## Reproduction / A–B test

1. Using the existing firmware, run continuous Windows pings from a
   PC on the LAN: `ping -t 192.168.1.237`. In another terminal,
   ping the local router (for example `ping -t 192.168.1.1`).
   If the router ping also spikes, investigate the client/router path.
   If only ReSpeaker spikes, focus on the device and its Wi-Fi connection.
2. Record 5–10 minutes **idle**, 5–10 minutes **internet radio**, and
   5–10 minutes **music** (repeat the content/source when comparing).
   Note maximum ping, lost packets, music interruptions, Wi-Fi RSSI,
   and any network disconnect/underrun/heap warnings.
3. Flash the diagnostic branch's compiled firmware on the same device.
   Leave the AP, power supply, location, audio source and other settings
   unchanged; repeat the three scenarios.
4. Check Home Assistant History for the diagnostic sensors. Sudden
   drops in heap or largest free block suggest memory pressure;
   prolonged loop-time increases point to main-loop stalls. Weak or
   rapidly changing RSSI suggests Wi-Fi interference or radio instability.
   Strong RSSI does **not** rule out retransmission or congestion.
5. Only if disabling power save does not help, test additional factors
   **one at a time**: Wi-Fi AP/channel/2.4 GHz coexistence, power supply,
   `output_power: 12.5`, `fast_connect`, and streaming workload.
   Observe the AP's Wi-Fi retry/roaming/association logs if possible.

If the lag happens with the device completely idle, or after a cold boot
without running the audio pipeline, that decreases confidence in an
audio-task starvation explanation. Compare results from radio streaming
versus LAN-hosted media to distinguish WAN/streaming-server problems.

## Follow-up decision

- **Improved with NONE:** consider carrying `power_save_mode: NONE` to
  the release branch, after hardware regression tests.
- **No improvement, poor RSSI:** investigate radio placement/AP channel
  and Wi-Fi transmit power before changing audio buffer sizes.
- **No improvement, good RSSI and loop/heap stress:** profile ESP-IDF
  task scheduling and audio source/decoder workloads.
- **No improvement, good RSSI and no CPU/memory symptoms:** obtain
  router/client wireless retransmission and AP isolation data.

This experimental branch is not a production fix and does not modify
the release manifest.

## References

- ESPHome Wi-Fi configuration: https://esphome.io/components/wifi/
- ESP-IDF ESP32-S3 Wi-Fi power save: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/wifi-driver/wifi-performance-and-power-save.html
- ESPHome debug component: https://esphome.io/components/debug/
- ESPHome Wi-Fi signal sensor: https://esphome.io/components/sensor/wifi_signal/
