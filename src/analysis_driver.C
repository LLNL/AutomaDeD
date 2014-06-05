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
 * analysis_driver.C
 *
 *  Created on: Mar 6, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *    Modified on: Nov 2, 2013
 *    Author: Subrata Mitra
 *    Contact: mitra4@purdue.edu
 */

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "analysis_driver.h"
#include "mpi_state.h"
#include "model_components.h"
#include "state_reduction.h"
#include "dependency_matrix.h"
#include "dep_matrix_mutator.h"
#include "range_set_table.h"
#include "markov_model.h"
#include "range_set_table.h"
#include "timer.h"
#include "utilities.h"
#include "io_utilities.h"
#include "edge_reduction.h"
#include "config.h"
#include "backtrace.h"
#include "graph_model.h"
#include "debugging.h"
#include <sys/resource.h>

#include <set>
#include <iostream>
#include <link.h>
#include <stdio.h>

using namespace std;
extern MarkovModel<State>* mergedMarkovModel;
extern StateFactory mergedSFactory;
extern MarkovModel<State> markovModel;

AnalysisDriver::AnalysisDriver(
		MPIState ms, MarkovModel<State> *m, StateFactory *f) :
		mpiState(ms), mm(m), factory(f)
{}

void AnalysisDriver::findLeastProgressedTasks(bool print)
{
	  //cout << "analysis started" << endl;
	int rank = mpiState.getProcessRank();
	
	bool useDist=true, ret=true;
	ret = AUTConfig::getBoolParameter("AUT_USE_DISTRIBUTED_ANALYSIS", useDist);
#if 0
	if (!ret || (ret && useDist)) {
		if (mpiState.isRoot())
			cout << "Dependence analysis: distributed" << endl;
		// Call distributed algorithm - PACT'12
		distributedLPTAlgorithm(print);
	} else {
		if (mpiState.isRoot())
			cout << "Dependence analysis: gathered (local)" << endl;
		// New algorithm
		gatheredLPTAlgorithm(print);
	}
#endif

	gatheredLPTAlgorithm(print);
}

void AnalysisDriver::distributedLPTAlgorithm(bool print)
{
	// Get last state seen locally
	State lastState = mm->getLastState();

	// Perform global reduction of local states
	StateReduction stateRed(factory, mpiState);
	ReducedStateVector reducedStateVector =
			stateRed.getLastStatesOrdered(lastState);

	// Build dependency matrix and perform global reduction
	DependencyMatrix depMatrix = buildDepMatrix(reducedStateVector);
	depMatrix.reduceGlobally();

	// Remove simple cycles
	DepMatrixMutator::removeCycles(depMatrix);

	// Remove undefined dependencies
	DepMatrixMutator::removeUndefinedDependencies(depMatrix);

	if (mpiState.isRoot()) {
		cout << "Matrix after removing undef. dependencies:" << endl;
		depMatrix.printTabulated();
	}

	// Perform global reduction of ranges of tasks in last states
	size_t lastStateIndex = reducedStateVector.localIndex;
	size_t numStates = reducedStateVector.vec.size();
	RangeSetTable rsTable(mpiState, numStates, lastStateIndex);
	rsTable.reduceTable();

	// Find states without outgoing edges, i.e. least-progressed tasks
	set<size_t> taskStates =
			DepMatrixMutator::findStatesWithoutDependencies(depMatrix);

	if (print)
		printLeastProgressedTasks(rsTable, taskStates, reducedStateVector);

	//printTaskAndProcessIDMap(mpiState);

	// Dump file for GUI
	if (mpiState.isRoot())
		dumpOutputForGUI(depMatrix, reducedStateVector, rsTable);
}

void AnalysisDriver::gatheredLPTAlgorithm(bool print)
{
	int rank = mpiState.getProcessRank();

	printTaskAndProcessIDMap(mpiState);
	MultiTimer timer;
	if(mpiState.isRoot())
	{
	      timer.start(rank);
	}
	//cout << " in gathered" << endl;
	// Get last state seen locally
	State lastState = mm->getLastState();
	
	mm->updateEdgeTransitionCountTaskMap(rank);

	bool dumpPerRank = false;
	bool r1 = AUTConfig::getBoolParameter("AUT_DUMP_MM_PER_RANK", dumpPerRank);
	if(dumpPerRank)
	{
	  dumpMarkovModelPerRankAsCSV(mm);
	  dumpStateFactory(*factory);
	  exit(EXIT_SUCCESS);
	}

	//cout << "Last state is: " << lastState.getId() << endl;
	//spinHere();
	// Perform global reduction of local states
	StateReduction stateRed(factory, mpiState);
	ReducedStateVector reducedStateVector =
			stateRed.getLastStatesOrdered(lastState);

	//dumpMarkovModel(mm);

	//spinHere();
	// Get edges from other processes and modify current Markov model
	// Effectively this creates a global Markov model
	EdgeReduction edgeRed(mpiState);
	edgeRed.reduce(mm, factory);

         // Perform global reduction of ranges of tasks in last states
	size_t lastStateIndex = reducedStateVector.localIndex;
	size_t numStates = reducedStateVector.vec.size();
	RangeSetTable rsTable(mpiState, numStates, lastStateIndex);
	rsTable.reduceTable();
	//cout << "after reduction" << endl;
	bool noBarrier = false;
	bool r = AUTConfig::getBoolParameter("AUT_NO_BARRIER", noBarrier);
	if(!noBarrier)
	{
          PMPI_Barrier(mpiState.getWorldComm());
	}
	if (mpiState.isRoot()) {
		struct rusage data;
		getrusage(RUSAGE_SELF, &data);
		//printf("*** MEMORY USED 1: %ld ***\n", data.ru_maxrss);
	      //cout << "after reduction " << endl;
	      mm = new MarkovModel<State>();
              //markovModel = *mm;

             Graph_Adjacency* graph_adj = new Graph_Adjacency();

             map<Edge,EdgeInfoContainer> edges = edgeRed.getEdges();

             map<Edge,EdgeInfoContainer>::const_iterator it;
             for (it = edges.begin(); it != edges.end(); ++it) {
                //EdgeAnnotation ea = (it->second).annotation; 
                mm->addEdge(((it->second).theEdge).getSourceState(), ((it->second).theEdge).getDestinationState(),(it->second).annotation);
                graph_adj->addEdge(((it->second).theEdge).getSourceState(), ((it->second).theEdge).getDestinationState(),(it->second).annotation);

             }

	     mergedMarkovModel = mm;
	     mergedSFactory = *factory;

             
             for (size_t i=0; i < reducedStateVector.vec.size(); ++i)
             {
               State s = reducedStateVector.vec[i];
               graph_adj->addBoundaryNode(s);
             }
              
	     bool doNotCompress = false;
	     bool r = AUTConfig::getBoolParameter("AUT_DO_NOT_COMPRESS", doNotCompress);
	     if(!doNotCompress)
	     {
               //now compress the mm graph to create a smaller search space...optimization for runtime
               graph_adj->compressGraph();
	     }

              loopAnalysis LA = loopAnalysis(graph_adj);
	      LA.setGraph(graph_adj);
              LA.populateAllLoopInformation();
	      //Graph_Edges* compressedGraph = graph_adj->convert_To_Graph_Edge_Format();
	      //dumpGraph(compressedGraph);
	      //dumpStateFactoryWithResolvedName(mergedSFactory);

	     getrusage(RUSAGE_SELF, &data);
	     //printf("*** MEMORY USED 2: %ld ***\n", data.ru_maxrss);
              //timer.start(rank);
              DependencyMatrix depMatrix = loopAwareDepMatrixBuilder(reducedStateVector,rsTable,LA);
              //timer.stop(rank, "GRAPH_ANALYSIS_TIME");
              //timer.printTimes();
	      // Remove simple cycles
	      DepMatrixMutator::removeCycles(depMatrix);

	     // Remove undefined dependencies
	     DepMatrixMutator::removeUndefinedDependencies(depMatrix);

	     //TODO: Some times we can not create dependency because he have not seen any behavior in the past
	     //Such as processes waiting in AllGather in one branch and a hang in another branch: test9
	     //Can we infer a dependency here? For example since one group is waiting at Collecive operation
	     //and another group is waiting for non-collective operation: Mostlikely Collective group is waiting 
	     // for non collective ones... ?
             
	     //timer.stop(rank, "TOTAL_ANALYSIS_TIME");

	     cout << "Matrix after removing undef. dependencies:" << endl;
	     depMatrix.printTabulated();

	     // Perform global reduction of ranges of tasks in last states
	     //size_t lastStateIndex = reducedStateVector.localIndex;
	     //size_t numStates = reducedStateVector.vec.size();
	     //RangeSetTable rsTable(mpiState, numStates, lastStateIndex);
	     //rsTable.reduceTable();

	     // Find states without outgoing edges, i.e. least-progressed tasks
	     set<size_t> taskStates =
			DepMatrixMutator::findStatesWithoutDependencies(depMatrix);
             


             timer.printTimes();

	     if (print)
		printLeastProgressedTasks(rsTable, taskStates, reducedStateVector);

	    //printTaskAndProcessIDMap(mpiState);
	    dumpOutputForGUI(depMatrix, reducedStateVector, rsTable);
	    bool dumpMM = false;
	    r = AUTConfig::getBoolParameter("AUT_DUMP_MM", dumpMM);
	    if(dumpMM)
	    {
	     dumpMarkovModel(mergedMarkovModel);
	     Graph_Edges* compressedGraph = graph_adj->convert_To_Graph_Edge_Format();
	     dumpGraph(compressedGraph);
	     dumpStateFactory(mergedSFactory);
	    }
     }
}

DependencyMatrix AnalysisDriver::buildDepMatrix(
		const ReducedStateVector &redVector)
{
	size_t indexLastState = redVector.localIndex;
	size_t matrixSize = redVector.vec.size();
	DependencyMatrix depMatrix(matrixSize, mpiState);

	State src = redVector.vec[indexLastState];
	for (size_t i=0; i < matrixSize; ++i) {
		if (i != indexLastState) {
			State dst = redVector.vec[i];
			double x = mm->transitionProbability(src, dst);
			double y = mm->transitionProbability(dst, src);
			unsigned int d = DependencyMatrix::translateDepFromProbability(x,y);
			if(d == 0)
			{
				if(src.getId() < dst.getId())
				{
					d = 2; //heuristic
				}
			}
			depMatrix.addDependency(indexLastState, i, d);
		} else {
			// FIXME: Do we need this variable 'dst'?
			State dst = redVector.vec[i];
			depMatrix.addDependency(indexLastState, i, 0);
		}
	}

	return depMatrix;
}

DependencyMatrix AnalysisDriver::loopAwareDepMatrixBuilder(
                const ReducedStateVector &redVector,RangeSetTable rsTable,
                loopAnalysis& LA)
{
        size_t matrixSize = redVector.vec.size();
        DependencyMatrix depMatrix(matrixSize, mpiState);

        for (size_t i = 0; i < matrixSize; ++i) {
                for (size_t j = 0; j < matrixSize; ++j) {
                       // if (i != j) {
                        if (j > i) { //only consider the upper triangular matrix... no need to check all combinations
                                State src = redVector.vec[i];
                                State dst = redVector.vec[j];
				//cout << "i to j:" << i << "-" << j << "=>src to dst:" << src.getId() << "-" << dst.getId() << endl;
				//set<Edge> loopSet = LA.getCommonLoops(src,dst);
				//set<Edge> loopSet = LA.getCommonMergedLoopsBackEdges(src,dst);
				set<Edge> loopSet = LA.getBackEdgesForCommonLoops(src,dst);
                                if(loopSet.size() == 0)
                                {
   				  //cout << "PROBABILITY BASED analysis between:"<< src.getId() << "-" << dst.getId() << endl;
                                  double x = mm->transitionProbability(src, dst);
                                  double y = mm->transitionProbability(dst, src);
                                  unsigned int d =
                                                DependencyMatrix::translateDepFromProbability(x,y);
                                  if(d == 0)
                                  {
                                    if(src.getId() < dst.getId())
                                    {
                                        //heuristic. If we can not resolve.
                                        // can we use the heuristic that lower numberd state will on above??
                                        //d = 2;
					// No!! think about more intelligent ways..
                                    }
                                  }
                                  depMatrix.addDependency(i, j, d);
                                }
                                else
                                {
					unsigned int d = calculateDependencyBasedOnLoop(LA,loopSet,src,i,dst,j,rsTable);
					//cout << "LOOP analysis returned: " << d << endl;
					if(d == 0)
					{
						//this means our loop based method could not resolve?
						//what is the problem?...fix that!!
						double x = mm->transitionProbability(src, dst);
						double y = mm->transitionProbability(dst, src);
   				                //cout << "PROBABILITY BASED analysis-2 between:"<< src.getId() << "-" << dst.getId() << endl;
                                                d = DependencyMatrix::translateDepFromProbability(x,y);
					}
                                        if(d == 0)
                                        {
                                          if(src.getId() < dst.getId())
                                          {
                                            //heuristic. If we can not resolve.
                                            // can we use the heuristic that lower numberd state will on above
                                            // d = 2;
					    // No!! think about more intelligent ways..
                                          }
                                        }
                                        depMatrix.addDependency(i, j, d);

                                }

                        } else {
                                depMatrix.addDependency(i, j, 0);
                        }
                }
        }

        return depMatrix;
}
DependencyMatrix AnalysisDriver::buildDepMatrixLocally(
		const ReducedStateVector &redVector)
{
	size_t matrixSize = redVector.vec.size();
	DependencyMatrix depMatrix(matrixSize, mpiState);

	for (size_t i = 0; i < matrixSize; ++i) {
		for (size_t j = 0; j < matrixSize; ++j) {
			if (i != j) {
				State src = redVector.vec[i];
				State dst = redVector.vec[j];
				double x = mm->transitionProbability(src, dst);
				double y = mm->transitionProbability(dst, src);
				unsigned int d =
						DependencyMatrix::translateDepFromProbability(x,y);
				depMatrix.addDependency(i, j, d);

			} else {
				depMatrix.addDependency(i, j, 0);
			}
		}
	}

	return depMatrix;
}

unsigned int AnalysisDriver::calculateDependencyBasedOnLoop(loopAnalysis& LA,set<Edge>& loopSet,
		                                              State& src,size_t srcIndex, State& dst, size_t dstIndex,
							      RangeSetTable& rsTable)
{
   //cout << "LOOP BASED analysis" << endl;
   //cout << "LOOP BASED analysis between:"<< src.getId() << "-" << dst.getId() << endl;
   //cout << "called with src = " << src.getId() << " and dst = " << dst.getId() << endl;
   list<Edge> lexicographicOrderedLoops = loopAnalysis::getLexicographicOrderdedLoop(loopSet);
   //set<size_t>::iterator loopSetStart = loopSet.begin(), loopSetEnd = loopSet.end();
   list <Edge>::iterator loopSetStart = lexicographicOrderedLoops.begin(), 
	                   loopSetEnd = lexicographicOrderedLoops.end();
   for(; loopSetStart != loopSetEnd; loopSetStart++)
   {
          //size_t thisLoop = (*loopSetStart);
          Edge backE = (*loopSetStart);
	  //Edge backE = LA.getBackEdge(thisLoop);
	  //cout << "Back edge for analysis is : " << backE.getSourceState().getId() << " - " << backE.getDestinationState().getId() <<  endl; 

	 RangeSet rs1 = rsTable.getRangeOfTasks(srcIndex);
	 RangeSet rs2 = rsTable.getRangeOfTasks(dstIndex);
	 unsigned int lowestProcForSrc = rs1.lowestItem();
	 unsigned int lowestProcForDst = rs2.lowestItem();
	 
	 bool found = false;
         //Edge e = LA.getCharacteristicEdge(thisLoop,found);
         Edge e = LA.getCharacteristicEdge(backE,found);
	 if(found)
	 {
            //charecteristic edge for this loop was found....
           //cout << "Char edge is: " << e.getSourceState().getId() << " - " << e.getDestinationState().getId() << endl;
	   //TODO: FIXME: We have just considered one process. Do we need to consider all process?
	   //unsigned long srcIterations = mm->getIterationCountForEdgeAndProcess(e,lowestProcForSrc);
	   //unsigned long dstIterations = mm->getIterationCountForEdgeAndProcess(e,lowestProcForDst);
	   unsigned long srcIterations = (LA.graph_adj)->getIterationCountForEdgeAndProcess(e,lowestProcForSrc);
	   unsigned long dstIterations = (LA.graph_adj)->getIterationCountForEdgeAndProcess(e,lowestProcForDst);
	   unsigned int dependency = 0;
	   if(srcIterations > dstIterations)
	   {
		 //src has progressed more . so dependency is src->dst
               dependency = 1;
		//TODO: FIXME: We have just considered one loop from a set of loop: we need to consider ALL loops
         	return dependency;
	   }
	   else if(srcIterations < dstIterations)
	   {
		 //dst has progressed more ...  so dependency is dst->src
		 dependency = 2;
		 //TODO: FIXME: We have just considered one loop from a set of loop: we need to consider ALL loops
         	return dependency;
	   }
	   else
	   { 
		 dependency = 0;
		 continue;
	   }
	 }
	 else
	 {
		 // no charecteristic edge for this loop
		 // TODO: solve system of Diaphontine equations to get iteration counts..
		 // TODO: get all the loops for thsi backedge backE and then .. 
		 //LA.resolveLoopIterationsFromLinearEquations(thisLoop,loopSet,lowestProcForSrc,lowestProcForDst);
		 //unsigned long srcIterations = LA.getLoopIterations(thisLoop,lowestProcForSrc);
		 //unsigned long dstIterations = LA.getLoopIterations(thisLoop,lowestProcForDst);

	 }

  }
  //If we are still here, this means there was a "tie" interms of iterations. We could not resolve what depends on what...
  // both states are stuck in the same iteration...
  // Now decide dependency interms of hops from loop entry point.

  //loopSetStart = loopSet.begin();
  //loopSetEnd = loopSet.end();
  //cout << "DISTANCE BASED analysis between:"<< src.getId() << "-" << dst.getId() << endl;
  loopSetStart = lexicographicOrderedLoops.begin();
  loopSetEnd = lexicographicOrderedLoops.end();
   for(; loopSetStart != loopSetEnd; loopSetStart++)
   {
          //size_t thisLoop = (*loopSetStart);
          Edge backE = (*loopSetStart);
	  unsigned int dependency = 0;
          int srcHopCount = LA.getHopCountFromLoopEntry(backE, src);
          int dstHopCount = LA.getHopCountFromLoopEntry(backE, dst);
	  if(srcHopCount == -1 || dstHopCount == -1)
	  {
		  continue;
	  }
	  if(srcHopCount > dstHopCount)
	  {
		  dependency = 1;
		  return dependency;
	  }
	  else if(srcHopCount < dstHopCount)
	  {
		  dependency = 2;
		  return dependency;
	  }

   }
  return 0;
}



void AnalysisDriver::printLeastProgressedTasks(const RangeSetTable &rsTable,
		const set<size_t> taskStates, const ReducedStateVector &redVector)
{
	if (mpiState.isRoot())
	{
		cout << "Number of states with LP-tasks: " << taskStates.size() << endl;

		set<size_t>::const_iterator it;
		for (it = taskStates.begin(); it != taskStates.end(); ++it) {
			State s = redVector.vec[*it];
			RangeSet rs = rsTable.getRangeOfTasks(*it);
			cout << "STATE " << *it << ", tasks: " << rs.toString() << endl;
		}

		// Print state names
		string name;
		cout << "States: " << endl;
		cout << "-------" << endl;
		for (size_t i=0; i < redVector.vec.size(); ++i) {
			State s = redVector.vec[i];
			factory->findAndGetName(name, s);
			cout << i << ": " << name << endl;
		}

		// Print location of tasks
		cout << "Task locations: " << endl;
		cout << "---------------" << endl;
		for (size_t i=0; i < redVector.vec.size(); ++i) {
			State s = redVector.vec[i];
			RangeSet r = rsTable.getRangeOfTasks(i);
			cout << i << ": " << r.toString() << endl;
		}
	}
}

void stateIsComputationCode(const string &state)
{


}

void AnalysisDriver::dumpOutputForGUI(const DependencyMatrix &matrix,
		const ReducedStateVector &redVector,
		const RangeSetTable &rsTable)
{

	bool doNotDump = false;
        bool r = false;
        r = AUTConfig::getBoolParameter("AUT_DO_NOT_DUMP", doNotDump);
        if(doNotDump)
        {
             // to resolve problem in BGQ - in BGQ we can not execute shell command to resolve functionname and address
             return;

        }
	bool usedcallpath = AUTConfig::getBoolParameter("AUT_USE_CALL_PATH", usedcallpath);
	if(usedcallpath)
	{
		cout<< "Stack created using callpath, support for filename and line number will be provided later" << endl;
		return;
	}
#if STATE_TRACKER_DEBUG
	cout << "Writing dump file..." << endl;
#endif

	string fileData("");

	fileData += "#START_DEPENDENCY_GRAPH\n";
	fileData += matrix.toCSVFormat();
	fileData += "#END_DEPENDENCY_GRAPH\n";

	fileData += "#START_STATES\n";
	string name;
	for (size_t i=0; i < redVector.vec.size(); ++i) {
		State s = redVector.vec[i];
		factory->findAndGetName(name, s);

		// eliminate first '|'
		name.erase(0,1);

		// get state id
		char stateId[5];
		itoa((int)i, stateId);

		fileData += string(stateId) + string(":") + name + string("\n");
	}
	fileData += "#END_STATES\n";

	fileData += "#START_TASK_LOC\n";
	for (size_t i=0; i < redVector.vec.size(); ++i) {
		State s = redVector.vec[i];
		RangeSet r = rsTable.getRangeOfTasks(i);
                //cout << "Original state was: " << s.getId() << "\n";
		// get state id
		char stateId[5];
		itoa((int)i, stateId);

		// get range set
		string rSet(r.toString());
		rSet.erase(0,1);
		rSet.erase(rSet.length()-1, 1);
		
		// get number of tasks
		unsigned int n = r.getNumberOfTasks();
		char tasks[128];
		sprintf(tasks, "%d", n);

		fileData += string(stateId) + string(":") + rSet + string(":") + string(tasks) + string("\n");
	}
	fileData += "#END_TASK_LOC\n";
	fileData += "#START_SOURCE_CODE_LINES\n";
	name = "";
	for (size_t i=0; i < redVector.vec.size(); ++i) {
		State s = redVector.vec[i];
		factory->findAndGetName(name, s);
		// eliminate first '|'
		name.erase(0,1);
		vector<string> tokens;
		Tokenize(name, tokens, "|");
		string setOfLines("");
		for (size_t j=0; j < tokens.size(); ++j) {
			//cout << tokens[j] << endl;
			FileAndFunction f = Backtrace::findFileAndFunctionFromObject(tokens[j]);
			if(f.fileNameAndLine.empty())
			{
				continue;
			}
			string line = f.fileNameAndLine.substr(0, f.fileNameAndLine.size()-1);
			setOfLines += line + "|" + f.functionName;
			if (f.fromTool)
				setOfLines += "|*\n";
			else
				setOfLines += "\n";
		}

		// get state id
		char stateId[5];
		itoa((int)i, stateId);

		fileData += string(stateId) + string(",") + setOfLines;
	}
	fileData += "#END_SOURCE_CODE_LINES\n";

	fileData += "#START_STATE_TYPES\n";
        name = "";
        for (size_t i=0; i < redVector.vec.size(); ++i) {
                State s = redVector.vec[i];
                factory->findAndGetName(name, s);

                name.erase(0,1);
                vector<string> tokens;
                Tokenize(name, tokens, "|");
                bool compState = false;

                for (size_t j=0; j < tokens.size(); ++j) {
                        FileAndFunction f = Backtrace::findFileAndFunctionFromObject(tokens[j]);
			if(f.functionName.empty())
			{
			      continue;
			}

			if (f.functionName.find("transitionAfterMPICall") != string::npos) {
				compState = true;
				break;
			}
                }

                char stateId[5];
                itoa((int)i, stateId);

                fileData += string(stateId) + string(":");
		if (compState)
			fileData += "COMPUTATION_CODE\n";
		else
			fileData += "COMMUNICATION_CODE\n";

        }
        fileData += "#END_STATE_TYPES\n";

	string fileName = writeFile(fileData, "dump");
        cout << "Name of the output file: " << fileName << endl;

	//printf("Pointer of _r_debug.r_map: %p\n",_r_debug.r_map);
        //struct link_map *map = _r_debug.r_map;
        //while(map)
        //{
        //        printf("Name: '%s' l_addr: %lx l_ld: %lx\n", map->l_name, map->l_addr, map->l_ld);
        //        map = map->l_next;
        //}

}
