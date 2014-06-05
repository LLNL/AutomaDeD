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
 * serial_driver.C
 *
 *  Created on: Jan 18, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_stateFactory.h"
#include "test_MarkovModel.h"
#include "test_RangeSet.h"

#include <iostream>

using namespace std;

int main()
{
	cout << "Single-process tests:" << endl;
	Test_StateFactory::canCreateStates();
	Test_StateFactory::canFindStates();
	Test_StateFactory::canFindNames();
	Test_StateFactory::canUpdateFactoryFromMultipleThreads();

	Test_MarkovModel::canUpdateProbabilityMatrix();
	Test_MarkovModel::canIterateOverProbabilityMatrix();
	Test_MarkovModel::canGetProbabilitiesFromMatrix();
	Test_MarkovModel::canUpdateMarkovModel();
	Test_MarkovModel::canIterateOverEdgesOfMarkovModel();
	Test_MarkovModel::canDoDFSearchinMarkovModel();
	Test_MarkovModel::canFindAllPathsInMarkovModel();
	Test_MarkovModel::canCalculateLargestProbability();
	Test_MarkovModel::canCreateTransitiveClosure();
	//Test_MarkovModel::performanceWhenUsingClosureIsBetter();
	Test_MarkovModel::canSaveLastState();
	Test_MarkovModel::multipleThreadsCanUpdateMarkovModel();
	Test_MarkovModel::canCalculateTransitionProbability();

	Test_RangeSet::canPerformBinarySearchInRangeSet();
	Test_RangeSet::canAddRangeSets();

	return 0;
}
