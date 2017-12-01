#include "SensorReadings.h"

void SensorReadings::serialize(uint8_t *dst) {

    #ifdef PAYLOAD_OFFICE
        data.payloadType = PAYLOAD_OFFICE;

        // default values
        data.isTemperaturePositive = true;
        data.temperature_f = UINT16_MAX;
        data.humidity_f = UINT16_MAX;
        data.dustConcentration_f = UINT32_MAX;
        data.lightResistance = UINT16_MAX;
        data.lightSensorValue = UINT16_MAX;
        data.loudness = UINT16_MAX;
        data.isPeopleDetected = false;
    #elif PAYLOAD_MOBILE
        data.payloadType = PAYLOAD_MOBILE;

        // default values
        data.isTemperaturePositive = true;
        data.temperature_f = UINT16_MAX;
        data.loudness = UINT16_MAX;
        data.pressure_f = UINT32_MAX;
        data.longitude = INT32_MAX;
        data.latitude = INT32_MAX;
    #endif

    memcpy(dst, &data, size());
}

void SensorReadings::deserialize(uint8_t *src, uint8_t size)
{
    memcpy(&data, src, size);
}

void SensorReadings::reset() {
    data.payloadType = 0;
    #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
        data.isTemperaturePositive = true;
        data.temperature_f = 0.0;
    #endif
    #ifdef TEMP_HUM
        data.humidity_f = 0.0;
    #endif
    #ifdef BARO
        data.pressure_f = 0;
    #endif
    #ifdef LIGHT
        data.lightSensorValue = 0;
        data.lightResistance = 0;
    #endif
    #if defined(SOUND) || defined(LOUDNESS)
        data.loudness = 0;
    #endif
    #ifdef DUST
        data.dustConcentration_f = 0.0;
    #endif
    #ifdef GPS
        data.longitude = 0;
        data.latitude = 0;
    #endif
    #ifdef PIR
        data.isPeopleDetected = false;
    #endif
    data.floatNormalizer = 0;
}

uint8_t SensorReadings::size() {
    return sizeof(data);
}

void SensorReadings::print() {
    uint8_t normalizer = data.floatNormalizer == 0
            ? 1
            : data.floatNormalizer;

    #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
        Serial.print(F("Temperature (*C): "));
        if (!data.isTemperaturePositive)
            Serial.print("-");
        Serial.println(data.temperature_f * 1.0 / normalizer);
    #endif
    #ifdef BARO
        Serial.print(F("Pressure (Pa): "));
        Serial.println(data.pressure_f * 1.0 / normalizer);
    #endif
    #ifdef TEMP_HUM
        Serial.print(F("Humidity (%): "));
        Serial.println(data.humidity_f * 1.0 / normalizer);
    #endif
    #ifdef DUST
        Serial.print(F("Dust concentration (pcs/0.01cf): "));
        Serial.println(data.dustConcentration_f * 1.0 / normalizer);
    #endif
    #ifdef LIGHT
        Serial.print(F("Light (raw): "));
        Serial.println(data.lightSensorValue);
        Serial.print(F("Light (resistance): "));
        Serial.println(data.lightResistance);
    #endif
    #if defined(SOUND) || defined(LOUDNESS)
        Serial.print(F("Loudness: "));
        Serial.println(data.loudness);
    #endif
    #ifdef GPS
        Serial.print(F("Longitude: "));
        Serial.println(data.longitude);
        Serial.print(F("Latitude: "));
        Serial.println(data.latitude);
    #endif
    #ifdef PIR
        Serial.print(F("People detected: "));
        Serial.println(data.isPeopleDetected);
    #endif
}
