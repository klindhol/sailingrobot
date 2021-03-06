/****************************************************************************************
*
* File:
* 		CANWindsensorNode.cpp
*
* Purpose:
*		 Process messages from the CAN-Service.
*
* Developer Notes:
*		PGN numbers for this node are:
*			130306, 130311, (more to be added later?)
*
***************************************************************************************/

#include "CANWindsensorNode.h"
#include "Math/Utility.h"


const int DATA_OUT_OF_RANGE	=	-2000;

CANWindsensorNode::CANWindsensorNode(MessageBus& msgBus, DBHandler& dbhandler, CANService& can_service)
: CANPGNReceiver(can_service, {130306, 130311}), ActiveNode(NodeID::WindSensor, msgBus), m_LoopTime(0.5), m_db(dbhandler)
{
    m_WindDir  = DATA_OUT_OF_RANGE;
    m_WindSpeed = DATA_OUT_OF_RANGE;
    m_WindTemperature = DATA_OUT_OF_RANGE;

  msgBus.registerNode(*this, MessageType::DataRequest);
  msgBus.registerNode(*this, MessageType::ServerConfigsReceived);
  updateConfigsFromDB();
}

CANWindsensorNode::~CANWindsensorNode(){}

void CANWindsensorNode::processPGN(N2kMsg &NMsg)
{


  if(NMsg.PGN == 130306){
    std::lock_guard<std::mutex> lock(m_lock);
    uint8_t SID, Ref;
    float WS, WA;
    parsePGN130306(NMsg, SID, WS, WA, Ref);
    m_WindDir = Utility::radianToDegree(WA);
    m_WindSpeed = WS;
  }
  else if(NMsg.PGN == 130311)
  {
    std::lock_guard<std::mutex> lock(m_lock);
    uint8_t SID, TI, HI;
    float Temp, Hum, AP;
    parsePGN130311(NMsg, SID, TI, HI, Temp, Hum, AP);
    m_WindTemperature = (Temp - 273.15); // To centigrade
  }
  else if(NMsg.PGN == 130312)
  {
    std::lock_guard<std::mutex> lock(m_lock);
    uint8_t SID, TI, TS;
    float ATemp, STemp;
    parsePGN130312(NMsg, SID, TI, TS, ATemp, STemp);
  }
  else if (NMsg.PGN == 130314)
  {
    std::lock_guard<std::mutex> lock(m_lock);
    uint8_t SID, PI, PS;
    double P;
    parsePGN130314(NMsg, SID, PI, PS, P);
  }

}

void CANWindsensorNode::parsePGN130306(N2kMsg &NMsg, uint8_t &SID, float &WindSpeed,				//WindData
    float &WindAngle, uint8_t &Reference)
{
    SID = NMsg.Data[0];
    uint16_t tmp = NMsg.Data[1] | (NMsg.Data[2]<<8);
    WindSpeed = tmp*0.01;
    tmp = NMsg.Data[3] | (NMsg.Data[4]<<8);
    WindAngle = tmp*0.0001;
    Reference = NMsg.Data[5] & 0x07;
}

void CANWindsensorNode::parsePGN130311(N2kMsg &NMsg, uint8_t &SID, uint8_t &TemperatureInstance,	//Environmental Parameters
    uint8_t &HumidityInstance, float &Temperature, float &Humidity, float &AtmosphericPressure)
{
    SID = NMsg.Data[0];
    TemperatureInstance = NMsg.Data[1] & 0x3f;
    HumidityInstance = NMsg.Data[1] >> 6;
    uint16_t tmp = NMsg.Data[2] | (NMsg.Data[3]<<8);
    Temperature = tmp*0.01;
    //tmp = NMsg.Data[4] | (NMsg.Data[5]<<8);
    //Humidity = tmp*0.004;
    Humidity = 0;
    tmp = NMsg.Data[6] | (NMsg.Data[7]<<8);
    AtmosphericPressure = tmp;		//hPa
}

void CANWindsensorNode::parsePGN130312(N2kMsg &NMsg, uint8_t &SID, uint8_t &TemperatureInstance,	//Temperature
    uint8_t &TemperatureSource, float &ActualTemperature, float &SetTemperature)
{
    SID = NMsg.Data[0];
    TemperatureInstance = NMsg.Data[1];
    TemperatureSource = NMsg.Data[2];
    uint16_t tmp = NMsg.Data[3] | (NMsg.Data[4]<<8);
    ActualTemperature = tmp*0.01;
    tmp = NMsg.Data[5] | (NMsg.Data[6]<<8);
    SetTemperature = tmp*0.01;
}

void CANWindsensorNode::parsePGN130314(N2kMsg &NMsg, uint8_t &SID, uint8_t &PressureInstance,		//ActualPressure
uint8_t &PressureSource, double &Pressure)
{
    SID = NMsg.Data[0];
    PressureInstance = NMsg.Data[1];
    PressureSource = NMsg.Data[2];

    uint32_t tmp = NMsg.Data[3] | (NMsg.Data[4]<<8) | (NMsg.Data[5]<<16) | (NMsg.Data[6]<<24);
    Pressure = tmp / 1000.0f; 			//hPa
}

void CANWindsensorNode::updateConfigsFromDB() {
    m_db.getConfigFrom(m_LoopTime, "loop_time", "config_wind_sensor");
}

void CANWindsensorNode::processMessage(const Message* message) {

  std::lock_guard<std::mutex> lock(m_lock);

  if(message->messageType() == MessageType::DataRequest)
  {
    // On system startup we won't have any valid data, so don't send any
    if( m_WindDir!= DATA_OUT_OF_RANGE ||  m_WindTemperature != DATA_OUT_OF_RANGE || m_WindSpeed != DATA_OUT_OF_RANGE)
    {
      MessagePtr windData = std::make_unique<WindDataMsg>(message->sourceID(), this->nodeID(), m_WindDir, m_WindSpeed, m_WindTemperature);
      m_MsgBus.sendMessage(std::move(windData));
    }
  }
  else if(message->messageType() == MessageType::ServerConfigsReceived)
  {
        updateConfigsFromDB();
  }
}

void CANWindsensorNode::start()
{
    runThread(CANWindSensorNodeThreadFunc);
}

void CANWindsensorNode::CANWindSensorNodeThreadFunc(ActiveNode* nodePtr) {

  CANWindsensorNode* node = dynamic_cast<CANWindsensorNode*> (nodePtr);
  Timer timer;
  timer.start();

  while(true)
	{

    node->m_lock.lock();

    if( not (node->m_WindDir == DATA_OUT_OF_RANGE &&  node->m_WindTemperature == DATA_OUT_OF_RANGE && node->m_WindSpeed == DATA_OUT_OF_RANGE) )
		{
		    MessagePtr windData = std::make_unique<WindDataMsg>(node->m_WindDir, node->m_WindSpeed, node->m_WindTemperature);
        node->m_MsgBus.sendMessage(std::move(windData));
    }
    node->m_lock.unlock();

    timer.sleepUntil(node->m_LoopTime);
    timer.reset();
  }
}
