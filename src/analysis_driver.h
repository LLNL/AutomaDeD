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
 * analysis_driver.h
 *
 *  Created on: Mar 6, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *      *  Modified on: Nov 2, 2013
 *       *  Author: Subrata Mitr
 *        *  Contact: mitra4@purdue.edu
 */

#ifndef ANALYSIS_DRIVER_H_
#define ANALYSIS_DRIVER_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "mpi_state.h"
#include "model_components.h"
#include "dependency_matrix.h"
#include "dep_matrix_mutator.h"
#include "markov_model.h"
#include "state_reduction.h"
#include "range_set_table.h"
#include "graph_algorithm.h"

#include <set>

using namespace std;

class AnalysisDriver {
private:
	MPIState mpiState;
	MarkovModel<State> *mm;
	StateFactory *factory;

	DependencyMatrix buildDepMatrix(const ReducedStateVector &redVector);

	/**
	 * Used in the gatheredLPTAlgorithm() method
	 */
	DependencyMatrix buildDepMatrixLocally(const ReducedStateVector &redVector);

	DependencyMatrix loopAwareDepMatrixBuilder(const ReducedStateVector &redVector,
						RangeSetTable rsTable,loopAnalysis& LA);

	unsigned int calculateDependencyBasedOnLoop(loopAnalysis& LA,set<Edge>& loopSet,
                                                   State& src,size_t srcIndex, State& dst, size_t dstIndex,
                                                   RangeSetTable& rsTable);

	void printLeastProgressedTasks(const RangeSetTable &rsTable,
			const set<size_t> taskStates,
			const ReducedStateVector &redVector);

	void dumpOutputForGUI(const DependencyMatrix &matrix,
			const ReducedStateVector &redVector,
			const RangeSetTable &rsTable);

	/**
	 * Distributed Least-Progressed Tasks (LPT) Algorithm
	 * As presented in PACT'12 paper.
	 */
	void distributedLPTAlgorithm(bool print=false);

	/**
	 * Gathered Least-Progressed Tasks (LPT) Algorithm
	 * All Markov models are gathered in a single task and the
	 * LP tasks are found from there.
	 * This is algorithm used in PLDI'14 paper.
	 */
	void gatheredLPTAlgorithm(bool print=false);

public:
	AnalysisDriver(MPIState ms, MarkovModel<State> *m, StateFactory *f);
	void findLeastProgressedTasks(bool print=false);

protected:
	AnalysisDriver();
};

#endif /* ANALYSIS_DRIVER_H_ */
