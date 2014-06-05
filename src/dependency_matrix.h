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
 * dependency_matrix.h
 *
 *  Created on: Feb 16, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#if HAVE_CONFIG_H
#include "statetracker-config.h"
#endif

#ifndef DEPENDENCY_MATRIX_H_
#define DEPENDENCY_MATRIX_H_

#include "mpi_state.h"
#include "debugging.h"
#include <mpi.h>
#include <vector>
#include <string>

using namespace std;

class DependencyMatrixIterator;

class DependencyMatrix {
private:
	vector< vector<unsigned int> > matrix;
	size_t dim;
	MPIState mpiState;

	// Returns the number of bytes required to pack it via MPI
	int packed_size(MPI_Comm comm) const;

	// Pack it via MPI
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;

	// Unpack it via MPI
	DependencyMatrix unpack(void *buf, int bufsize,
			int *position, MPI_Comm comm) const;

	void updateLocalData(const DependencyMatrix &depMatrix);

public:
	DependencyMatrix(size_t d, const MPIState &s) :
		matrix(vector< vector<unsigned int> >(d, vector<unsigned int>(d, 0))),
		dim(d), mpiState(s) {};

	DependencyMatrix(size_t d, unsigned int *data, const MPIState &s);

	void addDependency(size_t src, size_t dst, unsigned int dep);
	unsigned int getDependency(size_t src, size_t dst) const;
	string toString() const;
	size_t numEdges() const;
	size_t getDimension() const;
	static unsigned int translateDepFromProbability(double x, double y);
	void printTabulated() const;
	string toCSVFormat() const;

	void reduceGlobally();

	// Send/Receive functions used in binomial-tree reduction.
	void send(int receiver) const;
	void receive(int sender);

	DependencyMatrixIterator * createIterator();

	friend class test_DependencyMatrix;
	friend class DependencyMatrixIterator;

protected:
	DependencyMatrix();
};

class DependencyMatrixIterator {
private:
    const DependencyMatrix * matrix;
    size_t i;
    size_t j;

public:
    DependencyMatrixIterator(DependencyMatrix *m);

    // To iterate in a loop
    void firstState();
    void nextState();
    bool isDone() const;

    // Access functions
    size_t currentRow() const;
    size_t currentColumn() const;
    unsigned int currentDep() const;
};

#endif /* DEPENDENCY_MATRIX_H_ */
