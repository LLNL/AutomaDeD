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
 * markov_model_reduction.h
 *
 *  Created on: Jun 8, 2013
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 */

#ifndef MARKOV_MODEL_REDUCTION_H_
#define MARKOV_MODEL_REDUCTION_H_

#include "markov_model.h"
#include "mpi_state.h"
#include "model_components.h"

#include <mpi.h>

/**
 * This class implements a global reduction over all the Markov models
 * (of each process).
 *
 * FIXME:
 * One problem with the reduction is that the local Markov model of a process
 * is modified with the reduction. So it is not safe to rely on the model
 * for things like requesting the probability of a transition.
 */

/*class MarkovModelReduction {
private:
	MPIState mpiState;
	MPI_Comm comm;
	MarkovModel<State> *mModel;

	// Returns the number of bytes required to pack it via MPI
	int packed_size(MPI_Comm comm) const;

	// Pack it via MPI
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;

	// Unpack it via MPI
	static MarkovModel<State> unpack(void *buf, int bufsize, int *position,
			MPI_Comm comm);

	void updateLocalData(const MarkovModel<State> &recvData);

public:

	MarkovModelReduction(const MPIState &s) :
		mpiState(s), comm(MPI_COMM_WORLD) {};

	void reduce(MarkovModel<State> mModel);

	// Send function used in binomial-tree reduction.
	void send(int receiver) const;

	// Receive function used in binomial-tree reduction.
	void receive(int sender);

	MarkovModel<State> getMarkovModel() const;
};*/

#endif /* MARKOV_MODEL_REDUCTION_H_ */
