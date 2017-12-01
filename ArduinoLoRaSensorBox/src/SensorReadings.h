#ifndef SensorReadings_H
#define SensorReadings_H

#include <Arduino.h>

/**
 * Container struct that stores sensor data. These values are not kept inside the sensor readings class so no errors occur when calculating sizes of packet data (`sizeof(SensorData)`).
 */
typedef struct {

    /** payload/message type that an aggregator uses to read a message */
    uint8_t payloadType;
    #ifdef PAYLOAD_OFFICE
        // true if temperature is positive, false otherwise
        bool isTemperaturePositive;
        /** normalized temperature (temp * floatNormalizer) */
        uint16_t temperature_f;
        /** normalized humidity (humidity * floatNormalizer) */
        uint16_t humidity_f;
        /** normalized dust concentration (conc * floatNormalizer) from \ref DustCalculator */
        uint32_t dustConcentration_f;
        /** Calculated resistance value from \ref LightSensor */
        uint16_t lightResistance;
        /** Raw value of light sensor \ref LightSensor */
        uint16_t lightSensorValue;
        /** Loudness calculated in \ref SoundSensor */
        uint16_t loudness;
        /** true if people where detected */
        bool isPeopleDetected;
    #elif PAYLOAD_MOBILE
        // true if temperature is positive, false otherwise
        bool isTemperaturePositive;
        /** normalized temperature (temp * floatNormalizer) */
        uint16_t temperature_f;
        /** Loudness calculated in \ref SoundSensor */
        uint16_t loudness;
        /** normalized pressure in Pa */
        uint32_t pressure_f;
        int32_t longitude;
        int32_t latitude;
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
