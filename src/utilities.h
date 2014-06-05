//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// This file is part of AutomaDeD.
// Written by:
// Ignacio Laguna:lagunaperalt1@llnl.gov, Subrata Mitra: mitra4@purdue.edu.
// All rights reserved.
// LLNL-CODE-647182
//
// For details, see https://github.com/scalability-llnl/AutomaDeD
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License (as published by
// the Free Software Foundation) version 2.1 dated February 1999.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
// conditions of the GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//////////////////////////////////////////////////////////////////////////////
/*
 * utilities.h
 *
 *  Created on: Dec 13, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "mpi_state.h"

#include <iostream>
#include <vector>
#include <string>
#include <map>

using namespace std;

#define MAX_HOSTNAME_SIZE 256

enum OperationType {
	SEND_OP, RECV_OP, REGULAR_OP
};

typedef struct {
	string state;
	int bufferSize;
	int dataType;
	int remoteProcess;
	int tag;
	int comm;
} MPIParams;

void printMessage(const char *msg);
void Tokenize(const string& str, vector<string>& tokens,
		const string& delimiters = " ");
void string_trim(const char* src, char* buff, const unsigned int sizeBuff);
void itoa(int x, char *buffer);
void writeFile(const string &data);

/*
 * Useful for fault-injection
 */
void printTaskAndProcessIDMap(const MPIState &mpiState);
map<unsigned int, string> getTaskAndProcessIdMap(
		const MPIState &mpiState);

unsigned long convertStringHexToInteger(const string &hex);

#endif /* UTILITIES_H_ */
