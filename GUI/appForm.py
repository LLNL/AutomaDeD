##############################################################################
# Copyright (c) 2014, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory.
#
# This file is part of AutomaDeD.
# Written by:
# Ignacio Laguna:lagunaperalt1@llnl.gov, Subrata Mitra: mitra4@purdue.edu.
# All rights reserved.
# LLNL-CODE-647182
#
# For details, see https://github.com/scalability-llnl/AutomaDeD
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License (as published by
# the Free Software Foundation) version 2.1 dated February 1999.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
# conditions of the GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##############################################################################
import sys, os, random, getopt
from PyQt4.QtCore import *
#from PySide.QtCore import *
from PyQt4.QtGui import *
#from PySide.QtGui import *

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure

import codeEditor


def findFile(name, path):
        for root, dirs, files in os.walk(path):
                if name in files:
                        return os.path.join(root, name)
	
	return None

#import graph_drawing
#import file_parser

#This is the main container/QT main window...
class AppForm(QMainWindow):
    def __init__(self, ifile, binary, parent=None):
        QMainWindow.__init__(self, parent)
        self.setWindowTitle('AutomaDeD GUI')
	self.input_file = ifile
	self.binary_file = binary
	self.file_parser = None
	self.edit = codeEditor.PlainTextEdit()
        self.number_bar = codeEditor.NumberBar(self.edit)

        self.edit.blockCountChanged.connect(self.number_bar.adjustWidth)
        self.edit.updateRequest.connect(self.number_bar.updateContents)
        #self.create_menu()
        #self.create_main_frame()
        #self.create_status_bar()
        #self.on_draw()

    def locateSourceFile(self,originalFileName):
	notReadable =False
	if(os.path.isfile(originalFileName)):
           	try:
			with open(originalFileName): pass
		except IOError:
			notReadable = True
	else:
		notReadable = True

	if(notReadable == False):
		return originalFileName
	
	baseName = os.path.basename(originalFileName)
	sourcePaths = os.environ.get('AUTOMADED_SOURCE_PATH')
	if(sourcePaths is None):
		return None
	sourcePathList = sourcePaths.split(':')
	for path in sourcePathList:
		newFile = findFile(baseName,path)
		if(newFile is None):
			continue
		else:
			return newFile

	return None


    def open_file_goto_line(self,origFile,line_num):
	
	fileName = self.locateSourceFile(origFile)
	if(fileName is None):
		msgBox = QMessageBox();
		msgBox.critical(self,QString('Error: Source file not found!'),QString('Provide location through env variable: AUTOMADED_SOURCE_PATH'),QMessageBox.NoButton)
		msgBox.setFixedSize(500,200)
		msgBox.show()
		return
        #print fileName
	self.source_file_lable.setText(QString('Source File: ' + fileName))
	#fileName = "/Users/mitra3/work/MPI_benchmarks/NPB3.3/NPB3.3-MPI/IS/is.c"
        cfile = QFile(fileName);
        lines = QString()
        num_lines = 0
        if (cfile.open(QFile.ReadOnly | QFile.Text)):
                while(cfile.atEnd() == False):
                        lines = lines + cfile.readLine()
                        num_lines = num_lines + 1
                        #print lines
        self.edit.setPlainText(lines);
	self.edit.setMaxLineNumber(num_lines)
        self.edit.setReadOnly(True);
        self.number_bar.adjustWidth(num_lines)
        self.edit.highlight_lines(line_num,None)
