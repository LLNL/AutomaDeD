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
 * state_reduction.C
 *
 *  Created on: Feb 1, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */
//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "state_reduction.h"

#include "reduction.h"
#include "model_components.h"
#include "debugging.h"

#include <mpi.h>
#include <list>
#include <string>
#include <set>
#include <vector>
#include <algorithm>

using namespace std;

list<State> StateReduction::getLastStates(const State &s)
{
	// Start by putting last (local) state in the list
	string name;
	if (!(factory->findAndGetName(name, s))) {
		handleError("in StateReduction::getLastStates: "
				"last state passed is not in the factory");
	}
	lastStates.insert(name);

	// Perform reduction of last states
	Reducer<StateReduction> *red = new BinomialReducer<StateReduction>;
	//Reducer<StateReduction> *red = new IntraReducer<StateReduction>;

	int numProcesses = mpiState.getCommWorldSize();
	int rank = mpiState.getProcessRank();
	red->reduce(0, numProcesses, rank, this);
	delete red;

#if STATE_TRACKER_DEBUG
	if(lastStates.size() == 0)
			handleError("StateReduction::getLastStates: "
					"local set of states is 0");
#endif

	list<State> tmpList = broadcastListOfStates();

	return tmpList;
}

list<State> StateReduction::broadcastListOfStates() const
{
	// Broadcast buffer size
	MPI_Comm comm = mpiState.getWorldComm();

	int buff_size;
	if (mpiState.isRoot())
		buff_size = packed_size(comm);

	PMPI_Bcast((void*)&buff_size, 1, MPI_INT, 0, comm);

	// All allocate a buffer for the set of states
	char buff[buff_size];

	// Broadcast states
	list<State> tmpList;
	int position = 0;
	if (mpiState.isRoot()) {
    	pack((void*)buff, buff_size, &position, comm);
    	PMPI_Bcast(buff, buff_size, MPI_PACKED, 0, comm);
    	tmpList = convertSetToList();
	} else {
		PMPI_Bcast(buff, buff_size, MPI_PACKED, 0, comm);
		tmpList = unpack(buff, buff_size, &position, comm);
	}

#if STATE_TRACKER_DEBUG
	if(tmpList.size() == 0)
		handleError("StateReduction::broadcastListOfStates: "
				"size of returning list is 0");
#endif

	return tmpList;
}

list<State> StateReduction::convertSetToList() const
{
	list<State> tmpList;
	set<string>::const_iterator it;
	for (it = lastStates.begin(); it != lastStates.end(); ++it) {
		State state;
		if (!factory->findAndGetState(state, *it)) {
			handleError("in StateReduction::convertSetToList: "
					"state not found after reduction");
		}
		tmpList.push_back(state);
	}

#if STATE_TRACKER_DEBUG
	if(tmpList.size() == 0)
		handleError("StateReduction::convertSetToList: "
				"size of returning list is 0");
#endif

	return tmpList;
}

int StateReduction::packed_size(MPI_Comm comm) const
{
	int size=0, tmp=0;

	// Integer for number of state names
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	size += tmp;

	set<string>::const_iterator it;
	for (it=lastStates.begin(); it != lastStates.end(); it++) {
		// Number of characters of state name
		PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
		size += tmp;

		// The state name itself
		int strSize = static_cast<int>(it->size());
		PMPI_Pack_size(strSize, MPI_CHAR, comm, &tmp); // string
		size += tmp;
	}

	return size;
}

void StateReduction::pack(void *buf, int bufsize,
		int *position, MPI_Comm comm) const
{
	// Pack number of state names
	unsigned int n = static_cast<unsigned int>(lastStates.size());
	PMPI_Pack((void *)&n, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

	set<string>::const_iterator it;
	for (it=lastStates.begin(); it != lastStates.end(); it++) {
		// Number of characters of state name
		unsigned int strSize = static_cast<unsigned int>(it->size());
		PMPI_Pack((void *)&strSize, 1, MPI_UNSIGNED,
				buf, bufsize, position, comm);

		// Pack state name
		PMPI_Pack((void *)(it->c_str()), strSize, MPI_CHAR,
				buf, bufsize, position, comm);
	}
}

list<State> StateReduction::unpack(void *buf, int bufsize,
		int *position, MPI_Comm comm) const
{
	unsigned int n = 0;
	PMPI_Unpack(buf, bufsize, position, &n, 1, MPI_UNSIGNED, comm);

#if STATE_TRACKER_DEBUG
	if (n == 0)
		handleError("in StateReduction::unpack: number of states is 0");
#endif

	list<State> tmpList;

	for (unsigned int i = 0; i < n; ++i) {
		// Unpack number of characters
		unsigned int strSize;
		PMPI_Unpack(buf, bufsize, position, &strSize, 1, MPI_UNSIGNED, comm);

		char buffer[strSize+1];
		buffer[0] = '\0'; // clean buffer
		PMPI_Unpack(buf, bufsize, position, (void *)buffer,
				strSize, MPI_CHAR, comm);
		buffer[strSize] = '\0'; // make sure the string is null-terminated
		string name(buffer);

		State s = factory->createState(name);
		tmpList.push_back(s);
	}

#if STATE_TRACKER_DEBUG
	if (tmpList.size() == 0)
			handleError("in StateReduction::unpack: list is empty");
#endif

	return tmpList;
}

void StateReduction::send(int receiver) const
{
	MPI_Comm comm = mpiState.getWorldComm();
	int tag = mpiState.getTag();

	// Send buffer size
	int buff_size = packed_size(comm);
	PMPI_Send((void *)(&buff_size), 1, MPI_INT, receiver, tag, comm);

	// Allocate buffer
	char buff[buff_size];
	buff[0] = '\0';
	int position = 0;

	// Pack and send list
	pack(buff, buff_size, &position, comm);
	PMPI_Send(buff, buff_size, MPI_PACKED, receiver, tag, comm);
}

void StateReduction::receive(int sender)
{
	MPI_Status status;
	MPI_Comm comm = mpiState.getWorldComm();
	int tag = mpiState.getTag();

	// Receive buffer size
	int buff_size;
	PMPI_Recv((void *)(&buff_size), 1, MPI_INT, sender, tag, comm, &status);

	// Allocate buffer
	char buff[buff_size];
	buff[0] = '\0';
	int position = 0;

	// Receive list
	PMPI_Recv(buff, buff_size, MPI_PACKED, sender, tag, comm, &status);

	list<State> tmpList = unpack(buff, buff_size, &position, comm);

	// Update local data
	updateLocalData(tmpList);
}

void StateReduction::updateLocalData(const list<State> &l)
{
	list<State>::const_iterator it;
	for (it = l.begin(); it != l.end(); ++it) {
		string name;
		if (!factory->findAndGetName(name, *it)) {
			handleError("in StateReduction::updateLocalData: "
					"state not in factory");
		}
		set<string>::iterator jt = lastStates.find(name);
		if (jt == lastStates.end())
			lastStates.insert(name);
	}
}

ReducedStateVector StateReduction::getLastStatesOrdered(const State &s)
{
	list<State> tmpList = getLastStates(s);
	string states[tmpList.size()];

	// Convert List of states to string vector
	list<State>::const_iterator it;
	size_t index=0;
	for (it=tmpList.begin(); it != tmpList.end(); ++it) {
		string name;
		if (!factory->findAndGetName(name, *it)) {
			handleError("StateReduction::getLastStatesOrdered: "
					"state not in factory");
		}
		states[index++] = name;
	}

	// Sort string vector
	size_t elements = tmpList.size();
	sort(states, states + elements);

	// Fill structure
	ReducedStateVector ret;
	for (size_t i=0; i < tmpList.size(); ++i) {
		State tmpState;
		if (!factory->findAndGetState(tmpState, states[i])) {
			handleError("StateReduction::getLastStatesOrdered: "
					"name not in factory");
		}
		ret.vec.push_back(tmpState);
		if (tmpState == s)
			ret.localIndex = i;
	}

	return ret;
}
