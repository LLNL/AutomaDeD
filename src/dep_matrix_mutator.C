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
 * dep_matrix_mutator.C
 *
 *  Created on: Mar 6, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *         Modified on: Nov 2, 2013
 *          *    Author: Subrata Mitra
 *           *    Contact: mitra4@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "dep_matrix_mutator.h"
#include "dependency_matrix.h"

#include <set>

using namespace std;

//bool DepMatrixMutator::hasCycle(const DependencyMatrix &matrix)
//{}

/*
 * Eliminates simple cycles.
 *
 * Algorithm:
 * ----------
 * Check for edges in both directions for each state: x->y and y->x.
 * If there is an edge in both directions for an edge,
 * set the dependency as 0;
 */
void DepMatrixMutator::removeCycles(DependencyMatrix &matrix)
{
	DependencyMatrixIterator *it = matrix.createIterator();
	for (it->firstState(); !it->isDone(); it->nextState()) {
		size_t row = it->currentRow();
		size_t col = it->currentColumn();
		if (col > row) {
			if (it->currentDep() > 0) {
				if (matrix.getDependency(row,col) ==
						matrix.getDependency(col,row)) {
					matrix.addDependency(row,col,0);
					matrix.addDependency(col,row,0);
				}
			}
		}
	}
	delete it;
}

/*
 * Eliminates dependencies of type 3. And convert dependency of type 2 to type 1
 */
void DepMatrixMutator::removeUndefinedDependencies(DependencyMatrix &matrix)
{
	DependencyMatrixIterator *it = matrix.createIterator();
	for (it->firstState(); !it->isDone(); it->nextState()) {
		if (it->currentDep() == 3) {
			size_t row = it->currentRow();
			size_t col = it->currentColumn();
			matrix.addDependency(row,col,0);
		}
		else if(it->currentDep() == 2)
		{
			size_t row = it->currentRow();
			size_t col = it->currentColumn();
			matrix.addDependency(row,col,0);
			matrix.addDependency(col,row,1);
		}
	}
	delete it;

}

/*
 * Algorithm:
 * ----------
 * Find states without outgoing edges.
 */
set<size_t> DepMatrixMutator::findStatesWithoutDependencies(
			DependencyMatrix &matrix)
{
	// Initialize set with all the states of the matrix, which
	// correspond to the dimension of the matrix
	set<size_t> ret;
	for (size_t i=0; i < matrix.getDimension(); ++i)
		ret.insert(i);

	DependencyMatrixIterator *it = matrix.createIterator();
	for (it->firstState(); !it->isDone(); it->nextState()) {
		size_t row = it->currentRow();
		size_t col = it->currentColumn();
		unsigned int dep = it->currentDep();
		if (dep == 1 || dep == 3)
			ret.erase(row);
		if (dep == 2 || dep == 3)
			ret.erase(col);
	}
	delete it;

	return ret;
}
