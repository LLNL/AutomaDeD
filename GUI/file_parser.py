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
import string
import numpy as np
import commands
import re
from state_class import *

class FileParser:
	def __init__(self,fname,binary):
		self.file_name = fname
		self.binary_name = binary
		self.state_dict = {}
		self.task_dict = {}
		self.task_count_dict = {}
		self.current_state = -1
		self.current_BT = []

	def creat_adjacency_matrix(self,value_list):
		arr = []
		arr = [[int(i) for i in line.split(',')] for line in value_list]
		self.adjacency_matrix = np.matrix(arr)

	def populate_states(self,line):
		tokens = line.split(':')
		state_index = int(tokens[0])
		state_BTrace = tokens[1].split('|')
		#for state in states:
			
		#print state_index, " =>", state_BTrace
		self.state_dict[state_index] = state(state_index,state_BTrace,'')

	def populate_tasks(self, line):
		tokens = line.split(':')
		state_index = int(tokens[0])
		task_list = tokens[1].split(',')
		#print state_index, " =>", task_list
		self.task_dict[state_index] = str(','.join(task_list))
		self.task_count_dict[state_index] = tokens[2]
		
	def find_list_progressed_task(self):
		self.LP_tasks = []
		(m,n) = self.adjacency_matrix.shape
		for row in range(m):
			total = 0
			for col in range(n):
				val = self.adjacency_matrix.item(row,col)
				if(val == 2):
					val = 0
				total = total + val
			#print total
			if(total == 0):
				self.LP_tasks.append("s" + str(row))
		
	def populate_source_code(self,line):
		#match = re.search(r'^[0-9]+,',line)
		match = line.find(',')
		#print line
		if(match != -1):
			if(self.current_state != -1):
		 		#print "*************** I am here **********", self.current_state
        	        	if self.current_state in self.state_dict:
                	        	the_state = self.state_dict[self.current_state]
					self.current_BT.reverse()
                        		self.state_dict[self.current_state] = state(the_state.getIndex(),self.current_BT,the_state.getType())
					#print "state: " , self.current_state , "\n" , "BT : " , self.current_BT 
                		else:
					self.current_BT.reverse()
                        		self.state_dict[self.current_state] = state(self.current_state,self.current_BT,'')
			
			toks = line.split(',')
			self.current_state = int(toks[0])
			self.current_BT = []
			self.current_BT.append(toks[1])
		else:
			self.current_BT.append(line)


	def populate_state_type(self,line):                            
                tokens = line.split(':')                          
                state_index = int(tokens[0])                            
                state_type = tokens[1]
		#print "type : " , state_type
                if state_index in self.state_dict:                      
                        the_state = self.state_dict[state_index]        
                        self.state_dict[state_index] = state(the_state.getIndex(),the_state.getBackTrace(),state_type)
                else:
                        self.state_dict[state_index] = state(state_index,'',state_type)

	def parse_input_file(self):
		if(self.file_name == None):
			return
		iFile = open(self.file_name, 'r')
		line_list = [line.strip() for line in iFile]
		matrix_started = False
		matrix_ended = False
		matrix_values = []

		states_started = False
		states_ended = False

		tasks_started = False
		tasks_ended = False

		source_code_started = False
		source_code_ended = False

		state_types_started = False
		state_types_ended = False
		
		last_source_added = False

		for line in line_list:
			#print line
			if(-1 != string.find(line,'#START_DEPENDENCY_GRAPH')):
				matrix_started = True
				continue
			if(-1 != string.find(line,'#END_DEPENDENCY_GRAPH')):	
				matrix_ended = True
				continue

			if(-1 != string.find(line,'#START_STATES')):
				states_started = True
				continue
			if(-1 != string.find(line,'#END_STATES')):	
				states_ended = True
				continue

			if(-1 != string.find(line,'#START_TASK_LOC')):
				tasks_started = True
				continue
			if(-1 != string.find(line,'#END_TASK_LOC')):	
				tasks_ended = True
				continue
			
			if(-1 != string.find(line,'#START_SOURCE_CODE_LINES')):
                                source_code_started = True
                                continue	

			if(-1 != string.find(line,'#END_SOURCE_CODE_LINES')):
                                source_code_ended = True
                                continue

			if(-1 != string.find(line,'#START_STATE_TYPES')):
                                state_types_started = True
                                continue

			if(-1 != string.find(line,'#END_STATE_TYPES')):
                                state_types_ended = True
                                continue

			if(matrix_started and not(matrix_ended)):
				line = line.rstrip(',')
				matrix_values.append(line)
				continue
			
			if(states_started and not(states_ended)):
				#self.populate_states(line)
				continue

			if(source_code_started and not(source_code_ended)):
				self.populate_source_code(line)
				continue
			if(source_code_ended and (last_source_added == False)):
				last_source_added = True
				if(self.current_state != -1):
                           		#print "*************** I am here **********", self.current_state
                                	if self.current_state in self.state_dict:
                                        	the_state = self.state_dict[self.current_state]
						self.current_BT.reverse()
                                        	self.state_dict[self.current_state] = state(the_state.getIndex(),self.current_BT,the_state.getType())
                                        	#print "state: " , self.current_state , "\n" , "BT : " , self.current_BT 
                                	else:
						reverse_BT = self.current_BT.reverse()
                                        	self.state_dict[self.current_state] = state(self.current_state,self.current_BT,'')

			if(tasks_started and not(tasks_ended)):
                                self.populate_tasks(line)
				continue

			if(state_types_started and not(state_types_ended)):
                                self.populate_state_type(line)
				continue

		self.creat_adjacency_matrix(matrix_values)
 		self.find_list_progressed_task()
		
		#print self.adjacency_matrix
		#print self.state_dict
		#print self.task_dict
		#print self.LP_tasks

	
