#!/usr/bin/env python3

"""
4. In Python, design a simple simulator/tester to drive your IMU parser. 

- The simulator should send a string of bytes in the proper format over the UART for your parser to accept
- The data contained in the byte string can be generated in any way of your choosing to best test the parser.
- Subsequently, it should then read the broadcasted results from 3) on the localhost network to validate the output.
"""

# NOTE: to me, simulator is less high fidelity than emulator - emulator usually means simulator with actual hardware emulator (FPGA). Just semantics.

import time
import socket
import struct
import serial
import argparse

# UART serial port
BAUD_RATE = 921600
SERIAL_PORT = '/dev/pts/7' # Set this to be the second argument of the socat output (second is arbitrary)

# UDP
ADDRESS = '127.0.0.1' # localhost
UDP_PORT = 9000

# Simulated IMU properties (guess)
SLEEP_SEC = 0.020 # 20ms would be like 4 packets every 80ms period - could go way faster if we wanted to do lots of samples/second

# TODO: make the SERIAL_PORT a command line argument because I don't know what the pty will be ahead of time

# open virtual serial port (we will write emulated IMU packet bytes to this)
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Open UDP socket to receive parsed packets parsed in the C++ "driver" (imu_parser.cpp)
udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_sock.bind(('127.0.0.1', UDP_PORT)) # localhost:9000 same as C++ program, could add to argparse later
udp_sock.settimeout(1)

# builds a valid 20 byte IMU packet
def make_packet(count, x, y, z):
    sync = b'\x7F\xF0\x1C\xAF' # sync header bytes, hopefully will all be binary
    count_bytes = struct.pack('>I', count)  # big endian uint32
    x_bytes = struct.pack('>f', x)          # big endian float
    y_bytes = struct.pack('>f', y)
    z_bytes = struct.pack('>f', z)
    return sync + count_bytes + x_bytes + y_bytes + z_bytes
   
# main logic below (I guess I could put it in a function called main C++ style)
   
print(f"Sending to {SERIAL_PORT}, listening on localhost address {ADDRESS} UDP port {UDP_PORT}...")

count = 1 # originally I didn't know if this was the packet payload length or just an incrementing id to differentiate packets, assuming the latter
while True:
    pkt = make_packet(count, 10.0, 20.0, 40.0)
    ser.write(pkt)
    count += 1 # ok if it wraps

    try:
        data, _ = udp_sock.recvfrom(1024)
        print("RECV:", data.decode('utf-8'))
    except socket.timeout:
        pass

    time.sleep(SLEEP_SEC)  # 1 packet every 20ms (just a guess to simulate IMU)


