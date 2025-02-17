import socket
import struct
import random

# STUN 服务器地址
STUN_SERVER = ('195.208.107.138', 3478)

# STUN 头部格式 (类型, 长度, 魔法 cookie, 事务 ID)
STUN_HEADER_FORMAT = '!HHI12s'
STUN_MAGIC_COOKIE = 0x2112A442


transaction_id = random.randbytes(12)


stun_request = struct.pack(STUN_HEADER_FORMAT, 0x0001, 0, STUN_MAGIC_COOKIE, transaction_id)


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(2)

try:
    sock.sendto(stun_request, STUN_SERVER)
    
    data, addr = sock.recvfrom(1024)
    print(f'Received response from {addr}: {data.hex()}')
except socket.timeout:
    print('No response received.')
finally:
    sock.close()