#!/usr/bin/env python3
import socket
import subprocess
import time
import threading

# Configuration:
INTERFACE = "eth0"                # The Ethernet interface on the Pi
PI_IP_CIDR = "192.168.0.132/24"    # The static IP you want for the Pi on eth0
TEENSY_IP = "192.168.0.177"        # The Teensy's IP address
UDP_PORT = 8888                   # UDP port that both the Teensy and Pi use

def set_static_ip(interface, ip_cidr):
    """
    Set a static IP address on the specified interface using the `ip` command.
    """
    try:
        # Remove any existing IP on the interface (optional)
        subprocess.run(["sudo", "ip", "addr", "flush", "dev", interface], check=True)
        # Add the new IP
        subprocess.run(["sudo", "ip", "addr", "add", ip_cidr, "dev", interface], check=True)
        # Bring the interface up
        subprocess.run(["sudo", "ip", "link", "set", interface, "up"], check=True)
        print(f"Set {interface} to {ip_cidr}")
    except subprocess.CalledProcessError as e:
        print(f"Error setting static IP on {interface}: {e}")

def send_udp_message(message, sock_send):
    """
    Sends a UDP packet containing the given message to the Teensy's IP/port.
    """
    try:
        sock_send.sendto(message.encode("utf-8"), (TEENSY_IP, UDP_PORT))
    except Exception as e:
        print(f"Error sending UDP message: {e}")

def sender_loop():
    """
    Continuously send UDP packets.
    """
    sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    i = 0
    while True:
        steering_angle = round(i - 50, 3)
        throttle = i
        emergency = 1 if (i > 90 or i < 10) else 0
        message = f"{steering_angle},{throttle},{emergency}"
        send_udp_message(message, sock_send)
        
        time.sleep(0.25)
        if i > 100:
            i = 0
        else:
            i += 1

def receiver_loop():
    """
    Continuously listen for incoming UDP packets and print them.
    """
    # Extract the IP address from PI_IP_CIDR (ignoring the subnet mask)
    pi_ip = PI_IP_CIDR.split('/')[0]
    sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_recv.bind((pi_ip, UDP_PORT))
    print(f"Listening for UDP packets on {pi_ip}:{UDP_PORT}")
    while True:
        try:
            data, addr = sock_recv.recvfrom(4096)
            print(f"Received {len(data)} bytes from {addr}: {data.decode(errors='replace')}")
        except Exception as e:
            print(f"Error receiving UDP message: {e}")
    sock_recv.close()

def main():
    # Step 1: Set static IP on eth0
    set_static_ip(INTERFACE, PI_IP_CIDR)
    time.sleep(2)  # Allow time for the configuration to take effect

    # Optional: Print the current configuration of eth0 for verification
    print("\nCurrent eth0 configuration:")
    subprocess.run(["ip", "addr", "show", INTERFACE])
    print("\n")

    # Step 2: Start sender and receiver threads
    sender_thread = threading.Thread(target=sender_loop, daemon=True)
    receiver_thread = threading.Thread(target=receiver_loop, daemon=True)

    sender_thread.start()
    receiver_thread.start()

    # Keep the main thread alive.
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
