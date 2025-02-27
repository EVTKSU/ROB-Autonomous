#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p "python3.withPackages(ps: with ps; [ pyserial ])"

#BENS DOCUMENTATION: SOMEHOW I FIGURED OUT HOW TO GET IT WORKING ALL IN ONE SCRIPT> I'M NOT SURE HOW I DID IT, BUT I'M GOING TO TRY TO REPLICATE IT HERE
import struct
import time
import serial
from dataclasses import dataclass
from typing import Optional, Union, Tuple

@dataclass
class VescData:
    temp_mosfet: float = 0.0
    temp_motor: float = 0.0
    avg_motor_current: float = 0.0
    avg_input_current: float = 0.0
    duty_cycle_now: float = 0.0
    rpm: float = 0.0
    input_voltage: float = 0.0
    amp_hours: float = 0.0
    amp_hours_charged: float = 0.0
    watt_hours: float = 0.0
    watt_hours_charged: float = 0.0
    tachometer: int = 0
    tachometer_abs: int = 0
    error: int = 0
    pid_pos: float = 0.0
    controller_id: int = 0

@dataclass
class NunchuckValues:
    value_x: int = 127
    value_y: int = 127
    lower_button: bool = False
    upper_button: bool = False

@dataclass
class FirmwareVersion:
    major: int = 0
    minor: int = 0

class VescUart:
    # VESC Command IDs
    COMM_FW_VERSION = 0
    COMM_GET_VALUES = 4
    COMM_SET_DUTY = 5
    COMM_SET_CURRENT = 6
    COMM_SET_CURRENT_BRAKE = 7
    COMM_SET_RPM = 8
    COMM_SET_CHUCK_DATA = 23
    COMM_ALIVE = 29
    COMM_FORWARD_CAN = 33

    def __init__(self, serial_port: str, baudrate: int = 115200, timeout_ms: int = 100):
        """Initialize VESC UART communication"""
        self.serial = serial.Serial(serial_port, baudrate, timeout=timeout_ms/1000)
        self.timeout = timeout_ms
        self.data = VescData()
        self.nunchuck = NunchuckValues()
        self.fw_version = FirmwareVersion()

    def _crc16(self, data: bytes) -> int:
        """Calculate CRC16 for data validation"""
        crc = 0
        for byte in data:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc = crc << 1
            crc &= 0xFFFF
        return crc

    def _pack_payload(self, payload: bytes) -> bytes:
        """Pack payload with start byte, length, CRC, and end byte"""
        if len(payload) <= 256:
            message = bytes([2, len(payload)]) + payload
        else:
            message = bytes([3, len(payload) >> 8, len(payload) & 0xFF]) + payload
        
        crc = self._crc16(payload)
        message += struct.pack('>H', crc) + bytes([3])
        return message

    def _receive_uart_message(self) -> Tuple[bool, bytes]:
        """Receive and validate UART message"""
        start_time = time.time()
        message = bytearray()
        
        while (time.time() - start_time) * 1000 < self.timeout:
            if self.serial.in_waiting:
                byte = self.serial.read()
                message.append(byte[0])
                
                if len(message) >= 2:
                    if message[0] == 2:  # Short message
                        expected_length = message[1] + 5
                        if len(message) == expected_length and message[-1] == 3:
                            payload = message[2:2+message[1]]
                            received_crc = (message[-3] << 8) | message[-2]
                            if self._crc16(payload) == received_crc:
                                return True, payload
                            return False, b''
                            
        return False, b''

    def get_values(self, can_id: int = 0) -> bool:
        """Get all VESC values"""
        payload = bytes([self.COMM_FORWARD_CAN, can_id, self.COMM_GET_VALUES]) if can_id else bytes([self.COMM_GET_VALUES])
        self.serial.write(self._pack_payload(payload))
        
        success, message = self._receive_uart_message()
        if success and len(message) > 55:
            self._process_read_packet(message)
            return True
        return False

    def _process_read_packet(self, message: bytes) -> None:
        """Process received packet and update data structure"""
        packet_id = message[0]
        payload = message[1:]
        
        if packet_id == self.COMM_GET_VALUES:
            idx = 0
            self.data.temp_mosfet = struct.unpack_from('>h', payload, idx)[0] / 10.0; idx += 2
            self.data.temp_motor = struct.unpack_from('>h', payload, idx)[0] / 10.0; idx += 2
            self.data.avg_motor_current = struct.unpack_from('>f', payload, idx)[0] / 100.0; idx += 4
            self.data.avg_input_current = struct.unpack_from('>f', payload, idx)[0] / 100.0; idx += 4
            idx += 8  # Skip avg_id and avg_iq
            self.data.duty_cycle_now = struct.unpack_from('>h', payload, idx)[0] / 1000.0; idx += 2
            self.data.rpm = struct.unpack_from('>f', payload, idx)[0]; idx += 4
            self.data.input_voltage = struct.unpack_from('>h', payload, idx)[0] / 10.0; idx += 2
            # ... continue unpacking other values

    def set_current(self, current: float, can_id: int = 0) -> None:
        """Set motor current"""
        payload = struct.pack('>Bf', self.COMM_SET_CURRENT, current * 1000)
        if can_id:
            payload = bytes([self.COMM_FORWARD_CAN, can_id]) + payload
        self.serial.write(self._pack_payload(payload))

    def set_rpm(self, rpm: float, can_id: int = 0) -> None:
        """Set motor RPM"""
        payload = struct.pack('>Bi', self.COMM_SET_RPM, int(rpm))
        if can_id:
            payload = bytes([self.COMM_FORWARD_CAN, can_id]) + payload
        self.serial.write(self._pack_payload(payload))

    def set_duty(self, duty: float, can_id: int = 0) -> None:
        """Set duty cycle"""
        payload = struct.pack('>Bi', self.COMM_SET_DUTY, int(duty * 100000))
        if can_id:
            payload = bytes([self.COMM_FORWARD_CAN, can_id]) + payload
        self.serial.write(self._pack_payload(payload))

    def send_keepalive(self, can_id: int = 0) -> None:
        """Send keepalive message"""
        payload = bytes([self.COMM_ALIVE])
        if can_id:
            payload = bytes([self.COMM_FORWARD_CAN, can_id]) + payload
        self.serial.write(self._pack_payload(payload))

    def close(self) -> None:
        """Close serial connection"""
        self.serial.close()

if __name__ == "__main__":
    try:
        # Initialize VESC UART connection
        # Note: Replace "/dev/ttyUSB0" with your actual serial port
        vesc = VescUart("COM6", baudrate=115200)
        
        # Set duty cycle to 20%
        vesc.set_duty(.50)
        print("Set duty cycle to 20%")
        
        # Get and print current values
        if vesc.get_values():
            print(f"\nCurrent VESC Values:")
            print(f"Duty Cycle: {vesc.data.duty_cycle_now * 100:.1f}%")
            print(f"RPM: {vesc.data.rpm:.0f}")
            print(f"Current: {vesc.data.avg_motor_current:.1f}A")
            print(f"Voltage: {vesc.data.input_voltage:.1f}V")
        
    except serial.SerialException as e:
        print(f"Error: Could not open serial port - {e}")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Make sure to close the connection
        if 'vesc' in locals():
            vesc.close() 