#ifndef vesc_can_h
#define vesc_can_h

#include <stdint.h>

#define MAX_NODE_ID 127  // CAN node id must be less than this

void vesc_can_begin();

bool vesc_can_read();

float vesc_can_get_erpm(uint8_t controller_id);
float vesc_can_get_rpm(uint8_t controller_id);
float vesc_can_get_current(uint8_t controller_id);
float vesc_can_get_duty(uint8_t controller_id);
float vesc_can_get_voltage(uint8_t controller_id);

void vesc_can_set_motor_poles(
    uint8_t controller_id,
    uint32_t motor_poles);  // used for rpm -> erpm conversion
bool vesc_can_set_duty(uint8_t controller_id, float duty);
bool vesc_can_set_current(uint8_t controller_id, float current);
bool vesc_can_set_current_brake(uint8_t controller_id, float current);
bool vesc_can_set_erpm(uint8_t controller_id, float erpm);
bool vesc_can_set_rpm(uint8_t controller_id, float rpm);
bool vesc_can_set_pos(uint8_t controller_id, float pos);

#endif  // vesc_can_h
