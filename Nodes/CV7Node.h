/****************************************************************************************
 *
 * File:
 * 		CV7Node.h
 *
 * Purpose:
 *		The CV7 node provides wind data to the system using the CV7 wind sensor.
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#pragma once

#include "ActiveNode.h"
#include <map>


class CV7Node : public ActiveNode{
public:
	CV7Node(MessageBus& msgBus, std::string m_PortName, unsigned int baudRate);

	///----------------------------------------------------------------------------------
 	/// Attempts to connect to the CV7 wind sensor.
 	///
 	///----------------------------------------------------------------------------------
	bool init();

	///----------------------------------------------------------------------------------
 	/// Starts the wind sensors thread so that it actively pumps data into the message 
 	/// bus.
 	///
 	///----------------------------------------------------------------------------------
	void start();

	///----------------------------------------------------------------------------------
 	/// The CV7 Node only processes data request messages.
 	///
 	///----------------------------------------------------------------------------------
	void processMessage(const Message* message);

private:
	///----------------------------------------------------------------------------------
 	/// The CV7 Node's thread function.
 	///
 	///----------------------------------------------------------------------------------
	static void WindSensorThread(void* nodePtr);

	///----------------------------------------------------------------------------------
 	/// Parses a NMEA wind sensor string and returns true if the parse was successful.
	/// The data that is parsed is put into a number of referenced values.
	///
	///	@param buffer		Pointer to the char buffer with data in it
	///	@param windDir		Extracted from the NMEA string, the direction of the wind.
	///	@param windSpeed	Extracted from the NMEA string, the speed of the wind.
	/// @param windTemp		Extracted from the NMEA string, the temperature of the wind.
	///
	/// @returns			Returns true if the buffer was successfully parsed.
 	///
 	///----------------------------------------------------------------------------------
	static bool parseString(const char* buffer, float& windDir, float& windSpeed, float& windTemp);

	bool m_Initialised;		// Indicates that the node was correctly initialised
	int m_fd;
	std::string m_PortName;
	unsigned int m_BaudRate;
	float m_MeanWindDir;
	float m_MeanWindSpeed;
	float m_MeanWindTemp;
};
