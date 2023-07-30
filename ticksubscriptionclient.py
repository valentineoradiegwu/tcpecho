import socket
import binascii
import sys
import time
import struct

SUBS_HEADER_SIZE = 1;
SUBS_CRC_SIZE    = 4;

BBG_LEN = 9
CUSIP_LEN = 12

BBG_ID = 0
CUSIP_ID = 1

def read_n_bytes(skt, n):
    buff = bytearray(n)
    pos = 0
    while pos < n:
        nread = skt.recv_into(memoryview(buff)[pos:])
        if nread == 0:
            raise EOFError
        elif nread < 0:
            raise IOError
        pos += nread
    return buff

def parse_subscriptions(b):
    subs = []
    msg_len = len(b)
    start = 0
    while start < msg_len:
        msg_type = b[start]
        start += 1
        if msg_type == BBG_ID:
            subs.append(bytes(b[start: start + BBG_LEN]).decode('utf-8'))
            start += BBG_LEN
        elif msg_type == CUSIP_ID:
            subs.append(bytes(b[start: start + CUSIP_LEN]).decode('utf-8'))
            start += CUSIP_LEN
        else:
            raise TypeError(f"Unknown symbol type {msg_type}")
    return subs
                    

def get_tickers(skt):
    header = read_n_bytes(skt, SUBS_HEADER_SIZE)
    msg_len = header[0]
    body = read_n_bytes(skt, msg_len + SUBS_CRC_SIZE)
    #crc_incoming = socket.ntohl(int.from_bytes(body[-SUBS_CRC_SIZE:], sys.byteorder))
    crc_incoming = socket.ntohl(struct.unpack('I', body[-SUBS_CRC_SIZE:])[0])
    crc_calculated = binascii.crc32(header + body[:-SUBS_CRC_SIZE])
    print("crc_incoming = ", crc_incoming, "crc_calculated = ", crc_calculated)
    if not crc_incoming == crc_calculated:
        raise IOError("Data Integrity Error")
    return parse_subscriptions(body[:-SUBS_CRC_SIZE])

if __name__ == "__main__":
    addr = ("127.0.0.1", 7686)
    print("Python client starting")

    while True:
        try:
            skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            skt.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            skt.connect(addr)
            print(f"Connected to {addr}")
            tickers = get_tickers(skt)
            print("Subscribe to: ", tickers)
            break
        except socket.error:
            print("error connecting: retrying")
            skt.close()
            time.sleep(3)

