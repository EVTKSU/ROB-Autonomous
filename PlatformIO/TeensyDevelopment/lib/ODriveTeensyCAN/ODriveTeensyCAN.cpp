#include "ODriveTeensyCAN.h"

#include <FlexCAN_T4.h>
#include <stdint.h>
#include <util.h>

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

static const float feedforwardFactor = 1 / 0.001;

static const int CANBaudRate = 250000;

// Print with stream operator
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

#ifdef ARDUINO_TEENSY35
FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> Can0;
#elif ARDUINO_TEENSY40
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
#elif ARDUINO_TEENSY41
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> Can0;
#endif

struct axis_async_data {
  float position_est;
  float velocity_est;
  float bus_voltage;
  float bus_current;
};
axis_async_data axis_data[16];

ODriveTeensyCAN::ODriveTeensyCAN() {
  Can0.begin();
  Can0.setBaudRate(CANBaudRate);
}

bool ODriveTeensyCAN::readAsyncMessages() {
  CAN_message_t msg;

  // Serial.printf("Async Read");

  bool got_message = false;
  while (Can0.read(msg)) {
    got_message = true;

    uint32_t cmd_id =
        0b00000011111 & msg.id;  // last 5 bits of CAN id is command ID
    uint32_t axis_id =
        (0b11111100000 & msg.id) >> 5;  // first 6 bits of CAN id is axis ID

    //Serial.printf("cmd_id: %li\n", cmd_id);
    //Serial.printf("axis_id: %li\n", axis_id);
    switch (cmd_id) {
      case CMD_ID_GET_ENCODER_ESTIMATES: {
        memcpy(&axis_data[axis_id].position_est, msg.buf, sizeof(float));
        memcpy(&axis_data[axis_id].velocity_est, msg.buf + 4, sizeof(float));

        //Serial.printf("%x ", axis_id);
        //Serial.printf("[ ");
        //for (int i = 0; i < msg.len; i++) {
        //  Serial.printf("%02x ", msg.buf[i]);
        //}
        //Serial.printf("] ");
        //Serial.printf("%f ", axis_data[axis_id].position_est);
        //Serial.printf("%f \n", axis_data[axis_id].velocity_est);
      } break;

      case CMD_ID_GET_BUS_VOLTAGE_CURRENT: {
        memcpy(&axis_data[axis_id].bus_voltage, msg.buf, sizeof(float));
        memcpy(&axis_data[axis_id].bus_current, msg.buf + 4, sizeof(float));
      } break;

    }
  }

  return got_message;
};

void ODriveTeensyCAN::RequestPositionEstimate(int axis_id) {
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  sendMessageCool(axis_id, CMD_ID_GET_ENCODER_ESTIMATES, true, 8, msg_data);
}

float ODriveTeensyCAN::GetLastPositionEstimate(int axis_id) {
  return axis_data[axis_id].position_est;
}

void ODriveTeensyCAN::RequestBusVoltageCurrent(int axis_id) {
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  sendMessageCool(axis_id, CMD_ID_GET_BUS_VOLTAGE_CURRENT, true, 8, msg_data);
}

float ODriveTeensyCAN::GetLastBusVoltage(int axis_id) {
  return axis_data[axis_id].bus_voltage;
}

// Length must be < 8
// Length must be the size of the signal_bytes array
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
  CAN_message_t return_msg;

  msg.id = (axis_id << CommandIDLength) + cmd_id;
  msg.flags.remote = remote_transmission_request;
  msg.len = length;

  memcpy(msg.buf, signal_bytes, length);
  Can0.write(msg);
  return;
}

int ODriveTeensyCAN::Heartbeat() {
  CAN_message_t return_msg;
  Serial.println(Can0.read(return_msg));
  if (Can0.read(return_msg) == 1) {
    return (int)(return_msg.id >> 5);
  } else {
    return -1;
  }
}

void ODriveTeensyCAN::SetPosition(int axis_id, float position) {
  SetPosition(axis_id, position, 0.0f, 0.0f);
}

void ODriveTeensyCAN::SetPosition(int axis_id, float position,
                                  float velocity_feedforward) {
  SetPosition(axis_id, position, velocity_feedforward, 0.0f);
}

void ODriveTeensyCAN::SetPosition(int axis_id, float position,
                                  float velocity_feedforward,
                                  float current_feedforward) {
  int16_t vel_ff = (int16_t)(feedforwardFactor * velocity_feedforward);
  int16_t curr_ff = (int16_t)(feedforwardFactor * current_feedforward);

  byte *position_b = (byte *)&position;
  byte *velocity_feedforward_b = (byte *)&vel_ff;
  byte *current_feedforward_b = (byte *)&curr_ff;
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  msg_data[0] = position_b[0];
  msg_data[1] = position_b[1];
  msg_data[2] = position_b[2];
  msg_data[3] = position_b[3];
  msg_data[4] = velocity_feedforward_b[0];
  msg_data[5] = velocity_feedforward_b[1];
  msg_data[6] = current_feedforward_b[0];
  msg_data[7] = current_feedforward_b[1];

  sendMessage(axis_id, CMD_ID_SET_INPUT_POS, false, 8, msg_data);
}

void ODriveTeensyCAN::SetVelocity(int axis_id, float velocity) {
  SetVelocity(axis_id, velocity, 0.0f);
}

void ODriveTeensyCAN::SetVelocity(int axis_id, float velocity,
                                  float current_feedforward) {
  byte *velocity_b = (byte *)&velocity;
  byte *current_feedforward_b = (byte *)&current_feedforward;
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  msg_data[0] = velocity_b[0];
  msg_data[1] = velocity_b[1];
  msg_data[2] = velocity_b[2];
  msg_data[3] = velocity_b[3];
  msg_data[4] = current_feedforward_b[0];
  msg_data[5] = current_feedforward_b[1];
  msg_data[6] = current_feedforward_b[2];
  msg_data[7] = current_feedforward_b[3];

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
  // memcpy with length 0 still needs a valid pointer by the standard
  byte garbage = 0;
  sendMessage(axis_id, CMD_ID_CLEAR_ERRORS, false, 0, &garbage);
}

float ODriveTeensyCAN::GetPosition(int axis_id) {
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  sendMessage(axis_id, CMD_ID_GET_ENCODER_ESTIMATES, true, 8, msg_data);

  float_t output;
  *((uint8_t *)(&output) + 0) = msg_data[0];
  *((uint8_t *)(&output) + 1) = msg_data[1];
  *((uint8_t *)(&output) + 2) = msg_data[2];
  *((uint8_t *)(&output) + 3) = msg_data[3];
  return output;
}

float ODriveTeensyCAN::GetVelocity(int axis_id) { //
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  sendMessage(axis_id, CMD_ID_GET_ENCODER_ESTIMATES, true, 8, msg_data); //

  float_t output;
  *((uint8_t *)(&output) + 0) = msg_data[4];
  *((uint8_t *)(&output) + 1) = msg_data[5];
  *((uint8_t *)(&output) + 2) = msg_data[6];
  *((uint8_t *)(&output) + 3) = msg_data[7];
  return output;
}

uint32_t ODriveTeensyCAN::GetMotorError(int axis_id) {
  byte msg_data[4] = {0, 0, 0, 0};

  sendMessage(axis_id, CMD_ID_GET_MOTOR_ERROR, true, 4, msg_data);

  uint32_t output;
  *((uint8_t *)(&output) + 0) = msg_data[0];
  *((uint8_t *)(&output) + 1) = msg_data[1];
  *((uint8_t *)(&output) + 2) = msg_data[2];
  *((uint8_t *)(&output) + 3) = msg_data[3];
  return output;
}

uint32_t ODriveTeensyCAN::GetEncoderError(int axis_id) {
  byte msg_data[4] = {0, 0, 0, 0};

  sendMessage(axis_id, CMD_ID_GET_ENCODER_ERROR, true, 4, msg_data);

  uint32_t output;
  *((uint8_t *)(&output) + 0) = msg_data[0];
  *((uint8_t *)(&output) + 1) = msg_data[1];
  *((uint8_t *)(&output) + 2) = msg_data[2];
  *((uint8_t *)(&output) + 3) = msg_data[3];
  return output;
}

uint32_t ODriveTeensyCAN::GetAxisError(int axis_id) {
  byte msg_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t output;

  CAN_message_t return_msg;

  int msg_id = (axis_id << CommandIDLength) + CMD_ID_ODRIVE_HEARTBEAT_MESSAGE;

  while (true) {
    if (Can0.read(return_msg) && (return_msg.id == msg_id)) {
      memcpy(msg_data, return_msg.buf, sizeof(return_msg.buf));
      *((uint8_t *)(&output) + 0) = msg_data[0];
      *((uint8_t *)(&output) + 1) = msg_data[1];
      *((uint8_t *)(&output) + 2) = msg_data[2];
      *((uint8_t *)(&output) + 3) = msg_data[3];
      return output;
    }
  }
}

uint32_t ODriveTeensyCAN::GetCurrentState(int axis_id) {
  CAN_message_t return_msg;

  int msg_id = (axis_id << CommandIDLength) + CMD_ID_ODRIVE_HEARTBEAT_MESSAGE;

  while (true) {
    if (Can0.read(return_msg) && (return_msg.id == msg_id)) {
      // Serial.printf("%d: [ ", axis_id);
      // for (int i = 0; i < 8; i++) {
      //     Serial.printf("%x", return_msg.buf[i]);
      //     if (i != 7) {
      //         Serial.print(", ");
      //     }
      // }
      // Serial.println(" ]");

      return (uint32_t)return_msg.buf[4];
    }
  }
}

bool ODriveTeensyCAN::RunState(int axis_id, int requested_state) {
  sendMessage(axis_id, CMD_ID_SET_AXIS_REQUESTED_STATE, false, 4,
              (byte *)&requested_state);
  return true;
}
