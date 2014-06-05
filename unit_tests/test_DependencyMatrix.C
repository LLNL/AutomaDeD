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
 * test_DependencyMatrix.C
 *
 *  Created on: Feb 17, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_DependencyMatrix.h"
#include "dependency_matrix.h"
#include "debugging.h"
#include "assert_functions.h"
#include "mpi_state.h"

#include <iostream>
#include <mpi.h>

using namespace std;

void test_DependencyMatrix::canSendAndReceiveMatrix(int rank, int numProcs)
{
	ASSERT((numProcs >= 2), ("Number of processes is >= 2"));

	MPIState mpiState;
	mpiState.initialize();
	MPI_Comm comm = mpiState.getWorldComm();
	size_t dim = 25;
	DependencyMatrix matrix(dim, mpiState);

	for (size_t i=0; i < dim*dim; ++i) {
		size_t y = i % dim;
		size_t x = i / dim;
		matrix.addDependency(x, y, i);
	}

	if (rank == 0) {
		int s = matrix.packed_size(comm);
		char buff[s];
		int position = 0;
		matrix.pack((void*)buff, s, &position, comm);
		PMPI_Send(buff, s, MPI_PACKED, 1, 0, comm);

	} else if (rank == 1) {
		MPI_Status status;
		int s = matrix.packed_size(comm);
		char buff[s];
		int position = 0;
		PMPI_Recv(buff, s, MPI_PACKED, 0, 0, comm, &status);
		DependencyMatrix ret = matrix.unpack((void*)buff, s, &position, comm);

		bool cond = true;
		for (size_t i=0; i < matrix.dim; ++i)
			for (size_t j=0; j < matrix.dim; ++j)
				if (matrix.matrix[i][j] != ret.matrix[i][j])
					cond = false;

		ASSERT_TRUE(cond, "Can send and receive Dependency-Matrix");
	}
}

void test_DependencyMatrix::canReduceDependencyMatrix(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));
	if (numProcs != 8) // cannot perform reduction with < 8 procs
		return;

	MPIState mpiState;
	mpiState.initialize();
	MPI_Comm comm = mpiState.getWorldComm();
	size_t dim = 4;

	if (rank == 0 || rank == 4) {
		unsigned int data[] = {0,0,1,2, 1,0,0,2, 1,1,0,0, 3,1,1,0};
		DependencyMatrix matrix(dim, data, mpiState);
		matrix.reduceGlobally();

		if (rank ==0) {
			bool cond = true;
			unsigned int final[] = {0,3,1,3, 3,0,1,3, 1,1,0,3, 3,1,1,0};
			for (size_t i=0; i < matrix.dim; ++i) {
				for (size_t j=0; j < matrix.dim; ++j) {
					//cout << matrix.matrix[i][j] << " ";
					if (matrix.matrix[i][j] != final[j+i*dim])
						cond = false;
				}
				//cout << endl;
			}
			ASSERT_TRUE(cond, "Can reduce Dependency-Matrix");
		}

	} else if (rank == 1 || rank == 5) {
		unsigned int data[] = {0,2,0,0, 2,0,1,0, 0,0,0,3, 0,0,0,0};
		DependencyMatrix matrix(dim, data, mpiState);
		matrix.reduceGlobally();

	} else if (rank == 2 || rank == 6) {
		unsigned int data[] = {0,1,0,0, 0,0,0,1, 1,0,0,1, 3,0,0,0};
		DependencyMatrix matrix(dim, data, mpiState);
		matrix.reduceGlobally();

	} else if (rank == 3 || rank == 7) {
		unsigned int data[] = {0,1,0,1, 1,0,0,1, 0,0,0,0, 0,0,0,0};
		DependencyMatrix matrix(dim, data, mpiState);
		matrix.reduceGlobally();
	}

}

void test_DependencyMatrix::canIterateOverMatrix(int rank, int numProcs)
{
	MPIState mpiState;
	mpiState.initialize();
	MPI_Comm comm = mpiState.getWorldComm();
	size_t dim = 4;

	if (rank == 0) {
		unsigned int data[] = {0,0,1,2, 1,0,0,2, 1,1,0,0, 3,1,1,0};
		DependencyMatrix matrix(dim, data, mpiState);
		DependencyMatrixIterator *it = matrix.createIterator();

		bool cond = true;
		size_t index = 0;
		for (it->firstState(); !it->isDone(); it->nextState()) {
			size_t x = index / dim;
			size_t y = index % dim;
			if (it->currentRow() != x || it->currentColumn() != y ||
					it->currentDep() != data[index])
				cond = false;
			index++;
		}
		delete it;

		ASSERT_TRUE(cond, "Can iterate over Dependency-Matrix");
	}
}
