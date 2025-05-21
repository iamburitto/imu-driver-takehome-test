#!/usr/bin/env python3

"""
4. In Python, design a simple simulator/tester to drive your IMU parser. 

- The simulator should send a string of bytes in the proper format over the UART for your parser to accept
- The data contained in the byte string can be generated in any way of your choosing to best test the parser.
- Subsequently, it should then read the broadcasted results from 3) on the localhost network to validate the output.

# localhost:9000 same as C++ program
"""

# NOTE: to me, simulator is less high fidelity than emulator - emulator usually means simulator with actual hardware emulator (FPGA). Just semantics.

import time
import socket
import struct
import serial
import argparse

# ---------------- Constants ---------------- #

# Simulated IMU properties (guess)
SLEEP_SEC = 0.020 # 1 packet every 20ms == 50Hz, c++ processes every 80ms (12.5hz) - just a guess

# ---------------- Argument Parsing ---------------- #
parser = argparse.ArgumentParser(description="Fake IMU Simulator/Tester")
parser.add_argument("serial_port", help="Path to virtual UART device PTY (e.g., /dev/pts/7), second argument of the socat output")
parser.add_argument("--udp_port", type=int, default=9000, help="UDP port to listen on (default: 9000)")
args = parser.parse_args()

# ---------------- Serial Port (out) ---------------- #
# open virtual serial port (we will write emulated IMU packet bytes to this)
ser = serial.Serial(args.serial_port, 921600, timeout=1)

# ---------------- UDP Socket (in) ---------------- #
# Open UDP socket to receive packets parsed in the C++ "driver" (imu_parser.cpp) and re-broadcast out over UDP on localhost:9000
udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_sock.bind(('127.0.0.1', args.udp_port)) # localhost
udp_sock.settimeout(1)

# ---------------- Packet Builder ---------------- #
# builds a valid 20 byte IMU packet
def make_packet(count, x, y, z):
    sync = b'\x7F\xF0\x1C\xAF' # sync header bytes, hopefully will all be binary
    count_bytes = struct.pack('>I', count)  # big endian uint32
    x_bytes = struct.pack('>f', x)          # big endian float
    y_bytes = struct.pack('>f', y)
    z_bytes = struct.pack('>f', z)
    return sync + count_bytes + x_bytes + y_bytes + z_bytes

# ---------------- Main Loop ---------------- #
# main logic below (I guess I could put it in a function called main C++ style)
   
print(f"Sending IMU packets to {args.serial_port}, listening for UDP on localhost port {args.udp_port}...")
  
count = 1 # originally I didn't know if this was the packet payload length or just an incrementing id to differentiate packets, assuming the latter
while True:
    pkt = make_packet(count, 10.0 + count % 3, 20.0 + count % 5, 40.0 + count % 7)
    ser.write(pkt) # terminal: echo -ne '\x7f\xf0\x1c\xaf\x00\x00\x00\x01\x41\x20\x00\x00\x41\xa0\x00\x00\x42\x20\x00\x00' > /dev/pts/7
    count += 1 # ok if wraps

    try:
        data, _ = udp_sock.recvfrom(1024)
        print("RECV:", data.decode('utf-8')) # terminal: netcat -ul 9000
    except socket.timeout:
        pass
    
    time.sleep(SLEEP_SEC) # 0.020 sec == 20ms == 50Hz, c++ processes every 80ms (12.5hz)




