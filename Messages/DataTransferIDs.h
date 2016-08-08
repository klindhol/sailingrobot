/****************************************************************************************
 *
 * File:
 * 		DataID.h
 *
 * Purpose:
 *		Provides a enum containing all the message types. Used in the base message class
 *		so that when a message pointer is passed around you know what type of message to
 *		cast it to.
 *
 * Developer Notes:
 *
 *
 ***************************************************************************************/

#pragma once


#include <string>

enum DataID {
	ActuatorCommands = 0x01,
	ActuatorAndWindVaneCommands = 0x02,
	ActuatorFeedback = 0x80,
	WindSensorFeedback = 0x40,
	AnalogFeedback = 0x20

};


inline std::string DataIDToString(uint8_t dataID)
{
	switch(dataID)
	{
	case DataID::ActuatorCommands:
		return "ActuatorCommands";
	case DataID::ActuatorAndWindVaneCommands:
		return "ActuatorAndWindVaneCommands";
	case DataID::ActuatorFeedback:
		return "ActuatorFeedback";
	case DataID::WindSensorFeedback:
		return "WindSensorFeedback";
	case DataID::AnalogFeedback:
		return "AnalogFeedback";
	}
	return "";
}

inline int DataIDToByteLength(uint8_t dataID)
{
	switch(dataID)
	{
	case DataID::ActuatorCommands:
		return 4;
	case DataID::ActuatorAndWindVaneCommands:
		return 5;
	case DataID::ActuatorFeedback:
		return 4;
	case DataID::WindSensorFeedback:
		return 4;
	case DataID::AnalogFeedback:
		return 4;
	}
	return 0;
}
