/* Copyright (C) 2013 gsfan, MIT License
 *  port to the GainSpan Wi-FI module GS1011
 */

#include "GSwifiInterface.h"

GSwifiInterface::GSwifiInterface( PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm, int baud) :
    GSwifi(tx, rx, cts, rts, reset, alarm, baud)
{
}

int GSwifiInterface::init(const char* name)
{
    return setAddress(name);
}

int GSwifiInterface::init(const char* ip, const char* netmask, const char* gateway, const char* dns, const char* name)
{
    return setAddress(ip, netmask, gateway, dns, name);
}

int GSwifiInterface::connect(Security sec, const char* ssid, const char* phrase, WiFiMode mode)
{
    setSsid(sec, ssid, phrase);
    switch (mode) {
    case WM_INFRASTRUCTURE:
        return join();
    case WM_ADHOCK:
        return adhock();
    case WM_LIMITEDAP:
        return limitedap();
    }
    return -1;
}

int GSwifiInterface::disconnect()
{
    return GSwifi::dissociate();
}

char * GSwifiInterface::getMACAddress()
{
    return _state.mac;
}

char * GSwifiInterface::getIPAddress()
{
    return _state.ip;
}

char * GSwifiInterface::getGateway()
{
    return _state.gateway;
}

char * GSwifiInterface::getNetworkMask()
{
    return _state.netmask;
}

