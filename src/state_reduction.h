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
 * state_reduction.h
 *
 *  Created on: Feb 1, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef STATE_REDUCTION_H_
#define STATE_REDUCTION_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "reduction.h"
#include "model_components.h"
#include "mpi_state.h"

#include <mpi.h>
#include <list>
#include <set>
#include <string>
#include <vector>

using namespace std;

typedef struct {
	vector<State> vec; // ordered vector of states
	size_t localIndex; // index of local (last) state in the vector
} ReducedStateVector;

// Get last states from each process and broadcast them to all processes
class StateReduction {
private:
	StateFactory *factory;
	MPIState mpiState;

	set<string> lastStates;

	// Returns the number of bytes required to pack it via MPI
	int packed_size(MPI_Comm comm) const;

	// Pack it via MPI
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;

	// Unpack it via MPI
	list<State> unpack(void *buf, int bufsize,
			int *position, MPI_Comm comm) const;

	void updateLocalData(const list<State> &l);
	list<State> convertSetToList() const;
	list<State> broadcastListOfStates() const;

public:
	StateReduction(StateFactory *f, const MPIState &s) :
		factory(f), mpiState(s)
	{};

	// Send function used in binomial-tree reduction.
	void send(int receiver) const;

	// Receive function used in binomial-tree reduction.
	void receive(int sender);

	// Non ordered list of states
	list<State> getLastStates(const State &s);

	// Ordered (based on string-name) array of states
	ReducedStateVector getLastStatesOrdered(const State &s);

	friend class Test_StateReductions;
};

#endif /* STATE_REDUCTION_H_ */
