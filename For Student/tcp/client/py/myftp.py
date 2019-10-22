import socket
import re
import logging
import time

MODE_PASV = 1
MODE_PORT = 2
RECV_SIZE = 2048
FILE_UNIT = 8192

class MyTFP:
  def __init__(self, url, port = 21):
    self.url = url
    self.port = port
    self.logged = False
    self.fileList = []
    self.rtInfo = logging.Logger(__name__)
    self.transferMode = MODE_PASV
  

  def login(self, username = 'anonymous', password = 'k@no.com'):
    self.username = username
    self.password = password
    self.connfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # init connection
    self.connfd.connect((self.url, self.port))
    res = self.sock.recv(RECV_SIZE)
    if not res.startswith('220'):
      self.rtInfo.error(res[: -2])
      return False
    self.rtInfo.info(res[: -2])
    
    # 'USER' command
    self.connfd.sendall('USER ' + self.username + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith('331'):
      self.rtInfo.error(res[: -2])
      return False
    self.rtInfo.info(res[: 2])
    
    # 'PASS' command
    self.connfd.sendall('PASS' + self.password + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith('230'):
      self.rtInfo.error(res[: -2])
      return False
    self.logged = True
    self.rtInfo.info(res[: -2])
    return True
  

  def changeMode(self, mode):
    self.transferMode = mode


  def establishDatafd(self):
    datafd = None
    # estabilish datafd
    if self.transferMode == MODE_PASV
      self.connfd.sendall('PASV\r\n')
      res = self.connfd.recv(RECV_SIZE)
      if not res.startswith('227'):
        self.rtInfo.error(res[: -2])
        return None
      self.rtInfo.info(res[: -2])
      addr = res[27:-4].split(',')
      pasvIp = '.'.join(res[:4])
      pasvPort = int(res[4])*256 + int(res[5])
      self.connfd.sendall('RETR ' + netName + '\r\n')
      if not res.startswith('150'):
        self.rtInfo.error(res[: -2])
        return None
      self.rtInfo.info(res[: -2])
      datafd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      datafd.connect((pasvIp, pasvPort))
    elif self.transferMode == MODE_PORT
      listenfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      listenfd.setblocking = None
      listenPort = ['50', '5']
      listenfd.bind(('127.0.0.1', listenPort[0] * 128 + listenPort[1]))
      listenfd.listen(1)
      connfd.sendall('PORT 127,0,0,1,' + listenPort[0] + ',' + listenPort[1] + '\r\n')
      res = self.connfd.recv(RECV_SIZE)
      if not res.startswith('227'):
        self.rtInfo.error(res[: -2])
        return None
      self.rtInfo.info(res[: -2])
      self.connfd.sendall('RETR ' + netName + '\r\n')
      if not res.startswith('150'):
        self.rtInfo.error(res[: -2])
        return None
      self.rtInfo.info(res[: -2])
      flag = False
      for retries in range(5):
        time.sleep(0.2)
        try:
          datafd = accept(listenfd)
          datafd.setblocking(True)
          flag = True
          listenfd.close()
          break
        except Exception, e:
          time.sleep(1)
          pass
      if not flag:
        self.rtInfo.error('connection timeout')
        listenfd.close()
        return None
    else:
      self.rtInfo.error('invalid mode')
      return None
    return datafd


  def downloadFile(self, localName, netName):
    datafd = self.establishDatafd()
    if datafd is None:
      return False
    
    # transfer file
    localFile = open(localName, 'wb')
    while True:
      data = datafd.recv(FILE_UNIT)
      if not data:
        break
      localFile.write(data)
    datafd.close()
    localFile.close()
    res = self.connfd.recv(RECV_SIZE)
    if not '226' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True

  
  def downloadList(self):
    datafd = self.establishDatafd()
    if datafd is None:
      return False
    
    # transfer list
    resList = ''
    while True:
      data = datafd.recv(FILE_UNIT)
      if not data:
        break
      resList = resList + data
    datafd.close()
    res = self.connfd.recv(RECV_SIZE)
    if not '226' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    self.fileList = []
    resList = resList.split('\r\n')
    for line in resList:
      if 'total' in line:
        continue
      infoLine = re.findall(r'[\w|\-|>|/|\.|\:]+', line)
      infoItem = {}
      infoItem['permission'] = infoLine[0]
      infoItem['linkNum'] = infoLine[1]
      infoItem['owner'] = infoLine[2]
      infoItem['ownerGroup'] = infoLine[3]
      infoItem['size'] = infoLine[4]
      if ':' in infoLine[7]:
        infoItem['dateTime'] = infoLine[5] + ' ' + infoLine[6] + ' ' + infoLine[7]
      else:
        infoItem['dateTime'] = infoLine[7] + ' ' + infoLine[5] + ' ' + infoLine[6]
      infoItem['name'] = infoLine[8]
      self.fileList.append(infoItem)
    res = self.connfd.recv(RECV_SIZE)
    if not '226' in res:
      self.rtInfo.error(res[: -2])
      return False
    self.rtInfo.info([: -2])
    return True


  def uploadFile(self, localName, netName):
    datafd = self.establishDatafd()
    if datafd is None:
      return False
    
    # transfer file
    localFile = open(localName, 'rb')
    data = localFile.read(FILE_UNIT)
    datafd.sendall(data)
    datafd.close()
    localFile.close()
    res = self.connfd.recv(RECV_SIZE)
    if not '226' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True


  def sendCWD(self, dirname):
    self.connfd.sendall('CWD ' + dirname + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not '250' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True

  def sendRMD(self, dirname):
    self.connfd.sendall('RMD ' + dirname + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not '250' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True
  
  def sendDELE(self, filename):
    self.connfd.sendall('DELE ' + filename + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not '250' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True

  def rename(self, oldname, newname):
    self.connfd.sendall('RNFR ' + oldname + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not '350' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True
    self.connfd.sendall('RNTO ' + newname + '\r\n')
    res = self.connfd.recv(RECV_SIZE)
    if not '250' in res:
      self.rtInfo.error([: -2])
      return False
    self.rtInfo.info([: -2])
    return True