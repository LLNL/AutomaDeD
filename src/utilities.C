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
 * utilities.C
 *
 *  Created on: Dec 13, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "utilities.h"
#include "mpi_state.h"
#include "debugging.h"
#include "config.h"

#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

using namespace std;

extern MPIState mpiState;

void printMessage(const char *msg)
{
	if (mpiState.isRoot()) {
		cout << "[PROC 0]: " << msg << endl;
	}
}

void Tokenize(const string& str, vector<string>& tokens,
		const string& delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

/*
 * Delete spaces in a string
 */
void string_trim(const char* src, char* buff, const unsigned int sizeBuff)
{
    if(sizeBuff < 1)
    return;

    const char* current = src;
    unsigned int i = 0;
    while(current != '\0' && i < sizeBuff-1)
    {
        if(*current != ' ' && *current != '\t')
                buff[i++] = *current;
        ++current;
    }
    buff[i] = '\0';
}

void itoa(int x, char *buffer)
{
	sprintf(buffer, "%d", x);
}

void printTaskAndProcessIDMap(const MPIState &mpiState)
{
	bool p=true, r=true;
	r = AUTConfig::getBoolParameter("AUT_TASK_MAPPING", p);
	if (!p && !r)
		return;

	map<unsigned int, string> taskToProcs =
			getTaskAndProcessIdMap(mpiState);

	if (mpiState.isRoot()) {
		cout << "\nPROCS_TO_TASKS_MAPPING" << endl;
		map<unsigned int, string>::const_iterator it;
		for (it=taskToProcs.begin(); it != taskToProcs.end(); ++it) {
			cout << it->second << ":" << it->first << ",";
		}
		cout << endl;
	}
}

map<unsigned int, string>
getTaskAndProcessIdMap(const MPIState &mpiState)
{
	MPI_Comm comm = mpiState.getWorldComm();

	/* Get hostname */
	char hostName[MAX_HOSTNAME_SIZE];
	if (gethostname(hostName, MAX_HOSTNAME_SIZE) != 0)
		handleError("in getTaskAndProcessIdMap(): could not get hostname");

	unsigned int myPID = static_cast<unsigned int>(getpid());
	unsigned int myRank = static_cast<unsigned int>(mpiState.getProcessRank());

	/* Get size of packet */
	unsigned int numTasks =
			static_cast<unsigned int>(mpiState.getCommWorldSize());

	int intTypeSize=0, tmp=0;
	/* Space for rank */
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	intTypeSize += tmp;

	/* Space for node-processId */
	PMPI_Pack_size(MAX_HOSTNAME_SIZE, MPI_CHAR, comm, &tmp);
	intTypeSize += tmp;
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	intTypeSize += tmp;

	char recvBuffer[intTypeSize * numTasks];
	char sendBuffer[intTypeSize];

	/* Pack send buffer */
	int position=0;
	PMPI_Pack((void *)&myRank, 1, MPI_UNSIGNED, (void*)sendBuffer,
			intTypeSize, &position, comm);

	PMPI_Pack((void *)hostName, MAX_HOSTNAME_SIZE, MPI_CHAR, (void*)sendBuffer,
				intTypeSize, &position, comm);

	PMPI_Pack((void *)&myPID, 1, MPI_UNSIGNED, (void*)sendBuffer,
				intTypeSize, &position, comm);

	PMPI_Gather((void *)sendBuffer, intTypeSize, MPI_PACKED,
			(void *)recvBuffer, intTypeSize, MPI_PACKED, 0, comm);

	/* Unpack and print */
	map<unsigned int, string> ret;
	position = 0;
	if (mpiState.isRoot()) {
		for (unsigned int i=0; i < numTasks; ++i) {
			unsigned int rank;
			PMPI_Unpack((void *)recvBuffer, intTypeSize*numTasks,
					&position, (void*)&rank, 1, MPI_UNSIGNED, comm);

			char name[MAX_HOSTNAME_SIZE+1];
			name[0] = '\0';
			PMPI_Unpack((void *)recvBuffer, intTypeSize*numTasks,
					&position, (void*)name, MAX_HOSTNAME_SIZE, MPI_CHAR, comm);
			name[MAX_HOSTNAME_SIZE] = '\0';

			unsigned int pid;
			PMPI_Unpack((void *)recvBuffer, intTypeSize*numTasks,
					&position, (void*)&pid, 1, MPI_UNSIGNED, comm);

			string nodePid("");
			nodePid += string(name) + string("-");
			char b[512];
			sprintf(b, "%d", pid);
			nodePid += string(b);

			ret.insert(pair<unsigned int, string>(rank, nodePid));
		}
	}

	return ret;
}

unsigned long convertStringHexToInteger(const string &h)
{
	std::stringstream str;
	str << h;
	unsigned long ret;
	str >> std::hex >> ret;

	return ret;
}
