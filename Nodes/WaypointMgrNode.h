/****************************************************************************************
 *
 * File:
 * 		WaypointNode.h
 *
 * Purpose:
 *		The WaypointNode sends information about the waypoints to the sailing logic
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#pragma once


#include "Node.h"
#include "Messages/GPSDataMsg.h"
#include "dbhandler/DBHandler.h"
#include "utility/CourseMath.h"
#include "utility/Timer.h"


class WaypointMgrNode : public Node {
public:
	WaypointMgrNode(MessageBus& msgBus, DBHandler& db);
    virtual ~WaypointMgrNode(){};

	bool init();

	void processMessage(const Message* message);

private:
	void processGPSMessage(GPSDataMsg* msg);

    ///----------------------------------------------------------------------------------
 	/// Function called after every received message. Checks to see if waypoint harvested
    /// and if a new message should be sent
 	///----------------------------------------------------------------------------------
    bool waypointReached();
    ///----------------------------------------------------------------------------------
 	/// Function called after every received message when boat is avoiding an obstacle.
    /// Checks if the waypoint sent by CollisionAvoidanceNode is reached.
 	///----------------------------------------------------------------------------------
    bool collisionWaypointReached();

	///----------------------------------------------------------------------------------
 	/// Sends message with data about the next waypoint
 	///----------------------------------------------------------------------------------
    void sendMessage();
    ///----------------------------------------------------------------------------------
 	/// Sends message with data about the next collision avoidance waypoint
 	///----------------------------------------------------------------------------------
    void sendCAMessage();
    ///----------------------------------------------------------------------------------
 	/// Function that checks if waypoint can be harvested or not. Returns false if 
    /// distance to waypoint it further than radius or if boat hasn't stayed for the
    /// designated time. Returns true if it has both of the above.
 	///----------------------------------------------------------------------------------
    bool harvestWaypoint();

    DBHandler &m_db;
    bool writeTime;

    int     m_nextId;
    double  m_nextLongitude;
    double  m_nextLatitude;
    int     m_nextDeclination;
    int     m_nextRadius;
    int     m_nextStayTime;

    int     m_prevId;
    double  m_prevLongitude;
    double  m_prevLatitude;
    int     m_prevDeclination;
    int     m_prevRadius;

    bool    m_collisionAvoidance;
    int     m_caCounter;
    int     m_caId;
    int     m_caDeclination;
    int     m_caRadius;
    int     m_caStayTime;
    double  m_caStartLon; //Collision Avoidance Waypoints
    double  m_caStartLat;
    double  m_caMidLon;
    double  m_caMidLat;
    double  m_caEndLon;   
    double  m_caEndLat;

    double  m_gpsLongitude;
    double  m_gpsLatitude;

    Timer   m_waypointTimer;
};
