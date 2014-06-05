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
 * test_AnalysisDriver.C
 *
 *  Created on: Mar 6, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_AnalysisDriver.h"
#include "analysis_driver.h"
#include "mpi_state.h"
#include "model_components.h"
#include "markov_model.h"
#include "debugging.h"
#include "assert_functions.h"


void Test_AnalysisDriver::canPrintLeastProgressedTask(int rank, int numProcs)
{
	ASSERT((numProcs >= 8), ("Number of processes is >= 8"));
	if (numProcs != 8) // cannot perform reduction with < 8 procs
		return;

	MPIState mpiState;
	mpiState.initialize();

	StateFactory factory;
	State s0 = factory.createState("Zero");
	State s1 = factory.createState("Uno");
	State s2 = factory.createState("Dos");
	State s3 = factory.createState("Tres");
	State s4 = factory.createState("Cuatro");
	State s5 = factory.createState("Cinco");
	State s6 = factory.createState("Seis");
	State s7 = factory.createState("Siete");

	EdgeAnnotation ea (1);
	MarkovModel<State> mm;
	mm.addEdge(s0, s1,ea);
	mm.addEdge(s1, s2,ea);
	mm.addEdge(s2, s3,ea);
	mm.addEdge(s3, s4,ea);
	mm.addEdge(s4, s5,ea);
	mm.addEdge(s5, s6,ea);
	mm.addEdge(s6, s2,ea);
	mm.addEdge(s5, s7,ea);

	if (rank==0) {
		mm.addEdge(s0, s1,ea); // last state
		AnalysisDriver driver(mpiState, &mm, &factory);
		driver.findLeastProgressedTasks();

		// No condition to past this test so far
		ASSERT_TRUE(true, "Driver can print least-progressed task");

	} else if (rank==3 || rank==4 || rank==5) {
		mm.addEdge(s3, s4,ea); // last state
		AnalysisDriver driver(mpiState, &mm, &factory);
		driver.findLeastProgressedTasks();
	} else if (rank==1 || rank==2 || rank==6) {
		mm.addEdge(s5, s6,ea); // last state
		AnalysisDriver driver(mpiState, &mm, &factory);
		driver.findLeastProgressedTasks();
	} else if (rank==7) {
		mm.addEdge(s5, s7,ea); // last state
		AnalysisDriver driver(mpiState, &mm, &factory);
		driver.findLeastProgressedTasks();
	}
}
