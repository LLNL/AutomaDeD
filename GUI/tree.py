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

class myQ:
	def __init__(self):
		self.elements = []
	def push(self,elm):
		self.elements.append(elm)
	def pop(self):
		if (len(self.elements) == 0):
			return None
		elm = self.elements[0]
		del(self.elements[0])
		return elm
	def size(self):
		return len(self.elements)


class TreeNode:
	def __init__ (self,nodeName,nodeWeight,stateName):
		self.name = nodeName
		self.weight = int(nodeWeight)
		self.process_info = []
		self.children = []
		self.parent = None
		self.state_names = stateName
		#print "creating " , nodeName

	def addChild(self,childNode):
		#print "adding a child ", childNode.name
		self.children.append(childNode)
		childNode.parent = self

	def addWeight(self,moreWeight):
		self.weight = self.weight + int(moreWeight)
	
	def addInfo(self,moreInfo):
		self.process_info.append( moreInfo)
	def addStateNames(self,name):
		self.state_names = self.state_names + ',' +  name

class Tree:
	
	def __init__ (self):
		self.rootNode = 0
		self.actualRoot = 0
		self.currentNode = 0

        def createTree(self,stateList, task_count = 1, task_info = None,StateName = None):
		i = 0
		for onestate in stateList:
			star = str(onestate).find('*')
                        wrapper_line = str(onestate).find('mpi_wrappers.h')
                        if((star != -1) and (wrapper_line == -1)):
                        	continue
			node = TreeNode(onestate,task_count,StateName)
			if( i == 0):
				self.rootNode = node
			else:
				self.currentNode.addChild(node)

			i = i + 1;
			self.currentNode = node

		self.actualRoot = TreeNode('actual root',task_count,'no state')
		self.actualRoot.addChild(self.rootNode)
		return self.actualRoot

	def mergeIntoTree(self,otherStateList,task_count = 1,task_info = None, StateName = None):
		if(self.actualRoot == 0):
			return 0

		self.currentNode = self.actualRoot
		for onestate in otherStateList:
			star = str(onestate).find('*')
                        wrapper_line = str(onestate).find('mpi_wrappers.h')
                        if((star != -1) and (wrapper_line == -1)):
                                continue
			found_child = False
			for child in self.currentNode.children:
				if(child.name == onestate):
					child.addWeight(task_count)
					child.addStateNames(StateName)
					info = child.addInfo(task_info)
					self.currentNode = child
					found_child = True
					break

			if(found_child):
				continue
			else:
                        	node = TreeNode(onestate,task_count,StateName)
                                self.currentNode.addChild(node)
                        	self.currentNode = node

                return self.actualRoot


	def printTree(self):
		if(self.actualRoot == 0):
                        return 0
		queue = myQ()
		tempnode =  self.rootNode
		#print "first node is ", tempnode 
		while(tempnode is not None):
			#print "Node is=", tempnode.name, " weight is = " , str(tempnode.weight)
			for child in tempnode.children:
				queue.push(child)

			tempnode = queue.pop()
		

#l1 = ('A','B','C','D','E','F','G')
#l2 = ('A','B','C','P','Q','R')
#l3 = ('A','B','C','P','Q','X')
#t = Tree()
#t.createTree(l1)
#t.mergeIntoTree(l2)
#t.mergeIntoTree(l3)
#t.printTree()
