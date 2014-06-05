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
 * main_controller.C
 *
 *  Created on: Dec 12, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *        Modified on: Nov 2, 2013
 *         *    Author: Subrata Mitra
 *          *    Contact: mitra4@purdue.edu
 *
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "mpi_wrappers.h"
#include "mpi_state.h"
#include "model_components.h"
#include "backtrace.h"
#include "transitions.h"
#include "utilities.h"
#include "debugging.h"
#include "config.h"
#include "signal_handlers.h"
#include "helper_thread.h"
#include "markov_model.h"
#include "io_utilities.h"
#include "analysis_driver.h"

#include <mpi.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <cstdio>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <cstdarg>
#include <cstring>

using namespace std;

/*-----------------------------------------------------------------*/
/*-- Globals                                                       */
/*-----------------------------------------------------------------*/
MPIState mpiState;
TransitionsManager *transitionsManager;
StateFactory *sFactory;
bool savedTraces;

/* Helper thread */
pthread_t thread;
ThreadData threadData;

/* Markov Model */
MarkovModel<State> markovModel;

/*Merged Markov model from all the processes */
MarkovModel<State>* mergedMarkovModel;
StateFactory mergedSFactory;

//list< MPIParams > sentParams;
//list< MPIParams > receivedParams;

map<string, OperationType> globalOpType;
OperationType getOpType(const char *);
void initGlobalOpTypeTable();

/*-----------------------------------------------------------------*/
/*-- MPI-wrappers calls                                            */
/*-----------------------------------------------------------------*/

/**
 * Called right after PMPI_Init.
 */
void afterInit(int *argc, char ***argv)
{
	mpiState.initialize();

	// Build first state
	string bt = Backtrace::getBacktrace();
	sFactory = new StateFactory();
	//SnapShot firstSnap = TransitionsManager::createSnapShot();
	State firstState = sFactory->createState(bt);
	transitionsManager = new TransitionsManager(firstState);

	// Register signal handlers
	//signal(SIGINT, sigterm_handler);
	//signal(SIGKILL, sigterm_handler);
	//signal(SIGQUIT, sigterm_handler);
	//signal(SIGTERM, sigterm_handler);
	//signal(SIGSEGV, sigterm_handler);
	//signal(SIGABRT, sigterm_handler);

	savedTraces = false;

	// Init helper thread
	initHelperThread(&thread, &threadData);
}

/**
 * Called right before PMPI_Finalize.
 */
void beforeFinalize()
{
	//PMPI_Barrier(MPI_COMM_WORLD);

	//OperationType t = getOpType("MPI_Finalize");
	//MPIParams params; // null object
	transitionsManager->transitionBeforeMPICall();
	//transitionsManager->transitionBeforeMPICall(t, params);

	// Get memory usage
	struct rusage data;
	getrusage(RUSAGE_SELF, &data);
	if (mpiState.isRoot())
		printf("*** MEMORY USED: %ld ***\n", data.ru_maxrss);
#if 0
	AnalysisDriver driver(mpiState, &markovModel, sFactory);
	driver.findLeastProgressedTasks(true);
	if (mpiState.isRoot())
	{
		//dumpMarkovModel(markovModel);
	        //dumpStateFactoryWithResolvedName(mergedSFactory);
	        //dumpMarkovModel(mergedMarkovModel);
	}
#endif
	//PMPI_Barrier(MPI_COMM_WORLD);
}

/**
 * Called before calling an MPI routine.
 * @param functionName Name of the MPI routine
 */
void beforeMPICall(const char *functionName,...)
{

#if 0
	OperationType t = getOpType(functionName);

	MPIParams params;
	if (t == SEND_OP || t == RECV_OP) {
		// initialize
        va_list par;
        va_start(par, functionName);

        // Get parameters
        void *unused = va_arg(par, void *);
        params.bufferSize = va_arg(par, int);
        params.dataType = (int)(va_arg(par, MPI_Datatype));
        params.remoteProcess = va_arg(par, int);
        params.tag = va_arg(par, int);
        params.comm = (int)(va_arg(par, MPI_Comm));
        va_end(par);
	}
#endif
	transitionsManager->transitionBeforeMPICall();
	//transitionsManager->transitionBeforeMPICall(t, params);
}

/**
 * Called after calling an MPI routine.
 * @param functionName Name of the MPI routine
 */
void afterMPICall(const char *functionName)
{
	transitionsManager->transitionAfterMPICall();
}

OperationType getOpType(const char *functionName)
{
	OperationType t = REGULAR_OP;

	string name(functionName);
	map<string, OperationType>::const_iterator it = globalOpType.find(name);

	if (it != globalOpType.end())
		t = it->second;

	return t;
}

/*-----------------------------------------------------------------*/
/*-- Utilities                                                     */
/*-----------------------------------------------------------------*/

void initGlobalOpTypeTable()
{
	// Send operations
	globalOpType.insert(
			pair<string, OperationType>("MPI_Bsend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Issend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Rsend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Irsend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Ibsend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Ssend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Isend", SEND_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Send", SEND_OP));

	// Receive operations
	globalOpType.insert(
			pair<string, OperationType>("MPI_Recv", RECV_OP));
	globalOpType.insert(
			pair<string, OperationType>("MPI_Irecv", RECV_OP));
}


