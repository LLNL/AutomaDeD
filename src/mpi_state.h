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
 * mpi_state.h
 *
 *  Created on: Jan 10, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef MPI_STATE_H_
#define MPI_STATE_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include <string>
#include <mpi.h>

using namespace std;

/**
 * Variables of local MPI process.
 * Examples of variables are process rank and global communicator size.
 */
class MPIState {
private:
	int processRank;
	int commWorldSize;
	string progName;
	MPI_Comm comm;
	int tag;
public:

	/**
	 * Initialize states variables
	 */
	void initialize();

	/**
	 * Get MPI process rank of local process
	 * @return rank using MPI_Comm_rank and MPI_COMM_WORLD
	 */
	int getProcessRank() const;

	/**
	 * Get communicator size
	 * @return size of the group using MPI_Comm_size and MPI_COMM_WORLD
	 */
	int getCommWorldSize() const;

	/**
	 * Whether the local process is the root process
	 * @return true if process is root; false otherwise
	 */
	bool isRoot() const;

	void setProgramName(const string &name);
	string getProgramName() const;

	MPI_Comm getWorldComm() const;
	int getTag() const;
};

#endif /* MPI_STATE_H_ */
