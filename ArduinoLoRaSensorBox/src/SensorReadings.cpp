#include "SensorReadings.h"

void SensorReadings::serialize(uint8_t *dst) {

    uint8_t idx = 0;

    dst[idx++] = data.payloadType;

    dst[idx++] = data.temperature_f;
    dst[idx++] = data.temperature_f >> 8;

    dst[idx++] = data.humidity_f;
    dst[idx++] = data.humidity_f >> 8;

    if (data.dustConcentration_f > 0) {

        dst[idx++] = data.dustConcentration_f;
        dst[idx++] = data.dustConcentration_f >> 8;
        dst[idx++] = data.dustConcentration_f >> 16;
        dst[idx++] = data.dustConcentration_f >> 24;
    }

    dst[idx++] = data.lightSensorValue;
    dst[idx++] = data.lightSensorValue >> 8;

    dst[idx++] = data.lightResistance;
    dst[idx++] = data.lightResistance >> 8;

    dst[idx++] = data.loudness;
    dst[idx++] = data.loudness >> 8;

    dst[idx++] = data.floatNormalizer;
}

void SensorReadings::deserialize(uint8_t *src, uint8_t size) {

    uint8_t full_size = sizeof(SensorData);
    uint8_t idx = 0;

    data.payloadType = src[idx++];

    data.temperature_f = src[idx++];
    data.temperature_f |= src[idx++] << 8;

    data.humidity_f = src[idx++];
    data.humidity_f |= src[idx++] << 8;

    if (size == full_size) {

        data.dustConcentration_f = src[idx++];
        data.dustConcentration_f |= src[idx++] << 8;
        data.dustConcentration_f |= ((uint32_t) src[idx++]) << 16;
        data.dustConcentration_f |= ((uint32_t) src[idx++]) << 24;
    }

    data.lightSensorValue = src[idx++];
    data.lightSensorValue |= src[idx++] << 8;

    data.lightResistance = src[idx++];
    data.lightResistance |= src[idx++] << 8;

    data.loudness = src[idx++];
    data.loudness |= src[idx++] << 8;

    data.floatNormalizer = src[idx++];
}

void SensorReadings::reset() {
    data.payloadType = 0;
    data.temperature_f = 0.0;
    data.humidity_f = 0.0;
    data.dustConcentration_f = 0.0;
    data.lightSensorValue = 0;
    data.lightResistance = 0;
    data.loudness = 0;
    data.floatNormalizer = 0;
}

uint8_t SensorReadings::size() {
    uint8_t size = sizeof(SensorData);
    return data.dustConcentration_f > 0
            ? size
            : size - sizeof(data.dustConcentration_f);
}

void SensorReadings::print() {
    uint8_t normalizer = data.floatNormalizer == 0
            ? 1
            : data.floatNormalizer;

    Serial.print(F("Temperature (*C): "));
    Serial.println(data.temperature_f * 1.0 / normalizer);
    Serial.print(F("Humidity (%): "));
    Serial.println(data.humidity_f * 1.0 / normalizer);
    Serial.print(F("Dust concentration (pcs/0.01cf): "));
    Serial.println(data.dustConcentration_f * 1.0 / normalizer);
    Serial.print(F("Light (raw): "));
    Serial.println(data.lightSensorValue);
    Serial.print(F("Light (resistance): "));
    Serial.println(data.lightResistance);
    Serial.print(F("Loudness: "));
    Serial.println(data.loudness);
}
