#include "solar_service.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

namespace {

const char kOpenMeteoRootCA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

std::vector<float> s_generationForecastKw;

} // namespace

namespace SolarService {

bool fetchForecast() {
#if SOLAR_FORECAST_ENABLED
  WiFiClientSecure client;
  client.setCACert(kOpenMeteoRootCA);

  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(SOLAR_LATITUDE, 4);
  url += "&longitude=";
  url += String(SOLAR_LONGITUDE, 4);
  url += "&hourly=global_tilted_irradiance&tilt=";
  url += String(SOLAR_PANEL_TILT_DEG, 1);
  url += "&azimuth=";
  url += String(SOLAR_PANEL_AZIMUTH_DEG, 1);
  url += "&forecast_days=2&timezone=auto";

  HTTPClient http;
  if (!http.begin(client, url)) {
    return false;
  }
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("Open-Meteo HTTP request failed: %d (%s)\n", status,
                  http.errorToString(status).c_str());
    http.end();
    return false;
  }

  const String response = http.getString();
  http.end();
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, response);
  if (error) {
    const String preview = response.substring(0, 96);
    Serial.printf("Open-Meteo JSON parse failed: %s; bytes=%u; response=%s\n",
                  error.c_str(), (unsigned)response.length(), preview.c_str());
    return false;
  }

  const JsonArrayConst irradiance =
      doc["hourly"]["global_tilted_irradiance"].as<JsonArrayConst>();
  if (irradiance.size() < 24) {
    Serial.println("Open-Meteo response has insufficient irradiance data.");
    return false;
  }

  std::vector<float> freshForecast;
  freshForecast.reserve(irradiance.size());
  for (JsonVariantConst value : irradiance) {
    const float wattsPerSquareMeter = value | 0.0f;
    const float generationKw = wattsPerSquareMeter *
                               SOLAR_ARRAY_CAPACITY_KWP *
                               SOLAR_SYSTEM_EFFICIENCY / 1000.0f;
    freshForecast.push_back(generationKw);
  }
  s_generationForecastKw = std::move(freshForecast);
  return true;
#else
  return false;
#endif
}

const std::vector<float> &generationForecastKw() {
  return s_generationForecastKw;
}

} // namespace SolarService