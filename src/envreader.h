#pragma once

#include <ScioSense_ENS16x.h>
#include <Wire.h>

#include "AHTxx.h"
#include "ens16x_i2c_interface.h"

using namespace ScioSense;

#define I2C_SDA 22
#define I2C_SCL 27

#define I2C_ADDRESS 0x53
// #define USE_INTERRUPT
// #define INTN 2

class EnvReader {
   protected:
    I2cInterface i2c;
    ENS160 ens160;
    AHTxx aht21 = AHTxx(AHTXX_ADDRESS_X38, AHT2x_SENSOR);
    unsigned long t0 = 0;


   public:
    bool valid = false;
    int counter = 0;

    float temperature = 20;
    float humidity = 20;
    ENS16x::AirQualityIndex_UBA aqi_uba = ENS16x::AirQualityIndex_UBA::Unknown;
    uint16_t tvoc = 0;
    uint16_t eco2 = 0;

    EnvReader() {}

    void init() {
        Wire.begin(I2C_SDA, I2C_SCL);
        i2c.begin(Wire, I2C_ADDRESS);

        while (ens160.begin(&i2c) != true) {
            Serial.println(F("ens160 not connected"));
            delay(3000);
        }

        while (aht21.begin() != true) {
            Serial.println(
                F("AHT2x not connected or fail to load calibration "
                  "coefficient"));  //(F()) save string to flash & keeps
                                    // dynamic memory free
            delay(3000);
        }

#ifdef USE_INTERRUPT
        ens160.setInterruptPin(INTN);
        ens160.writeConfiguration(ENS160::Configuration::InterruptEnable |
                                  ENS160::Configuration::NewGeneralPurposeData |
                                  ENS160::Configuration::NewData);
#endif

        ens160.startStandardMeasure();
    }

    void tick() {
        unsigned long t1 = millis();
        long delta = t1 - t0;

        if (delta < 0) {
            delta = 0xFFFFFFFFFFFFFFFF - t0 + t1;
        }

        if (delta < 10000) {
            return;
        }

        t0 = t1;
        valid = false;

        Serial.println(F("sample env"));

        bool reset_aht21 = false;
        temperature = aht21.readTemperature();

        if (temperature != AHTXX_ERROR) {
            Serial.print(temperature);
            Serial.println(F(" +-0.3C"));

            humidity = aht21.readHumidity(AHTXX_USE_READ_DATA);

            if (humidity != AHTXX_ERROR) {
                Serial.print(humidity);
                Serial.println(F(" +-2%"));
            } else {
                reset_aht21 = true;
            }
        } else {
            reset_aht21 = true;
        }

        if (reset_aht21) {
            delay(2000);
            printStatus();
            if (aht21.softReset())
                Serial.println(F("aht21 reset success"));
            else
                Serial.println(F("aht21 reset failed"));
            return;
        }

        ens160.writeCompensation(round((temperature + 273.15) * 64),
                                 round(humidity * 512));

        //ens160.wait();

        if (ens160.update() == ENS16x::Result::Ok) {
            if (hasFlag(ens160.getDeviceStatus(),
                        ENS16x::DeviceStatus::NewData)) {
                aqi_uba = ens160.getAirQualityIndex_UBA();
                tvoc = ens160.getTvoc();
                eco2 = ens160.getEco2();
                valid = true;
                counter++;

                Serial.print("AQI UBA:");
                Serial.print((int)aqi_uba);
                Serial.print("\tTVOC:");
                Serial.print(tvoc);
                Serial.print("\tECO2:");
                Serial.println(eco2);
            }
        }
    }

    void printStatus() {
        switch (aht21.getStatus()) {
            case AHTXX_NO_ERROR:
                Serial.println(F("no error"));
                break;

            case AHTXX_BUSY_ERROR:
                Serial.println(F("sensor busy, increase polling time"));
                break;

            case AHTXX_ACK_ERROR:
                Serial.println(
                    F("sensor didn't return ACK, not connected, broken, long "
                      "wires "
                      "(reduce speed), bus locked by slave (increase stretch "
                      "limit)"));
                break;

            case AHTXX_DATA_ERROR:
                Serial.println(F(
                    "received data smaller than expected, not connected, "
                    "broken, "
                    "long wires (reduce speed), bus locked by slave (increase "
                    "stretch limit)"));
                break;

            case AHTXX_CRC8_ERROR:
                Serial.println(
                    F("computed CRC8 not match received CRC8, this feature "
                      "supported only by AHT2x sensors"));
                break;

            default:
                Serial.println(F("unknown status"));
                break;
        }
    }

    char *aqi() {
        switch (aqi_uba) {
            case ENS16x::AirQualityIndex_UBA::Excellent:
                return (char*)F("Excellent");
            case ENS16x::AirQualityIndex_UBA::Good:
                return (char*)F("Good");
            case ENS16x::AirQualityIndex_UBA::Moderate:
                return (char*)F("Moderate");
            case ENS16x::AirQualityIndex_UBA::Poor:
                return (char*)F("Poor");
            case ENS16x::AirQualityIndex_UBA::Unhealthy:
                return (char*)F("Unhealthy");
            default:
                return (char*)F("Unknown");
        }
    }
};
