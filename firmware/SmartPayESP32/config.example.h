#pragma once
// Copy to config.h (ignored by git). Never commit real credentials.
// Optional fallback on first boot; NVS Wi-Fi set through the portal takes priority.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define BACKEND_URL "https://your-backend.example.com"
#define DEVICE_ID "ESP32-001"
#define DEVICE_TOKEN ""
// PEM root CA for your HTTPS backend. Empty CA blocks production connections.
static const char ROOT_CA[] = "";
// Only for a trusted local mock server. Never use HTTP in production.
#define ALLOW_LOCAL_HTTP 0
// Display QR + play 150000 VND once, without networking, for hardware validation.
#define HARDWARE_SMOKE_TEST 0