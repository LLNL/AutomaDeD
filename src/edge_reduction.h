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
 * edge_reduction.h
 *
 *  Created on: Jun 10, 2013
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 *         Modified on: Nov 2, 2013
 *          *    Author: Subrata Mitra
 *           *    Contact: mitra4@purdue.edu
 */

#ifndef EDGE_REDUCTION_H_
#define EDGE_REDUCTION_H_

#include "mpi_state.h"
#include "model_components.h"
#include "markov_model.h"

#include <set>
#include <mpi.h>

typedef struct {
	std::map<Edge,EdgeInfoContainer> edges;
	StateFactory sFactory;
} FactoryAndEdges;

class EdgeReduction {
private:
	MPIState mpiState;
	MPI_Comm comm;
	std::map<Edge,EdgeInfoContainer> edges;
	StateFactory *sFactory;
	const MarkovModel<State> *mModel;

	// Returns the number of bytes required to pack it via MPI
	int packed_size(MPI_Comm comm) const;

	// Pack it via MPI
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;

	// Unpack it via MPI
	static FactoryAndEdges unpack(void *buf, int bufsize, int *position,
			MPI_Comm comm);

	void updateLocalData(const FactoryAndEdges &recvData);

public:

	EdgeReduction(const MPIState &s) :
		mpiState(s), comm(s.getWorldComm()), sFactory(NULL), mModel(NULL) {};

	void reduce(const MarkovModel<State> *m, StateFactory *f);

	// Send function used in binomial-tree reduction.
	void send(int receiver) const;

	// Receive function used in binomial-tree reduction.
	void receive(int sender);

	std::map<Edge,EdgeInfoContainer> getEdges() const;

	//void printEdges(int forRank = -1);
};

#endif /* EDGE_REDUCTION_H_ */
