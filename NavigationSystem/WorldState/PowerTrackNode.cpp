/****************************************************************************************
 *
 * File:
 * 		PowerTrackNode.h
 *
 * Purpose:
 *      Collects voltage and current data from the actuators, ECUs, and solar
 *      panel to track the power, sends it to the logger.
 *		
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

 #include "PowerTrackNode.h"

 #include <chrono>
 #include <thread>

 #include "../Messages/PowerTrackMsg.h"
 #include "../Math/CourseMath.h"
 #include "../SystemServices/Logger.h"
 #include "../SystemServices/Timer.h"
 #include "../Database/DBHandler.h"

 #define POWER_TRACK_INITIAL_SLEEP 100

 PowerTrackNode::PowerTrackNode(MessageBus& msgBus, DBHandler& dbhandler, double loopTime)
    : ActiveNode(NodeID::PowerTrack, msgBus),
            m_ArduinoPressure(0), m_ArduinoRudder(0), m_ArduinoSheet(0), m_ArduinoBattery(0),
            m_CurrentSensorDataCurrent(0), m_CurrentSensorDataVoltage(0), 
            m_CurrentSensorDataElement((SensedElement)0),m_Looptime(loopTime), m_db(dbhandler)
{
	msgBus.registerNode(*this, MessageType::ArduinoData);
	msgBus.registerNode(*this, MessageType::CurrentSensorData);
}

//----------------------------------------------------------------------------------
PowerTrackNode::~PowerTrackNode()
{
	//server.shutdown();
}

//----------------------------------------------------------------------------------
bool PowerTrackNode::init()
{
	//return server.start( 9600 );
	return true;
}

void PowerTrackNode::start()
{
	runThread(PowerTrackThreadFunc);
}

void PowerTrackNode::processMessage(const Message* msg)
{
	MessageType type = msg->messageType();

	switch(type)
	{
		case MessageType::ArduinoData:
			processArduinoMessage((ArduinoDataMsg*) msg);
			break;

		case MessageType::CurrentSensorData:
			processCurrentSensorDataMessage((CurrentSensorDataMsg*) msg);
			break;

		default:
			return;
	}
}

void PowerTrackNode::processArduinoMessage(ArduinoDataMsg* msg)
{
	m_ArduinoPressure = msg->pressure();
	m_ArduinoRudder = msg->rudder();
	m_ArduinoSheet = msg->sheet();
	m_ArduinoBattery = msg->battery();
}

void PowerTrackNode::processCurrentSensorDataMessage(CurrentSensorDataMsg* msg)
{
	m_CurrentSensorDataCurrent = msg->getCurrent();
	m_CurrentSensorDataVoltage = msg->getVoltage();
	m_CurrentSensorDataElement = msg->getSensedElement();
	m_Power = m_CurrentSensorDataVoltage * m_CurrentSensorDataCurrent;

	switch(m_CurrentSensorDataElement)
	{
		case SAILDRIVE :
			m_PowerBalance += m_Power;
			break;
		case WINDVANE_SWITCH :
			m_PowerBalance -= m_Power;
			break;
		default: 
			break;
	}
}

void PowerTrackNode::PowerTrackThreadFunc(ActiveNode* nodePtr)
{
	PowerTrackNode* node = dynamic_cast<PowerTrackNode*> (nodePtr);

	// Initial sleep time in order for enough data to be transmitted on the first message
	std::this_thread::sleep_for(std::chrono::milliseconds(POWER_TRACK_INITIAL_SLEEP));

	//char buffer[1024];

	Timer timer;
	timer.start();
	while(true)
	{
		// Accept and recieve connections
		//node->server.AcceptConnections();

		//Regulate the rate at whcih the messages are sent
		timer.sleepUntil(node->m_Looptime);

		MessagePtr powerTrack = std::make_unique<PowerTrackMsg>(
			node->m_ArduinoPressure, node->m_ArduinoRudder, node->m_ArduinoSheet,
			node->m_ArduinoBattery, node->m_CurrentSensorDataCurrent, node->m_CurrentSensorDataVoltage,
			node->m_CurrentSensorDataElement);

		node->m_MsgBus.sendMessage(std::move(powerTrack));

		Logger::info("PowerTrackInfo: %f,%f,%f,%f,%d", (float)node->m_CurrentSensorDataCurrent, 
			(float)node->m_CurrentSensorDataVoltage, (float)node->m_PowerBalance, (float)node->m_Power,
			(uint8_t)node->m_CurrentSensorDataElement);

		//int size = snprintf(buffer, 1024, "%d,%d,%d,%d,%f,%f,%d\n",
		//					(int)node->m_ArduinoPressure, (int)node->m_ArduinoRudder,
		//					(int)node->m_ArduinoSheet, (int)node->m_ArduinoBattery,
		//					(float)node->m_CurrentSensorDataCurrent, 
		//                  (float)node->m_CurrentSensorDataVoltage,
		//					(uint8_t)node->m_CurrentSensorDataElement);
		//if( size > 0 )
		//{
			//node->server.broadcast( (uint8_t*)buffer, size );
		//}
		timer.reset();
	}
}