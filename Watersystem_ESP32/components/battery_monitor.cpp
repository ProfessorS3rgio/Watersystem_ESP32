/**
 * Battery Monitor for ESP32
 * Copyright (c) 2021 clausgf@github. See LICENSE.md for legal information.
 */

#include <esp32-hal-adc.h>
#include <Adafruit_MCP23X17.h>
#include <math.h>

#include "battery_monitor.h"

extern Adafruit_MCP23X17 mcp;


// ***************************************************************************

BatteryMonitor::BatteryMonitor(int pin,
    int voltageFactor_perMille, int voltageBias_mV, 
    int noOfMeasurements, 
    int minVoltage_mV, int maxVoltage_mV,
    int chargingPin_mcp)
{
    _pin = pin;
    _voltageFactor_perMille = voltageFactor_perMille;
    _voltageBias_mV = voltageBias_mV;
    _noOfMeasurements = noOfMeasurements;
    _minVoltage_mV = minVoltage_mV;
    _maxVoltage_mV = maxVoltage_mV;
    _chargingPin_mcp = chargingPin_mcp;
    _voltage_mV = -1;
    _adc_mV = -1;
    _lastPercentage = 0;
    _hasLastPercentage = false;
    _notChargingCount = 0;
};

BatteryMonitor& BatteryMonitor::measure()
{
    analogReadResolution(_bits);  // 11 bit is alldgedly more linear than 12 bit
    analogSetAttenuation(ADC_11db);

    uint64_t sum = 0;
    for (int i=0; i<_noOfMeasurements; i++)
    {
        sum += analogReadMilliVolts(_pin);  // requires a newer IDF/Arduino (tested with espressif32@3.2)
    }

    const int adc_mV = sum / _noOfMeasurements;
    _adc_mV = adc_mV;
    _voltage_mV = (adc_mV * _voltageFactor_perMille) / 1000 + _voltageBias_mV;

    return *this;
}

int BatteryMonitor::getVoltage_mV()
{
    if (_voltage_mV < 0)
    {
        measure();
    }
    return _voltage_mV;
}

int BatteryMonitor::getPercentage()
{
    const int voltage_mV = getVoltage_mV();
    int rawPercentage = 0;

    // If user keeps the default Li-ion window, apply a more realistic nonlinear
    // OCV -> SOC curve. For custom min/max settings, preserve old linear behavior.
    if (_minVoltage_mV == 3400 && _maxVoltage_mV == 4200)
    {
        rawPercentage = mapVoltageToSocCurve(voltage_mV);
    }
    else
    {
        if (voltage_mV > _maxVoltage_mV)
        {
            rawPercentage = 100;
        }
        else if (voltage_mV < _minVoltage_mV)
        {
            rawPercentage = 0;
        }
        else
        {
            rawPercentage = (voltage_mV - _minVoltage_mV) * 100 / (_maxVoltage_mV - _minVoltage_mV);
        }
    }

    if (!_hasLastPercentage)
    {
        _lastPercentage = rawPercentage;
        _hasLastPercentage = true;
        return _lastPercentage;
    }

    const bool charging = isCharging();
    if (charging)
    {
        _notChargingCount = 0;
    }
    else if (_notChargingCount < 255)
    {
        _notChargingCount++;
    }

    int filteredPercentage = rawPercentage;

    if (charging)
    {
        // While charging, do not allow percentage to drop due to ADC noise.
        if (filteredPercentage < _lastPercentage)
        {
            filteredPercentage = _lastPercentage;
        }
    }
    else
    {
        // While not charging, percentage must be monotonic non-increasing.
        if (filteredPercentage > _lastPercentage)
        {
            filteredPercentage = _lastPercentage;
        }

        // Limit discharge to at most 1% per update to avoid sudden dips.
        if (filteredPercentage < _lastPercentage - 1)
        {
            filteredPercentage = _lastPercentage - 1;
        }
    }

    if (filteredPercentage < 0)
    {
        filteredPercentage = 0;
    }
    if (filteredPercentage > 100)
    {
        filteredPercentage = 100;
    }

    _lastPercentage = filteredPercentage;
    return _lastPercentage;
}

int BatteryMonitor::mapVoltageToSocCurve(int voltage_mV) const
{
    struct Point
    {
        int mV;
        int soc;
    };

    // Realistic 1-cell Li-ion resting-voltage SOC points.
    // Note: under load, effective SOC appears lower; while charging, a bit higher.
    static const Point curve[] = {
        {4200, 100},
        {4000, 85},
        {3850, 65},
        {3750, 55},
        {3700, 45},
        {3600, 35},
        {3500, 20},
        {3400, 10},
        {3300, 5},
        {3200, 0}
    };

    const int n = sizeof(curve) / sizeof(curve[0]);

    if (voltage_mV >= curve[0].mV)
    {
        return 100;
    }
    if (voltage_mV <= curve[n - 1].mV)
    {
        return 0;
    }

    for (int i = 0; i < n - 1; i++)
    {
        const int highV = curve[i].mV;
        const int lowV = curve[i + 1].mV;
        if (voltage_mV <= highV && voltage_mV >= lowV)
        {
            const int highSoc = curve[i].soc;
            const int lowSoc = curve[i + 1].soc;
            const float slope = (float)(highSoc - lowSoc) / (float)(highV - lowV);
            const float soc = (float)lowSoc + slope * (float)(voltage_mV - lowV);
            return (int)lroundf(soc);
        }
    }

    return 0;
}

bool BatteryMonitor::isCharging()
{
    if (_chargingPin_mcp == -1) {
        return false;  // Not configured
    }
    // Active high: high = charging
    return mcp.digitalRead(_chargingPin_mcp);
}

int BatteryMonitor::getAdc_mV()
{
    if (_voltage_mV < 0)
    {
        measure();
    }
    return _adc_mV;
}