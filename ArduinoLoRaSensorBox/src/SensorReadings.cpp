#include "SensorReadings.h"

void SensorReadings::serialize(uint32_t value, uint8_t * dst, uint8_t * idx, uint8_t size)
{
    uint8_t i = 0;
    do {
        dst[*idx] = value;
        value = value >> 8;
        *idx += 1;
        i += 1;
    } while (i < size);
}

void SensorReadings::serializePayloadOffice(uint8_t * dst, uint8_t * idx)
{
    #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
        serialize(data.isTemperaturePositive, dst, idx, sizeof(data.isTemperaturePositive));
        serialize(data.temperature_f, dst, idx, sizeof(data.temperature_f));
    #else
        serialize(UINT32_MAX, dst, idx, 3);
    #endif

    #ifdef TEMP_HUM
        serialize(data.humidity_f, dst, idx, sizeof(data.humidity_f));
    #else
        serialize(UINT32_MAX, dst, idx, 2);
    #endif

    #ifdef DUST
        uint32_t dc = data.dustConcentration_f > 0
                    ? data.dustConcentration_f
                    : UINT32_MAX;

        serialize(dc, dst, idx, sizeof(data.dustConcentration_f));
    #endif

    #ifdef LIGHT
        serialize(data.lightSensorValue, dst, idx, sizeof(data.lightSensorValue));
        serialize(data.lightResistance, dst, idx, sizeof(data.lightResistance));
    #else
        serialize(UINT32_MAX, dst, idx, 4);
    #endif

    #if defined(LOUDNESS) || defined(SOUND)
        serialize(data.loudness, dst, idx, sizeof(data.loudness));
    #else
        serialize(UINT32_MAX, dst, idx, 2);
    #endif

    #ifdef PIR
        serialize(data.isPeopleDetected, dst, idx, sizeof(data.isPeopleDetected));
    #else
        serialize(UINT32_MAX, dst, idx, 1);
    #endif
}

void SensorReadings::serializePayloadMobile(uint8_t * dst, uint8_t * idx)
{
    #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
        serialize(data.isTemperaturePositive, dst, idx, sizeof(data.isTemperaturePositive));
        serialize(data.temperature_f, dst, idx, sizeof(data.temperature_f));
    #else
        serialize(UINT32_MAX, dst, idx, sizeof(uint16_t));
    #endif

    #if defined(LOUDNESS) || defined(SOUND)
        serialize(data.loudness, dst, idx, sizeof(data.loudness));
    #else
        serialize(UINT32_MAX, dst, idx, 2);
    #endif

    #ifdef BARO
        serialize(data.pressure_f, dst, idx, sizeof(data.pressure_f));
    #else
        serialize(UINT32_MAX, dst, idx, 4);
    #endif

    #ifdef GPS
        serialize(data.longitude, dst, idx, sizeof(data.longitude));
        serialize(data.latitude, dst, idx, sizeof(data.latitude));
    #else
        serialize(UINT32_MAX, dst, idx, 8);
    #endif
}

void SensorReadings::serialize(uint8_t *dst) {

    uint8_t idx = 0;

    #ifdef PAYLOAD_OFFICE
        data.payloadType = PayloadTypeOffice;
    #elif PAYLOAD_MOBILE
        data.payloadType = PayloadTypeMobile;
    #endif

    serialize(data.payloadType, dst, &idx, sizeof(data.payloadType));

    #ifdef PAYLOAD_OFFICE
        serializePayloadOffice(dst, &idx);
    #elif PAYLOAD_MOBILE
        serializePayloadMobile(dst, &idx);
    #endif

    serialize(data.floatNormalizer, dst, &idx, sizeof(data.floatNormalizer));
}

void SensorReadings::deserialize(uint8_t *src, uint8_t size)
{
    uint8_t idx = 0;

    data.payloadType = src[idx++];

    #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
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

    #if defined(SOUND) || defined(LOUDNESS)
        data.loudness = src[idx++];
        data.loudness |= src[idx++] << 8;
    #endif

    #ifdef BARO
        data.pressure_f = src[idx++];
        data.pressure_f |= src[idx++] << 8;
        data.pressure_f |= ((uint32_t) src[idx++]) << 16;
        data.pressure_f |= ((uint32_t) src[idx++]) << 24;
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
    #ifdef PIR
        data.isPeopleDetected = src[idx++];
    #endif
    data.floatNormalizer = src[idx++];
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
    uint8_t size = sizeof(data.payloadType);        // 1
    size += sizeof(data.floatNormalizer);           // 1

    #ifdef PAYLOAD_OFFICE
        #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
            size += sizeof(data.isTemperaturePositive); // 1
            size += sizeof(data.temperature_f);         // 2
        #endif
        #ifdef DUST
            size += data.dustConcentration_f > 0
                    ? sizeof(data.dustConcentration_f)  // 4
                    : 0;
        #endif
        #ifdef TEMP_HUM
            size += sizeof(data.humidity_f);            // 2
        #endif
        #ifdef LIGHT
            size += sizeof(data.lightSensorValue);      // 2
            size += sizeof(data.lightResistance);       // 2
        #endif
        #if defined(SOUND) || defined(LOUDNESS)
            size += sizeof(data.loudness);              // 2
        #endif
        #ifdef PIR
            size += sizeof(data.isPeopleDetected);      // 1
        #endif
    #elif PAYLOAD_MOBILE
        #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
            size += sizeof(data.isTemperaturePositive); // 1
            size += sizeof(data.temperature_f);         // 2
        #endif
        #ifdef BARO
            size += sizeof(data.pressure_f);            // 4
        #endif
        #ifdef GPS
            size += sizeof(data.longitude);             // 4
            size += sizeof(data.latitude);              // 4
        #endif
        #if defined(SOUND) || defined(LOUDNESS)
            size += sizeof(data.loudness);              // 2
        #endif
    #endif

    return size;
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
