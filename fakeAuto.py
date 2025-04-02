#!/usr/bin/env python3
import socket
import subprocess
import time

# run with sudo


# Configuration:
INTERFACE = "eth0"                # The Ethernet interface on the Pi
PI_IP_CIDR = "192.168.0.132/24"    # The static IP you want for the Pi on eth0
TEENSY_IP = "192.168.0.177"        # The Teensy's IP address
UDP_PORT = 8888                  # UDP port that Teensy is listening on

def set_static_ip(interface, ip_cidr):
    """
    Set a static IP address on the specified interface using the `ip` command.
    """
    try:
        # First, remove any existing IP on the interface (optional)
        subprocess.run(["sudo", "ip", "addr", "flush", "dev", interface], check=True)
        # Add the new IP
        subprocess.run(["sudo", "ip", "addr", "add", ip_cidr, "dev", interface], check=True)
        # Bring the interface up
        subprocess.run(["sudo", "ip", "link", "set", interface, "up"], check=True)
        print(f"Set {interface} to {ip_cidr}")
    except subprocess.CalledProcessError as e:
        print(f"Error setting static IP on {interface}: {e}")

def send_udp_message(message):
    """
    Sends a UDP packet containing the given message to the Teensy's IP/port.
    """
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Send the message (encoded in UTF-8)
        sock.sendto(message.encode("utf-8"), (TEENSY_IP, UDP_PORT))
        print(f"Sent UDP packet to {TEENSY_IP}:{UDP_PORT}: '{message}'")
    except Exception as e:
        print(f"Error sending UDP message: {e}")
    finally:
        sock.close()

def main():
    # Step 1: Set static IP on eth0
    set_static_ip(INTERFACE, PI_IP_CIDR)
    
    # Allow some time for the changes to take effect
    time.sleep(2)
    
    # Optional: Print the current configuration of eth0 for verification
    print("\nCurrent eth0 configuration:")
    subprocess.run(["ip", "addr", "show", INTERFACE])
    print("\n")
    
    # Step 2: Send UDP packets in a loop
    print("Starting UDP sender...")
    steering_angle = float(0)
    throttle = float(0)
    emergency = 0
    while True:
        for j in range(100000):
            i = round(j/1000,3)
            steering_angle = 2.2
            throttle = i
            if i > 90 or i < 10:
                emergency = 1
            else:
                emergency = 0
            message = str(steering_angle) +","+str(throttle)+","+str(emergency)
            send_udp_message(message)
            time.sleep(1/(j+1))

if __name__ == "__main__":
    main()
