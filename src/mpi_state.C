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
 * mpi_state.cxx
 *
 *  Created on: Jan 10, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "mpi_state.h"
#include "debugging.h"

#include <mpi.h>

using namespace std;

int MPIState::getProcessRank() const
{
	return processRank;
}

int MPIState::getCommWorldSize() const
{
	return commWorldSize;
}

void MPIState::initialize()
{
	// Create new communicator
	MPI_Comm world = MPI_COMM_WORLD, newWorld;
	MPI_Group world_group;
	PMPI_Comm_group(world, &world_group);
	PMPI_Comm_create(world, world_group, &newWorld);
	comm = newWorld;
	//comm = MPI_COMM_WORLD;

	//tag = MPI_TAG_UB-1;
	tag = 1;
	int ret;

	ret = PMPI_Comm_rank(comm, &(processRank));
#if STATE_TRACKER_DEBUG
	if (ret != MPI_SUCCESS)
		handleError("in MPIState::MPIState(): unable to get mpi rank");
#endif

	ret = PMPI_Comm_size(comm, &(commWorldSize));
#if STATE_TRACKER_DEBUG
	if (ret != MPI_SUCCESS)
		handleError("in MPIState::MPIState(): unable to get group size");
#endif
}

bool MPIState::isRoot() const
{
	return (processRank == 0);
}

void MPIState::setProgramName(const string &name)
{
	progName = name;
}

string MPIState::getProgramName() const
{
	return progName;
}

MPI_Comm MPIState::getWorldComm() const
{
	return comm;
}

int MPIState::getTag() const
{
	return tag;
}
