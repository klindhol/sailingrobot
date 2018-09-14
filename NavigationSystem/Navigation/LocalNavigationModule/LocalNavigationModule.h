/****************************************************************************************
 *
 * File:
 * 		LocalNavigationModule.h
 *
 * Purpose:
 *
 *
 * License:
 *      This file is subject to the terms and conditions defined in the file
 *      'LICENSE.txt', which is part of this source code package.
 *
 ***************************************************************************************/

#pragma once

#include <vector>
#include "Database/DBHandler.h"
#include "MessageBus/ActiveNode.h"
#include "ASRArbiter.h"
#include "Navigation/LocalNavigationModule/ASRVoter.h"
#include "BoatState.h"

class LocalNavigationModule : public ActiveNode {
   public:
    friend class VoterTCPDebugger;
    ///----------------------------------------------------------------------------------
    /// Constructs the LocalNavigationModule
    ///----------------------------------------------------------------------------------
    LocalNavigationModule(MessageBus& msgBus, DBHandler& dbhandler);

    ///----------------------------------------------------------------------------------
    /// Does nothing
    ///----------------------------------------------------------------------------------
    bool init();

    ///----------------------------------------------------------------------------------
    /// Starts the quick hack wakeup thread
    ///----------------------------------------------------------------------------------
    void start();

    ///----------------------------------------------------------------------------------
    /// Processes the following messages:
    ///     * GPS Data Messages
    ///     * Compass Messages
    ///     * Wind Messages
    ///     * New Waypoint Messages
    ///     * Course Request Message
    ///----------------------------------------------------------------------------------
    void processMessage(const Message* msg);

    ///----------------------------------------------------------------------------------
    /// Registers a voter, this voter will then be asked to vote when a ballot is held.
    ///----------------------------------------------------------------------------------
    void registerVoter(ASRVoter* voter);

   private:
    ///----------------------------------------------------------------------------------
    /// Starts a ballot, asking every registered voter to vote. Once that is done it
    /// the final result is generated and a Local Navigation message is created.
    ///----------------------------------------------------------------------------------
    void startBallot();

    ///----------------------------------------------------------------------------------
    /// Returns true if the desired tack of the vessel is starboard (wind blowing from the right
    /// side )
    ///----------------------------------------------------------------------------------
    bool getTargetTackStarboard(double targetCourse);

    ///----------------------------------------------------------------------------------
    /// Update values from the database as the loop time of the thread and others parameters
    ///----------------------------------------------------------------------------------
    void updateConfigsFromDB();

    ///----------------------------------------------------------------------------------
    /// Just a little hack for waking up the navigation module for now
    ///----------------------------------------------------------------------------------
    static void WakeupThreadFunc(ActiveNode* nodePtr);

    std::vector<ASRVoter*> voters;
    BoatState_t boatState;
    ASRArbiter arbiter;
    double m_LoopTime;
    DBHandler& m_db;
    double m_trueWindDir;

    std::mutex m_lock; // Only for the cast vote step as the VoterTCPDebugger seems to conflict with it
};
