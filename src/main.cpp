#include <Arduino.h>

//Very much stolen/borrowed from https://github.com/patrickpoirier51/POC 

// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 20 times per second.
// ---------------------------------------------------------------------------
// Arduino MAVLink  http://forum.arduino.cc/index.php?topic=382592.0
// https://github.com/ArduPilot/ardupilot_wiki/blob/master/dev/source/docs/code-overview-object-avoidance.rst

/*
 *  The system id of the message should match the system id of the vehicle 
 *  (default is "1" but can be changed using the SYSID_THISMAV parameter). 
 *  The component id can be anything but MAV_COMP_ID_PATHPLANNER (195) 
 *  or MAV_COMP_ID_PERIPHERAL (158) are probably good choices.
 *  
 * # Define function to send distance_message mavlink message for mavlink based rangefinder, must be >10hz
# http://mavlink.org/messages/common#DISTANCE_SENSOR
def send_distance_message(dist):
    msg = vehicle.message_factory.distance_sensor_encode(
        0,          # time since system boot, not used
        1,          # min distance cm
        10000,      # max distance cm
        dist,       # current distance, must be int
        0,          # type = 0 MAV_DISTANCE_SENSOR_LASER Laser rangefinder, e.g. LightWare SF02/F or PulsedLight units
        0,          # onboard id, not used
        mavutil.mavlink.MAV_SENSOR_ROTATION_PITCH_270, # must be set to MAV_SENSOR_ROTATION_PITCH_270 for mavlink rangefinder, represents downward facing
        0           # covariance, not used
    )
    vehicle.send_mavlink(msg)
    vehicle.flush()
    if args.verbose:
        log.debug("Sending mavlink distance_message:" +str(dist))
        */
#include <ardupilotmega\mavlink.h>
#include <ardupilotmega\mavlink_msg_rangefinder.h>

#include <NewPing.h>

HardwareSerial Serial3(PA3, PA2);

const int MIN = 25;
const int idle = 200;
/*
Valid values are (even numbers only):
Pre: 12 to 18 (initialized to 14 by default)
Final: 8 to 14 (initialized to 10 by default)
*/
const int PreRng = 18;  
const int PostRng = 14;

const int Scale = 10;
#define bRate 115200

#define TRIGGER_PIN  PB9  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     PB8  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar0(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar1(PB4, PB3, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
void command_heartbeat();
void command_distance(int s);

void setup() {
  Serial3.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  //Serial1.begin(bRate);
}

void loop() {
  command_heartbeat();
  command_distance(0);
  command_distance(1);
}

void command_heartbeat() {

  //< ID 1 for this system
  int sysid = 100;                   
  //< The component sending the message.
  int compid = MAV_COMP_ID_PATHPLANNER;    
  
  // Define the system type, in this case ground control station
  uint8_t system_type =MAV_TYPE_GCS;
  uint8_t autopilot_type = MAV_AUTOPILOT_INVALID;
  
  uint8_t system_mode = 0; 
  uint32_t custom_mode = 0;                
  uint8_t system_state = 0;
  
  // Initialize the required buffers
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];
  
  // Pack the message
  mavlink_msg_heartbeat_pack(sysid,compid, &msg, system_type, autopilot_type, system_mode, custom_mode, system_state);
  
  // Copy the message to the send buffer
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  
  // Send the message 
  //delay(1);
  Serial3.write(buf, len);
}

void command_distance(int s) {

  //MAVLINK DISTANCE MESSAGE
  int sysid = 1;                   
  //< The component sending the message.
  int compid = 158;    

  uint32_t time_boot_ms = 0; /*< Time since system boot*/
  uint16_t min_distance = 25; /*< Minimum distance the sensor can measure in centimeters*/
  uint16_t max_distance = 200; /*< Maximum distance the sensor can measure in centimeters*/
  uint16_t current_distance = (s == 0) ? sonar0.ping_cm() : sonar1.ping_cm(); /*< Current distance reading*/
  uint8_t type = 0; /*< Type from MAV_DISTANCE_SENSOR enum.*/
  uint8_t id = s + 1; /*< Onboard ID of the sensor*/
  uint8_t orientation = (s == 0) ? 1 : 7; /*(0=forward, each increment is 45degrees more in clockwise direction), 24 (upwards) or 25 (downwards)*/
// Consumed within ArduPilot by the proximity class

  uint8_t covariance = 0; /*< Measurement covariance in centimeters, 0 for unknown / invalid readings*/


  // Initialize the required buffers
  mavlink_message_t msg;
  uint8_t buf[MAVLINK_MAX_PACKET_LEN];

  // Pack the message
 mavlink_msg_distance_sensor_pack(sysid,compid,&msg,time_boot_ms,min_distance,max_distance,current_distance,type,id,orientation,covariance,70,70,0);

  // Copy the message to the send buffer
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  // Send the message (.write sends as bytes) 
  //delay(1);
  Serial3.write(buf, len);
}