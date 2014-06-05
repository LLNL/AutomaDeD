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
 * dependency_matrix.C
 *
 *  Created on: Feb 17, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#if HAVE_CONFIG_H
#include "statetracker-config.h"
#endif

#include "dependency_matrix.h"
#include "reduction.h"
#include "utilities.h"
#include <mpi.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

/*
 * --------------------------------------------------------------------------
 * Main class functions
 * --------------------------------------------------------------------------
 */

DependencyMatrix::DependencyMatrix(
		size_t d, unsigned int *data, const MPIState &s) :
		dim(d), mpiState(s)
{
	for (size_t i=0; i < dim; ++i) {
		unsigned int buff[dim];
		for (size_t j=0; j < dim; ++j)
			buff[j] = data[dim*i + j];
		vector<unsigned int> v(buff, buff+dim);
		matrix.push_back(v);
	}
}

void DependencyMatrix::reduceGlobally()
{
	// Perform reduction of last states
	Reducer<DependencyMatrix> *red = new BinomialReducer<DependencyMatrix>;
	//Reducer<DependencyMatrix> *red = new IntraReducer<DependencyMatrix>;

	int numProcesses = mpiState.getCommWorldSize();
	int rank = mpiState.getProcessRank();
	red->reduce(0, numProcesses, rank, this);
	delete red;
}

void DependencyMatrix::send(int receiver) const
{
	MPI_Comm comm = mpiState.getWorldComm();
	int tag = mpiState.getTag();

	// Allocate buffer
	int buff_size = packed_size(comm);
	char buff[buff_size];
	buff[0] = '\0';
	int position = 0;

	// Pack and send matrix
	pack(buff, buff_size, &position, comm);
	PMPI_Send(buff, buff_size, MPI_PACKED, receiver, tag, comm);
}

void DependencyMatrix::receive(int sender)
{
	MPI_Status status;
	MPI_Comm comm = mpiState.getWorldComm();
	int tag = mpiState.getTag();

	// Allocate buffer
	int buff_size = packed_size(comm);
	char buff[buff_size];
	buff[0] = '\0';
	int position = 0;

	// Receive matrix
	PMPI_Recv(buff, buff_size, MPI_PACKED, sender, tag, comm, &status);

	DependencyMatrix depMatrix = unpack(buff, buff_size, &position, comm);

	// Update local data
	updateLocalData(depMatrix);
}

void DependencyMatrix::updateLocalData(const DependencyMatrix &depMatrix)
{
#if STATE_TRACKER_DEBUG
	if (depMatrix.dim != dim)
		handleError("in DependencyMatrix::updateLocalData: "
						"matrices of different dimensions");
#endif

	for (size_t i=0; i < dim; ++i) {
		for (size_t j=0; j < dim; ++j) {
			unsigned int val = depMatrix.matrix[i][j];
			matrix[i][j] = matrix[i][j] | val; // bitwise OR operation
		}
	}
}

int DependencyMatrix::packed_size(MPI_Comm comm) const
{
	int size;
	int numElems = static_cast<int>(dim*dim);
	PMPI_Pack_size(numElems, MPI_UNSIGNED, comm, &size);
	return size;
}

void DependencyMatrix::pack(void *buf, int bufsize,
		int *position, MPI_Comm comm) const
{
	for (size_t i=0; i < dim; ++i) {
		for (size_t j=0; j < dim; ++j) {
			unsigned int tmp = matrix[i][j];
			PMPI_Pack((void *)&tmp, 1, MPI_UNSIGNED,
					buf, bufsize, position, comm);
		}
	}
}

DependencyMatrix DependencyMatrix::unpack(void *buf, int bufsize,
		int *position, MPI_Comm comm) const
{
	size_t numElems = dim*dim;
	unsigned int recvBuffer[numElems];
	PMPI_Unpack(buf, bufsize, position, recvBuffer,
			(int)numElems, MPI_UNSIGNED, comm);

	DependencyMatrix m(dim, mpiState);
	for (size_t i=0; i < numElems; ++i) {
		//std::cout << "[" << i << "]: " << recvBuffer[i] << std::endl;
		size_t y = i % dim;
		size_t x = i / dim;
		m.addDependency(x,y,recvBuffer[i]);
	}
	return m;
}

void DependencyMatrix::addDependency(size_t src, size_t dst, unsigned int dep)
{
#if STATE_TRACKER_DEBUG
	if (src >= dim || dst >= dim)
		handleError("in DependencyMatrix::addDependency: "
				"dimensions out of bounds in matrix");
#endif

	matrix[src][dst] = dep;
}

unsigned int DependencyMatrix::getDependency(size_t src, size_t dst) const
{
#if STATE_TRACKER_DEBUG
	if (src >= dim || dst >= dim)
		handleError("in DependencyMatrix::getDependency: "
				"dimensions out of bounds in matrix");
#endif

	return matrix[src][dst];
}

DependencyMatrixIterator * DependencyMatrix::createIterator()
{
	DependencyMatrixIterator * ptr = new DependencyMatrixIterator(this);
	return ptr;
}

string DependencyMatrix::toString() const
{
	string ret("Dependency Matrix:\n------------------\n");
	for (size_t i=0; i < matrix.size(); ++i) {
		/*int n = 0;
		for (size_t j=0; j < matrix[i].size(); ++j)
			if (matrix[i][j] > 0)
				n++;
		if (n==0) {
			char buff[512];
			sprintf(buff, "(%d):\n", (int)i);
			ret += buff;
			continue;
		}*/

		for (size_t j=0; j < matrix[i].size(); ++j) {
			unsigned int d = matrix[i][j];
			if (d > 0) {
				char buff[512];
				sprintf(buff, "(%d,%d): %d\n", (int)i, (int)j, (int)d);
				ret += buff;
			}
		}
	}

	return ret;
}

void DependencyMatrix::printTabulated() const
{
	cout << "    ";
	for (size_t i=0; i < matrix.size(); ++i) {
		cout << "s" << setw(2) << i << " ";
	}
	cout << endl;

	cout << "    ";
	for (size_t i=0; i < matrix.size(); ++i) {
		cout << "----";
	}
	cout << endl;

	for (size_t i=0; i < matrix.size(); ++i) {
		cout << setw(1) << "s" << setw(2) << i << "|";
		for (size_t j=0; j < matrix.size(); ++j) {
			unsigned int d = matrix[i][j];
			cout << setw(3) << d << " ";
		}
		cout << endl;
	}
}

string DependencyMatrix::toCSVFormat() const
{
	string ret("");
	for (size_t i=0; i < matrix.size(); ++i) {
		for (size_t j=0; j < matrix.size(); ++j) {
			unsigned int d = matrix[i][j];
			char buffer[3];
			itoa((int)d, buffer);
			ret += buffer;
			ret += ",";
		}
		ret += "\n";
	}
	return ret;
}

size_t DependencyMatrix::getDimension() const
{
	return dim;
}

size_t DependencyMatrix::numEdges() const
{
	size_t ret = 0;
	for (size_t i=0; i < matrix.size(); ++i) {
		for (size_t j=0; j < matrix[i].size(); ++j) {
			if (matrix[i][j] > 0) {
				ret++;
			}
		}
	}

	return ret;
}

unsigned int DependencyMatrix::translateDepFromProbability(double x, double y)
{
	unsigned int ret = 0;

	if ((0<x && x<1) && (0<y && y<1)) {
		ret = 3; // undefined
	} else if (x==1 && y==1) {
		ret = 3; // also undefined
	} else if (x==0 && y>0) {
		ret = 1;
	} else if ((0<x && x<1) && y==1) {
		ret = 1;
	} else if (x>0 && y==0) {
		ret = 2;
	} else if (x==1 && (0<y && y<1)) {
		ret = 2;
	}

	// In all other cases there is no dependence
	return ret;
}

/*
 * --------------------------------------------------------------------------
 * Iterator class functions
 * --------------------------------------------------------------------------
 */

DependencyMatrixIterator::DependencyMatrixIterator(DependencyMatrix *m) :
		matrix(m), i(0), j(0){};

void DependencyMatrixIterator::firstState()
{
	i = 0;
	j = 0;
}

void DependencyMatrixIterator::nextState()
{
	if (j+1 == matrix->matrix.size()) {
		i++;
		j=0;
	} else {
		j++;
	}
}

bool DependencyMatrixIterator::isDone() const
{
	return (i==matrix->matrix.size() && j==0)? true : false;
}

size_t DependencyMatrixIterator::currentRow() const
{
	return i;
}

size_t DependencyMatrixIterator::currentColumn() const
{
	return j;
}

unsigned int DependencyMatrixIterator::currentDep() const
{
	return matrix->matrix[i][j];
}
