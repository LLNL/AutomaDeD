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
import matplotlib
#matplotlib.use('Qt4Agg')  #http://matplotlib.org/examples/user_interfaces/embedding_in_tk.html
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch
import nx_pylab as nxpylab
import networkx as nx
import pylab
import numpy
from PyQt4.QtCore import *
#from PySide.QtCore import *
from PyQt4.QtGui import *
#from PySide.QtGui import *
import Queue
import pydot

import file_parser
#import automadedGui
import appForm
from state_class import *
from stack_tree import *
from tree import *

class AnnoteFinder:
	def __init__(self, xdata, ydata, annotes, axis=None, xtol=None, ytol=None):
                self.data = zip(xdata, ydata, annotes)
                if xtol is None: xtol = ((max(xdata) - min(xdata))/float(len(xdata)))/2
                if ytol is None: ytol = ((max(ydata) - min(ydata))/float(len(ydata)))/2
                self.xtol = xtol
                self.ytol = ytol
                if axis is None: self.axis = pylab.gca()
                else: self.axis= axis
                self.drawnAnnotations = {}
                self.links = []
                #self.thiscd=cd

	def __call__(self, event):
                #print "here"
                if event.inaxes:
                    clickX = event.xdata
                    clickY = event.ydata
                    if self.axis is None or self.axis==event.inaxes:
                        annotes = []
                        for x,y,a in self.data:
                            if  clickX-self.xtol < x < clickX+self.xtol and  clickY-self.ytol < y < clickY+self.ytol :
                                dx,dy=x-clickX,y-clickY
                                annotes.append((dx*dx+dy*dy,x,y, a) )
                        if annotes:
                            annotes.sort() # to select the nearest node
                            distance, x, y, annote = annotes[0]

def calculatePosition(pos,level_dict,rootx,rooty):
	for level in level_dict:
		node_list = level_dict[level]
		size = len(node_list)
		w = 50
		i = 0
		for node in node_list:
			pos[node] = [(rootx - w + ((w*i)/size)),(rooty + (50*level))]
		


def populateLevel(pos,adjMat,level,root,node_dict,level_dict):
	columns = adjMat[:, root]
	#print "root is: " , root, " x= ", rootx, " y= ", rooty
	#total_width = columns.length*5*level
	j = 0
	#print columns
	level = level + 1
	for column in columns:
		#print column
		col = column.item()
		#print col
		if(col == 1):
			#[cx,cy] = [(rootx + j*5), (rooty - 5)]
   			#pos[j] = [cx,cy]	
			node_dict[j] =  level
			#print level
			#print level_dict
			#if level in level_dict:
			#	lst = level_dict[level]
			#	print "list is: ", lst
			#	if (lst == None):
			#		lst = []
			#	level_dict[level] =  lst + [j] 
			#else:
			#	level_dict[level] = [j]
			populateLevel(pos,adjMat,level,j,node_dict,level_dict)
		j = j+1
			

def createTreeLayout(graph, adjMat, LP_nodes):
			print adjMat
			print adjMat[1,:]
			print adjMat[:,0]
			dimension = adjMat.shape[0]
			number_of_LP = len(LP_nodes)
			area_width = dimension * 200
			area_height = area_width
			rootx  = area_width/(number_of_LP)
			rooty = area_height
			pos = dict()
			node_dict = dict()
			level_dict = dict()
			i = 0
			for lpnodes in LP_nodes:
				pos[lpnodes] = [(rootx + i*area_width/number_of_LP), rooty]
				level = 1
				node_dict[lpnodes] = level
				#level_dict[level] = [lpnodes]
				populateLevel(pos,adjMat,level,lpnodes,node_dict,level_dict)
			print "node dict is: ",node_dict

			for n in node_dict:
				l = node_dict[n]
				nd = []
				if l in level_dict:
					lst = level_dict[l]
					nd = lst + [n]
				else:
					nd = [n]
				level_dict[l] = nd
			#print "level dict is: ",level_dict
			calculatePosition(pos,level_dict,rootx,rooty)
			return pos
			
			

class GraphHolder:
	node_positions = dict()
	selected_nodes = dict()
	dblClick = 0

	def __init__(self):
        	Graph = None
		Appform = None
		fParser = None

   		axis = pylab.gca()
		data = None
		xtol = None
		ytol = None
		
	@staticmethod
	def visitNode(selNodes,lastNodeName):
		last_index = int(lastNodeName[1:])
		last_state = GraphHolder.fParser.state_dict[last_index]
                last_state_type = last_state.getType()

		line = lastNodeName + ', Type: ' + last_state_type
		GraphHolder.Appform.stacktrace_editor.clear()
		theTree = Tree()
		i = 0
		for node in selNodes:
			node_index = int(node[1:])
			the_state = GraphHolder.fParser.state_dict[node_index]
			state_traces = the_state.getBackTrace()
			state_type = the_state.getType()
			#print "Type is: ", state_type
			#print "Back traces is: " , state_traces
			task_count = GraphHolder.fParser.task_count_dict[node_index]
			task_info = GraphHolder.fParser.task_dict[node_index]
			if(i == 0):
				theTree.createTree(state_traces,task_count,task_info,node)
			else:
				theTree.mergeIntoTree(state_traces,task_count,task_info,node)

			i = i + 1
		GraphHolder.Appform.stacktrace_editor.populateWidget(theTree)
		GraphHolder.Appform.stacktrace_editor.resizeColumnToContents(0)
		GraphHolder.Appform.status_text.setText(QString('Last selected state ' + str(line)))
		#GraphHolder.Appform.stacktrace_editor.setPlainText(line)
		GraphHolder.Appform.stacktrace_editor.update()
		QApplication.processEvents()
			
	@staticmethod	
        def getNodeIndex(clickX, clickY):
		annote = -1
		annotes = []
                for x,y,a in GraphHolder.data:
                	if  clickX-GraphHolder.xtol < x < clickX+GraphHolder.xtol and  clickY-GraphHolder.ytol < y < clickY+GraphHolder.ytol :
                		dx,dy=x-clickX,y-clickY
                        	annotes.append((dx*dx+dy*dy,x,y, a) )
                if annotes:
                        annotes.sort() # to select the nearest node
                        distance, x, y, annote = annotes[0]
		
		return annote

	@staticmethod
	def getNodeName(nodeIndex):
		#return "s" + str(nodeIndex)
		return str(nodeIndex)

        @staticmethod
	def onKeyPress(event):
		#print('you pressed', event.key, event.xdata, event.ydata)
		if(event.dblclick == True):
			GraphHolder.dblClick = 1
			#print "Double clicked"
			if event.inaxes:
                    		clickX = event.xdata
                    		clickY = event.ydata
				node_index = GraphHolder.getNodeIndex(clickX,clickY)
				if node_index == -1:
					return
				
				GraphHolder.selected_nodes.clear()
				lastSelectedStateName = GraphHolder.getNodeName(node_index)
				GraphHolder.selected_nodes[lastSelectedStateName] = 1
                       		GraphHolder.visitNode(GraphHolder.selected_nodes,lastSelectedStateName)
				#print "I double clicked on visit node=",GraphHolder.getNodeName(node_index)

				

			

	
        @staticmethod
	def onClickCall(event):
		if(event.dblclick == True):
			return
		if event.inaxes:
                    	clickX = event.xdata
                    	clickY = event.ydata
			node_index = GraphHolder.getNodeIndex(clickX,clickY)
			if node_index == -1:
					return

			lastSelectedStateName = GraphHolder.getNodeName(node_index)
			GraphHolder.selected_nodes[lastSelectedStateName] = 1
                       	GraphHolder.visitNode(GraphHolder.selected_nodes,lastSelectedStateName)
			#print "I am on visit node=",GraphHolder.getNodeName(node_index)

	#in the output matrix from automaDeD, some entries may be '2' which means the dependency is reversed...
	# create a new matrix after resolving those...
	def update_dependencies(self,inputMatrix):
                (xdim,ydim) = inputMatrix.shape
                G = nx.MultiDiGraph()
                for i in range(xdim):
                        for j in range(ydim):
                                val = int(inputMatrix.item(i,j))
				if(val == 2):
                                	inputMatrix[j,i] = 1
                                	inputMatrix[i,j] = 0
		
		return inputMatrix
					

	def create_graph_from_matrix(self,depMatrix,useTopoSort):
		inputMatrix = self.update_dependencies(depMatrix)
		(xdim,ydim) = inputMatrix.shape
		G = nx.MultiDiGraph()
		for i in range(xdim):
			for j in range(ydim):
				val = int(inputMatrix.item(i,j))
				if(val == 1):
					src = "s" + str(i)
					dst = "s" + str(j)
					G.add_edge(src, dst)
			#	else if(val == 2):
			#		print "Why 2 in matrix?"
		if(useTopoSort == True):
			TG = nx.MultiDiGraph()
			nodestopo = nx.topological_sort(G)
			lastNode = None
			for node in nodestopo:
				if(lastNode != None):
					TG.add_edge(lastNode,node)
				lastNode = node


			return TG

		else:
			return G
				

	# main function which plots the graph... appform is the QtMain window container...

	def create_graph(self,appform,useTopologocalSort=False):

		GraphHolder.Appform = appform
		GraphHolder.fParser = appform.file_parser
		A = GraphHolder.fParser.adjacency_matrix
		fig = plt.figure()
		#fig = plt.figure(figsize= (8,6))
		ax = fig.add_subplot(111)
		#ax.set_title('Progress Dependent Graph')  

		#G=nx.from_numpy_matrix(A,create_using=nx.MultiDiGraph())
		G = self.create_graph_from_matrix(A,useTopologocalSort)
        	G.edges(data=True)
  
		#for node in G.nodes():
		#	print node
		#labels = dict()
		labels=dict((node,GraphHolder.fParser.task_dict[int(node[1:])]) for node in G.nodes())  # node names are like s1, s2 etc, here we want only 1, 2 etd 
		#print labels
		
		
                LPnodes = GraphHolder.fParser.LP_tasks
		#print "LPnodes " , LPnodes

		#pos=nx.spring_layout(G) # the layout gives us the nodes position
		#print pos
		#pos = createTreeLayout(G,A,LPnodes)
		
		#get the tree like layout using graphviz...but this gives the reverse of what we want...
		pos=nx.graphviz_layout(G,prog='dot')

		#print pos
		allXSame = True
		allYSame = True
		(prevX,prevY) = pos['s0']
		#invert the above layout to get what we want...
		max_y  =0
		for p in pos:
			(x,y) = pos[p]
			if(y > max_y):
				max_y = y
				
			#print x, prevX, y, prevY
			if((allXSame) and (prevX == x)):
				allXSame =  True
			else:
				allXSame = False
			if((allYSame) and (prevY == y)):
                            	allYSame = True
			else:
				allYSame = False
			prevX = x
			prevY = y
		
		if((allXSame == True) or (allYSame == True)):
			pos=nx.spring_layout(G) # the layout gives us the nodes position
		if(True):	
			for p in pos:
				(x,y) = pos[p]
				#print x,"   ", y
				#mx = 100/x
				mx = x
				my = -y
				#print mx, " ==> " , my
				pos[p] = [x,my]

		
		#print pos
		#useful for creating tree layout 
		#nx.write_dot(G,'test.dot')
		#pos=nx.graphviz_layout(G,prog='dot')
		#print pos

                GraphHolder.Graph = G
		x,y,annotes=[],[],[]
		for key in pos:
			d=pos[key]
        		#print key
        		#print "x=", d[0],"y=",d[1]
			annotes.append(key)
			x.append(d[0])
			y.append(d[1])
		
		#nx.draw(G,pos,node_size=sizelist,node_color=ncolors,edge_color=ecolors,font_size=8)
		#nx.draw(G,pos,node_color='y',labels=labels,node_size=1500)

		#nx.draw_networkx_nodes(G,pos,node_color='b',labels=labels,node_size=500)
		#nxpylab.draw_networkx_edges(G,pos)
		#nxpylab.draw(G,pos,labels=labels,node_color='r',node_size=1500)
		computation_nodes = []
		communication_nodes = []
		for n in G.nodes():
			node_index = int(n[1:])
                        the_state = GraphHolder.fParser.state_dict[node_index]
                        state_type = the_state.getType()
			if(state_type == "COMPUTATION_CODE"):
				computation_nodes.append(n)
			elif(state_type == "COMMUNICATION_CODE"):
				communication_nodes.append(n)

				
		nxpylab.draw(G,pos,nodelist=G.nodes(),node_color='y',node_size=4000)
                #nodes = G.nodes()
		#nx.draw_networkx_nodes(G,pos,nodelist=LPnodes,node_color='y',labels=labels,node_size=500)
		#nx.draw_networkx_nodes(G,pos,nodelist=LPnodes,node_color='r',node_size=900)
		nxpylab.draw_networkx_nodes(G,pos,nodelist=computation_nodes,node_color='c',node_size=4000)
		#nxpylab.draw_networkx_nodes(G,pos,nodelist=communication_nodes,node_color='y',node_size=4000)

                LP_computation_nodes = []
		LP_communication_nodes = []
		for lp in LPnodes:
			if(lp in communication_nodes):
				LP_communication_nodes.append(lp)
			elif(lp in computation_nodes):
				LP_computation_nodes.append(lp)

		nxpylab.draw_networkx_nodes(G,pos,nodelist=LP_communication_nodes,node_color='y',node_size=2000)
		nxpylab.draw_networkx_nodes(G,pos,nodelist=LP_computation_nodes,node_color='c',node_size=2000)
		#nxpylab.draw(G,pos,nodelist=LPnodes,node_color='r',node_size=900)
		
		for node in G.nodes():
			node_x,node_y = pos[node]
			label_text = labels[node].split(',')
			#print label_text
			label_small = label_text[0]
			#quote = label_text.find('\'')
			dash = label_small.find('-')
			if(dash != -1):
				label_small = label_small[:dash]
			
			if((len(label_text) == 1 ) and (dash == -1)):
				label_small = '\'' + label_small + '\'' 
			else:
				label_small = '\'' + label_small + '\'' + ' ..'
			node_index = int(node[1:])
			task_count = GraphHolder.fParser.task_count_dict[node_index]
			label_small = str(task_count) + ' : ' + label_small
			plt.text(node_x-12,node_y+12,s=label_small, bbox=dict(facecolor='yellow'),horizontalalignment='center')

		#af =  AnnoteFinder(x,y, annotes)
		#fig.canvas.mpl_connect('button_press_event', af)
		GraphHolder.data = zip(x, y, annotes)
                XTOL = ((max(x) - min(x))/float(len(x)))/2
                YTOL = ((max(y) - min(y))/float(len(y)))/2
                GraphHolder.xtol = XTOL
                GraphHolder.ytol = YTOL
		
		#GraphHolder.node_positions=nx.spring_layout(GraphHolder.Graph) # the layout gives us the nodes position


		#plt.show()
        	return fig
	



