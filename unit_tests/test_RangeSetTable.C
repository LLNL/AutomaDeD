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
 * test_RangeSetTable.C
 *
 *  Created on: Mar 5, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_RangeSetTable.h"
#include "assert_functions.h"
#include "debugging.h"
#include "mpi_state.h"
#include "range_set_table.h"

#include <mpi.h>
#include <iostream>

using namespace std;

void test_RangeSetTable::canCreateRangeSetTable(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));
	if (numProcs != 8) // cannot perform reduction with < 8 procs
		return;

	MPIState mpiState;
	mpiState.initialize();

	size_t myState = static_cast<size_t>(rank);
	RangeSetTable rsTable(mpiState, numProcs, myState);

	if (rank == 0) {
		int num=0;
		for (size_t i=0; i < rsTable.table.size(); ++i)
			if (rsTable.table[i].get() != 0)
				num++;

		//cout << "Num of non-empty ranges: " << num << endl;
		ASSERT_TRUE((num==1), "Can create RangeSetTable");
	}
}

void test_RangeSetTable::canReduceRangeSetTable(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));
	if (numProcs != 8) // cannot perform reduction with < 8 procs
		return;

	MPIState mpiState;
	mpiState.initialize();

	size_t tableSize = 3;
	if (rank==0 || rank==1 || rank==3) {
		RangeSetTable rsTable(mpiState, tableSize, 2);
		rsTable.reduceTable();

		if (rank == 0) {
			//for (size_t i=0; i < rsTable.table.size(); ++i) {
			//	cout << "[" << i << "]: " << rsTable.table[i]->toString() << endl;
			//}
			bool cond1 = rsTable.table[0]->toString().compare("[2,4]") == 0;
			bool cond2 = rsTable.table[1]->toString().compare("[5-7]") == 0;
			bool cond3 = rsTable.table[2]->toString().compare("[0-1,3]") == 0;

			RangeSet r = rsTable.getRangeOfTasks(1);
			bool cond4 = r.toString().compare("[5-7]") == 0;
			//cout << "index 1: " << r.toString() << endl;

			ASSERT_TRUE((cond1 && cond2 && cond3 && cond4),
					"Can reduce RangeSetTable");
		}

	} else if (rank==2 || rank==4) {
		RangeSetTable rsTable(mpiState, tableSize, 0);
		rsTable.reduceTable();
	} else if (rank==5 || rank==6 || rank==7) {
		RangeSetTable rsTable(mpiState, tableSize, 1);
		rsTable.reduceTable();
	}
}
