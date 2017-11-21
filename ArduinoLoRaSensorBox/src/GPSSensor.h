#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include <SoftwareSerial.h>
#include <TinyGPS.h>

/**
 * With an instance of GPS one is able to calculate longitude and latitude of this instance. Before lon and lat can be retrieved, `GPS::calculatePosition()` must be called.
 */
class GPSSensor
{
public:
    GPSSensor(TinyGPS * gps, SoftwareSerial * sose);
    void init();
    void loop();
    void calculatePosition();
    int32_t getLongitude();
    int32_t getLatitude();
private:
    TinyGPS *_gps;
    SoftwareSerial *_sose;
    int32_t _longitude;
    int32_t _latitude;
};

#endif
