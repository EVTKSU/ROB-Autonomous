/*
   Complete ODriveTeensyCAN Example Program (Direct Function Calls)

   This program defines the ODriveTeensyCAN class (merging header and cpp)
   and demonstrates direct calls to all available control and data request
   functions for an ODrive. All function calls are made in the setup() function.
*/

#include "Arduino.h"
#include <FlexCAN_T4.h>
#include <stdint.h>
#include <string.h> // for memcpy

//-------------------------------------------------------------------------------------------------
// ODriveTeensyCAN Class Declaration and Definition
//-------------------------------------------------------------------------------------------------
class ODriveTeensyCAN {
public:
  // Axis states (for RunState)
  enum AxisState_t {
    AXIS_STATE_UNDEFINED = 0,
    AXIS_STATE_IDLE = 1,
    AXIS_STATE_STARTUP_SEQUENCE = 2,
    AXIS_STATE_FULL_CALIBRATION_SEQUENCE = 3,
    AXIS_STATE_MOTOR_CALIBRATION = 4,
    AXIS_STATE_SENSORLESS_CONTROL = 5,
    AXIS_STATE_ENCODER_INDEX_SEARCH = 6,
    AXIS_STATE_ENCODER_OFFSET_CALIBRATION = 7,
    AXIS_STATE_CLOSED_LOOP_CONTROL = 8,
    AXIS_STATE_HOMING = 11
  };

  // Command IDs for CAN messages
  enum CommandId_t {
    CMD_ID_CANOPEN_NMT_MESSAGE = 0x000,
    CMD_ID_ODRIVE_HEARTBEAT_MESSAGE = 0x001,
    CMD_ID_ODRIVE_ESTOP_MESSAGE = 0x002,
    CMD_ID_GET_MOTOR_ERROR = 0x003,
    CMD_ID_GET_ENCODER_ERROR = 0x004,
    CMD_ID_GET_SENSORLESS_ERROR = 0x005,
    CMD_ID_SET_AXIS_NODE_ID = 0x006,
    CMD_ID_SET_AXIS_REQUESTED_STATE = 0x007,
    CMD_ID_SET_AXIS_STARTUP_CONFIG = 0x008,
    CMD_ID_GET_ENCODER_ESTIMATES = 0x009,
    CMD_ID_GET_ENCODER_COUNT = 0x00A,
    CMD_ID_SET_CONTROLLER_MODES = 0x00B,
    CMD_ID_SET_INPUT_POS = 0x00C,
    CMD_ID_SET_INPUT_VEL = 0x00D,
    CMD_ID_SET_INPUT_TORQUE = 0x00E,
    CMD_ID_SET_VELOCITY_LIMIT = 0x00F,
    CMD_ID_START_ANTICOGGING = 0x010,
    CMD_ID_SET_TRAJ_VEL_LIMIT = 0x011,
    CMD_ID_SET_TRAJ_ACCEL_LIMITS = 0x012,
    CMD_ID_SET_TRAJ_INERTIA = 0x013,
    CMD_ID_GET_IQ = 0x014,
    CMD_ID_GET_SENSORLESS_ESTIMATES = 0x015,
    CMD_ID_REBOOT_ODRIVE = 0x016,
    CMD_ID_GET_BUS_VOLTAGE_CURRENT = 0x017,
    CMD_ID_CLEAR_ERRORS = 0x018,
    CMD_ID_CANOPEN_HEARTBEAT_MESSAGE = 0x700
  };

  ODriveTeensyCAN();

  // CAN message sending functions
  void sendMessage(int axis_id, int cmd_id, bool remote_transmission_request, int length, byte *signal_bytes);
  void sendMessageCool(int axis_id, int cmd_id, bool remote_transmission_request, int length, byte *signal_bytes);
  bool readAsyncMessages();

  // Heartbeat
  int Heartbeat();

  // Commands for controlling motion
  void SetPosition(int axis_id, float position);
  void SetPosition(int axis_id, float position, float velocity_feedforward);
  void SetPosition(int axis_id, float position, float velocity_feedforward, float current_feedforward);
  void SetVelocity(int axis_id, float velocity);
  void SetVelocity(int axis_id, float velocity, float current_feedforward);
  void SetVelocityLimit(int axis_id, float velocity_limit);
  void SetTorque(int axis_id, float torque);
  void ClearErrors(int axis_id);

  // Async (non-rtc) getters
  void RequestPositionEstimate(int axis_id);
  float GetLastPositionEstimate(int axis_id);
  void RequestBusVoltageCurrent(int axis_id);
  float GetLastBusVoltage(int axis_id);

  // Synchronous getters
  float GetPosition(int axis_id);
  float GetVelocity(int axis_id);
  uint32_t GetMotorError(int axis_id);
  uint32_t GetEncoderError(int axis_id);
  uint32_t GetAxisError(int axis_id);
  uint32_t GetCurrentState(int axis_id);

  // State helper
  bool RunState(int axis_id, int requested_state);
};

//-------------------------------------------------------------------------------------------------
// Internal Constants and Global Variables
//-------------------------------------------------------------------------------------------------
static const int kMotorOffsetFloat = 2;
static const int kMotorStrideFloat = 28;
static const int kMotorOffsetInt32 = 0;
static const int kMotorStrideInt32 = 4;
static const int kMotorOffsetBool = 0;
static const int kMotorStrideBool = 4;
static const int kMotorOffsetUint16 = 0;
static const int kMotorStrideUint16 = 2;

static const int NodeIDLength = 6;
static const int CommandIDLength = 5;

static const float feedforwardFactor = 1 / 0.001; // scaling factor
static const int CANBaudRate = 250000;

#ifdef ARDUINO_TEENSY35
FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;
#elif defined(ARDUINO_TEENSY40)
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
#elif defined(ARDUINO_TEENSY41)
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> Can0;
#else
FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;
#endif

// Async data storage for each axis (assuming up to 16 axes)
struct axis_async_data {
  float position_est;
  float velocity_est;
  float bus_voltage;
  float bus_current;
};
axis_async_data axis_data[16];

// Overload stream operator for printing (including float with 4 decimal precision)
template <class T>
inline Print &operator<<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}
template <>
inline Print &operator<<(Print &obj, float arg) {
  obj.print(arg, 4);
  return obj;
}

//-------------------------------------------------------------------------------------------------
// ODriveTeensyCAN Class Implementation
//-------------------------------------------------------------------------------------------------
ODriveTeensyCAN::ODriveTeensyCAN() {
  Can0.begin();
  Can0.setBaudRate(CANBaudRate);
}

bool ODriveTeensyCAN::readAsyncMessages() {
  CAN_message_t msg;
  bool got_message = false;
  while (Can0.read(msg)) {
    got_message = true;
    uint32_t cmd_id = (msg.id & 0b00000011111);       // last 5 bits are command ID
    uint32_t axis_id = ((msg.id & 0b11111100000) >> 5); // first 6 bits are axis ID

    switch (cmd_id) {
      case CMD_ID_GET_ENCODER_ESTIMATES:
        memcpy(&axis_data[axis_id].position_est, msg.buf, sizeof(float));
        memcpy(&axis_data[axis_id].velocity_est, msg.buf + 4, sizeof(float));
        break;
      case CMD_ID_GET_BUS_VOLTAGE_CURRENT:
        memcpy(&axis_data[axis_id].bus_voltage, msg.buf, sizeof(float));
        memcpy(&axis_data[axis_id].bus_current, msg.buf + 4, sizeof(float));
        break;
      default:
        break;
    }
  }
  return got_message;
}

void ODriveTeensyCAN::RequestPositionEstimate(int axis_id) {
  byte msg_data[8] = {0,0,0,0,0,0,0,0};
  sendMessageCool(axis_id, CMD_ID_GET_ENCODER_ESTIMATES, true, 8, msg_data);
}

float ODriveTeensyCAN::GetLastPositionEstimate(int axis_id) {
  return axis_data[axis_id].position_est;
}

void ODriveTeensyCAN::RequestBusVoltageCurrent(int axis_id) {
  byte msg_data[8] = {0,0,0,0,0,0,0,0};
  sendMessageCool(axis_id, CMD_ID_GET_BUS_VOLTAGE_CURRENT, true, 8, msg_data);
}

float ODriveTeensyCAN::GetLastBusVoltage(int axis_id) {
  return axis_data[axis_id].bus_voltage;
}

void ODriveTeensyCAN::sendMessage(int axis_id, int cmd_id,
                                  bool remote_transmission_request, int length,
                                  byte *signal_bytes) {
  CAN_message_t msg;
  CAN_message_t return_msg;
  msg.id = (axis_id << CommandIDLength) + cmd_id;
  msg.flags.remote = remote_transmission_request;
  msg.len = length;
  if (!remote_transmission_request) {
    memcpy(msg.buf, signal_bytes, length);
    Can0.write(msg);
    return;
  }
  Can0.write(msg);
  while (true) {
    if (Can0.read(return_msg) && (return_msg.id == msg.id)) {
      memcpy(signal_bytes, return_msg.buf, length);
      return;
    }
  }
}

void ODriveTeensyCAN::sendMessageCool(int axis_id, int cmd_id,
                                      bool remote_transmission_request, int length,
                                      byte *signal_bytes) {
  CAN_message_t msg;
  msg.id = (axis_id << CommandIDLength) + cmd_id;
  msg.flags.remote = remote_transmission_request;
  msg.len = length;
  memcpy(msg.buf, signal_bytes, length);
  Can0.write(msg);
}

int ODriveTeensyCAN::Heartbeat() {
  CAN_message_t return_msg;
  if (Can0.read(return_msg) == 1) {
    return (int)(return_msg.id >> 5);
  } else {
    return -1;
  }
}

void ODriveTeensyCAN::SetPosition(int axis_id, float position) {
  SetPosition(axis_id, position, 0.0f, 0.0f);
}

void ODriveTeensyCAN::SetPosition(int axis_id, float position, float velocity_feedforward) {
  SetPosition(axis_id, position, velocity_feedforward, 0.0f);
}

void ODriveTeensyCAN::SetPosition(int axis_id, float position, float velocity_feedforward, float current_feedforward) {
  int16_t vel_ff = (int16_t)(feedforwardFactor * velocity_feedforward);
  int16_t curr_ff = (int16_t)(feedforwardFactor * current_feedforward);
  byte *position_b = (byte *)&position;
  byte *velocity_feedforward_b = (byte *)&vel_ff;
  byte *current_feedforward_b = (byte *)&curr_ff;
  byte msg_data[8] = { position_b[0], position_b[1], position_b[2], position_b[3],
                       velocity_feedforward_b[0], velocity_feedforward_b[1],
                       current_feedforward_b[0], current_feedforward_b[1] };
  sendMessage(axis_id, CMD_ID_SET_INPUT_POS, false, 8, msg_data);
}

void ODriveTeensyCAN::SetVelocity(int axis_id, float velocity) {
  SetVelocity(axis_id, velocity, 0.0f);
}

void ODriveTeensyCAN::SetVelocity(int axis_id, float velocity, float current_feedforward) {
  byte *velocity_b = (byte *)&velocity;
  byte *current_feedforward_b = (byte *)&current_feedforward;
  byte msg_data[8] = { velocity_b[0], velocity_b[1], velocity_b[2], velocity_b[3],
                       current_feedforward_b[0], current_feedforward_b[1],
                       current_feedforward_b[2], current_feedforward_b[3] };
  sendMessage(axis_id, CMD_ID_SET_INPUT_VEL, false, 8, msg_data);
}

void ODriveTeensyCAN::SetVelocityLimit(int axis_id, float velocity_limit) {
  byte *velocity_limit_b = (byte *)&velocity_limit;
  sendMessage(axis_id, CMD_ID_SET_VELOCITY_LIMIT, false, 4, velocity_limit_b);
}

void ODriveTeensyCAN::SetTorque(int axis_id, float torque) {
  byte *torque_b = (byte *)&torque;
  sendMessage(axis_id, CMD_ID_SET_INPUT_TORQUE, false, 4, torque_b);
}

void ODriveTeensyCAN::ClearErrors(int axis_id) {
  byte garbage = 0;
  sendMessage(axis_id, CMD_ID_CLEAR_ERRORS, false, 0, &garbage);
}

float ODriveTeensyCAN::GetPosition(int axis_id) {
  byte msg_data[8] = {0,0,0,0,0,0,0,0};
  sendMessage(axis_id, CMD_ID_GET_ENCODER_ESTIMATES, true, 8, msg_data);
  float output;
  memcpy(&output, msg_data, sizeof(float));
  return output;
}

float ODriveTeensyCAN::GetVelocity(int axis_id) {
  byte msg_data[8] = {0,0,0,0,0,0,0,0};
  sendMessage(axis_id, CMD_ID_GET_ENCODER_ESTIMATES, true, 8, msg_data);
  float output;
  memcpy(&output, msg_data + 4, sizeof(float));
  return output;
}

uint32_t ODriveTeensyCAN::GetMotorError(int axis_id) {
  byte msg_data[4] = {0,0,0,0};
  sendMessage(axis_id, CMD_ID_GET_MOTOR_ERROR, true, 4, msg_data);
  uint32_t output;
  memcpy(&output, msg_data, sizeof(uint32_t));
  return output;
}

uint32_t ODriveTeensyCAN::GetEncoderError(int axis_id) {
  byte msg_data[4] = {0,0,0,0};
  sendMessage(axis_id, CMD_ID_GET_ENCODER_ERROR, true, 4, msg_data);
  uint32_t output;
  memcpy(&output, msg_data, sizeof(uint32_t));
  return output;
}

uint32_t ODriveTeensyCAN::GetAxisError(int axis_id) {
  byte msg_data[8] = {0,0,0,0,0,0,0,0};
  uint32_t output;
  CAN_message_t return_msg;
  int msg_id = (axis_id << CommandIDLength) + CMD_ID_ODRIVE_HEARTBEAT_MESSAGE;
  while (true) {
    if (Can0.read(return_msg) && (return_msg.id == msg_id)) {
      memcpy(msg_data, return_msg.buf, sizeof(return_msg.buf));
      memcpy(&output, msg_data, sizeof(uint32_t));
      return output;
    }
  }
}

uint32_t ODriveTeensyCAN::GetCurrentState(int axis_id) {
  CAN_message_t return_msg;
  int msg_id = (axis_id << CommandIDLength) + CMD_ID_ODRIVE_HEARTBEAT_MESSAGE;
  while (true) {
    if (Can0.read(return_msg) && (return_msg.id == msg_id)) {
      return (uint32_t)return_msg.buf[4];
    }
  }
}

bool ODriveTeensyCAN::RunState(int axis_id, int requested_state) {
  sendMessage(axis_id, CMD_ID_SET_AXIS_REQUESTED_STATE, false, 4, (byte *)&requested_state);
  return true;
}

//-------------------------------------------------------------------------------------------------
// Main Program: Setup (Direct Function Calls) and Loop
//-------------------------------------------------------------------------------------------------
ODriveTeensyCAN odrive;

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial port
  Serial.println(F("ODriveTeensyCAN Demo Program Started"));

  int axis_id = 0;  // using axis 0 for demonstration

  // Directly call functions without using a Serial command menu:

  // 1. Heartbeat
  int hb = odrive.Heartbeat();
  Serial.print("Heartbeat: ");
  Serial.println(hb);

  // 2. SetPosition Variants
  float position = 1.23f;
  odrive.SetPosition(axis_id, position);
  Serial.println("Called SetPosition (position only)");

  odrive.SetPosition(axis_id, position, 2.34f);
  Serial.println("Called SetPosition (with velocity feedforward)");

  odrive.SetPosition(axis_id, position, 2.34f, 3.45f);
  Serial.println("Called SetPosition (with velocity & current feedforward)");

  // 3. SetVelocity Variants
  float velocity = 4.56f;
  odrive.SetVelocity(axis_id, velocity);
  Serial.println("Called SetVelocity (velocity only)");

  odrive.SetVelocity(axis_id, velocity, 1.11f);
  Serial.println("Called SetVelocity (with current feedforward)");

  // 4. SetVelocityLimit
  odrive.SetVelocityLimit(axis_id, 5.67f);
  Serial.println("Called SetVelocityLimit");

  // 5. SetTorque
  odrive.SetTorque(axis_id, 6.78f);
  Serial.println("Called SetTorque");

  // 6. ClearErrors
  odrive.ClearErrors(axis_id);
  Serial.println("Called ClearErrors");

  // 7. Async Data Requests
  odrive.RequestPositionEstimate(axis_id);
  delay(100);  // allow time for async response
  float posEst = odrive.GetLastPositionEstimate(axis_id);
  Serial.print("Last Position Estimate: ");
  Serial.println(posEst);

  odrive.RequestBusVoltageCurrent(axis_id);
  delay(100);  // allow time for async response
  float busVolt = odrive.GetLastBusVoltage(axis_id);
  Serial.print("Last Bus Voltage: ");
  Serial.println(busVolt);

  // 8. Synchronous Getters
  float syncPos = odrive.GetPosition(axis_id);
  Serial.print("Synchronous GetPosition: ");
  Serial.println(syncPos);

  float syncVel = odrive.GetVelocity(axis_id);
  Serial.print("Synchronous GetVelocity: ");
  Serial.println(syncVel);

  uint32_t motorErr = odrive.GetMotorError(axis_id);
  Serial.print("Motor Error: ");
  Serial.println(motorErr, HEX);

  uint32_t encoderErr = odrive.GetEncoderError(axis_id);
  Serial.print("Encoder Error: ");
  Serial.println(encoderErr, HEX);

  uint32_t axisErr = odrive.GetAxisError(axis_id);
  Serial.print("Axis Error: ");
  Serial.println(axisErr, HEX);

  uint32_t currState = odrive.GetCurrentState(axis_id);
  Serial.print("Current State: ");
  Serial.println(currState);

  // 9. RunState
  odrive.RunState(axis_id, ODriveTeensyCAN::AXIS_STATE_CLOSED_LOOP_CONTROL);
  Serial.println("Called RunState (set to CLOSED_LOOP_CONTROL)");

  // 10. Read any async messages (if available)
  bool asyncRead = odrive.readAsyncMessages();
  Serial.print("Async messages read: ");
  Serial.println(asyncRead ? "Yes" : "No");
}

void loop() {
  // No further actions; all function calls are executed once in setup.
}
