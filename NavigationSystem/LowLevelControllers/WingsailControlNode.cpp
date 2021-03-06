/****************************************************************************************
 *
 * File:
 * 		WingsailControlNode.cpp
 *
 * Purpose:
 *      Calculates the desired tail wing angle of the wingsail.
 *      It sends a WingSailComandMsg corresponding to the command angle of the tail wing.
 *
 * Developer Notes:
 *      Two functions have been developed to calculate the desired tail angle :
 *          - calculateTailAngle(),
 *          - simpleCalculateTailAngle().
 *      You can choose the one you want to use by commenting/uncommenting lines 
 *      in WingsailControlNodeThreadFunc().   
 *
 ***************************************************************************************/

#include "WingsailControlNode.h"

#define DATA_OUT_OF_RANGE -2000
const int INITIAL_SLEEP = 2000; // milliseconds
const float NO_COMMAND = -1000;

const double LIFTS[53] = {15.964797400676879, 15.350766731420075, 14.736736062163272, 14.122705392906468, 13.508674723649664, 12.894644054392863, 12.280613385136059, 11.666582715879256, 11.052552046622454, 10.438521377365651, 9.8244907081088488, 9.2104600388520446, 8.5964293695952421, 7.9823987003384396, 7.3683680310816362, 6.7543373618248319, 6.1403066925680294, 5.5262760233112269, 4.9122453540544244, 4.298214684797621, 3.6841840155408181, 3.0701533462840147, 2.4561226770272122, 1.842092007770409, 1.2280613385136061, 0.61403066925680305, 0.0, -0.61403066925680305, -1.2280613385136061, -1.842092007770409, -2.4561226770272122, -3.0701533462840147, -3.6841840155408181, -4.298214684797621, -4.9122453540544244, -5.5262760233112269, -6.1403066925680294, -6.7543373618248319, -7.3683680310816362, -7.9823987003384396, -8.5964293695952421, -9.2104600388520446, -9.8244907081088488, -10.438521377365651, -11.052552046622454, -11.666582715879256, -12.280613385136059, -12.894644054392863, -13.508674723649664, -14.122705392906468, -14.736736062163272, -15.350766731420075, -15.964797400676879};
const double  DRAGS[53] = {-3.6222976277233707, -3.3490177771111052, -3.0864547833855935, -2.8346086465468385, -2.5934793665948392, -2.3630669435295952, -2.1433713773511069, -1.9343926680593737, -1.7361308156543964, -1.5485858201361751, -1.3717576815047086, -1.2056463997599975, -1.0502519749020425, -0.90557440693084268, -0.77161369584639838, -0.64836984164870981, -0.53584284433777674, -0.4340327039135991, -0.34293942037617714, -0.26256299372551062, -0.1929034239615996, -0.13396071108444418, -0.085734855094044285, -0.048225855990399899, -0.021433713773511071, -0.0053584284433777678, -0.0, -0.0053584284433777678, -0.021433713773511071, -0.048225855990399899, -0.085734855094044285, -0.13396071108444418, -0.1929034239615996, -0.26256299372551062, -0.34293942037617714, -0.4340327039135991, -0.53584284433777674, -0.64836984164870981, -0.77161369584639838, -0.90557440693084268, -1.0502519749020425, -1.2056463997599975, -1.3717576815047086, -1.5485858201361751, -1.7361308156543964, -1.9343926680593737, -2.1433713773511069, -2.3630669435295952, -2.5934793665948392, -2.8346086465468385, -3.0864547833855935, -3.3490177771111052, -3.6222976277233707};


///----------------------------------------------------------------------------------
WingsailControlNode::WingsailControlNode(MessageBus& msgBus, DBHandler& dbhandler):
    ActiveNode(NodeID::WingsailControlNode,msgBus), m_db(dbhandler), m_LoopTime(0.5), m_MaxCommandAngle(13),
    m_ApparentWindDir(DATA_OUT_OF_RANGE), m_TargetCourse(DATA_OUT_OF_RANGE), m_TargetTackStarboard(0)
    
{
    msgBus.registerNode( *this, MessageType::WindState);
    msgBus.registerNode( *this, MessageType::LocalNavigation);
    msgBus.registerNode( *this, MessageType::ServerConfigsReceived);
}

///----------------------------------------------------------------------------------
WingsailControlNode::~WingsailControlNode(){}

///----------------------------------------------------------------------------------
bool WingsailControlNode::init(){
    updateConfigsFromDB();
    return true;
}

///----------------------------------------------------------------------------------
void WingsailControlNode::start()
{
    runThread(WingsailControlNodeThreadFunc);
}

///----------------------------------------------------------------------------------
void WingsailControlNode::processMessage( const Message* msg)
{
    switch( msg->messageType() )
    {
    case MessageType::WindState:
        processWindStateMessage(static_cast< const WindStateMsg*>(msg));
        break;
    case MessageType::LocalNavigation:
        processLocalNavigationMessage(static_cast< const LocalNavigationMsg*>(msg));
        break;
    case MessageType::ServerConfigsReceived:
        updateConfigsFromDB();
        break;
    default:
        return;
    }
}

///----------------------------------------------------------------------------------
void WingsailControlNode::updateConfigsFromDB()
{
    m_db.getConfigFrom(m_LoopTime, "loop_time", "config_wingsail_control");
    m_db.getConfigFrom(m_MaxCommandAngle, "max_cmd_angle", "config_wingsail_control");
}

///----------------------------------------------------------------------------------
void WingsailControlNode::processWindStateMessage(const WindStateMsg* msg)
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    m_ApparentWindDir = msg->apparentWindDirection();
}

///----------------------------------------------------------------------------------
void WingsailControlNode::processLocalNavigationMessage(const LocalNavigationMsg* msg)
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    m_TargetCourse = msg->targetCourse();
    m_TargetTackStarboard = msg->targetTackStarboard();
}

///----------------------------------------------------------------------------------
double WingsailControlNode::restrictWingsail(double val)
{
    if( val > m_MaxCommandAngle)        { return m_MaxCommandAngle; }
    else if ( val < -m_MaxCommandAngle) { return -m_MaxCommandAngle; }
    return val;
}

///------------------------------------------------------------------------------------
float WingsailControlNode::calculateTailAngle()
{
    std::lock_guard<std::mutex> lock_guard(m_lock);

    if(m_ApparentWindDir != DATA_OUT_OF_RANGE)
    {
        // lists that will contain the forces on X and Y in the boat coordinates system
        std::vector<double> xBoat_Forces ;
        std::vector<double> yBoat_Forces ;

        // transforming given calculated lifts and drags into forces in the boat coordinates system
        double apparentWindDir_counterClock = -m_ApparentWindDir;
        apparentWindDir_counterClock = Utility::degreeToRadian(apparentWindDir_counterClock);
        apparentWindDir_counterClock = Utility::limitRadianAngleRange(apparentWindDir_counterClock);
        for (int i = 0;i < 53;i++)
        {
            xBoat_Forces.push_back(DRAGS[i]*cos(apparentWindDir_counterClock) - LIFTS[i]*sin (apparentWindDir_counterClock));
            yBoat_Forces.push_back(LIFTS[i]*cos(apparentWindDir_counterClock) + DRAGS[i]*sin (apparentWindDir_counterClock));
        }

        std::vector<double> maxAndIndex_xBoat_Forces;
        maxAndIndex_xBoat_Forces = Utility::maxAndIndex(xBoat_Forces);
        double orderTail_counterClock;
        double orderTail;
        orderTail_counterClock = maxAndIndex_xBoat_Forces[1] - 26;
        orderTail = - orderTail_counterClock;
        orderTail = restrictWingsail(orderTail);

        return orderTail;
    }
    else
    {
        return NO_COMMAND;
    }
}

///----------------------------------------------------------------------------------
float WingsailControlNode::simpleCalculateTailAngle() {
    std::lock_guard <std::mutex> lock_guard(m_lock);

    ///   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------


    //Logger::info("APP WIND %f", sin(Utility::degreeToRadian(m_ApparentWindDir)));
    if(m_TargetCourse != DATA_OUT_OF_RANGE && m_TargetCourse != NO_COMMAND)
    {

        // TODO : take the speed of the boat into account somewhere there when tacking
        // The use of m_TargetTackStarboard still better than sin(Utility::degreeToRadian(m_ApparentWindDir)) >= 0
        // in 'bad' wind conditions
        if (m_TargetTackStarboard)
        {
            return -m_MaxCommandAngle;
        }
        else
        {
            return  m_MaxCommandAngle;
        }
    }
    else
    {
        return NO_COMMAND;
    }
}

///----------------------------------------------------------------------------------
void WingsailControlNode::WingsailControlNodeThreadFunc(ActiveNode* nodePtr)
{
    WingsailControlNode* node = dynamic_cast<WingsailControlNode*> (nodePtr);

    // An initial sleep, its purpose is to ensure that most if not all the sensor data arrives
    // at the start before we send out the state message.
    std::this_thread::sleep_for(std::chrono::milliseconds(INITIAL_SLEEP));

    Timer timer;
    timer.start();

    while(true)
    {
        //float wingSailCommand = (float)node->calculateTailAngle();
        float wingSailCommand = (float)node->simpleCalculateTailAngle();
        if (wingSailCommand != NO_COMMAND)
        {
            MessagePtr wingSailCommandMsg = std::make_unique<WingSailCommandMsg>(wingSailCommand);
            node->m_MsgBus.sendMessage(std::move(wingSailCommandMsg));
        }
        timer.sleepUntil(node->m_LoopTime);
        timer.reset();
    }
}
