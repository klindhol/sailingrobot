/****************************************************************************************
 *
 * File:
 * 		WaypointNode.cpp
 *
 * Purpose:
 *		The WaypointNode sends information about the waypoints to the sailing logic
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#include "WaypointNode.h"
#include "Messages/WaypointDataMsg.h"
#include "SystemServices/Logger.h"
#include "dbhandler/DBHandler.h"
#include "utility/Utility.h"
#include <string>
#include <vector>

WaypointNode::WaypointNode(MessageBus& msgBus, DBHandler& db)
: Node(NodeID::Waypoint, msgBus), m_db(db),
    m_nextId(0),
    m_nextLongitude(0),
    m_nextLatitude(0),
    m_nextDeclination(0),
    m_nextRadius(0),

    m_prevId(0),
    m_prevLongitude(0),
    m_prevLatitude(0),
    m_prevDeclination(0),
    m_prevRadius(0)
{
    msgBus.registerNode(this, MessageType::GPSData);
}

bool WaypointNode::init()
{
    sendMessage();
    return true;
}


void WaypointNode::processMessage(const Message* msg)
{
	MessageType type = msg->messageType();

	switch(type)
	{
        case MessageType::GPSData:
            processGPSMessage((GPSDataMsg*)msg);
            break;
        default:
            return;
	}

    //TODO Oliver - ADD waypoint time to message
    if(waypointReached()/* || httpsync sent info that waypoints have been updated*/)
    {
        sendMessage();
    }
}

void WaypointNode::processGPSMessage(GPSDataMsg* msg)
{
    m_gpsLongitude = msg->longitude();
    m_gpsLatitude = msg->latitude();
}

bool WaypointNode::waypointReached()
{
    double distanceAfterWaypoint = Utility::calculateWaypointsOrthogonalLine(m_nextLongitude, m_nextLatitude, m_prevLongitude,
                m_prevLatitude, m_gpsLongitude, m_gpsLatitude); //Checks if boat has passed the waypoint following the line, without entering waypoints radius

    if(m_courseMath.calculateDTW(m_gpsLongitude, m_gpsLatitude, m_nextLongitude, m_nextLatitude) < m_nextRadius || distanceAfterWaypoint > 0)
    {
        if(not m_db.changeOneValue("waypoints", std::to_string(m_nextId),"1","harvested"))
        {
            Logger::error("Failed to harvest waypoint");
        }
        Logger::info("Reached waypoint");

        return true;
    }
    else
    {
        return false;
    }
}

void WaypointNode::sendMessage()
{
    Logger::info("Preparing to send WaypointNode");

    if(m_db.getWaypointValues(m_nextId, m_nextLongitude, m_nextLatitude, m_nextDeclination, m_nextRadius,
                        m_prevId, m_prevLongitude, m_prevLatitude, m_prevDeclination, m_prevRadius))
    {
        WaypointDataMsg* msg = new WaypointDataMsg(m_nextId, m_nextLongitude, m_nextLatitude, m_nextDeclination, m_nextRadius,
                        m_prevId, m_prevLongitude, m_prevLatitude, m_prevDeclination, m_prevRadius);
        m_MsgBus.sendMessage(msg);
    }
    else
    {
        Logger::warning("%s No waypoint found, boat is using old waypoint data. No message sent.", __func__);
    }
}