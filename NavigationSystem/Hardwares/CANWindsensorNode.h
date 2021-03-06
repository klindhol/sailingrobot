/****************************************************************************************
 *
 * File:
 * 		CANWindsensorNode.h
 *
 * Purpose:
 *		Process messages from the CAN-Service
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#pragma once

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "Database/DBHandler.h"
#include "Hardwares/CAN_Services/CANPGNReceiver.h"
#include "Hardwares/CAN_Services/CANService.h"
#include "MessageBus/ActiveNode.h"
#include "Messages/WindDataMsg.h"
#include "SystemServices/Timer.h"

class CANWindsensorNode : public CANPGNReceiver, public ActiveNode {
   public:
    CANWindsensorNode(MessageBus& msgBus, DBHandler& dbhandler, CANService& can_service);
    ~CANWindsensorNode();

    /* data */
    void processPGN(N2kMsg& NMsg);

    void parsePGN130306(N2kMsg& NMsg,
                        uint8_t& SID,
                        float& WindSpeed,  // WindData
                        float& WindAngle,
                        uint8_t& Reference);

    void parsePGN130311(N2kMsg& Msg,
                        uint8_t& SID,
                        uint8_t& TemperatureInstance,  // Environmental Parameters
                        uint8_t& HumidityInstance,
                        float& Temperature,
                        float& Humidity,
                        float& AtmosphericPressure);

    void parsePGN130312(N2kMsg& NMsg,
                        uint8_t& SID,
                        uint8_t& TemperatureInstance,  // Temperature
                        uint8_t& TemperatureSource,
                        float& ActualTemperature,
                        float& SetTemperature);

    void parsePGN130314(N2kMsg& Msg,
                        uint8_t& SID,
                        uint8_t& PressureInstance,  // ActualPressure
                        uint8_t& PressureSource,
                        double& Pressure);

    ///----------------------------------------------------------------------------------
    /// Update values from the database as the loop time of the thread and others parameters
    ///----------------------------------------------------------------------------------
    void updateConfigsFromDB();

    ///----------------------------------------------------------------------------------
    /// Attempts to connect to the wind sensor.
    ///
    ///----------------------------------------------------------------------------------
    virtual bool init() { return true; };

    virtual void processMessage(const Message* message);

    void start();

   private:
    static void CANWindSensorNodeThreadFunc(ActiveNode* nodePtr);

    float m_WindDir;          // in degree 0 - 360 (273)
    float m_WindSpeed;        // in m/s
    float m_WindTemperature;  // in degree Celsius
    double m_LoopTime;        // in seconds
    DBHandler& m_db;

    std::mutex m_lock;
    std::vector<uint32_t> PGNs{130306, 130311};
};
