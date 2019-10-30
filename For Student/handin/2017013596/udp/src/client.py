import socket

num = 0 

try:
  size = 8192
  
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  for i in range(51):
    msg = bytes(str(i), 'utf-8')
    sock.sendto(msg, ('localhost', 9876))
    print(sock.recv(size))
    num += 1
  sock.close()  
except:
  print("cannot reach the server")