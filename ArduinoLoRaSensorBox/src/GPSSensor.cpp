#include "GPSSensor.h"

GPSSensor::GPSSensor(TinyGPS * gps, SoftwareSerial * sose)
{
    _gps = gps;
    _sose = sose;
}

void GPSSensor::init()
{
    _sose->begin(9600);
}

void GPSSensor::loop()
{
    while (_sose->available())
    {
        _gps->encode(_sose->read());
    }
}

void GPSSensor::calculatePosition()
{
    _gps->get_position(&_latitude, &_longitude);
}

int32_t GPSSensor::getLongitude()
{
    return _longitude;
}

int32_t GPSSensor::getLatitude()
{
    return _latitude;
}
