/*
 * rosserial Publisher Example
 * Prints "hello world!"
 */
#define USE_TEENSY_HW_SERIAL
#include <Bounce2.h>
#include <ros.h>
#include <iarc7_msgs/LandingGearContactsStamped.h>
#include <iarc7_msgs/Float64Stamped.h>
#include <sensor_msgs/Range.h>
#include <Wire.h>
#include <VL53L0X.h>

//https://github.com/pololu/vl53l0x-arduino for VL53L0X library

VL53L0X sensor;

Bounce left;
Bounce right;
Bounce front;
Bounce back;

ros::NodeHandle nh;

iarc7_msgs::LandingGearContactsStamped foot_switches_state;
ros::Publisher foot_switches("landing_gear_contact_switches", &foot_switches_state);

sensor_msgs::Range range_msg;
ros::Publisher rangefinder_pub("short_distance_lidar", &range_msg);
char frameid[] = "/short_distance_lidar";

iarc7_msgs::Float64Stamped battery_msg;
ros::Publisher battery_pub("motor_battery", &battery_msg);

const int LS_LEFT=2;
const int LS_RIGHT=5;
const int LS_FRONT=3;
const int LS_BACK=4;

const int BATTERY_PIN = A2;
const float BATTERY_VOLTAGE_DIVIDER_RATIO = (3.3/1024.0) / 0.0838;

const int rate_hz = 50;
const int loop_delay = 1000/rate_hz;

const int HEART_BEAT_HALF_PERIOD = 500;
const int HEART_BEAT_PIN = 13;
long delay_counter = 0;
bool heart = false;

// Debounce time in milliseconds
const  int debounce_time = 5;

void setup()
{
  nh.getHardware()->setBaud(115200);
  nh.initNode();
  nh.advertise(foot_switches);
  nh.advertise(rangefinder_pub);
  nh.advertise(battery_pub);

  pinMode(LS_LEFT, INPUT_PULLUP);
  pinMode(LS_RIGHT, INPUT_PULLUP);
  pinMode(LS_FRONT, INPUT_PULLUP);
  pinMode(LS_BACK, INPUT_PULLUP);

  // After setting up the button, setup the object
  left.attach(LS_LEFT);
  left.interval(debounce_time);
    
  right.attach(LS_RIGHT);
  right.interval(debounce_time);
    
  front.attach(LS_FRONT);
  front.interval(debounce_time);
    
  back.attach(LS_BACK);
  back.interval(debounce_time);

  range_msg.radiation_type = sensor_msgs::Range::INFRARED;
  range_msg.header.frame_id =  frameid;

  range_msg.field_of_view = 0.3;
  range_msg.min_range = 0.01;
  range_msg.max_range = 0.800;

  Wire.begin();
  sensor.init();
  sensor.setTimeout(500);

  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();

  delay_counter = millis();
  pinMode(HEART_BEAT_PIN, OUTPUT);
}

void loop()
{
  int start_time = millis();

  left.update();
  right.update();
  front.update();
  back.update();

  foot_switches_state.header.stamp = nh.now();
  foot_switches_state.left = left.read();
  foot_switches_state.right = right.read();
  foot_switches_state.front = front.read();
  foot_switches_state.back = back.read();
  foot_switches.publish( &foot_switches_state );

  range_msg.header.stamp = nh.now();
  range_msg.range = sensor.readRangeContinuousMillimeters()/1000.0;
  rangefinder_pub.publish(&range_msg);

  battery_msg.header.stamp = nh.now();
  battery_msg.data = (float)analogRead(BATTERY_PIN) * BATTERY_VOLTAGE_DIVIDER_RATIO;
  battery_pub.publish(&battery_msg);

  nh.spinOnce();


  if(millis() - start_time < loop_delay) {
    delay(loop_delay - (millis() - start_time));
  }

  // Blink heartbeat pin
  if(millis() - delay_counter > HEART_BEAT_HALF_PERIOD) {
    heart = heart ^ 1;
    digitalWrite(HEART_BEAT_PIN, heart);
    delay_counter = millis();
  }
}
