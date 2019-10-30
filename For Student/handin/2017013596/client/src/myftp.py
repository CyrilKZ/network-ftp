import socket
import re
import logging
import time

MODE_PASV = 1
MODE_PORT = 2
RECV_SIZE = 2048
FILE_UNIT = 8192

def getBytes(s):
  return bytes(s, 'utf8')

def getStr(b):
  return str(b, 'utf8')

class MyTFP:
  def __init__(self, url, port = 21):
    self.url = url
    self.port = port
    self.logged = False
    self.fileList = []
    self.rtInfo = logging.Logger(__name__)
    self.transferMode = MODE_PASV
    self.path = ''



  def debugcheck(self):
    self.rtInfo.info('checking')
  
  def setAddr(self, url):
    self.url = url

  def login(self, username = 'anonymous', password = 'k@no.com'):
    self.username = username
    self.password = password
    self.connfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # init connection
    try:
      self.connfd.connect((socket.gethostbyname(self.url), self.port))
    except Exception:
      self.rtInfo.error('cannot connect to ' + socket.gethostbyname(self.url))
      return False
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'220'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    
    # 'USER' command
    self.connfd.sendall(getBytes('USER ' + self.username + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'331'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    
    # 'PASS' command
    self.connfd.sendall(getBytes('PASS ' + self.password + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'230'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.logged = True
    self.rtInfo.info(getStr(res[: -2]))
    return True
  

  def changeMode(self, mode):
    self.transferMode = mode


  def establishDatafd(self, command, param=''):
    datafd = None
    # estabilish datafd
    if self.transferMode == MODE_PASV:
      self.connfd.sendall(getBytes('PASV\r\n'))
      res = self.connfd.recv(RECV_SIZE)
      if not res.startswith(b'227'):
        self.rtInfo.error(getStr(res[: -2]))
        return None
      self.rtInfo.info(getStr(res[: -2]))
      addr = getStr(res[27:-4]).split(',')
      pasvIp = '.'.join(addr[: -2])
      pasvPort = int(addr[4])*256 + int(addr[5])
      self.connfd.sendall(getBytes(command + ' ' + param + '\r\n'))
      datafd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      datafd.connect((pasvIp, pasvPort))
      res = self.connfd.recv(RECV_SIZE)
      if not res.startswith(b'150'):
        self.rtInfo.error(getStr(res[: -2]))
        return None
      self.rtInfo.info(getStr(res[: -2]))
    elif self.transferMode == MODE_PORT:
      listenfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      listenfd.setblocking(False)
      listenPort = [50, 5]
      listenfd.bind(('127.0.0.1', listenPort[0] * 256 + listenPort[1]))
      listenfd.listen(1)
      myip = ','.join(self.connfd.getsockname()[0].split('.'))
      self.connfd.sendall(getBytes('PORT ' + myip + ',' + str(listenPort[0]) + ',' + str(listenPort[1]) + '\r\n'))
      res = self.connfd.recv(RECV_SIZE)
      if not res.startswith(b'200'):
        self.rtInfo.error(getStr(res[: -2]))
        return None
      self.rtInfo.info(getStr(res[: -2]))
      self.connfd.sendall(getBytes(command + ' ' + param + '\r\n'))
      res = self.connfd.recv(RECV_SIZE)
      if not res.startswith(b'150'):
        self.rtInfo.error(getStr(res[: -2]))
        return None
      self.rtInfo.info(getStr(res[: -2]))
      flag = False
      for retries in range(5):
        time.sleep(0.2)
        try:
          datafd = listenfd.accept()[0]
          flag = True
          datafd.setblocking(True)
          listenfd.close()
          break
        except Exception:
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


  def downloadFile(self, localName, netName, offset, total):
    self.connfd.sendall(getBytes('REST ' + str(offset) + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'350'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))

    datafd = self.establishDatafd('RETR', netName)
    if datafd is None:
      return False
    
    
    self.rtInfo.info('starting: '+ str(total - offset) + ' bytes to download')
    totalstr = str(total)
    # transfer file
    fileMode = 'wb'
    if offset > 0:
      fileMode = 'ab'
    localFile = open(localName, fileMode)
    while True:
      data = datafd.recv(FILE_UNIT)
      if not data:
        break
      offset += data.__len__()
      self.rtInfo.info('downloading: ' + str(offset) +  '/' + totalstr + ' bytes received')
      localFile.write(data)
    datafd.close()
    localFile.close()
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'226'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True

  
  def downloadList(self):
    datafd = self.establishDatafd('LIST')
    if datafd is None:
      return False
    
    # transfer list
    resList = ''
    while True:
      data = datafd.recv(FILE_UNIT)
      if not data:
        break
      resList = resList + getStr(data)
    datafd.close()
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'226'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    self.fileList = []
    resList = resList.split('\r\n')
    for line in resList:
      if 'total' in line:
        continue
      
      infoLine = re.findall(r'[\w|\-|>|/|\.|\:]+', line)
      if infoLine.__len__() == 0:
        continue
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
    return True


  def uploadFile(self, localName, netName, total):
    datafd = self.establishDatafd('STOR', netName)
    if datafd is None:
      return False
    
    offset = 0
    
    total = str(total)
    self.rtInfo.info('starting: ' + total + ' bytes to upload')
    # transfer file
    localFile = open(localName, 'rb')
    while True:
      data = localFile.read(FILE_UNIT)
      if not data:
        break
      datafd.sendall(data)
      offset += data.__len__()
      self.rtInfo.info('uploading: ' + str(offset) + '/' + total + ' bytes received')
    datafd.close()
    localFile.close()
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'226'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True

  def sendMKD(self, dirname):
    self.connfd.sendall(getBytes('MKD ' + dirname + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'250'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True

  def sendCWD(self, dirname):
    self.connfd.sendall(getBytes('CWD ' + dirname + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'250'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True

  def sendRMD(self, dirname):
    self.connfd.sendall(getBytes('RMD ' + dirname + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'250'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True
  
  def sendDELE(self, filename):
    self.connfd.sendall(getBytes('DELE ' + filename + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'250'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True

  def sendPWD(self):
    self.connfd.sendall(getBytes('PWD\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'250'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    self.path = getStr(res[5: -4])
    return True

  def rename(self, oldname, newname):
    self.connfd.sendall(getBytes('RNFR ' + oldname + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'350'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    self.connfd.sendall(getBytes('RNTO ' + newname + '\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'250'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True
  
  def quit(self):
    self.connfd.sendall(getBytes('QUIT\r\n'))
    res = self.connfd.recv(RECV_SIZE)
    if not res.startswith(b'221'):
      self.rtInfo.error(getStr(res[: -2]))
      return False
    self.rtInfo.info(getStr(res[: -2]))
    return True