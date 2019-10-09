import socket

size = 8192

total = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

try:
  while True:
    data, address = sock.recvfrom(size)
    total += 1
    res = bytes(str(total) + ' ', 'utf-8')
    sock.sendto(res + data.upper(), address)
finally:
  sock.close()