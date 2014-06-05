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
 * test_stateFactory.C
 *
 *  Created on: Jan 18, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_stateFactory.h"
#include "model_components.h"
#include "assert_functions.h"

#include <stdio.h>
#include <set>
using namespace std;

void Test_StateFactory::canCreateStates()
{
	StateFactory factory;
	set<State> tableOfStates;
	size_t numStates = 100;
	char buffer[50];
	for (size_t i=0; i < numStates; i++) {
		sprintf(buffer, "%d", (int)i);
		string name(buffer);
		State s = factory.createState(name);
		tableOfStates.insert(s);
	}

	// We should have 'numStates' different states in the table
	ASSERT_TRUE(tableOfStates.size() == numStates,
				("Factory can create different states for different strings"));
}

void Test_StateFactory::canFindStates()
{
	StateFactory factory;
	size_t numStates = 100;
	char buffer[50];
	for (size_t i=0; i < numStates; i++) {
		sprintf(buffer, "%d", (int)i);
		string name(buffer);
		State s = factory.createState(name);
	}

	// create a state that has been saved in the table
	sprintf(buffer, "%d", (int)(numStates-1));
	string name(buffer);
	State s = factory.createState(name);
	bool found = factory.findAndGetState(s, name);

	ASSERT_TRUE(found,("Factory can find states that have been created"));

	name = "RANDOM_STATE";
	bool notFound = factory.findAndGetState(s, name);
	ASSERT_TRUE(!notFound,("Factory can distinguish uncreated states"));
}

void Test_StateFactory::canFindNames()
{
	StateFactory factory;
	size_t numStates = 100;
	char buffer[50];
	for (size_t i=0; i < numStates; i++) {
		sprintf(buffer, "%d", (int)i);
		string name(buffer);
		State s = factory.createState(name);

		// In last iteration
		if (i == numStates-1) {
			bool found = factory.findAndGetName(name, s);
			ASSERT_TRUE(found,("Factory can find names that have been saved"));
		}
	}
}

typedef struct {
	StateFactory fact;
	size_t result;
} ThreadData;

void* factory_thread(void *params)
{
	ThreadData *td = static_cast< ThreadData * >(params);
	StateFactory factory = td->fact;
	set<State> tableOfStates;
	size_t numStates = 100;
	char buffer[50];
	for (size_t i=0; i < numStates; i++) {
		sprintf(buffer, "%d", (int)i);
		string name(buffer);
		State s = factory.createState(name);
		tableOfStates.insert(s);
	}

	// Save result
	td->result = tableOfStates.size();

	pthread_exit(NULL);
}

void Test_StateFactory::canUpdateFactoryFromMultipleThreads()
{
	size_t numThreads = 64;
	pthread_t thread[numThreads];

	ThreadData td[numThreads];

	// Create threads
	for (size_t i=0; i < numThreads; i++) {
		int ret = pthread_create(&(thread[i]), NULL, factory_thread,
				static_cast<void *>(&(td[i])));
		if (ret != 0)
			cout << "Error creating thread!" << endl;
	}

	// Wait for threads
	for (size_t i=0; i < numThreads; i++)
		pthread_join(thread[i], NULL);

	// All threads should finish with the same value
	bool allEqual = true;
	size_t val = td[0].result;
	for (size_t i=1; i < numThreads; i++) {
		size_t tmp = td[i].result;
		if (tmp != val) {
			allEqual = false;
			break;
		}
		val = tmp;
		//cout << "Res: " << td[i].result << endl;
	}

	ASSERT_TRUE(allEqual, ("Can update Factory from multiple threads"));
}
