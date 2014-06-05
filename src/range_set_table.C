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
 * range_set_table.C
 *
 *  Created on: Feb 29, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "range_set_table.h"
#include "debugging.h"
#include "reduction.h"

#include <stdio.h>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

using namespace std;

RangeSetTable::RangeSetTable(const MPIState &s,
			const size_t &numStates,
			const size_t &myState):
				mpiState(s), table(numStates), state(myState)
{
#if STATE_TRACKER_DEBUG
	if (myState >= numStates)
		handleError("in RangeSetTable::RangeSetTable: "
				"state index is larger than number of states");
	if (table.size() < 1)
		handleError("in RangeSetTable::RangeSetTable: size of table is < 1");
	if (table[myState].get() != 0)
		handleError("in RangeSetTable::RangeSetTable: table element isn't null");
#endif

	unsigned int rank = static_cast<unsigned int>(mpiState.getProcessRank());
	RangeSetPtr rs(new RangeSet(rank));
	//RangeSet *rs = new RangeSet(rank);
	table[myState] = rs;
}

int RangeSetTable::packed_size(MPI_Comm comm) const
{
	int size=0, tmp=0;

	// Integer for number of range-sets
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	size += tmp;

	for (size_t i=0; i < table.size(); ++i) {
		if (table[i].get() != 0) { // null pointer
			// Integer for state index
			PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
			size += tmp;

			// Integer for the size of RangeSet
			//PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
			//size += tmp;

			// Size of the RangeSet itself
			tmp = table.at(i)->packed_size(comm);
			size += tmp;
		}
	}

	return size;
}

void RangeSetTable::pack(void *buf, int bufsize,
		int *position, MPI_Comm comm) const
{
	// Pack number of range-sets
	unsigned int n = 0;
	for (size_t i=0; i < table.size(); ++i)
		if (table[i].get() != 0)
			n++;
	PMPI_Pack((void *)&n, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

	for (size_t i=0; i < table.size(); ++i) {
		if (table[i].get() != 0) {
			unsigned int tmp = static_cast<unsigned int>(i);
			PMPI_Pack((void *)&tmp, 1, MPI_UNSIGNED,
					buf, bufsize, position, comm);

			//tmp = table.at(i)->packed_size(comm);
			//PMPI_Pack((void *)&tmp, 1, MPI_UNSIGNED,
			//					buf, bufsize, position, comm);

			table[i]->pack(buf, bufsize, position, comm);
		}
	}
}

void RangeSetTable::unpack(void *buf, int bufsize,
		int *position, MPI_Comm comm)
{
	unsigned int n = 0;
	PMPI_Unpack(buf, bufsize, position, &n, 1, MPI_UNSIGNED, comm);

#if STATE_TRACKER_DEBUG
	if (n == 0)
		handleError("in RangeSetTable::unpack: number of ranges is 0");
#endif


	for (unsigned int i = 0; i < n; ++i) {
		// Unpack integer for state index
		unsigned int index;
		PMPI_Unpack(buf, bufsize, position, &index, 1, MPI_UNSIGNED, comm);
		//PMPI_Unpack(buf, bufsize, position, &size, 1, MPI_UNSIGNED, comm);

		// Unpack RangeSet and add it to corresponding RangeSet in the table
		RangeSet rs = RangeSet::unpack(buf, bufsize, position, comm);
		if (table[index].get() == 0) {
			RangeSetPtr ptr(new RangeSet(rs));
			table[index] = ptr;
		} else {
			*(table[index]) += rs;
		}
	}
}

void RangeSetTable::send(int receiver) const
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

	// Pack and send table
	pack(buff, buff_size, &position, comm);
	PMPI_Send(buff, buff_size, MPI_PACKED, receiver, tag, comm);
}

void RangeSetTable::receive(int sender)
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

	// Receive table
	PMPI_Recv(buff, buff_size, MPI_PACKED, sender, tag, comm, &status);
	unpack(buff, buff_size, &position, comm);
}

void RangeSetTable::reduceTable()
{
	// Perform reduction of last states
	Reducer<RangeSetTable> *red = new BinomialReducer<RangeSetTable>;
	//Reducer<RangeSetTable> *red = new IntraReducer<RangeSetTable>;

	int numProcesses = mpiState.getCommWorldSize();
	int rank = mpiState.getProcessRank();
	red->reduce(0, numProcesses, rank, this);
	delete red;
}

RangeSet RangeSetTable::getRangeOfTasks(const size_t &stateIndex) const
{
#if STATE_TRACKER_DEBUG
	if (table[stateIndex].get() == 0)
		handleError("RangeSetTable::getRangeOfTasks: range set is empty");
#endif

	return *(table[stateIndex]);
}
