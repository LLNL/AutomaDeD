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
 * parallel_driver.C
 *
 *  Created on: Feb 8, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_stateReductions.h"
#include "test_DependencyMatrix.h"
#include "test_RangeSetTable.h"
#include "test_DepMatrixMutator.h"
#include "test_AnalysisDriver.h"

#include <mpi.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

int main()
{
	PMPI_Init(NULL, NULL);

    int rank, numprocs;
    MPI_Comm comm = MPI_COMM_WORLD;
    PMPI_Comm_size(comm,&numprocs);
    PMPI_Comm_rank(comm,&rank);

    if (numprocs < 2) {
    	cout << "Error: number of processes has to be > 1" << endl;
    	exit(EXIT_FAILURE);
    }

    if (rank == 0)
    	cout << "Multi-process (parallel) tests (" << numprocs << " procs):" << endl;

    Test_StateReductions::canSendAndReceiveStates(rank, numprocs);
    Test_StateReductions::canPerformStateReduction(rank, numprocs);
    Test_StateReductions::reductionBroadcastLastStates(rank, numprocs);
    Test_StateReductions::reductionOrderStates(rank, numprocs);

    test_DependencyMatrix::canSendAndReceiveMatrix(rank, numprocs);
    test_DependencyMatrix::canReduceDependencyMatrix(rank, numprocs);
    test_DependencyMatrix::canIterateOverMatrix(rank, numprocs);

    test_RangeSetTable::canCreateRangeSetTable(rank, numprocs);
    test_RangeSetTable::canReduceRangeSetTable(rank, numprocs);

    Test_DepMatrixMutator::canRemoveCycles(rank, numprocs);
    Test_DepMatrixMutator::canRemoveUndefinedDependencies(rank, numprocs);
    Test_DepMatrixMutator::canFindStatesWithoutDep(rank, numprocs);

    Test_AnalysisDriver::canPrintLeastProgressedTask(rank, numprocs);

	PMPI_Finalize();
	return 0;
}
