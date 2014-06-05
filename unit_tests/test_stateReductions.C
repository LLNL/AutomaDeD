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
 * test_stateReductions.C
 *
 *  Created on: Feb 8, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_stateReductions.h"
#include "state_reduction.h"
#include "debugging.h"
#include "assert_functions.h"

#include <string>
#include <vector>

using namespace std;

void Test_StateReductions::canSendAndReceiveStates(int rank, int numProcs)
{
	ASSERT((numProcs >= 2), ("Number of processes is >= 2"));

	StateFactory factory;
	MPIState mpiState;
	mpiState.initialize();

	StateReduction red(&factory, mpiState);

	string name;
	State s;
	if (rank == 0) {
		name = "Uno";
		s = factory.createState(name);
		red.lastStates.insert(name);
		red.send(1);
	} else if (rank == 1) {
		name = "Dos";
		s = factory.createState(name);
		red.lastStates.insert(name);
		red.receive(0);

		bool cond = true;
		set<string>::const_iterator it;
		for (it = red.lastStates.begin(); it != red.lastStates.end(); ++it) {
			if (!((*it).compare("Uno")==0 || (*it).compare("Dos")==0))
				cond = false;
			//cout << "State: " << *it << endl;
		}
		ASSERT_TRUE(cond, "StateReduction can send and receive states");
	}
}

void Test_StateReductions::canPerformStateReduction(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));

	StateFactory factory;
	MPIState mpiState;
	mpiState.initialize();

	StateReduction red(&factory, mpiState);

	string name;
	State s;
	if (rank == 0) {
		name = "Uno";
		s = factory.createState(name);
		list<State> myList = red.getLastStates(s);
		//cout << "Num states: " << myList.size() << endl;

		bool cond = true;
		list<State>::const_iterator it;
		for (it=myList.begin(); it != myList.end(); ++it) {
			string name;
			if (!factory.findAndGetName(name, *it)) {
				handleError("in Test_StateReductions::canPerformStateReduction:: "
						"state not in factory");
			}
			//cout << "State: " << name <<  endl;

			if (!(name.compare("Uno")==0 || name.compare("Dos")==0 ||
					name.compare("Tres")==0))
				cond = false;
		}

		if (myList.size() != 3)
			cond = false;

		ASSERT_TRUE(cond, "Can perform state-reduction");

	} else if (rank == 1) {
		name = "Dos";
		s = factory.createState(name);
		list<State> myList = red.getLastStates(s);

	} else if (rank == 2) {
		name = "Tres";
		s = factory.createState(name);
		list<State> myList = red.getLastStates(s);

	} else if (rank == 3) {
		name = "Tres"; // repeated
		s = factory.createState(name);
		list<State> myList = red.getLastStates(s);
	} else if (rank == 4 || rank == 5 || rank == 6 || rank == 7) {
		name = "Tres"; // repeated
		s = factory.createState(name);
		list<State> myList = red.getLastStates(s);
	}
}

void Test_StateReductions::reductionBroadcastLastStates(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));

	StateFactory factory;
	MPIState mpiState;
	mpiState.initialize();

	StateReduction red(&factory, mpiState);

	string name;
	State s;
	list<State> myList;
	if (rank == 0) {
		name = "Uno";
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 1) {
		name = "Dos";
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 2) {
		name = "Tres";
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 3) {
		name = "Tres"; // repeated
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 4) {
		name = "Cuatro";
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 5) {
		name = "Cinco"; // repeated
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 6) {
		name = "Cinco"; // repeated
		s = factory.createState(name);
		myList = red.getLastStates(s);
	} else if (rank == 7) {
		name = "Seis";
		s = factory.createState(name);
		myList = red.getLastStates(s);
	}

	int sendbuff = static_cast<int>(myList.size());
	int recvbuff[numProcs];
	PMPI_Gather((void *)&sendbuff, 1, MPI_INT,(void *)recvbuff,
			1, MPI_INT,0, MPI_COMM_WORLD);

	bool cond = true;
	if (rank == 0) {
		for (int i=0; i < numProcs; ++i) {
			if (recvbuff[i] != 6) {
				cond = false;
				break;
			}
			//cout << "Size: " << recvbuff[i] << endl;
		}
		ASSERT_TRUE(cond, "State reduction broadcasts last states");
	}
}

void Test_StateReductions::reductionOrderStates(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));

	StateFactory factory;
	MPIState mpiState;
	mpiState.initialize();

	StateReduction red(&factory, mpiState);

	string name;
	State s;
	ReducedStateVector vec;

	if (rank == 0) {
		name = "Uno";
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 1) {
		name = "Dos";
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 2) {
		name = "Tres";
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 3) {
		name = "Tres"; // repeated
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 4) {
		name = "Cuatro";
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 5) {
		name = "Cinco"; // repeated
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 6) {
		name = "Cinco"; // repeated
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	} else if (rank == 7) {
		name = "Seis";
		s = factory.createState(name);
		vec = red.getLastStatesOrdered(s);
	}

	// Each task check ordering
	int correct = 1;
	string lastName;
	ASSERT((factory.findAndGetName(lastName, vec.vec[0])), "State was found");
	State myState = vec.vec[vec.localIndex];
	for (size_t i=1; i < vec.vec.size(); ++i) {
		State tmpS = vec.vec[i];
		string name;
		ASSERT((factory.findAndGetName(name, tmpS)), "State was found");
		if (name < lastName) {
			correct = 0;
			cout << "Rank: " << rank << " found unordered string" << endl;
			break;
		}
		lastName = name;

		if (i==vec.localIndex) {
			if (myState != s) {
				correct = 0;
				break;
			}
		}
	}

	int recvbuff[numProcs];
	PMPI_Gather((void *)&correct, 1, MPI_INT,(void *)recvbuff,
				1, MPI_INT,0, MPI_COMM_WORLD);

	bool cond = true;
	if (rank == 0) {
		for (int i=0; i < numProcs; ++i) {
			if (recvbuff[i] != 1) {
				cond = false;
				break;
			}
		}
		ASSERT_TRUE(cond, "Reduction can order states");
	}
}
