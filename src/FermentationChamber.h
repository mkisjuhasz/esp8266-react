#ifndef FermentationChamber_h
#define FermentationChamber_h

#include <SettingsService.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <JsonUtil.h>
#include <CircularLog.h>

#define COOLER_PIN 12
#define HEATER_PIN 13
#define TEMP_SENSOR_PIN 14

// We store (60 * 24) slots of 60 second intervals (one day) worth of logging data
// in a circular buffer in SPIFFS. This results in very low wear on the flash chip
// only 1 write per sector per day.
#define LOG_SLOTS_PER_HOUR 60
#define LOG_SLOTS 60 * 24
#define LOG_PERIOD_SECONDS 60
#define LOG_MAX_PAGE_SIZE 6
#define LOG_DEFAULT_PAGE_SIZE 1

// We evaluate the sensors and state every 5 seconds
#define EVALUATION_INTERVAL 5000

#define CHAMBER_SETTINGS_FILE "/config/chamberSettings.json"
#define CHAMBER_SETTINGS_SERVICE_PATH "/rest/chamberSettings"

#define CHAMBER_STATUS_SERVICE_PATH "/rest/chamberStatus"
#define LOG_DATA_SERVICE_PATH "/rest/logData"

#define YEAR_TWO_THOUSAND_UNIX_TIMESTAMP 946684800

#define STATUS_IDLE 0
#define STATUS_HEATING 1
#define STATUS_COOLING 2

struct ChamberLogEntry {
  unsigned long time;
  uint8_t status;
  float chamberTemp;
  float ambientTemp;
  float targetTemp;
};

class ChamberSettings {
 public:
  // configurable addresses for sensors
  DeviceAddress chamberSensorAddress;
  DeviceAddress ambientSensorAddress;

  // target temps and hysteresis thresholds
  float targetTemp;
  float hysteresisHigh;
  float hysteresisLow;
  float hysteresisFactor;

  // cycle limits
  unsigned long minHeaterOnDuration;
  unsigned long minHeaterOffDuration;
  unsigned long minCoolerOnDuration;
  unsigned long minCoolerOffDuration;

  // flags for enabling/disabling the device
  bool enableHeater;
  bool enableCooler;
};

class FermentationChamber : public SettingsService<ChamberSettings> {
 public:
  FermentationChamber(AsyncWebServer* server, FS* fs);
  ~FermentationChamber();

  void begin();
  void loop();

 protected:
  void onConfigUpdated();
  void readFromJsonObject(JsonObject& root);
  void writeToJsonObject(JsonObject& root);

 private:
  // sensor objects
  OneWire _ds = OneWire(TEMP_SENSOR_PIN);
  DallasTemperature _tempSensors = DallasTemperature(&_ds);
  CircularLog<ChamberLogEntry> _circularLog;

  // calculated off/on temps
  float _heaterOffTemp;
  float _coolerOffTemp;
  float _heaterOnTemp;
  float _coolerOnTemp;

  // status variables
  uint8_t _status = STATUS_IDLE;
  unsigned long _heaterToggledAt;
  unsigned long _coolerToggledAt;
  unsigned long _evaluatedAt;
  unsigned long _loggedAt;

  void performLogging();
  void evaluateChamberStatus();
  void chamberStatus(AsyncWebServerRequest* request);
  void logData(AsyncWebServerRequest* request);
  void prepareNextControllerLoop();
  void changeStatus(uint8_t newStatus, unsigned long* previousToggle, unsigned long* toggleLimitDuration);
  void transitionToStatus(uint8_t newStatus);
};

#endif  // end FermentationChamber_h