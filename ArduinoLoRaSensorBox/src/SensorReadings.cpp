#include "SensorReadings.h"

void SensorReadings::serialize(uint8_t *dst) {

    uint8_t idx = 0;

    dst[idx++] = temperature_f;
    dst[idx++] = temperature_f >> 8;

    dst[idx++] = humidity_f;
    dst[idx++] = humidity_f >> 8;

    if (dustConcentration_f > 0) {

        dst[idx++] = dustConcentration_f;
        dst[idx++] = dustConcentration_f >> 8;
        dst[idx++] = dustConcentration_f >> 16;
        dst[idx++] = dustConcentration_f >> 24;
    }

    dst[idx++] = lightSensorValue;
    dst[idx++] = lightSensorValue >> 8;

    dst[idx++] = lightResistance;
    dst[idx++] = lightResistance >> 8;

    dst[idx++] = loudness;
    dst[idx++] = loudness >> 8;

    dst[idx++] = floatNormalizer;
}

void SensorReadings::reset() {
    temperature_f = 0.0;
    humidity_f = 0.0;
    dustConcentration_f = 0.0;
    lightSensorValue = 0;
    lightResistance = 0;
    loudness = 0;
    floatNormalizer = 0;
}

uint16_t SensorReadings::size() {
    uint16_t size = sizeof(SensorReadings);
    return dustConcentration_f > 0
            ? size
            : size - sizeof(dustConcentration_f);
}

void SensorReadings::print() {
    uint8_t normalizer = floatNormalizer == 0
            ? 1
            : floatNormalizer;

    Serial.print(F("Temperature (*C): "));
    Serial.println(temperature_f * 1.0 / normalizer);
    Serial.print(F("Humidity (%): "));
    Serial.println(humidity_f * 1.0 / normalizer);
    Serial.print(F("Dust concentration (pcs/0.01cf): "));
    Serial.println(dustConcentration_f * 1.0 / normalizer);
    Serial.print(F("Light (raw): "));
    Serial.println(lightSensorValue);
    Serial.print(F("Light (resistance): "));
    Serial.println(lightResistance);
    Serial.print(F("Loudness: "));
    Serial.println(loudness);
}
