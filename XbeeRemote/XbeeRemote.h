/****************************************************************************************
 *
 * File:
 * 		XbeeRemote.h
 *
 * Purpose:
 *
 *
 * Developer Notes:
 *
 ***************************************************************************************/

#pragma once

#include "../Messages/Message.h"
#include "../Network/DataLink.h"
#include "../Network/XbeePacketNetwork.h"
#include "UDPReceiver.h"
#include "UDPRelay.h"

class XbeeRemote {
   public:
    XbeeRemote(std::string portName);
    ~XbeeRemote();

    ///----------------------------------------------------------------------------------
    /// Initialises the Xbee remote data link and network
    ///----------------------------------------------------------------------------------
    bool initialise();

    ///----------------------------------------------------------------------------------
    /// Runs the Xbee packet network, this function only returns when something has gone
    /// wrong.
    ///----------------------------------------------------------------------------------
    void run();

    void printMessage(Message* msgPtr, MessageDeserialiser& deserialiser);

    void sendToUI(Message* msgPtr, MessageDeserialiser& deserialiser);

    bool parseActuatorMessage(uint8_t* data, int& rudder, int& sail);

    ///----------------------------------------------------------------------------------
    /// Called when a message is received over the Xbee network
    ///----------------------------------------------------------------------------------
    static void incomingData(uint8_t* data, uint8_t size);

   private:
    static XbeeRemote* m_Instance;
    DataLink* m_DataLink;
    XbeePacketNetwork* m_Network;
    std::string m_PortName;
    unsigned long m_LastReceived;
    bool m_Connected;
    UDPRelay m_Relay;
    UDPReceiver m_msgReceiver;
    UDPReceiver m_actReceiver;
};
