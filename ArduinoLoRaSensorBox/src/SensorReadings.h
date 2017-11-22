#ifndef SensorReadings_H
#define SensorReadings_H

#include <Arduino.h>

/**
 * Container struct that stores sensor data. These values are not kept inside the sensor readings class so no errors occur when calculating sizes of packet data (`sizeof(SensorData)`).
 */
typedef struct {

    /** payload/message type that an aggregator uses to read a message */
    uint8_t payloadType;
    #ifdef DUST
        /** normalized dust concentration (conc * floatNormalizer) from \ref DustCalculator */
        uint32_t dustConcentration_f;
    #endif
    #if defined(TEMP_HUM) || defined(BARO) || defined(TEMPERATURE)
        /** normalized temperature (temp * floatNormalizer) \ref TemperatureHumiditySensor */
        uint16_t temperature_f;
    #endif
    #ifdef TEMP_HUM
        /** normalized humidity (humidity * floatNormalizer) */
        uint16_t humidity_f;
    #endif
    #ifdef BARO
        /** normalized pressure in Pa */
        uint32_t pressure_f;
    #endif
    #ifdef LIGHT
        /** Raw value of light sensor \ref LightSensor */
        uint16_t lightSensorValue;
        /** Calculated resistance value from \ref LightSensor */
        uint16_t lightResistance;
    #endif
    #if defined(SOUND) || defined(LOUDNESS)
        /** Loudness calculated in \ref SoundSensor */
        uint16_t loudness;
    #endif
    #ifdef GPS
        int32_t longitude;
        int32_t latitude;
    #endif
    #ifdef PIR
        bool isPeopleDetected;
    #endif
    /** Value to denormalize sensor data */
    uint8_t floatNormalizer;
} SensorData;

/**
 * An instance of sensor readings should be used to save and serialize senor data that should be send via radio.
 */
class SensorReadings {
public:
    /** data container */
    SensorData data;

    /**
     * Serialize data by the use of little endian.
     * @param dst destination to write to
     */
    void serialize(uint8_t *dst);
    /**
     * Deserialize data by the use of little endian.
     * @param src  source to read from
     * @param size length of the source
     */
    void deserialize(uint8_t *src, uint8_t size);
    /** Returns the size that is used be the data */
    uint8_t size();
    /** Reset all data to default values */
    void reset();
    /** Print instance data onto the serial output */
    void print();
};

#endif
