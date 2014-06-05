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
 * range_set_table.h
 *
 *  Created on: Feb 29, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef RANGE_SET_TABLE_H_
#define RANGE_SET_TABLE_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "mpi_state.h"
#include "range_set.h"

#include <vector>
#include <boost/shared_ptr.hpp>

using namespace std;

typedef boost::shared_ptr<RangeSet> RangeSetPtr;

class RangeSetTable {
private:
	MPIState mpiState;
	vector< RangeSetPtr > table;
	size_t state;

	// Returns the number of bytes required to pack it via MPI
	int packed_size(MPI_Comm comm) const;

	// Pack it via MPI
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;

	// Unpack it via MPI
	void unpack(void *buf, int bufsize,
			int *position, MPI_Comm comm);

public:
	RangeSetTable(const MPIState &s,
			const size_t &numStates,
			const size_t &myState);

	// Send/Receive functions used in binomial-tree reduction.
	void send(int receiver) const;
	void receive(int sender);

	// Performs global binomial-tree reduction
	void reduceTable();

	// Getter functions (after reduction has been made)
	RangeSet getRangeOfTasks(const size_t &stateIndex) const;

	friend class test_RangeSetTable;
};

#endif /* RANGE_SET_TABLE_H_ */
