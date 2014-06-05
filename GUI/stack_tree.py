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
from tree import *
from PyQt4.QtCore import *
#from PySide.QtCore import *
from PyQt4.QtGui import *

class stackTreeWidget(QTreeWidget):
	def __init__(self,theAppform,parent=None):
		QTreeWidget.__init__(self,parent)
		self.appform = theAppform
		self.function_dict = {}
		self.rootItem = None
                columnList = QStringList()
		columnList.append(QString('Stack'))
		columnList.append(QString('Process count'))
		columnList.append(QString('Process group states'))
		self.setHeaderLabels(columnList)
		#self.setColumnCount(4)
		self.connect(self, SIGNAL('itemClicked(QTreeWidgetItem*, int)'), self.onItemClick)
	
	
	def onItemClick(self, item, column):
		filename_linenum = str(item.toolTip (column))
		#print filename_linenum
		tokens = filename_linenum.split(':')
		file = tokens[0]
		current_line = []
		current_line.append(int(tokens[1]))
		self.appform.open_file_goto_line(file,current_line)
	def populateWidget(self, stack_tree):
		if(stack_tree.actualRoot == 0):
                        return 0
                queue = myQ()
                tempnode =  stack_tree.rootNode
                #print "first node is ", tempnode.name
		i = 0
                while(tempnode is not None):
                        #print "Node is=", tempnode.name, " weight is = " , tempnode.weight
                        for child in tempnode.children:
                                queue.push(child)
			
                        tempnode = queue.pop()
			if(tempnode is None):
				continue
			#if (self.rootItem is not None):
			if (i == 0):
				theItem = self.createFunctionItem(tempnode,True)
				self.rootItem = theItem
			else:
				theItem = self.createFunctionItem(tempnode)
			i = i + 1
			
		self.expandToDepth(3)
		for c in [0,1,2]:
		        self.resizeColumnToContents(c);
			

	def createFunctionItem(self,node,isRoot = False):
		if(node is None):
			return None
		#print node.name
		if node in self.function_dict:
			return self.function_dict[node]
		item = None
		parentNode = node.parent
		if((parentNode is None) or (isRoot) or (parentNode not in self.function_dict)):
			item = QTreeWidgetItem(self)
		else:
			parentItem = self.function_dict[parentNode]
			item = QTreeWidgetItem(parentItem)
		
		#print node.state_names
		node_state_names = node.state_names;
		node_states = sorted(node_state_names.split(','))
		state_names = ','.join(node_states)

		nodename = node.name
		tokens = nodename.split('|')
		if(len(tokens) < 2):
			print "wrong backtarce format"
		asterix_tokens = True if((len(tokens) > 2) and (-1 != tokens[2].find('*'))) else False
                item.setText(0,QString(tokens[1]))
		item.setToolTip(0,QString(tokens[0]))
                item.setText(1,QString(str(node.weight)))
		item.setToolTip(1,QString(tokens[0]))
		item.setText(2,QString(str(state_names)))
		item.setToolTip(2,QString(tokens[0]))

		#button = QPushButton(QString("All tasks"), self);
		#self.setItemWidget(item,3,button)
		#button.connect(button, QtCore.SIGNAL("clicked()"), app, QtCore.SLOT("quit()"))
		self.function_dict[node] = item
		return item

	#def onProcessInfoButtonClick()
