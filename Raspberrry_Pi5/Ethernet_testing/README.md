# Raspberry Pi Ethernet UDP Sender

This directory contains a Python script that configures the Raspberry Pi's Ethernet interface (eth0) with a static IP address and sends UDP packets to a connected Teensy device.

## Overview

- **Static IP Configuration:**  
  The script flushes any existing IP configuration on `eth0` and assigns a static IP (`192.168.0.132/24`) to the Pi's Ethernet interface.

- **UDP Packet Sending:**  
  Every 2 seconds, the script sends a UDP packet containing a test message to the Teensy device at IP `192.168.0.177` on UDP port `8888`.

- **Direct Connection:**  
  This setup assumes a direct Ethernet cable connection (or connection via a switch/router) between the Raspberry Pi and the Teensy.

## Requirements

- Raspberry Pi running a Linux-based OS (e.g., Raspberry Pi OS)
- Python 3 installed
- Sudo privileges (required to modify network settings)
- A direct Ethernet connection between the Pi and the Teensy

## How It Works

1. **Static IP Assignment:**  
   The script uses Linux `ip` commands to:
   
   > ```bash
   > sudo ip addr flush dev eth0
   > sudo ip addr add 192.168.0.132/24 dev eth0
   > sudo ip link set eth0 up
   > ```
   
   This sets a static IP on `eth0`.

2. **UDP Packet Transmission:**  
   Using Python's `socket` module, the script creates a UDP socket, and every 2 seconds it sends a message to the Teensy's IP (`192.168.0.177`) on port `8888`.

3. **Debug Output:**  
   The script prints out the current configuration of `eth0` and status messages for each packet sent. This helps in verifying that the packets are being transmitted.

## Usage Instructions

1. **Save the Script:**  
   Place the file (for example, `udp_sender_eth0.py`) in this directory.

2. **Make the Script Executable (Optional):**  
   Open a terminal and run:
   
   > ```bash
   > chmod +x udp_sender_eth0.py
   > ```

3. **Run the Script with Sudo:**  
   Because the script changes network settings, run it with elevated privileges:
   
   > ```bash
   > sudo python3 udp_sender_eth0.py
   > ```

4. **Verify the Configuration:**  
   The script will flush any existing IP on `eth0` and set the static IP. It will print the current configuration of `eth0` using:
   
   > ```bash
   > ip addr show eth0
   > ```

5. **Monitor UDP Traffic (Optional):**  
   To verify that UDP packets are being sent, use:
   
   > ```bash
   > sudo tcpdump -i eth0 'udp and port 8888'
   > ```

6. **Ensure Teensy Connectivity:**  
   Make sure your Teensy is configured with the static IP `192.168.0.177` and is running code that listens on UDP port `8888`.

## Troubleshooting

- **No Packets Observed:**  
  - Confirm the physical Ethernet connection between the Pi and Teensy.
  - Check that `eth0` has the correct static IP (`192.168.0.132/24`) using:
    
    > ```bash
    > ip addr show eth0
    > ```
    
  - Verify the routing for packets to the Teensy:
    
    > ```bash
    > ip route get 192.168.0.177
    > ```

- **Ping Failure:**  
  Test connectivity by pinging the Teensy:
  
  > ```bash
  > ping 192.168.0.177
  > ```
  
  If ping fails, the issue might be with the cable, network configuration, or the Teensy itself.

- **Firewall Issues:**  
  Check that no firewall rules are blocking UDP traffic:
  
  > ```bash
  > sudo ufw status verbose
  > sudo iptables -L -n
  > ```
