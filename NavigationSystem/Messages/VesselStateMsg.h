/****************************************************************************************
 *
 * File:
 * 		VesselStateMsg.h
 *
 * Purpose:
 *		A VesselStateMsg contains the state of the vessel at a given time.
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#pragma once

#include "MessageBus/Message.h"

class VesselStateMsg : public Message {
   public:
    VesselStateMsg(NodeID destinationID,
                   NodeID sourceID,
                   int compassHeading,
                   int compassPitch,
                   int compassRoll,
                   bool gpsFix,
                   bool gpsOnline,
                   double lat,
                   double lon,
                   double unixTime,
                   double gpsSpeed,
                   int gpsSatellite,
                   double heading,
                   float windDir,
                   float windSpeed,
                   float windTemp,
                   int arduinoPressure,
                   int arduinoRudder,
                   int arduinoSheet,
                   int arduinoBattery)
        : Message(MessageType::VesselState, sourceID, destinationID),
          m_CompassHeading(compassHeading),
          m_CompassPitch(compassPitch),
          m_CompassRoll(compassRoll),
          m_GPSHasFix(gpsFix),
          m_GPSOnline(gpsOnline),
          m_GPSLat(lat),
          m_GPSLon(lon),
          m_GPSUnixTime(unixTime),
          m_GPSSpeed(gpsSpeed),
          m_GPSCourse(heading),
          m_GPSSatellite(gpsSatellite),
          m_WindDir(windDir),
          m_WindSpeed(windSpeed),
          m_WindTemp(windTemp),
          m_ArduinoPressure(arduinoPressure),
          m_ArduinoRudder(arduinoRudder),
          m_ArduinoSheet(arduinoSheet),
          m_ArduinoBattery(arduinoBattery) {}

    VesselStateMsg(int compassHeading,
                   int compassPitch,
                   int compassRoll,
                   bool gpsFix,
                   bool gpsOnline,
                   double lat,
                   double lon,
                   double unixTime,
                   double gpsSpeed,
                   int gpsSatellite,
                   double heading,
                   float windDir,
                   float windSpeed,
                   float windTemp,
                   int arduinoPressure,
                   int arduinoRudder,
                   int arduinoSheet,
                   int arduinoBattery)
        : Message(MessageType::VesselState, NodeID::None, NodeID::None),
          m_CompassHeading(compassHeading),
          m_CompassPitch(compassPitch),
          m_CompassRoll(compassRoll),
          m_GPSHasFix(gpsFix),
          m_GPSOnline(gpsOnline),
          m_GPSLat(lat),
          m_GPSLon(lon),
          m_GPSUnixTime(unixTime),
          m_GPSSpeed(gpsSpeed),
          m_GPSCourse(heading),
          m_GPSSatellite(gpsSatellite),
          m_WindDir(windDir),
          m_WindSpeed(windSpeed),
          m_WindTemp(windTemp),
          m_ArduinoPressure(arduinoPressure),
          m_ArduinoRudder(arduinoRudder),
          m_ArduinoSheet(arduinoSheet),
          m_ArduinoBattery(arduinoBattery) {}

    VesselStateMsg(MessageDeserialiser deserialiser) : Message(deserialiser) {
        if (!deserialiser.readInt(m_CompassHeading) || !deserialiser.readInt(m_CompassPitch) ||
            !deserialiser.readInt(m_CompassRoll) || !deserialiser.readBool(m_GPSHasFix) ||
            !deserialiser.readBool(m_GPSOnline) || !deserialiser.readDouble(m_GPSLat) ||
            !deserialiser.readDouble(m_GPSLon) || !deserialiser.readDouble(m_GPSUnixTime) ||
            !deserialiser.readDouble(m_GPSSpeed) || !deserialiser.readDouble(m_GPSCourse) ||
            !deserialiser.readInt(m_GPSSatellite) || !deserialiser.readFloat(m_WindDir) ||
            !deserialiser.readFloat(m_WindSpeed) || !deserialiser.readFloat(m_WindTemp) ||
            !deserialiser.readInt(m_ArduinoPressure) || !deserialiser.readInt(m_ArduinoRudder) ||
            !deserialiser.readInt(m_ArduinoSheet) || !deserialiser.readInt(m_ArduinoBattery)) {
            m_valid = false;
        }
    }

    virtual ~VesselStateMsg() {}

    int compassHeading() { return m_CompassHeading; }
    int compassPitch() { return m_CompassPitch; }
    int compassRoll() { return m_CompassRoll; }

    bool gpsHasFix() { return m_GPSHasFix; }
    bool gpsOnline() { return m_GPSOnline; }
    double latitude() { return m_GPSLat; }
    double longitude() { return m_GPSLon; }
    double unixTime() { return m_GPSUnixTime; }
    double speed() { return m_GPSSpeed; }
    double gpsHeading() { return m_GPSCourse; }
    int gpsSatellite() { return m_GPSSatellite; }

    float windDir() { return m_WindDir; }
    float windSpeed() { return m_WindSpeed; }
    float windTemp() { return m_WindTemp; }

    int arduinoPressure() { return m_ArduinoPressure; }
    int arduinoRudder() { return m_ArduinoRudder; }
    int arduinoSheet() { return m_ArduinoSheet; }
    int arduinoBattery() { return m_ArduinoBattery; }

    ///----------------------------------------------------------------------------------
    /// Serialises the message into a MessageSerialiser
    ///----------------------------------------------------------------------------------
    virtual void Serialise(MessageSerialiser& serialiser) const {
        Message::Serialise(serialiser);

        serialiser.serialise(m_CompassHeading);
        serialiser.serialise(m_CompassPitch);
        serialiser.serialise(m_CompassRoll);
        serialiser.serialise(m_GPSHasFix);
        serialiser.serialise(m_GPSOnline);
        serialiser.serialise(m_GPSLat);
        serialiser.serialise(m_GPSLon);
        serialiser.serialise(m_GPSUnixTime);
        serialiser.serialise(m_GPSSpeed);
        serialiser.serialise(m_GPSCourse);
        serialiser.serialise(m_GPSSatellite);
        serialiser.serialise(m_WindDir);
        serialiser.serialise(m_WindSpeed);
        serialiser.serialise(m_WindTemp);
        serialiser.serialise(m_ArduinoPressure);
        serialiser.serialise(m_ArduinoRudder);
        serialiser.serialise(m_ArduinoSheet);
        serialiser.serialise(m_ArduinoBattery);
    }

   private:
    int m_CompassHeading;
    int m_CompassPitch;
    int m_CompassRoll;
    bool m_GPSHasFix;
    bool m_GPSOnline;
    double m_GPSLat;
    double m_GPSLon;
    double m_GPSUnixTime;
    double m_GPSSpeed;
    double m_GPSCourse;
    int m_GPSSatellite;
    float m_WindDir;
    float m_WindSpeed;
    float m_WindTemp;
    int m_ArduinoPressure;
    int m_ArduinoRudder;
    int m_ArduinoSheet;
    int m_ArduinoBattery;
};
