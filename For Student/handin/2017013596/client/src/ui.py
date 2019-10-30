import sys
import time
import os
from logging import Logger, Handler, getLogger, Formatter
from PyQt5 import QtGui, QtCore, QtWidgets
from myftp import MyTFP

def appender(widget, obj):
  widget.append(obj)

class loggerInfo(Handler, QtCore.QObject): 
  signal = QtCore.pyqtSignal(str)
  progress = QtCore.pyqtSignal(str)
  def __init__(self, widget1, widget2):
    Handler.__init__(self)
    QtCore.QObject.__init__(self)
    self.signal.connect(widget1.append, type=QtCore.Qt.QueuedConnection)
    self.progress.connect(widget2.setText, type=QtCore.Qt.QueuedConnection)

  def emit(self, msg):
    text = self.format(msg)
    if 'downloading:' in text or 'uploading' in text:
      self.progress.emit(text)
    else:
      self.signal.emit(text)

class WorkerThread(QtCore.QThread):
  interface_signal_toLogin = QtCore.pyqtSignal()
  interface_signal_toMain = QtCore.pyqtSignal()
  interface_signal_error = QtCore.pyqtSignal(str)
  interface_signal_info = QtCore.pyqtSignal(str)
  interface_signal_refresh = QtCore.pyqtSignal()
  interface_signal_update = QtCore.pyqtSignal()
  def __init__(self, ouput1, ouput2):
    self.loggerinfo = loggerInfo(ouput1, ouput2)
    QtCore.QThread.__init__(self)
    self.lock = QtCore.QMutex()
    self.params = {
      'address': '',
      'username': '',
      'password': '',
      'netname': '',
      'localname': '',
      'tranfermode': 1,
      'offset': 0,
      'total': 0
    }
    self.tasks = {
      'login': self.login,
      'logout': self.logout,
      'getList': self.getList,
      'changeDir': self.changeDir,
      'makeDir': self.makeDir,
      'deleteNode': self.deleteNode,
      'removeDir': self.removeDir,
      'rename': self.rename,
      'uploadFile': self.uploadFile,
      'downloadFile': self.downloadFile,
      'nothing': self.donothing,
    }
    self.task = 'nothing'
    self.ftp = MyTFP('')
    self.loggerinfo.setFormatter(Formatter('%(asctime)s %(levelname)s: %(message)s'))
    self.ftp.rtInfo.addHandler(self.loggerinfo)

  def login(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot connect to server'
    try:
      self.ftp.setAddr(self.params['address'])
      res = self.ftp.login(self.params['username'], self.params['password'])
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_toMain.emit()
      return res, 'Info: login successful'

  def logout(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot connect to server'
    try:
      res = self.ftp.quit()
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_toLogin.emit()
      return res, 'Info: logout successful'

  def getList(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot refresh list'       
    try:
      self.ftp.changeMode(self.params['tranfermode'])
      res = self.ftp.downloadList() and self.ftp.sendPWD() 
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_refresh.emit()
      return res, 'Info: Filelist refreshed'

  def donothing(self):
    return False, 'Error: unkonwn command'   

  def setParams(self, params):
    if not self.lock.tryLock(): 
      self.interface_signal_error.emit('Waring: working, please wait')
      return False
    self.params.update(params)
    self.lock.unlock()
    return True

  def setTask(self, t):
    self.task = t

  def changeDir(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot change directory'
    try:
      res = self.ftp.sendCWD(self.params['netname'])
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Info: CD successful'

  def makeDir(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot make directory'
    try:
      res = self.ftp.sendMKD(self.params['localname'])
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Info: MKD successful'

  def removeDir(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot remove directory'
    try:
      res = self.ftp.sendRMD(self.params['netname'])
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Info: RMD successful'

  def deleteNode(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot delete file'
    try:
      res = self.ftp.sendDELE(self.params['netname'])
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Info: DELE successful'

  def rename(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot rename file/dir'
    try:
      res = self.ftp.rename(self.params['netname'], self.params['localname'])
    except Exception:
      pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Info: Rename successful'

  
  def uploadFile(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot upload file'
    # try:
    self.ftp.changeMode(self.params['tranfermode'])
    res = self.ftp.uploadFile(self.params['localname'], self.params['netname'], self.params['total'])
    # except Exception:
    #   pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Important: upload successful'

  def downloadFile(self):
    if not self.lock.tryLock(): 
      return False, 'Waring: working, please wait'
    res, info = False, 'Error: cannot download file'
    # try:
    self.ftp.changeMode(self.params['tranfermode'])
    res = self.ftp.downloadFile(self.params['localname'], self.params['netname'], self.params['offset'], self.params['total'])
    # except Exception:
    #   pass
    self.lock.unlock()
    if not res:
      return res, info
    else:
      self.interface_signal_update.emit()
      return res, 'Important: download successful'

  def run(self):
    res, info = self.tasks[self.task]()
    if not res:
      self.interface_signal_error.emit(info)
    elif info.startswith('Import'):
      self.interface_signal_info.emit(info)
    
    


class MyInterface(QtWidgets.QWidget):

  def __init__(self):
    super(MyInterface, self).__init__()
    
    # login page
    self.addrLabel = QtWidgets.QLabel('serverIP:', self)
    self.addrText = QtWidgets.QLineEdit(self)
    self.addrText.setText('127.0.0.1')

    self.usernameLabel = QtWidgets.QLabel('username:', self)
    self.usernameText = QtWidgets.QLineEdit(self)
    self.usernameText.setText('anonymous')

    self.passwordLabel = QtWidgets.QLabel('password:', self)
    self.passwordText = QtWidgets.QLineEdit(self)
    self.passwordText.setText('nopsd')

    self.loginButton = QtWidgets.QPushButton('login', self)
    self.loginButton.clicked.connect(self.login)

    # main page
    self.logoutButton = QtWidgets.QPushButton('logout', self)
    self.logoutButton.clicked.connect(self.logout)

    self.downloadButton = QtWidgets.QPushButton('download...', self)
    self.downloadButton.clicked.connect(self.download)

    self.uploadButton = QtWidgets.QPushButton('upload...', self)
    self.uploadButton.clicked.connect(self.upload)

    self.pathLabel = QtWidgets.QLabel('current path:')
    self.pathText = QtWidgets.QLineEdit(self)
    self.pathText.setReadOnly(True)

    self.mainTableLabel = QtWidgets.QLabel('file list:', self)
    self.mainTable = QtWidgets.QTreeWidget()
    self.mainTable.setHeaderLabels(['filename', 'size', 'type', 'time', 'premission', 'owner group'])
    self.mainTable.itemDoubleClicked.connect(self.changeDir)

    self.updateButton = QtWidgets.QPushButton('refresh', self)
    self.updateButton.clicked.connect(self.updateFromServer)

    self.deleButton = QtWidgets.QPushButton('delete', self)
    self.deleButton.clicked.connect(self.deleteNode)

    self.renameButton = QtWidgets.QPushButton('rename', self)
    self.renameButton.clicked.connect(self.renameNode)

    self.paramLabel = QtWidgets.QLabel('upload as / rename to / add as :', self)
    self.paramText = QtWidgets.QLineEdit(self)

    self.makedirButton = QtWidgets.QPushButton('add dir', self)
    self.makedirButton.clicked.connect(self.makeDir)

    self.patchButton = QtWidgets.QPushButton('patch...', self)
    self.patchButton.clicked.connect(self.patch)

    self.reportLabel = QtWidgets.QLabel('', self)
    self.report = QtWidgets.QTextBrowser(self)
    self.report.setFixedHeight(80)

    self.progressBar = QtWidgets.QLineEdit(self)
    self.progressBar.setReadOnly(True)

    self.modeLabel = QtWidgets.QLabel('', self)
    self.modeButton = QtWidgets.QPushButton('change mode', self)
    self.modeButton.clicked.connect(self.changeMode)

    self.worker = WorkerThread(self.report, self.progressBar)
    self.worker.interface_signal_error.connect(self.errorAlert, type=QtCore.Qt.QueuedConnection)
    self.worker.interface_signal_info.connect(self.infoAlert, type=QtCore.Qt.QueuedConnection)
    self.worker.interface_signal_toLogin.connect(self.toLogin, type=QtCore.Qt.QueuedConnection)
    self.worker.interface_signal_toMain.connect(self.toMain, type=QtCore.Qt.QueuedConnection)
    self.worker.interface_signal_refresh.connect(self.refresh, type=QtCore.Qt.QueuedConnection)
    self.worker.interface_signal_update.connect(self.updateFromServer, type=QtCore.Qt.QueuedConnection)

    # layout
    self.grid = QtWidgets.QGridLayout()
    self.grid.addWidget(self.addrLabel, 0, 0, 1, 1)
    self.grid.addWidget(self.addrText, 0, 1, 1, 7)
    self.grid.addWidget(self.usernameLabel, 1, 0, 1, 1)
    self.grid.addWidget(self.usernameText, 1, 1, 1, 3)
    self.grid.addWidget(self.passwordLabel, 1, 4, 1, 1)
    self.grid.addWidget(self.passwordText, 1, 5, 1, 3)
    self.grid.addWidget(self.loginButton, 3, 0, 1, 8)   
    self.grid.addWidget(self.pathLabel, 4, 0, 1, 1)
    self.grid.addWidget(self.pathText, 4, 1, 1, 7)
    self.grid.addWidget(self.mainTableLabel, 5, 0, 1, 8)
    self.grid.addWidget(self.mainTable, 6, 0, 1, 8)
    self.grid.addWidget(self.downloadButton, 7, 0, 1, 2)
    self.grid.addWidget(self.patchButton, 7, 2, 1, 2)
    self.grid.addWidget(self.updateButton, 7, 4, 1, 2)
    self.grid.addWidget(self.deleButton, 7, 6, 1, 2)
    self.grid.addWidget(self.paramLabel, 8, 0, 1, 2)
    self.grid.addWidget(self.paramText, 8, 2, 1, 6)
    self.grid.addWidget(self.uploadButton, 9, 0, 1, 4)
    self.grid.addWidget(self.renameButton, 9, 4, 1, 2)
    self.grid.addWidget(self.makedirButton, 9, 6, 1, 2)
    self.grid.addWidget(self.modeLabel, 10, 0, 1, 2)
    self.grid.addWidget(self.modeButton, 10, 2, 1, 6)
    self.grid.addWidget(self.logoutButton, 11, 0, 1, 8)
    self.grid.addWidget(self.reportLabel, 12, 0, 1, 8)
    self.grid.addWidget(self.progressBar, 13, 0, 1, 8)
    self.grid.addWidget(self.report, 14, 0, 1, 8)
    self.setLayout(self.grid)
    
    self.toLogin()

  def login(self):
    addr = self.addrText.text()
    username = self.usernameText.text()
    password = self.passwordText.text()
    if not (addr and username and password):
      self.errorAlert('need serverIP, username and password')
      return
    flag = self.worker.setParams({
      'address': self.addrText.text(),
      'username': self.usernameText.text(),
      'password': self.passwordText.text()
    })
    if not flag:
      return
    flag = self.worker.setTask('login')
    self.worker.start()

  def refresh(self):
    self.progressBar.setText('')
    self.paramText.setText('')
    self.pathText.setText(self.worker.ftp.path)
    self.mainTable.clear()
    if self.worker.params['tranfermode'] == 1:
      self.modeLabel.setText('current mode: PASV')
    else: 
      self.modeLabel.setText('current mode: PORT')
    root = QtWidgets.QTreeWidgetItem()
    root.setText(0, '..')
    self.mainTable.addTopLevelItem(root)
    for item in self.worker.ftp.fileList:
      node = QtWidgets.QTreeWidgetItem()
      node.setText(0, item['name'])
      node.setText(1, item['size'])
      if item['permission'][0] == 'd':
        node.setText(2, 'DIR')
      else:
        node.setText(2, 'FILE')
      node.setText(3, item['dateTime'])
      node.setText(4, item['permission'])
      node.setText(5, item['owner'] + ' ' + item['ownerGroup'])
      self.mainTable.addTopLevelItem(node)
    return

  def logout(self):
    self.worker.setTask('logout')
    self.worker.start()

  def updateFromServer(self):
    self.worker.setTask('getList')
    self.worker.start()

  def deleteNode(self):
    item = self.mainTable.currentItem()
    if (not item) or item.text(0) == '':
      self.errorAlert('please select node')
      return
    flag = self.worker.setParams({'netname': item.text(0)})
    if not flag:
      return
    if item.text(2) == 'FILE':
      self.worker.setTask('deleteNode')
    elif item.text(2) == 'DIR':
      self.worker.setTask('removeDir')
    self.worker.start()
    return

  def renameNode(self):
    item = self.mainTable.currentItem()
    if (not item) or item.text(0) == '' or self.paramText.text() == '':
      self.errorAlert('please select node and set new name')
      return
    flag = self.worker.setParams({'netname': item.text(0), 'localname': self.paramText.text()})
    if not flag:
      return
    self.worker.setTask('rename')
    self.worker.start()
    return

  def patch(self):
    fname = QtWidgets.QFileDialog.getOpenFileName()
    item = self.mainTable.currentItem()
    if fname[0] == '':
      return
    elif (not item) or item.text(0) == '':
      self.errorAlert('please select file')
      return
    offset = os.path.getsize(fname[0])
    if offset >= int(item.text(1)):
      self.errorAlert('file already fully downloaded')
      return
    flag = self.worker.setParams({'netname': item.text(0), 'localname': fname[0], 'offset': offset, 'total': int(item.text(1))})
    if not flag:
      return
    self.worker.setTask('downloadFile')
    self.worker.start()
    return

  def download(self):
    fname = QtWidgets.QFileDialog.getSaveFileName()
    item = self.mainTable.currentItem()
    if fname[0] == '':
      return
    elif (not item) or item.text(0) == '':
      self.errorAlert('please select file')
      return
    flag = self.worker.setParams({'netname': item.text(0), 'localname': fname[0], 'offset': 0, 'total': int(item.text(1))})
    if not flag:
      return
    self.worker.setTask('downloadFile')
    self.worker.start()
    return

  def upload(self):
    fname = QtWidgets.QFileDialog.getOpenFileName()
    if fname[0] == '':
      return
    netName = self.paramText.text()
    if netName == '':
      netName = fname[0].split('/')[-1]
    flag = self.worker.setParams({'netname': netName, 'localname': fname[0], 'total': os.path.getsize(fname[0])})
    if not flag:
      return
    self.worker.setTask('uploadFile')
    self.worker.start()
    return

  def makeDir(self):
    if self.paramText.text() == '':
      self.errorAlert('please set new directory name on sever')
      return
    flag = self.worker.setParams({'localname': self.paramText.text()})
    if not flag:
      return
    self.worker.setTask('makeDir')
    self.worker.start()

  def changeDir(self):
    item = self.mainTable.currentItem()
    if item.text(2) == 'FILE':
      return
    flag = self.worker.setParams({'netname': item.text(0)})
    if not flag:
      return
    self.worker.setTask('changeDir')
    self.worker.start()
  
  def changeMode(self):
    if self.worker.params['tranfermode'] == 1:
      self.worker.setParams({'tranfermode': 2})
    else:
      self.worker.setParams({'tranfermode': 1})
    self.refresh()

  def errorAlert(self, s):
    QtWidgets.QMessageBox.critical(self, 'ERROR', s)
  def infoAlert(self, s):
    QtWidgets.QMessageBox.information(self, 'INFO', s)


  def setMainIterface(self):
    self.loginButton.hide()
    self.addrText.hide()
    self.addrLabel.hide()
    self.usernameLabel.hide()
    self.usernameText.hide()
    self.passwordText.hide()
    self.passwordLabel.hide()

    self.logoutButton.show()
    self.mainTableLabel.show()
    self.mainTable.show()
    self.uploadButton.show()
    self.downloadButton.show()
    self.pathText.show()
    self.pathLabel.show()
    self.renameButton.show()
    self.deleButton.show()
    self.paramText.show()
    self.paramLabel.show()
    self.updateButton.show()
    self.makedirButton.show()
    self.modeButton.show()
    self.modeLabel.show()
    self.patchButton.show()
    self.progressBar.show()

    self.setGeometry(0, 0, 720, 480)

  def toMain(self):
    self.setMainIterface()
    self.updateFromServer()


  def toLogin(self):
    self.loginButton.show()
    self.addrText.show()
    self.addrLabel.show()
    self.usernameLabel.show()
    self.usernameText.show()
    self.passwordText.show()
    self.passwordLabel.show()

    self.logoutButton.hide()
    self.mainTableLabel.hide()
    self.mainTable.hide()
    self.uploadButton.hide()
    self.downloadButton.hide()
    self.pathLabel.hide()
    self.pathText.hide()
    self.renameButton.hide()
    self.deleButton.hide()
    self.paramText.hide()
    self.paramLabel.hide()
    self.updateButton.hide()
    self.makedirButton.hide()
    self.modeButton.hide()
    self.modeLabel.hide()
    self.patchButton.hide()
    self.progressBar.hide()

    self.setGeometry(0, 0, 720, 240)

class MyMainWindow(QtWidgets.QMainWindow):
  def __init__(self):
    super(MyMainWindow, self).__init__()
    self.mainBody = MyInterface()
    self.setCentralWidget(self.mainBody)
    self.setGeometry(100, 100, 720, 480)
    self.setWindowTitle('FTP Client by tht17')
    self.show()