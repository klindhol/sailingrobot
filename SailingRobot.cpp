#include "SailingRobot.h"
#include <cstdlib>
#include <iostream>
#include <wiringPi.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <cmath>
#include <chrono>
#include <thread>
#include "utility/Utility.h"


SailingRobot::SailingRobot(ExternalCommand* externalCommand,
						   SystemState *systemState, DBHandler *db) :

	m_mockCompass(db->retriveCellAsInt("mock", "1", "Compass")),
	m_mockPosition(db->retriveCellAsInt("mock", "1", "Position")),
	m_mockMaestro(db->retriveCellAsInt("mock", "1", "Maestro")),

	m_dbHandler(db),

	m_waypointModel(PositionModel(1.5,2.7),100,""),
	m_waypointRouting(m_waypointModel,
		0.5/*innerRadiusRatio*/,
		m_dbHandler->retriveCellAsInt("configs", "1", "cc_ang_tack"),
		m_dbHandler->retriveCellAsInt("configs", "1", "cc_ang_sect")),

	m_externalCommand(externalCommand),
	m_systemState(systemState),

	m_systemStateModel(
		SystemStateModel(
			GPSModel("",0,0,0,0,0,0),
			WindsensorModel(0,0,0),
			CompassModel(0,0,0),
			0,
			0
		)
	)
{

}

SailingRobot::~SailingRobot() {

}


void SailingRobot::init(std::string programPath, std::string errorFileName) {
	m_errorLogPath = programPath + errorFileName;

	m_getHeadingFromCompass = m_dbHandler->retriveCellAsInt("configs", "1",
		"flag_heading_compass");

	printf(" Starting HTTPSync\t\t");
	setupHTTPSync();
	printf("OK\n");

	printf(" Starting Compass\t\t");
	setupCompass();
	printf("OK\n");

	printf(" Starting Maestro\t\t");
	setupMaestro();
	printf("OK\n");

	printf(" Starting RudderServo\t\t");
	setupRudderServo();
	printf("OK\n");

	printf(" Starting RudderCommand\t\t");
	setupRudderCommand();
	printf("OK\n");

	printf(" Starting SailServo\t\t");
	setupSailServo();
	printf("OK\n");

	printf(" Starting SailCommand\t\t");
	setupSailCommand();
	printf("OK\n");

	printf(" Starting Waypoint\t\t");
	setupWaypoint();
	printf("OK\n");

	//updateState();
	//syncServer();
}

int SailingRobot::getHeading() {

	int newHeading = 0;

	if (m_getHeadingFromCompass) {
		newHeading = m_compass->getHeading();
	}
	else {
		newHeading = m_gpsReader->getHeading();
	}

	return newHeading;
}

int SailingRobot::mockLatitude(int oldLat) {

	double courseToSteer = m_waypointRouting.getCTS();

	if(courseToSteer > 90 && courseToSteer < 270 && courseToSteer != 0) {

		oldLat -= 0.1;
	}
	else if (courseToSteer != 0) {
		oldLat += 0.1;
	}

	return oldLat;
}

int SailingRobot::mockLongitude(int oldLong) {

	double courseToSteer = m_waypointRouting.getCTS();

	if (courseToSteer < 180 && courseToSteer != 0) {

		oldLong += 0.1;
	}
	else if (courseToSteer != 0) {
		
		oldLong -= 0.1;
	}

	return oldLong;
}

void SailingRobot::run() {

	m_running = true;
	int rudderCommand, sailCommand, windDir, heading = 0;
	std::vector<float> twdBuffer;
	const unsigned int twdBufferMaxSize =
		m_dbHandler->retriveCellAsInt("buffer_configs", "1", "true_wind");
	
	//double longitude = 4, latitude = -3;
	double latitude = 60.07506317, longitude = 19.89288243;
	
	std::string sr_loop_time =
		m_dbHandler->retriveCell("configs", "1", "sr_loop_time");
	std::chrono::duration<double> loop_time(atof(sr_loop_time.c_str()));
	std::chrono::steady_clock::time_point start, end;
	std::chrono::duration<double> time_span;
	int nanoSecondsToSleep;
	int toNano = 1000*1000*1000;

	printf("*SailingRobot::run() started.\n");
	std::cout << "Waypoint target - ID: " << m_waypointModel.id << " lon: " << m_waypointModel.positionModel.longitude  
		<< " lat : " << m_waypointModel.positionModel.latitude << std::endl;

	while(m_running) {
		start = std::chrono::steady_clock::now();

		//Get data from SystemStateModel to local object
		m_systemState->getData(m_systemStateModel);
		
		windDir = m_systemStateModel.windsensorModel.direction;

		m_compass->readValues();
		heading = getHeading();

		if (m_systemStateModel.gpsModel.online) {

			//calc DTW
			if (m_mockPosition) {

				longitude = mockLongitude(longitude);
				latitude = mockLatitude(latitude);
				
				double courseToSteer = m_waypointRouting.getCTS();

				if (heading > courseToSteer) {

					heading--;
				}
				else if (heading < courseToSteer) {

					heading++;
				}
				else heading = courseToSteer;

			}
			else {
				longitude = m_systemStateModel.gpsModel.longitude;
				latitude = m_systemStateModel.gpsModel.latitude;
			}

			//calc & set TWD
			twdBuffer.push_back(heading + windDir);
			while (twdBuffer.size() > twdBufferMaxSize) {
				twdBuffer.erase(twdBuffer.begin());
			}

			//calc BTW & CTS
			m_waypointRouting.calculateCourseToSteer(PositionModel(latitude, longitude),
				Utility::meanOfAngles(twdBuffer));
				

			//rudder position calculation
			rudderCommand = m_rudderCommand.getCommand(m_waypointRouting.getCTS(), heading);
			if(!m_externalCommand->getAutorun()) {
				rudderCommand = m_externalCommand->getRudderCommand();
			}

		}
		else {
			m_logger.error("SailingRobot::run(), gps NaN. Using values from last iteration.");
		}

		//sail position calculation
		sailCommand = m_sailCommand.getCommand(windDir);
		if(!m_externalCommand->getAutorun()) {
			sailCommand = m_externalCommand->getSailCommand();
		}

		//rudder adjustment
		m_rudderServo.setPosition(rudderCommand);
		//sail adjustment
		m_sailServo.setPosition(sailCommand);

		//update system state
		m_systemState->setCompassModel(CompassModel(
				m_compass->getHeading(),
				m_compass->getPitch(),
				m_compass->getRoll()
			));
		m_systemState->setRudder(rudderCommand);
		m_systemState->setSail(sailCommand);


		//logging
		m_dbHandler->insertDataLog(
			m_systemStateModel.gpsModel.timestamp,
			latitude,
			longitude,
			m_systemStateModel.gpsModel.speed,
			m_systemStateModel.gpsModel.heading,
			m_systemStateModel.gpsModel.satellitesUsed,
			sailCommand,
			rudderCommand,
			0, //sailservo getpos, to remove
			0, //rudderservo getpos, to remove
			m_waypointRouting.getDTW(),
			m_waypointRouting.getBTW(),
			m_waypointRouting.getCTS(),
			m_waypointRouting.getTack(),
			m_waypointRouting.getGoingStarboard(),
			windDir,
			m_systemStateModel.windsensorModel.speed,
			m_systemStateModel.windsensorModel.temperature,
			atoi(m_waypointModel.id.c_str()),
			m_compass->getHeading(),
			m_compass->getPitch(),
			m_compass->getRoll()
		);

//		syncServer();

		// check if we are within the radius of the waypoint
		// and move to next wp in that case
		if (m_waypointRouting.nextWaypoint(PositionModel(latitude, longitude))) {
			
			if (m_dbHandler->retriveCellAsInt("configs", "1", "scanning"))
			{
				try {
					m_dbHandler->insertScan(PositionModel(latitude,longitude),
						m_systemStateModel.windsensorModel.temperature);
				} catch (const char * error) {
					m_logger.error(error);
					std::cout << error << std::endl;
				}
			}

			nextWaypoint();
			setupWaypoint();
			m_waypointRouting.setWaypoint(m_waypointModel);
		}

		//nextWaypoint();
		//setupWaypoint();

		end = std::chrono::steady_clock::now();
		time_span = std::chrono::duration_cast<
			std::chrono::duration<double>>(end - start);
		time_span = loop_time - time_span;
		nanoSecondsToSleep = time_span.count() * toNano;

		std::this_thread::sleep_for(
			std::chrono::nanoseconds(nanoSecondsToSleep));
	}

	printf("*SailingRobot::run() exiting\n");
}

void SailingRobot::shutdown() {
//	syncServer();
	m_running=false;
	m_dbHandler->closeDatabase();
}

void SailingRobot::syncServer() {
	try {
		std::string response = m_httpSync.pushLogs( m_dbHandler->getLogs() );
		m_dbHandler->removeLogs(response);
	} catch (const char * error) {
		m_logger.error(error);
	}
}

void SailingRobot::updateState() {
	try {
		std::string setup = m_httpSync.getSetup();
		bool stateChanged = false;
		if (m_dbHandler->revChanged("cfg_rev", setup) ) {
			m_dbHandler->updateTable("configs", m_httpSync.getConfig());
			stateChanged = true;
			m_logger.info("config state updated");
		}
		if (m_dbHandler->revChanged("rte_rev", setup) ) {
			m_dbHandler->updateTable("waypoints", m_httpSync.getRoute());
			stateChanged = true;
			m_logger.info("route state updated");
		}
		if (stateChanged)  {
			m_dbHandler->updateTable("state", m_httpSync.getSetup());
		}
	} catch (const char * error) {
		m_logger.error(error);
	}
}

void SailingRobot::nextWaypoint() {

	try {
		m_dbHandler->changeOneValue("waypoints", m_waypointModel.id,"1","harvested");
	} catch (const char * error) {
		m_logger.error(error);
	}
	m_logger.info("SailingRobot::nextWaypoint(), waypoint reached");
	std::cout << "Waypoint reached!" << std::endl;
	
}

void SailingRobot::setupWaypoint() {

	try {
		m_dbHandler->getWaypointFromTable(m_waypointModel);		
	} catch (const char * error) {
		m_logger.error(error);
	}
	try {
		if (m_waypointModel.id.empty() ) {
			std::cout << "No waypoint found!"<< std::endl;			
			throw "No waypoint found!";			
		}
		else{
			std::cout << "New waypoint picked! ID:" << m_waypointModel.id <<" lat: "
			<< m_waypointModel.positionModel.latitude 
			<< " lon: " << m_waypointModel.positionModel.longitude << " rad: " 
			<< m_waypointModel.radius << std::endl;
		}
	} catch (const char * error) {
		m_logger.error(error);		
		//m_dbHandler->insertMessageLog("NOTIME", "NOTYPE", "NO WAYPOINT FOUND!");
		//throw;
	}

	m_logger.info("setupWaypoint() done");
}

void SailingRobot::setupMaestro() {
	if (m_mockMaestro) {
		m_maestroController.reset(new MockMaestroController());
	}
	else {
		m_maestroController.reset(new MaestroController());
	}

	std::string port_name;
	try {
		port_name = m_dbHandler->retriveCell("configs", "1", "mc_port");
	} catch (const char * error) {
		m_logger.error(error);
	}

	try {
		m_maestroController->setPort( port_name );
		int maestroErrorCode = m_maestroController->getError();
		if (maestroErrorCode != 0) {
			std::stringstream stream;
			stream << "maestro errorcode: " << maestroErrorCode;
			m_logger.error(stream.str());
		}
	} catch (const char * error) {
		m_logger.error(error);
		throw;
	}
	m_logger.info("setupMaestro() done");
}

void SailingRobot::setupRudderServo() {
	try {
		m_rudderServo.setController(m_maestroController.get());
		m_rudderServo.setChannel( m_dbHandler->retriveCellAsInt("configs", "1", "rs_chan") );
		m_rudderServo.setSpeed( m_dbHandler->retriveCellAsInt("configs", "1", "rs_spd") );
		m_rudderServo.setAcceleration( m_dbHandler->retriveCellAsInt("configs", "1", "rs_acc") );
	} catch (const char * error) {
		m_logger.error(error);
		throw;
	}
	m_logger.info("setupRudderServo() done");
}

void SailingRobot::setupSailServo() {
	try {
		m_sailServo.setController(m_maestroController.get());
		m_sailServo.setChannel( m_dbHandler->retriveCellAsInt("configs", "1", "ss_chan") );
		m_sailServo.setSpeed( m_dbHandler->retriveCellAsInt("configs", "1", "ss_spd") );
		m_sailServo.setAcceleration( m_dbHandler->retriveCellAsInt("configs", "1", "ss_acc") );
	} catch (const char * error) {
		m_logger.error(error);
		throw;
	}
	m_logger.info("setupSailServo() done");
}

void SailingRobot::setupRudderCommand() {
	try {
		m_rudderCommand.setCommandValues( m_dbHandler->retriveCellAsInt("configs", "1", "rc_cmd_xtrm"),
			m_dbHandler->retriveCellAsInt("configs", "1", "rc_cmd_mid"));

	} catch (const char * error) {
		m_logger.error(error);
		throw;
	}
	m_logger.info("setupRudderCommand() done");
}

void SailingRobot::setupSailCommand() {
	try {
		m_sailCommand.setCommandValues( m_dbHandler->retriveCellAsInt("configs", "1", "sc_cmd_clse"),
			m_dbHandler->retriveCellAsInt("configs", "1", "sc_cmd_run"));

	} catch (const char * error) {
		m_logger.error(error);
		throw;
	}
	m_logger.info("setupSailCommand() done");
}

void SailingRobot::setupHTTPSync() {
	try {
		m_httpSync.setShipID( m_dbHandler->retriveCell("server", "1", "boat_id") );
		m_httpSync.setShipPWD( m_dbHandler->retriveCell("server", "1", "boat_pwd") );
		m_httpSync.setServerURL( m_dbHandler->retriveCell("server", "1", "srv_addr") );
	} catch (const char * error) {
		m_logger.error("SailingRobot::setupHTTPSync() failed");
	}
	m_logger.info("setupHTTPSync() done");
}

void SailingRobot::setupCompass() {
	if (!m_mockCompass) {
		m_compass = new HMC6343(
			m_dbHandler->retriveCellAsInt("buffer_configs", "1", "compass") );
	}
	else {
		m_compass = new MockCompass;
	}
	try {
		m_compass->init();
	} catch (const char * error) {
		m_logger.error("SailingRobot::setupCompass() failed");
	}
	m_logger.info("setupCompass() done");
}
