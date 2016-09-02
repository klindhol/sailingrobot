/****************************************************************************************
 *
 * File:
 * 		UDPNode.cpp
 *
 * Purpose:
 *		
 *
 * Developer Notes:
 *
 ***************************************************************************************/


#include "UDPNode.h"
#include "Messages/CourseDataMsg.h"
#include "SystemServices/Logger.h"


UDPNode::UDPNode(MessageBus& msgBus, std::string address, int port)
:Node(NodeID::Network, msgBus), m_udpRelay({port}, address)
{
	msgBus.registerNode(*this, MessageType::VesselState);
	msgBus.registerNode(*this, MessageType::CourseData);
	msgBus.registerNode(*this, MessageType::WaypointData);
	msgBus.registerNode(*this, MessageType::ActuatorPosition);
}

UDPNode::~UDPNode()
{
	// TODO Auto-generated destructor stub
}

bool UDPNode::init()
{
	return m_udpRelay.isSetup();
}

void UDPNode::processMessage(const Message* message)
{
	MessageSerialiser serialiser;
	message->Serialise(serialiser);

	m_udpRelay.write(serialiser.data(), serialiser.size());
}

