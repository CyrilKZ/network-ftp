from PyQt5 import QtGui
from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QApplication
import ui, sys

def main():
  app = QApplication(sys.argv)
  wdw = ui.MyMainWindow()
  sys.exit(app.exec_())

if __name__ == '__main__':
  main()