#include "SensorReadings.h"

void SensorReadings::serialize(uint8_t *dst) {

    uint8_t idx = 0;

    dst[idx++] = data.payloadType;

    #if defined(TEMP_HUM) || defined(BARO)
        dst[idx++] = data.temperature_f;
        dst[idx++] = data.temperature_f >> 8;
    #endif

    #ifdef TEMP_HUM
        dst[idx++] = data.humidity_f;
        dst[idx++] = data.humidity_f >> 8;
    #endif

    #ifdef DUST
        dst[idx++] = data.dustConcentration_f;
        dst[idx++] = data.dustConcentration_f >> 8;
        dst[idx++] = data.dustConcentration_f >> 16;
        dst[idx++] = data.dustConcentration_f >> 24;
    #endif

    #ifdef LIGHT
        dst[idx++] = data.lightSensorValue;
        dst[idx++] = data.lightSensorValue >> 8;
        dst[idx++] = data.lightResistance;
        dst[idx++] = data.lightResistance >> 8;
    #endif

    #ifdef SOUND
        dst[idx++] = data.loudness;
        dst[idx++] = data.loudness >> 8;
    #endif

    #ifdef BARO
        dst[idx++] = data.pressure;
        dst[idx++] = data.pressure >> 8;
        dst[idx++] = data.pressure >> 16;
        dst[idx++] = data.pressure >> 24;
    #endif
    #ifdef GPS
        dst[idx++] = data.longitude;
        dst[idx++] = data.longitude >> 8;
        dst[idx++] = data.longitude >> 16;
        dst[idx++] = data.longitude >> 24;

        dst[idx++] = data.latitude;
        dst[idx++] = data.latitude >> 8;
        dst[idx++] = data.latitude >> 16;
        dst[idx++] = data.latitude >> 24;
    #endif
    dst[idx++] = data.floatNormalizer;
}

void SensorReadings::deserialize(uint8_t *src, uint8_t size)
{
    uint8_t idx = 0;

    data.payloadType = src[idx++];

    #if defined(TEMP_HUM) || defined(BARO)
        data.temperature_f = src[idx++];
        data.temperature_f |= src[idx++] << 8;
    #endif

    #ifdef TEMP_HUM
        data.humidity_f = src[idx++];
        data.humidity_f |= src[idx++] << 8;
    #endif

    #ifdef DUST
        data.dustConcentration_f = src[idx++];
        data.dustConcentration_f |= src[idx++] << 8;
        data.dustConcentration_f |= ((uint32_t) src[idx++]) << 16;
        data.dustConcentration_f |= ((uint32_t) src[idx++]) << 24;
    #endif

    #ifdef LIGHT
        data.lightSensorValue = src[idx++];
        data.lightSensorValue |= src[idx++] << 8;
        data.lightResistance = src[idx++];
        data.lightResistance |= src[idx++] << 8;
    #endif

    #ifdef SOUND
        data.loudness = src[idx++];
        data.loudness |= src[idx++] << 8;
    #endif

    #ifdef BARO
        data.pressure = src[idx++];
        data.pressure |= src[idx++] << 8;
        data.pressure |= ((uint32_t) src[idx++]) << 16;
        data.pressure |= ((uint32_t) src[idx++]) << 24;
    #endif
    #ifdef GPS
        data.longitude = src[idx++];
        data.longitude |= src[idx++] << 8;
        data.longitude |= ((uint32_t) src[idx++]) << 16;
        data.longitude |= ((uint32_t) src[idx++]) << 24;

        data.latitude = src[idx++];
        data.latitude |= src[idx++] << 8;
        data.latitude |= ((int32_t) src[idx++]) << 16;
        data.latitude |= ((int32_t) src[idx++]) << 24;
    #endif
    data.floatNormalizer = src[idx++];
}

void SensorReadings::reset() {
    data.payloadType = 0;
    #if defined(TEMP_HUM) || defined(BARO)
        data.temperature_f = 0.0;
    #endif
    #ifdef TEMP_HUM
        data.humidity_f = 0.0;
    #endif
    #ifdef BARO
        data.pressure = 0;
    #endif
    #ifdef LIGHT
        data.lightSensorValue = 0;
        data.lightResistance = 0;
    #endif
    #ifdef SOUND
        data.loudness = 0;
    #endif
    #ifdef DUST
        data.dustConcentration_f = 0.0;
    #endif
    #ifdef GPS
        data.longitude = 0;
        data.latitude = 0;
    #endif
    data.floatNormalizer = 0;
}

uint8_t SensorReadings::size() {
    return sizeof(SensorData);
}

void SensorReadings::print() {
    uint8_t normalizer = data.floatNormalizer == 0
            ? 1
            : data.floatNormalizer;

    #if defined(TEMP_HUM) || defined(BARO)
        Serial.print(F("Temperature (*C): "));
        Serial.println(data.temperature_f * 1.0 / normalizer);
    #endif
    #ifdef BARO
        Serial.print(F("Pressure (Pa): "));
        Serial.println(data.pressure * 1.0 / normalizer);
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
    #ifdef SOUND
        Serial.print(F("Loudness: "));
        Serial.println(data.loudness);
    #endif
    #ifdef GPS
        Serial.print(F("Longitude: "));
        Serial.println(data.longitude);
        Serial.print(F("Latitude: "));
        Serial.println(data.latitude);
    #endif
}
