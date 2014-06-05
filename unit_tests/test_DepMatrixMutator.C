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
 * test_DepMatrixMutator.C
 *
 *  Created on: Mar 6, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "dep_matrix_mutator.h"
#include "test_DepMatrixMutator.h"
#include "dependency_matrix.h"
#include "mpi_state.h"
#include "assert_functions.h"

#include <set>

using namespace std;

void Test_DepMatrixMutator::canRemoveCycles(int rank, int numProcs)
{
	MPIState mpiState;
	mpiState.initialize();

	size_t s = 6;
	DependencyMatrix matrix(s, mpiState);

	if (rank == 0) {
		matrix.addDependency(1, 0, 1);
		matrix.addDependency(3, 1, 1);
		matrix.addDependency(2, 1, 1);
		matrix.addDependency(4, 1, 2);
		matrix.addDependency(4, 3, 1);
		matrix.addDependency(5, 2, 2);
		matrix.addDependency(2, 5, 2);

		//cout << matrix.toString() << endl;
		//cout << "num edges: " << matrix.numEdges() << endl;
		bool cond1 = (matrix.numEdges() == 7);

		DepMatrixMutator::removeCycles(matrix);
		bool cond2 = (matrix.numEdges() == 5);
		//cout << matrix.toString() << endl;
		//cout << "num edges: " << matrix.numEdges() << endl;

		ASSERT_TRUE((cond1 && cond2),
					("Matrix mutator can remove simple cycles"));
	}
}

void Test_DepMatrixMutator::canRemoveUndefinedDependencies(
		int rank, int numProcs)
{
	MPIState mpiState;
	mpiState.initialize();

	size_t s = 3;
	DependencyMatrix matrix(s, mpiState);

	if (rank == 0) {
		matrix.addDependency(1, 0, 3);
		matrix.addDependency(2, 0, 3);
		matrix.addDependency(1, 1, 1);

		int n=0;
		DependencyMatrixIterator *it = matrix.createIterator();
		for (it->firstState(); !it->isDone(); it->nextState()) {
			size_t row = it->currentRow();
			size_t col = it->currentColumn();
			//cout << "(" << row << ", " << col << "): "
			//		<< it->currentDep() << endl;
			if (it->currentDep() == 3)
				n++;
		}
		delete it;

		DepMatrixMutator::removeUndefinedDependencies(matrix);

		//cout << endl;
		int m=0;
		it = matrix.createIterator();
		for (it->firstState(); !it->isDone(); it->nextState()) {
			size_t row = it->currentRow();
			size_t col = it->currentColumn();
			//cout << "(" << row << ", " << col << "): "
			//		<< it->currentDep() << endl;
			if (it->currentDep() == 3)
				m++;
		}
		delete it;

		bool cond1 = (n == 2);
		bool cond2 = (m == 0);
		ASSERT_TRUE((cond1 && cond2),
				("Matrix mutator can remove undefined dependencies"));
	}

}

void Test_DepMatrixMutator::canFindStatesWithoutDep(
		int rank, int numProcs)
{
	MPIState mpiState;
	mpiState.initialize();

	if (rank == 0) {
		DependencyMatrix matrix(6, mpiState);
		matrix.addDependency(1, 0, 1);
		matrix.addDependency(3, 1, 1);
		matrix.addDependency(2, 1, 1);
		matrix.addDependency(4, 1, 2);
		matrix.addDependency(4, 3, 1);

		set<size_t> states =
				DepMatrixMutator::findStatesWithoutDependencies(matrix);

		//cout << "Number of states: " << states.size() << endl;
		bool cond1 = true;
		set<size_t>::const_iterator it;
		for (it = states.begin(); it != states.end(); ++it) {
			if (*it != 0 && *it != 5)
				cond1 = false;
			//cout << "(" << *it << ")" << endl;
		}

		DependencyMatrix matrix2(6, mpiState);
		matrix2.addDependency(2, 0, 1);
		matrix2.addDependency(1, 2, 2);
		matrix2.addDependency(3, 2, 1);

		set<size_t> states2 =
				DepMatrixMutator::findStatesWithoutDependencies(matrix2);

		//cout << "Number of states: " << states2.size() << endl;
		bool cond2 = true;
		for (it = states2.begin(); it != states2.end(); ++it) {
			//cout << "(" << *it << ")" << endl;
			if (*it != 0 && *it != 1 && *it != 4 && *it != 5)
				cond1 = false;
		}

		ASSERT_TRUE((cond1 && cond2),
				("Matrix mutator can find states without dependencies"));
	}

}
