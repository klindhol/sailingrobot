/****************************************************************************************
 *
 * File:
 *      SimulationNode.cpp
 *
 * Purpose:
 *      Discuss with simulation via TCP, create message for the program from the
 *    data from simulation and send the command data to the simulation.
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#include "SimulationNode.h"

// For std::this_thread
#include <chrono>
#include <thread>
#include <memory>

#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <strings.h> //bzero strerror
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include "SystemServices/Logger.h"
#include "SystemServices/SysClock.h"
#include "Network/TCPServer.h"
#include "Math/CourseMath.h"


#define BASE_SLEEP_MS 200
#define COUNT_COMPASSDATA_MSG 1
#define COUNT_GPSDATA_MSG 1
#define COUNT_WINDDATA_MSG 1
#define COUNT_ARDUINO_MSG 1

#define SERVER_PORT 6900


enum SimulatorPacket {
    SailBoatData = 0,
    WingBoatData =1,
    AISData = 2,
    CameraData = 3
    
};


SimulationNode::SimulationNode(MessageBus& msgBus)
    : ActiveNode(NodeID::Simulator, msgBus),
        m_RudderCommand(0), m_SailCommand(0), m_TailCommand(0),
        m_CompassHeading(0), m_GPSLat(0), m_GPSLon(0), m_GPSSpeed(0),
        m_GPSHeading(0), m_WindDir(0), m_WindSpeed(0),
        m_ArduinoRudder(0), m_ArduinoSheet(0), collidableMgr(NULL)
{
    m_MsgBus.registerNode(*this, MessageType::ActuatorPosition);
    m_MsgBus.registerNode(*this, MessageType::WingSailCommand);
    m_MsgBus.registerNode(*this, MessageType::RudderCommand);
}

SimulationNode::SimulationNode(MessageBus& msgBus, CollidableMgr* collidableMgr)
    : ActiveNode(NodeID::Simulator, msgBus),
        m_RudderCommand(0), m_SailCommand(0), m_TailCommand(0),
        m_CompassHeading(0), m_GPSLat(0), m_GPSLon(0), m_GPSSpeed(0),
        m_GPSHeading(0), m_WindDir(0), m_WindSpeed(0),
        m_ArduinoRudder(0), m_ArduinoSheet(0), collidableMgr(collidableMgr)
{
    m_MsgBus.registerNode(*this, MessageType::ActuatorPosition);
    m_MsgBus.registerNode(*this, MessageType::WingSailCommand);
    m_MsgBus.registerNode(*this, MessageType::RudderCommand);
}

void SimulationNode::start()
{
    runThread(SimulationThreadFunc);
}

bool SimulationNode::init()
{
    bool success = false;

    int rc = server.start( SERVER_PORT );

    if( rc > 0 )
    {
        Logger::info("Waiting for simulation client...\n");

        // Block until connection, don't timeout
        server.acceptConnection( 0 );

        success = true;
    }
    else
    {
        Logger::error( "Failed to start the simulator server" );
        success = false;
    }

    return success;
}

void SimulationNode::processMessage(const Message* msg)
{
    MessageType type = msg->messageType();

    switch(type)
    {
    case MessageType::ActuatorPosition:
        processActuatorPositionMessage((ActuatorPositionMsg*)msg);
        break;
    case MessageType::WingSailCommand:
        processWingSailCommandMessage((WingSailCommandMsg*)msg);
        break;
    case MessageType::RudderCommand:
        processRudderCommandMessage((RudderCommandMsg*)msg);
        break;
    default:
        return;
    }
}

void SimulationNode::processActuatorPositionMessage(ActuatorPositionMsg* msg)
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    m_RudderCommand = msg->rudderPosition();
    m_SailCommand = msg->sailPosition();
}

void SimulationNode::processWingSailCommandMessage(WingSailCommandMsg* msg)
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    m_TailCommand = msg->tailAngle();
}

void SimulationNode::processRudderCommandMessage(RudderCommandMsg* msg)
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    m_RudderCommand = msg->rudderAngle();
    Logger::info("rudder command message :%f", m_RudderCommand);
}

void SimulationNode::createCompassMessage()
{
    MessagePtr msg = std::make_unique<CompassDataMsg>(CompassDataMsg( m_CompassHeading, 0, 0));
    m_MsgBus.sendMessage(std::move(msg));
}

void SimulationNode::createGPSMessage()
{
    MessagePtr msg = std::make_unique<GPSDataMsg>(GPSDataMsg(true, true, m_GPSLat, m_GPSLon, SysClock::unixTime(), m_GPSSpeed, m_GPSHeading, 0, GPSMode::LatLonOk));
    m_MsgBus.sendMessage(std::move(msg));
}

void SimulationNode::createWindMessage()
{
    MessagePtr windData = std::make_unique<WindDataMsg>( WindDataMsg(m_WindDir, m_WindSpeed, 21) );
    m_MsgBus.sendMessage( std::move(windData) );
}

void SimulationNode::createArduinoMessage()
{
    MessagePtr msg = std::make_unique<ArduinoDataMsg>(ArduinoDataMsg(0, m_ArduinoRudder, m_ArduinoSheet, 0, 0 ));
    m_MsgBus.sendMessage(std::move(msg));
}

///--------------------------------------------------------------------------------------
void SimulationNode::processSailBoatData( TCPPacket_t& packet )
{   
    if( packet.length - 1 == sizeof(SailBoatDataPacket_t) )
    {
        // The first byte is the packet type, lets skip that
        uint8_t* ptr = packet.data + 1;
        SailBoatDataPacket_t* boatData = (SailBoatDataPacket_t*)ptr;

        m_CompassHeading = boatData->heading;
        m_GPSLat = boatData->latitude;
        m_GPSLon = boatData->longitude;
        m_GPSSpeed = boatData->speed;
        m_GPSHeading = boatData->course;
        m_WindDir = boatData->windDir;
        m_WindSpeed = boatData->windSpeed;
        m_ArduinoRudder = boatData->rudder;
        m_ArduinoSheet = boatData->sail;

        // Send messages
        createCompassMessage();
        createGPSMessage();
        createWindMessage();
        createArduinoMessage();
    }
}

///--------------------------------------------------------------------------------------
void SimulationNode::processWingBoatData( TCPPacket_t& packet )
{   
    if( packet.length - 1 == sizeof(WingBoatDataPacket_t) )
    {
        // The first byte is the packet type, lets skip that
        uint8_t* ptr = packet.data + 1;
        WingBoatDataPacket_t* boatData = (WingBoatDataPacket_t*)ptr;

        m_CompassHeading = boatData->heading;
        m_GPSLat = boatData->latitude;
        m_GPSLon = boatData->longitude;
        m_GPSSpeed = boatData->speed;
        m_GPSHeading = boatData->course;
        m_WindDir = boatData->windDir;
        m_WindSpeed = boatData->windSpeed;
        m_ArduinoRudder = boatData->rudder;
        m_ArduinoSheet = boatData->tail;

        // Send messages
        createCompassMessage();
        createGPSMessage();
        createWindMessage();
        createArduinoMessage();
    }
}

///--------------------------------------------------------------------------------------
void SimulationNode::processAISContact( TCPPacket_t& packet )
{
    if( this->collidableMgr != NULL )
    {
        // The first byte is the packet type, lets skip that
        uint8_t* ptr = packet.data + 1;
        AISContactPacket_t* aisData = (AISContactPacket_t*)ptr;

        this->collidableMgr->addAISContact(aisData->mmsi, aisData->latitude, aisData->longitude, aisData->speed, aisData->course);
    }
}

///--------------------------------------------------------------------------------------
void SimulationNode::processVisualContact( TCPPacket_t& packet )
{
    if( this->collidableMgr != NULL )
    {
        // The first byte is the packet type, lets skip that
        uint8_t* ptr = packet.data + 1;
        VisualContactPacket_t* data = (VisualContactPacket_t*)ptr;

        uint16_t bearing = CourseMath::calculateBTW(m_GPSLon, m_GPSLat, data->longitude, data->latitude);

        this->collidableMgr->addVisualContact(data->id, bearing);
    }
}

///--------------------------------------------------------------------------------------
void SimulationNode::sendActuatorData( int socketFD , int boatType)
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    switch (boatType)
    {
        case 0:
            ActuatorDataSailPacket_t actuatorDataSail;
            actuatorDataSail.rudderCommand = m_RudderCommand;
            actuatorDataSail.sailCommand = m_SailCommand;
            server.sendData( socketFD, &actuatorDataSail, sizeof(ActuatorDataSailPacket_t) );        
            break;
        case 1:
            ActuatorDataWingPacket_t actuatorDataWing;
            actuatorDataWing.rudderCommand = m_RudderCommand;
            actuatorDataWing.tailCommand = m_TailCommand;
            std::cout << sizeof(ActuatorDataWingPacket_t)<<std::endl;
            server.sendData( socketFD, &actuatorDataWing, sizeof(ActuatorDataWingPacket_t) );
            Logger::info("rudder command :%f  tail command:%f", m_RudderCommand, m_TailCommand);
            break;
    }
        
}

//<<<<<<< HEAD
///--------------------------------------------------------------------------------------
//void SimulationNode::SimulationThreadFunc(void* nodePtr)

void SimulationNode::SimulationThreadFunc(ActiveNode* nodePtr)
{
    SimulationNode* node = dynamic_cast<SimulationNode*> (nodePtr);

    TCPPacket_t packet;
    int simulatorFD = 0;

    while(true)
    {
        // Don't timeout on a packet read
        node->server.readPacket( packet, 0 );

        // We only care about the latest packet, so clear out the old ones
        //node->server.clearSocketBuffer( packet.socketFD );

        // We can safely assume that the first packet we receive will actually be from
        // the simulator as we only should ever accept one connection, the first one/
        if( simulatorFD == 0 )
        {
            simulatorFD = packet.socketFD;
        }
        // First byte is the message type
        switch( packet.data[0] )
        {
        case SimulatorPacket::SailBoatData:
            node->processSailBoatData( packet );
            node->sendActuatorData( simulatorFD, 0 );
            Logger::info("SailBoatData");
            break;

        case SimulatorPacket::WingBoatData:
            node->processWingBoatData( packet );
            node->sendActuatorData ( simulatorFD, 1 );
            Logger::info("WingBoatData");
            break;

        case SimulatorPacket::AISData:
            node->processAISContact( packet );
            break;

        case SimulatorPacket::CameraData:
            node->processVisualContact( packet );
            break;

        // unknown or deformed packet
        default:
            continue;
        }
        // Reset our packet, better safe than sorry
        packet.socketFD = 0;
        packet.length = 0;

        //std::this_thread::sleep_for(std::chrono::milliseconds(BASE_SLEEP_MS));
    }
}
