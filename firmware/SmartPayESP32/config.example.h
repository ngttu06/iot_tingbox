#pragma once
// Copy to config.h (ignored by git). Never commit real credentials.
// Optional fallback on first boot; NVS Wi-Fi set through the portal takes priority.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
// Per-device SPK UUID is generated ONCE and kept in NVS, never hardcode a shared key.
#define SPEAKER_BASE_URL "https://tingbox.ngttu.com"
// Default CA is GTS Root R4 in SpeakerTrust.h. Override SPEAKER_ROOT_CA only
// when using another verified HTTPS server. HTTP/insecure TLS are not supported.
// Display QR + play 150000 VND once, without networking, for hardware validation.
#define HARDWARE_SMOKE_TEST 0