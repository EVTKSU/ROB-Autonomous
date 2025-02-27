#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p "python3.withPackages(ps: with ps; [ pyserial matplotlib ])"

import struct
import time
import serial
from dataclasses import dataclass
from typing import Tuple

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

# --------------------- TKINTER GUI CODE BELOW --------------------- #

import tkinter as tk
from tkinter import ttk
import threading
import csv
import os
from datetime import datetime
import serial.tools.list_ports
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class VescGUI:
    def __init__(self, master):
        self.master = master
        master.title("VESC UART Monitor")
        
        # Variables
        self.vesc = None
        self.logging_running = False
        self.log_data = []  # list to store log entries
        self.polling_thread = None
        
        # Top frame: Connection settings
        conn_frame = ttk.LabelFrame(master, text="Connection Settings")
        conn_frame.grid(row=0, column=0, padx=10, pady=10, sticky="ew")
        
        # Serial Port dropdown
        ttk.Label(conn_frame, text="Serial Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var, values=self.get_serial_ports(), state="readonly")
        self.port_combo.grid(row=0, column=1, padx=5, pady=5)
        if self.port_combo['values']:
            self.port_combo.current(0)
        
        # Baud Rate dropdown
        ttk.Label(conn_frame, text="Baud Rate:").grid(row=0, column=2, padx=5, pady=5, sticky="w")
        self.baud_var = tk.StringVar()
        self.baud_combo = ttk.Combobox(conn_frame, textvariable=self.baud_var, values=["9600", "115200", "230400", "460800", "921600"], state="readonly")
        self.baud_combo.grid(row=0, column=3, padx=5, pady=5)
        self.baud_combo.set("115200")
        
        # Start and Stop Logging buttons
        self.start_button = ttk.Button(conn_frame, text="Start Logging", command=self.start_logging)
        self.start_button.grid(row=1, column=0, padx=5, pady=5)
        self.stop_button = ttk.Button(conn_frame, text="Stop Logging", command=self.stop_logging, state="disabled")
        self.stop_button.grid(row=1, column=1, padx=5, pady=5)
        
        # Save CSV button
        self.save_button = ttk.Button(conn_frame, text="Save to CSV", command=self.save_csv, state="disabled")
        self.save_button.grid(row=1, column=2, padx=5, pady=5)
        
        # Frame for displaying current and voltage values
        disp_frame = ttk.LabelFrame(master, text="VESC Data")
        disp_frame.grid(row=1, column=0, padx=10, pady=10, sticky="ew")
        
        ttk.Label(disp_frame, text="Current (A):").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.current_label = ttk.Label(disp_frame, text="N/A")
        self.current_label.grid(row=0, column=1, padx=5, pady=5, sticky="w")
        
        ttk.Label(disp_frame, text="Voltage (V):").grid(row=0, column=2, padx=5, pady=5, sticky="w")
        self.voltage_label = ttk.Label(disp_frame, text="N/A")
        self.voltage_label.grid(row=0, column=3, padx=5, pady=5, sticky="w")
        
        # Graph frame for plotting current over time
        graph_frame = ttk.LabelFrame(master, text="Current Graph")
        graph_frame.grid(row=2, column=0, padx=10, pady=10, sticky="nsew")
        master.grid_rowconfigure(2, weight=1)
        master.grid_columnconfigure(0, weight=1)
        
        self.fig = Figure(figsize=(5, 3), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_title("Motor Current Over Time")
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Current (A)")
        self.line, = self.ax.plot([], [], 'b-')
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=1)
        
        # Data for graph
        self.graph_times = []
        self.graph_currents = []
        self.start_time = None

        # Motor Control frame for setting duty cycle
        control_frame = ttk.LabelFrame(master, text="Motor Control")
        control_frame.grid(row=3, column=0, padx=10, pady=10, sticky="ew")
        
        ttk.Label(control_frame, text="Duty Cycle (0-1):").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.duty_entry = ttk.Entry(control_frame)
        self.duty_entry.grid(row=0, column=1, padx=5, pady=5)
        self.set_duty_button = ttk.Button(control_frame, text="Set Duty", command=self.set_duty)
        self.set_duty_button.grid(row=0, column=2, padx=5, pady=5)

    def get_serial_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        return ports

    def start_logging(self):
        # Establish VESC connection using the selected serial port and baud rate.
        port = self.port_var.get()
        try:
            baud = int(self.baud_var.get())
        except ValueError:
            baud = 115200
        try:
            self.vesc = VescUart(port, baudrate=baud)
        except Exception as e:
            print(f"Error connecting to VESC: {e}")
            return
        
        self.logging_running = True
        self.start_button.config(state="disabled")
        self.stop_button.config(state="normal")
        self.save_button.config(state="disabled")
        self.log_data = []
        self.graph_times = []
        self.graph_currents = []
        self.start_time = time.time()
        
        # Start a separate thread to poll data from the VESC.
        self.polling_thread = threading.Thread(target=self.poll_data, daemon=True)
        self.polling_thread.start()

    def stop_logging(self):
        self.logging_running = False
        self.start_button.config(state="normal")
        self.stop_button.config(state="disabled")
        self.save_button.config(state="normal")
        if self.vesc:
            self.vesc.close()
            self.vesc = None

    def poll_data(self):
        while self.logging_running:
            if self.vesc and self.vesc.get_values():
                current = self.vesc.data.avg_motor_current
                voltage = self.vesc.data.input_voltage
                timestamp = time.time() - self.start_time
                self.log_data.append({"time": timestamp, "current": current, "voltage": voltage})
                
                # Update graph data
                self.graph_times.append(timestamp)
                self.graph_currents.append(current)
                
                # Update GUI display in the main thread
                self.master.after(0, self.update_display, current, voltage)
            time.sleep(0.1)

    def update_display(self, current, voltage):
        self.current_label.config(text=f"{current:.2f}")
        self.voltage_label.config(text=f"{voltage:.2f}")
        self.line.set_data(self.graph_times, self.graph_currents)
        self.ax.relim()
        self.ax.autoscale_view()
        self.canvas.draw_idle()

    def save_csv(self):
        if not self.log_data:
            return
        # Determine the directory where the script is located.
        script_dir = os.path.dirname(os.path.realpath(__file__))
        log_dir = os.path.join(script_dir, "logs")
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)
        filename = datetime.now().strftime("vesc_log_%Y%m%d_%H%M%S.csv")
        filepath = os.path.join(log_dir, filename)
        with open(filepath, "w", newline="") as csvfile:
            fieldnames = ["time", "current", "voltage"]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            for entry in self.log_data:
                writer.writerow(entry)
        print(f"Log saved to {filepath}")

    def set_duty(self):
        """Set the motor duty cycle based on the input box value."""
        try:
            duty = float(self.duty_entry.get())
        except ValueError:
            print("Invalid duty cycle value. Please enter a number between 0 and 1.")
            return
        if self.vesc:
            self.vesc.set_duty(duty)
            print(f"Duty cycle set to {duty}")
        else:
            print("Not connected to VESC. Start logging to establish a connection.")

def main():
    root = tk.Tk()
    app = VescGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
