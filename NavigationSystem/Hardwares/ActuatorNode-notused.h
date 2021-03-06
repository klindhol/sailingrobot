/****************************************************************************************
 *
 * File:
 * 		ActuatorNode.h
 *
 * Purpose:
 *		Controls an actuator on the vessel.
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#pragma once

#include "Database/DBHandler.h"
#include "MessageBus/Node.h"

class ActuatorNode : public Node {
   public:
    ActuatorNode(MessageBus& msgBus,
                 DBHandler& dbhandler,
                 NodeID id,
                 int channel,
                 int speed,
                 int acceleration);

    ///----------------------------------------------------------------------------------
    /// Setups the actuator.
    ///
    ///----------------------------------------------------------------------------------
    virtual bool init();

    ///----------------------------------------------------------------------------------
    /// Processes ActuatorPositionMsgs
    ///
    ///----------------------------------------------------------------------------------
    virtual void processMessage(const Message* message);

   private:
    int m_Channel;
    int m_Speed;
    int m_Acceleration;
    DBHandler& m_db;
};
