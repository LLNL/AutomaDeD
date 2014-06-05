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
 * test_MarkovModel.h
 *
 *  Created on: Jan 19, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#ifndef TEST_MARKOVMODEL_H_
#define TEST_MARKOVMODEL_H_

#include "markov_model.h"
#include "model_components.h"

class Test_MarkovModel {
private:
	static MarkovModel<State> createSimpleMM();
public:
	static void canUpdateProbabilityMatrix();
	static void canIterateOverProbabilityMatrix();
	static void canGetProbabilitiesFromMatrix();
	static void canUpdateMarkovModel();
	static void canIterateOverEdgesOfMarkovModel();
	static void canDoDFSearchinMarkovModel();
	static void canFindAllPathsInMarkovModel();
	static void canCalculateLargestProbability();
	static void canCreateTransitiveClosure();
	static void performanceWhenUsingClosureIsBetter();
	static void canSaveLastState();
	static void multipleThreadsCanUpdateMarkovModel();
	static void canCalculateTransitionProbability();
};


#endif /* TEST_MARKOVMODEL_H_ */
