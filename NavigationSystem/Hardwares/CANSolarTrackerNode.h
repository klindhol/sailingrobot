#pragma once

#include "Hardwares/CAN_Services/CANService.h"
#include "Hardwares/CAN_Services/CANFrameReceiver.h"
#include "Hardwares/CAN_Services/N2kMsg.h"
#include "MessageBus/ActiveNode.h"
#include "MessageBus/Message.h"
#include "MessageBus/MessageBus.h"
#include "SystemServices/Timer.h"
#include "Messages/StateMessage.h"
#include "Messages/SolarDataMsg.h"

#include <time.h>
#include <mutex>
#include <vector>
#include <iostream>

class CANSolarTrackerNode : public CANFrameReceiver, public ActiveNode {
public:
  CANSolarTrackerNode(MessageBus& msgBus, CANService& canService, double loopTime);
  ~CANSolarTrackerNode();

  bool init();
  void processMessage (const Message* message);
  void processFrame (CanMsg& Msg);
  void start();

private:

  static void CANSolarTrackerThreadFunc(ActiveNode* nodePtr);

  float	m_Lat;
  float	m_Lon;
  // static std::chrono::_V2::steady_clock::time_point	m_Time;
  int m_Hour;
  int m_Minute;
  float	m_Heading;
  bool m_initialised;
  double m_LoopTime;

  std::mutex m_lock;

  const int DATA_OUT_OF_RANGE = -2000;
};
