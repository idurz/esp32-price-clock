#include "price_service.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

namespace {

// Amazon Root CA 1 — validates api.tibber.com's certificate chain
// (fetched/verified via `openssl s_client -connect api.tibber.com:443`).
const char kTibberRootCA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEkjCCA3qgAwIBAgITBn+USionzfP6wq4rAfkI7rnExjANBgkqhkiG9w0BAQsF
ADCBmDELMAkGA1UEBhMCVVMxEDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNj
b3R0c2RhbGUxJTAjBgNVBAoTHFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4x
OzA5BgNVBAMTMlN0YXJmaWVsZCBTZXJ2aWNlcyBSb290IENlcnRpZmljYXRlIEF1
dGhvcml0eSAtIEcyMB4XDTE1MDUyNTEyMDAwMFoXDTM3MTIzMTAxMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaOCATEwggEtMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/
BAQDAgGGMB0GA1UdDgQWBBSEGMyFNOy8DJSULghZnMeyEE4KCDAfBgNVHSMEGDAW
gBScXwDfqgHXMCs4iKK4bUqc8hGRgzB4BggrBgEFBQcBAQRsMGowLgYIKwYBBQUH
MAGGImh0dHA6Ly9vY3NwLnJvb3RnMi5hbWF6b250cnVzdC5jb20wOAYIKwYBBQUH
MAKGLGh0dHA6Ly9jcnQucm9vdGcyLmFtYXpvbnRydXN0LmNvbS9yb290ZzIuY2Vy
MD0GA1UdHwQ2MDQwMqAwoC6GLGh0dHA6Ly9jcmwucm9vdGcyLmFtYXpvbnRydXN0
LmNvbS9yb290ZzIuY3JsMBEGA1UdIAQKMAgwBgYEVR0gADANBgkqhkiG9w0BAQsF
AAOCAQEAYjdCXLwQtT6LLOkMm2xF4gcAevnFWAu5CIw+7bMlPLVvUOTNNWqnkzSW
MiGpSESrnO09tKpzbeR/FoCJbM8oAxiDR3mjEH4wW6w7sGDgd9QIpuEdfF7Au/ma
eyKdpwAJfqxGF4PcnCZXmTA5YpaP7dreqsXMGz7KQ2hsVxa81Q4gLv7/wmpdLqBK
bRRYh5TmOTFffHPLkIhqhBGWJ6bt2YFGpn6jcgAKUj6DiAdjd4lpFw85hdKrCEVN
0FE6/V1dN2RMfjCyVSRCnTawXZwXgWHxyvkQAiSr6w10kY17RSlQOYiypok1JR4U
akcjMS9cmvqtmg5iUaQqqcT5NJ0hGA==
-----END CERTIFICATE-----
)EOF";

const char kEndpoint[] = "https://api.tibber.com/v1-beta/gql";
const char kQuery[] =
    "{\"query\":\"{ viewer { homes { currentSubscription { priceInfo { "
    "today { total startsAt level } tomorrow { total startsAt level } } } } "
    "} }\"}";

std::vector<PriceService::Slice> s_today;
std::vector<PriceService::Slice> s_tomorrow;

PriceService::Level mapLevel(const char *level, float total) {
  if (total <= 0.0f) {
    return PriceService::Level::DeliverBack;
  }
  if (!level) {
    return PriceService::Level::Normal;
  }
  if (strcmp(level, "VERY_CHEAP") == 0 || strcmp(level, "CHEAP") == 0) {
    return PriceService::Level::Cheap;
  }
  if (strcmp(level, "EXPENSIVE") == 0) {
    return PriceService::Level::Pricy;
  }
  if (strcmp(level, "VERY_EXPENSIVE") == 0) {
    return PriceService::Level::Expensive;
  }
  return PriceService::Level::Normal;
}

void appendArray(JsonArrayConst arr, std::vector<PriceService::Slice> &out) {
  for (JsonObjectConst entry : arr) {
    PriceService::Slice slice;
    slice.total = entry["total"] | 0.0f;
    slice.startsAt = entry["startsAt"] | "";
    slice.level = mapLevel(entry["level"] | (const char *)nullptr, slice.total);
    out.push_back(slice);
  }
}

} // namespace

namespace PriceService {

bool fetchPrices() {
  WiFiClientSecure client;
  client.setCACert(kTibberRootCA);

  HTTPClient http;
  if (!http.begin(client, kEndpoint)) {
    return false;
  }
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + TIBBER_API_TOKEN);
  http.addHeader("User-Agent", "ESP32C3/1.0 esp32-price-clock/0.1");

  const int status = http.POST((uint8_t *)kQuery, strlen(kQuery));
  if (status != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    return false;
  }

  JsonArrayConst homes = doc["data"]["viewer"]["homes"].as<JsonArrayConst>();
  if (homes.isNull() || homes.size() == 0) {
    return false;
  }
  JsonObjectConst priceInfo = homes[0]["currentSubscription"]["priceInfo"];

  std::vector<Slice> freshToday;
  std::vector<Slice> freshTomorrow;
  appendArray(priceInfo["today"].as<JsonArrayConst>(), freshToday);
  appendArray(priceInfo["tomorrow"].as<JsonArrayConst>(), freshTomorrow);
  if (freshToday.empty()) {
    return false;
  }

  s_today = std::move(freshToday);
  s_tomorrow = std::move(freshTomorrow);
  return true;
}

const std::vector<Slice> &today() { return s_today; }
const std::vector<Slice> &tomorrow() { return s_tomorrow; }

} // namespace PriceService
