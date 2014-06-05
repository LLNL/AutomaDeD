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
from stack_tree import *

import matplotlib
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure

import codeEditor

import graph_drawing
import file_parser
from appForm import *

class myHolderWidget(QWidget):
	def __init__(self,parent=None):
		QWidget.__init__(self,parent)

	def mousePressEvent(self, event):
        	#print 'mouse pressed'
        	QWidget.mousePressEvent(event)

class mainContainer():
    def __init__(self, ifile, binary, parent=None):
    	self.appform = AppForm(ifile,binary)
	self.create_menu()
        self.create_main_frame()
        self.create_status_bar()
        self.on_draw()

    def populateProcessTree(self):
	for state_index in self.appform.file_parser.task_dict:
		state_item = QTreeWidgetItem(self.appform.process_tree)
		state_name = 's' + str(state_index)
		isLP = False
		if state_name in self.appform.file_parser.LP_tasks:
			isLP =  True
		if(isLP):
			state_item.setBackground(0,QBrush(Qt.red,Qt.SolidPattern))

		state_item.setText(0,QString(state_name))
		taskString = self.appform.file_parser.task_dict[state_index]
		taskList = taskString.split(',')
		for task in taskList:
			item = QTreeWidgetItem(state_item)
			item.setText(1,QString(task))
			if(isLP):
				item.setBackground(1,QBrush(Qt.red,Qt.SolidPattern))

    def create_main_frame(self):
        self.appform.file_parser = file_parser.FileParser(self.appform.input_file, self.appform.binary_file) 
        self.appform.file_parser.parse_input_file()
	self.appform.main_frame = QWidget()
	self.appform.stacktrace_editor = stackTreeWidget(self.appform)
       	self.appform.stacktrace_editor.show()

	self.appform.process_tree = QTreeWidget(self.appform)
	columnList = QStringList()
	columnList.append(QString('States'))
	columnList.append(QString('Processes'))
	self.appform.process_tree.setHeaderLabels(columnList)
	self.populateProcessTree()
	self.appform.process_tree.show()

	gHolder = graph_drawing.GraphHolder()
        self.appform.fig = gHolder.create_graph(self.appform)
        self.appform.canvas = FigureCanvas(self.appform.fig)
        self.appform.canvas.setParent(self.appform.main_frame)
	self.appform.fig.canvas.mpl_connect('button_press_event', gHolder.onClickCall)
	self.appform.fig.canvas.mpl_connect('button_press_event', gHolder.onKeyPress)
	self.appform.canvas.mpl_connect('pick_event', self.on_pick)

        # Create the navigation toolbar, tied to the canvas
        #
        #self.appform.mpl_toolbar = NavigationToolbar(self.appform.canvas, self.appform.main_frame)
        
        font = QFont('Arial',10,QFont.Bold)

        #
        # Layout with box sizers
        #
	splitterTop = QSplitter(Qt.Horizontal)
	splitterBottom = QSplitter(Qt.Horizontal)
	splitterVertical = QSplitter(Qt.Vertical)
         
	
	vl1 = QVBoxLayout()
	lbl1 = QLabel('Progress Dependency Graph')
	lbl1.setFont(font)
	lbl1.setAlignment(Qt.AlignCenter)
	lbl1.setFixedHeight(18)
	vl1.addWidget(lbl1)
	vl1.addWidget(self.appform.canvas)
	wg1 = QWidget()
	wg1.setLayout(vl1)

        
	vl2 = QVBoxLayout()
	lbl2 = QLabel('Back Traces')
	lbl2.setFont(font)
	lbl2.setAlignment(Qt.AlignCenter)
	lbl2.setFixedHeight(18)
	vl2.addWidget(lbl2)
	vl2.addWidget(self.appform.stacktrace_editor)
	wg2 = QWidget()
	wg2.setLayout(vl2)

	splitterTop.addWidget(wg1)
	splitterTop.addWidget(wg2)
	
	hl1 = QHBoxLayout()
        hl1.addWidget(self.appform.number_bar)
        hl1.addWidget(self.appform.edit)

        vl3 = QVBoxLayout()
	self.appform.source_file_lable = QLabel('Source File')
	self.appform.source_file_lable.setFont(font)
        vl3.addWidget(self.appform.source_file_lable)
        vl3.addLayout(hl1)
	wg3 = QWidget()
	wg3.setLayout(vl3)

	vl4 = QVBoxLayout()
	lbl4 = QLabel('Process information')
	lbl4.setFont(font)
	lbl4.setFixedHeight(12)
	lbl4.setAlignment(Qt.AlignCenter)
	vl4.addWidget(lbl4)
	vl4.addWidget(self.appform.process_tree)
	wg4 = QWidget()
	wg4.setLayout(vl4)

	splitterBottom.addWidget(wg3)
	splitterBottom.addWidget(wg4)

	splitterVertical.addWidget(splitterTop)
	splitterVertical.addWidget(splitterBottom)

	
        vbox = QVBoxLayout()
        vbox.addWidget(splitterVertical)
        
	self.appform.installEventFilter(self.appform.canvas)
        self.appform.main_frame.setLayout(vbox)
        self.appform.setCentralWidget(self.appform.main_frame)

    def on_draw(self):
        """ Redraws the figure
        """
        #graph_drawing.create_graph(self.appform.fig)
        #self.appform.canvas.draw()

    def on_pick(self, event):
        # The event received here is of the type
        # matplotlib.backend_bases.PickEvent
        #
        # It carries lots of information, of which we're using
        # only a small amount here.
        # 
        box_points = event.artist.get_bbox().get_points()
        msg = "You've clicked on a bar with coords:\n %s" % box_points
        
        QMessageBox.information(self.appform, "Click!", msg)

    
    def create_status_bar(self):
        self.appform.status_text = QLabel("This is a GUI for AutomaDed")
        self.appform.statusBar().addWidget(self.appform.status_text, 1)
        
    def create_menu(self):        
        #self.appform.file_menu = self.appform.menuBar().addMenu("&File")
        
        quit_action = self.create_action("&Quit", slot=self.appform.close, 
            shortcut="Ctrl+Q", tip="Close the application")
        
        #self.appform.add_action(self.appform.file_menu, (None,quit_action))
        

    def add_actions(self, target, actions):
        for action in actions:
            if action is None:
                target.addSeparator()
            else:
                target.addAction(action)

    def create_action(  self, text, slot=None, shortcut=None, 
                        icon=None, tip=None, checkable=False, 
                        signal="triggered()"):
        action = QAction(text, self.appform)
        if icon is not None:
            action.setIcon(QIcon(":/%s.png" % icon))
        if shortcut is not None:
            action.setShortcut(shortcut)
        if tip is not None:
            action.setToolTip(tip)
            action.setStatusTip(tip)
        if slot is not None:
            self.appform.connect(action, SIGNAL(signal), slot)
        if checkable:
            action.setCheckable(True)
        return action



def main(argv):
    	inputfile = ''
   	automaDedBinary = ''
	try:
      		opts, args = getopt.getopt(argv,"hi:b:",["ifile="])
   	except getopt.GetoptError:
      		print 'automadedGui.py -i <inputfile>'
      		sys.exit(2)
   	for opt, arg in opts:
      		if opt == '-h':
         		print 'automadedGui.py -i <inputfile>'
         		sys.exit()
      		elif opt in ("-i", "--ifile"):
        		 inputfile = arg
	if(inputfile == ''):
		print 'automadedGui.py -i <inputfile>'
         	sys.exit()

    	app = QApplication(sys.argv)
    	form = mainContainer(inputfile,automaDedBinary)
    	form.appform.show()
    	app.exec_()


if __name__ == "__main__":
    	main(sys.argv[1:])

