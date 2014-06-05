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
 * test_MarkovModel.C
 *
 *  Created on: Jan 19, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 */

#include "test_MarkovModel.h"
#include "markov_model.h"
#include "model_components.h"
#include "assert_functions.h"
#include "timer.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <set>
#include <map>
#include <boost/graph/adjacency_list.hpp>

using namespace std;
using namespace boost;

void Test_MarkovModel::canUpdateProbabilityMatrix()
{
	ProbabilityMatrix<State> matrix;

	StateFactory factory;
	size_t numStates = 100;
	char buffer[50];
	for (size_t i=0; i < numStates; i++) {
		sprintf(buffer, "%d", (int)i);
		string name(buffer);
		State newState = factory.createState(name);
		State lastState(newState);
		EdgeAnnotation ea (1);
		if (i > 0) {
			matrix.updateMatrix(lastState, newState,ea);
		}
		lastState = newState;
	}

	bool cond1 = matrix.transitions.size() > 0;
	bool cond2 = matrix.transitions.size() == numStates-1;

	ASSERT_TRUE((cond1 && cond2),
				("Can update (Markov-Model) probability matrix"));
}

void Test_MarkovModel::canIterateOverProbabilityMatrix()
{
	ProbabilityMatrix<State> matrix;
	StateFactory factory;
	size_t numStates = 20;
	char buffer[50];
	EdgeAnnotation ea (1);

	// Create States and Edges
	State s1 = factory.createState("1");
	State s2 = factory.createState("2");
	State s3 = factory.createState("3");
	State s4 = factory.createState("4");
	State s5 = factory.createState("5");
	State s6 = factory.createState("6");
	State s7 = factory.createState("7");
	State s8 = factory.createState("8");
        matrix.updateMatrix(s1, s2,ea);
        matrix.updateMatrix(s2, s3,ea);
        matrix.updateMatrix(s2, s4,ea);
        matrix.updateMatrix(s3, s2,ea);
        matrix.updateMatrix(s3, s5,ea);
        matrix.updateMatrix(s5, s6,ea);
        matrix.updateMatrix(s6, s7,ea);
        matrix.updateMatrix(s6, s8,ea);

	// Create iterator
	ProbabilityMatrixIterator<State> it(&matrix);
	int numEdges = 0;
	for (it.firstTrans(); !it.isDone(); it.nextTrans()) {
		//pair<const State *, const State *> edge = it.currentTrans();
		EdgeInfo<State> edge = it.currentTrans();
		//cout << edge.first->getString() << " ==> "
		//		<< edge.second->getString() << endl;
		numEdges++;
	}

	bool cond1 = (numEdges == 8);
	ASSERT_TRUE(cond1, ("Can iterate over probability matrix"));
}

void Test_MarkovModel::canGetProbabilitiesFromMatrix()
{
	ProbabilityMatrix<State> matrix;

	StateFactory factory;
	string name;
        EdgeAnnotation ea (1);

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);

	// Fill probability matrix
	for (int i=0; i < 10; i++)
		matrix.updateMatrix(s1, s2,ea);
	for (int i=0; i < 5; i++)
		matrix.updateMatrix(s2, s3,ea);
	for (int i=0; i < 5; i++)
		matrix.updateMatrix(s3, s2,ea);
	for (int i=0; i < 10; i++)
		matrix.updateMatrix(s3, s1,ea);
	for (int i=0; i < 1; i++)
		matrix.updateMatrix(s3, s4,ea);

	double r1 = matrix.probability(s1, s2);
	//cout << "P(1,2)= " << r1 << endl;
	double r2 = matrix.probability(s2, s3);
	//cout << "P(2,3)= " << r2 << endl;
	double r3 = matrix.probability(s3, s2);
	//cout << "P(3,2)= " << r3 << endl;
	double r4 = matrix.probability(s3, s1);
	//cout << "P(3,1)= " << r4 << endl;
	double r5 = matrix.probability(s3, s4);
	//cout << "P(3,4)= " << r5 << endl;
	double r6 = matrix.probability(s4, s1);
	//cout << "P(4,1)= " << r6 << endl;
	double r7 = matrix.probability(s2, s4);
	//cout << "P(2,4)= " << r7 << endl;

	// Check test conditions
	bool cond1 = (r1 == 1);
	bool cond2 = (r2 == 1);
	bool cond3 = (r3 < 0.31251 && r3 > 0.31249);
	bool cond4 = (r4 < 0.6251 && r4 > 0.6249);
	bool cond5 = (r5 < 0.06251 && r5 > 0.06249);
	bool cond6 = (r6 == 0);
	bool cond7 = (r7 == 0);

	ASSERT_TRUE((cond1 && cond2 && cond3 && cond4 && cond5 && cond6 && cond7),
					("Probability matrix can get probability values"));
}

void Test_MarkovModel::canUpdateMarkovModel()
{
	MarkovModel<State> mm;
	StateFactory factory;
	string name;

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);

        EdgeAnnotation ea (1);
	mm.addEdge(s1, s2,ea);
	mm.addEdge(s2, s3,ea);
	// repeated edges
	mm.addEdge(s2, s3,ea);
	mm.addEdge(s2, s3,ea);

	//cout << "Num vertices: " << num_vertices(mm.graph) << endl;
	//cout << "Num edges: " << num_edges(mm.graph) << endl;

	ASSERT_TRUE((num_vertices(mm.graph)==3 && num_edges(mm.graph)==2) ,
						("Markov Model can be updated"));
}

MarkovModel<State> Test_MarkovModel::createSimpleMM()
{
	MarkovModel<State> mm;
	StateFactory factory;
	string name;
        EdgeAnnotation ea (1);

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);

	// Fill probability matrix
	for (int i=0; i < 10; i++)
		mm.addEdge(s1, s2,ea);
	for (int i=0; i < 5; i++)
		mm.addEdge(s2, s3,ea);
	for (int i=0; i < 5; i++)
		mm.addEdge(s3, s2,ea);
	for (int i=0; i < 10; i++)
		mm.addEdge(s3, s1,ea);
	for (int i=0; i < 1; i++)
		mm.addEdge(s3, s4,ea);

	return mm;
}

void Test_MarkovModel::canIterateOverEdgesOfMarkovModel()
{
	MarkovModel<State> mm = createSimpleMM();

	typedef boost::adjacency_list <
			boost::setS,
			boost::vecS,
			boost::directedS,
		    // All vertices are of type T pointer
			State
			> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;

	set<string> edges_set;

	boost::graph_traits<Graph>::edge_iterator ei, ei_end;
	for (tie(ei, ei_end) = edges(mm.graph); ei != ei_end; ++ei) {
		Vertex x = source(*ei, mm.graph);
		Vertex y = target(*ei, mm.graph);
		string edge = "(" + mm.graph[x].getString() + "-->" +
				mm.graph[y].getString() + ")";
		edges_set.insert(edge);
		//cout << edge << endl;
	}

	ASSERT_TRUE((edges_set.size() == 5),
			("Can iterate over edges of Markov-Model"));
}

void Test_MarkovModel::canDoDFSearchinMarkovModel()
{
	// --------- Create simple MM ----------------
	MarkovModel<State> mm;
	StateFactory factory;
	string name;
        EdgeAnnotation ea (1);

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);

	// Fill MM
	for (int i=0; i < 10; i++)
		mm.addEdge(s1, s2, ea);
	for (int i=0; i < 5; i++)
		mm.addEdge(s2, s3, ea);
	for (int i=0; i < 5; i++)
		mm.addEdge(s3, s2, ea);
	for (int i=0; i < 10; i++)
		mm.addEdge(s3, s1, ea);
	for (int i=0; i < 1; i++)
		mm.addEdge(s3, s4, ea);
	// -------------------------------------------

	typedef boost::adjacency_list <
			boost::setS,
			boost::vecS,
			boost::directedS,
		    // All vertices are of type T pointer
			State
			> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;

	map<State, Vertex >::const_iterator it = mm.states.find(s1);
	map<State, Vertex >::const_iterator jt = mm.states.find(s4);

	Vertex v = it->second;
	Vertex u = jt->second;
	mm.DFSearch(v, u);

	//cout << "Set size: " << mm.visitedVertices.size() << endl;
	ASSERT_TRUE((mm.visitedVertices.size() == 0),
				("Can do DFSearch in Markov-Model"));
}

void Test_MarkovModel::canFindAllPathsInMarkovModel()
{
	// --------- Create complex MM ----------------
	MarkovModel<State> mm;
	StateFactory factory;
	string name;
	EdgeAnnotation ea (1);

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);
	name = "Seis";
	State s6 = factory.createState(name);
	name = "Siete";
	State s7 = factory.createState(name);
	name = "Ocho";
	State s8 = factory.createState(name);

	// Fill MM
	mm.addEdge(s1, s2,ea);
	mm.addEdge(s2, s3,ea);
	mm.addEdge(s3, s4,ea);
	mm.addEdge(s4, s2,ea);
	mm.addEdge(s2, s5,ea);
	mm.addEdge(s5, s6,ea);
	mm.addEdge(s6, s5,ea);
	mm.addEdge(s6, s4,ea);
	mm.addEdge(s4, s7,ea);
	mm.addEdge(s6, s7,ea);
	mm.addEdge(s7, s2,ea);
	mm.addEdge(s7, s8,ea);
	// -------------------------------------------

	typedef boost::adjacency_list <
			boost::setS,
			boost::vecS,
			boost::directedS,
		    // All vertices are of type T pointer
			State
			> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;

	map<State, Vertex >::const_iterator it = mm.states.find(s1);
	map<State, Vertex >::const_iterator jt = mm.states.find(s7);
	Vertex v = it->second;
	Vertex u = jt->second;
	//mm.findPaths(v, u);
	mm.DFSearch(v, u);
	//cout << "num paths: " << mm.paths.size() << endl;
	ASSERT_TRUE((mm.paths.size() == 3),
					("Can find all paths in Markov Model"));
}

void Test_MarkovModel::canCalculateLargestProbability()
{
	// --------- Create complex MM ----------------
	MarkovModel<State> mm;
	StateFactory factory;
	string name;
        EdgeAnnotation ea (1);

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);
	name = "Seis";
	State s6 = factory.createState(name);
	name = "Siete";
	State s7 = factory.createState(name);
	name = "Ocho";
	State s8 = factory.createState(name);
	name = "Nueve";
	State s9 = factory.createState(name);

	// Fill MM
	mm.addEdge(s1, s2,ea);
	mm.addEdge(s2, s3,ea);
	mm.addEdge(s3, s4,ea);
	mm.addEdge(s4, s2,ea);
	mm.addEdge(s2, s5,ea);
	mm.addEdge(s5, s6,ea);
	mm.addEdge(s6, s5,ea);
	mm.addEdge(s6, s4,ea);
	mm.addEdge(s4, s7,ea);
	mm.addEdge(s6, s7,ea);
	mm.addEdge(s7, s2,ea);
	mm.addEdge(s7, s8,ea);
	// -------------------------------------------

	bool cond1 = (mm.largestPathProbability(s1, s7) == 0.25);
	bool cond2 = (mm.largestPathProbability(s7, s1) == 0);
	bool cond3 = (mm.largestPathProbability(s7, s7) == 0);
	bool cond4 = (mm.largestPathProbability(s3, s4) == 1);
	bool cond5 = (mm.largestPathProbability(s4, s7) == 0.5);
	bool cond6 = (mm.largestPathProbability(s5, s6) == 1);
	bool cond7 = (mm.largestPathProbability(s6, s6) == 0);
	bool cond8 = (mm.largestPathProbability(s1, s9) == 0);

	//cout << cond1 << " " << cond2 << " " << cond3 << " " <<
	//		cond4 << " " << cond5 << " " << cond6 << endl;

	ASSERT_TRUE((cond1 && cond2 && cond3 && cond4 && cond5
			&& cond6 && cond7 && cond8),
					("Can calculate largest probability"));
}

void Test_MarkovModel::canCreateTransitiveClosure()
{
	// --------- Create complex MM ----------------
	MarkovModel<State> mm;
	StateFactory factory;
	string name;
        EdgeAnnotation ea (1);

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);
	name = "Seis";
	State s6 = factory.createState(name);
	name = "Siete";
	State s7 = factory.createState(name);
	name = "Ocho";
	State s8 = factory.createState(name);
	name = "Nueve";
	State s9 = factory.createState(name);

	// Fill MM
	mm.addEdge(s1, s2,ea);
	mm.addEdge(s2, s3,ea);
	mm.addEdge(s3, s4,ea);
	mm.addEdge(s4, s2,ea);
	mm.addEdge(s2, s5,ea);
	mm.addEdge(s5, s6,ea);
	mm.addEdge(s6, s5,ea);
	mm.addEdge(s6, s4,ea);
	mm.addEdge(s4, s7,ea);
	mm.addEdge(s6, s7,ea);
	mm.addEdge(s7, s8,ea);
	// -------------------------------------------

	typedef boost::adjacency_list <
			boost::setS,
			boost::vecS,
			boost::directedS,
		    // All vertices are of type T pointer
			State
			> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;

	Vertex v,u;

	v = mm.states.find(s7)->second;
	u = mm.states.find(s5)->second;
	bool cond1 = (mm.sourceCanReachDestination(v, u) == false);

	v = mm.states.find(s1)->second;
	u = mm.states.find(s5)->second;
	bool cond2 = (mm.sourceCanReachDestination(v, u) == true);

	v = mm.states.find(s3)->second;
	u = mm.states.find(s6)->second;
	bool cond3 = (mm.sourceCanReachDestination(v, u) == true);

	v = mm.states.find(s8)->second;
	u = mm.states.find(s7)->second;
	bool cond4 = (mm.sourceCanReachDestination(v, u) == false);

	v = mm.states.find(s7)->second;
	u = mm.states.find(s8)->second;
	bool cond5 = (mm.sourceCanReachDestination(v, u) == true);

	//cout << cond1 << " " << cond2 << " " << cond3 << " " <<
			//cond4 << " " << cond5 << endl;

	ASSERT_TRUE((cond1 && cond2 && cond3 && cond4 && cond5),
						("Can create transitive closure"));
}

void Test_MarkovModel::performanceWhenUsingClosureIsBetter()
{
	// --------- Create complex MM ----------------
	StateFactory factory;
	string name;

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);
	name = "Seis";
	State s6 = factory.createState(name);
	name = "Siete";
	State s7 = factory.createState(name);
	name = "Ocho";
	State s8 = factory.createState(name);
	name = "Nueve";
	State s9 = factory.createState(name);
	name = "Dies";
	State s10 = factory.createState(name);
	name = "Once";
	State s11 = factory.createState(name);
	name = "Doce";
	State s12 = factory.createState(name);
	name = "Trece";
	State s13 = factory.createState(name);
	name = "Catorce";
	State s14 = factory.createState(name);
	name = "Quince";
	State s15 = factory.createState(name);
	name = "Diesiseis";
	State s16 = factory.createState(name);
	name = "Diesiciete";
	State s17 = factory.createState(name);
	name = "Diesiocho";
	State s18 = factory.createState(name);
	name = "Diesinueve";
	State s19 = factory.createState(name);

	MultiTimer timer;

       EdgeAnnotation ea (1);

        // Markov Model with transitive closure
        timer.start(0);
        MarkovModel<State> mm(true);
        mm.addEdge(s1, s2,ea);
        mm.addEdge(s2, s3,ea);
        mm.addEdge(s3, s5,ea);
        mm.addEdge(s5, s6,ea);
        mm.addEdge(s6, s3,ea);
        mm.addEdge(s6, s9,ea);
        mm.addEdge(s2, s4,ea);
        mm.addEdge(s4, s7,ea);
        mm.addEdge(s7, s8,ea);
        mm.addEdge(s8, s7,ea);
        mm.addEdge(s8, s9,ea);
        mm.addEdge(s9, s10,ea);
        mm.addEdge(s10, s11,ea);
        mm.addEdge(s10, s12,ea);
        mm.addEdge(s10, s13,ea);
        mm.addEdge(s11, s14,ea);
        mm.addEdge(s14, s15,ea);
        mm.addEdge(s15, s18,ea);
        mm.addEdge(s12, s16,ea);
        mm.addEdge(s16, s18,ea);
        mm.addEdge(s13, s17,ea);
        mm.addEdge(s17, s18,ea);
        mm.addEdge(s18, s19,ea);
	// -------------------------------------------

	for (int j=0; j < 50; j++) {
		for (int i=0; i < 50; i++) {
			mm.largestPathProbability(s1, s7);
			mm.largestPathProbability(s3, s6);
			mm.largestPathProbability(s4, s9);
			mm.largestPathProbability(s5, s7); // unreachable
			mm.largestPathProbability(s4, s6); // unreachable
			mm.largestPathProbability(s6, s8); // unreachable
			mm.largestPathProbability(s3, s8); // unreachable
			mm.largestPathProbability(s10, s1); // unreachable
			mm.largestPathProbability(s11, s17); // unreachable
			mm.largestPathProbability(s13, s15); // unreachable
			mm.largestPathProbability(s10, s2); // unreachable
		}
	}
	double t1 = timer.stop(0, "With Closure");

	cout << "Time with transitive closure: " << t1 << endl;

	// Markov Model without transitive closure
	timer.start(0);
	MarkovModel<State> mm2(false);
	mm2.addEdge(s1, s2,ea);
        mm2.addEdge(s2, s3,ea);
        mm2.addEdge(s3, s5,ea);
        mm2.addEdge(s5, s6,ea);
        mm2.addEdge(s6, s3,ea);
        mm2.addEdge(s6, s9,ea);
        mm2.addEdge(s2, s4,ea);
        mm2.addEdge(s4, s7,ea);
        mm2.addEdge(s7, s8,ea);
        mm2.addEdge(s8, s7,ea);
        mm2.addEdge(s8, s9,ea);
        mm2.addEdge(s9, s10,ea);
        mm.addEdge(s10, s11,ea);
        mm.addEdge(s10, s12,ea);
        mm.addEdge(s10, s13,ea);
        mm.addEdge(s11, s14,ea);
        mm.addEdge(s14, s15,ea);
        mm.addEdge(s15, s18,ea);
        mm.addEdge(s12, s16,ea);
        mm.addEdge(s16, s18,ea);
        mm.addEdge(s13, s17,ea);
        mm.addEdge(s17, s18,ea);
        mm.addEdge(s18, s19,ea);
	// -------------------------------------------

	for (int j=0; j < 50; j++) {
		for (int i=0; i < 50; i++) {
			mm2.largestPathProbability(s1, s7);
			mm2.largestPathProbability(s3, s6);
			mm2.largestPathProbability(s4, s9);
			mm2.largestPathProbability(s5, s7); // unreachable
			mm2.largestPathProbability(s4, s6); // unreachable
			mm2.largestPathProbability(s6, s8); // unreachable
			mm2.largestPathProbability(s3, s8); // unreachable
			mm2.largestPathProbability(s10, s1); // unreachable
			mm.largestPathProbability(s11, s17); // unreachable
			mm.largestPathProbability(s13, s15); // unreachable
			mm.largestPathProbability(s10, s2); // unreachable
		}
	}
	double t2 = timer.stop(0, "Without Closure");

	cout << "Time without transitive closure: " << t2 << endl;

	cout << "Improvement of " << (t2-t1)/t2 * 100.0 << "%" << endl;

	ASSERT_TRUE((t2 > t1),
			("Performance when using transitive closure is better"));
}

void Test_MarkovModel::canSaveLastState()
{
	StateFactory factory;
	string name;

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);

	MarkovModel<State> mm;
        EdgeAnnotation ea (1);
        mm.addEdge(s1, s2,ea);
        mm.addEdge(s2, s3,ea);
               bool cond1 = mm.getLastState() == s3;

        mm.addEdge(s3, s4,ea);
        bool cond2 = mm.getLastState() == s4;

        mm.addEdge(s4, s1,ea);
        bool cond3 = mm.getLastState() == s1;

        mm.addEdge(s2, s4,ea);
        bool cond4 = mm.getLastState() == s4;

        //cout << cond1 << " " << cond2 << " " << cond3 << " " << cond4 << endl;
        ASSERT_TRUE((cond1 && cond2 && cond3 && cond4),
                                                        ("Can save last state"));
}

typedef struct {
	MarkovModel<State> model;
	double result;
} ThreadData;

void* sample_thread(void *parameters)
{
	StateFactory factory;
	string name;
	ThreadData *td = static_cast< ThreadData * >(parameters);
	//MarkovModel<State> *mm =
			//static_cast< MarkovModel< State > * >(parameters);

	MarkovModel<State> mm = td->model;

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);
	name = "Seis";
	State s6 = factory.createState(name);
	name = "Siete";
	State s7 = factory.createState(name);
	name = "Ocho";
	State s8 = factory.createState(name);

        EdgeAnnotation ea (1);

        // Fill MM
        for (int i=0; i < 100; i++) {
                mm.addEdge(s1, s2,ea);
                mm.addEdge(s2, s3,ea);
                mm.addEdge(s3, s4,ea);
                mm.addEdge(s4, s2,ea);
                mm.addEdge(s2, s5,ea);
                mm.addEdge(s5, s6,ea);
                mm.addEdge(s6, s5,ea);
                mm.addEdge(s6, s4,ea);
                mm.addEdge(s4, s7,ea);
                mm.addEdge(s6, s7,ea);
                mm.addEdge(s7, s2,ea);
                mm.addEdge(s7, s8,ea);
        }

	double p = 0;
	for (int i=0; i < 100; i++) {
		p += mm.largestPathProbability(s3, s3);
		p += mm.largestPathProbability(s1, s7);
		p += mm.largestPathProbability(s7, s1);
		p += mm.largestPathProbability(s3, s5);
		p += mm.largestPathProbability(s6, s2);
		p += mm.largestPathProbability(s5, s4);
		p += mm.largestPathProbability(s8, s7);
		p += mm.largestPathProbability(s7, s8);
		p += mm.largestPathProbability(s1, s8);
		p += mm.largestPathProbability(s4, s3);
	}

	// Save result
	td->result = p;

	pthread_exit(NULL);
}

void Test_MarkovModel::multipleThreadsCanUpdateMarkovModel()
{
	//spinHere();
	MarkovModel<State> mm;

	size_t numThreads = 64;
	pthread_t thread[numThreads];

	ThreadData td[numThreads];

	// Create threads
	for (size_t i=0; i < numThreads; i++) {
		int ret = pthread_create(&(thread[i]), NULL, sample_thread,
				static_cast<void *>(&(td[i])));
		if (ret != 0)
			cout << "Error creating thread!" << endl;
	}

	// Wait for threads
	for (size_t i=0; i < numThreads; i++)
		pthread_join(thread[i], NULL);

	// All threads should finish with the same value
	bool allEqual = true;
	double val = td[0].result;
	for (size_t i=1; i < numThreads; i++) {
		double tmp = td[i].result;
		if (tmp != val) {
			allEqual = false;
			break;
		}
		val = tmp;
		//cout << "Res: " << td[i].result << endl;
	}

	ASSERT_TRUE(allEqual, ("Multiple threads can update Markov Model"));
}

void Test_MarkovModel::canCalculateTransitionProbability()
{
	// --------- Create complex MM ----------------
	MarkovModel<State> mm;
	StateFactory factory;
	string name;

	// Create temporal states
	name = "Uno";
	State s1 = factory.createState(name);
	name = "Dos";
	State s2 = factory.createState(name);
	name = "Tres";
	State s3 = factory.createState(name);
	name = "Cuatro";
	State s4 = factory.createState(name);
	name = "Cinco";
	State s5 = factory.createState(name);
	name = "Seis";
	State s6 = factory.createState(name);
	name = "Siete";
	State s7 = factory.createState(name);
	name = "Ocho";
	State s8 = factory.createState(name);

	 EdgeAnnotation ea (1);
        // Fill MM
        mm.addEdge(s1, s2,ea);
        mm.addEdge(s2, s3,ea);
        mm.addEdge(s3, s4,ea);
        mm.addEdge(s4, s2,ea);
        mm.addEdge(s2, s5,ea);
        mm.addEdge(s5, s6,ea);
        mm.addEdge(s6, s5,ea);
        mm.addEdge(s6, s4,ea);
        mm.addEdge(s4, s7,ea);
        mm.addEdge(s6, s7,ea);
        mm.addEdge(s7, s8,ea);
	// -------------------------------------------

	bool cond1 = (mm.transitionProbability(s1, s7) == 0.5);
	bool cond2 = (mm.transitionProbability(s1, s8) == 0.5);
	bool cond3 = (mm.transitionProbability(s4, s5) == 0.25);
	bool cond4 = (mm.transitionProbability(s7, s8) == 1);

	// Now check that the trans. probability is bever > 1.0
	int numRuns = 100;
	srand(time(NULL));
	bool probWithinRange = true;
	for (int i=0; i < numRuns; ++i) {
		int index_1 = rand() % 8; // from 0 - 7
		int index_2 = rand() % 8; // from 0 - 7

		State src, dst;

		if (index_1 == 0) src = s1;
		if (index_1 == 1) src = s2;
		if (index_1 == 2) src = s3;
		if (index_1 == 3) src = s4;
		if (index_1 == 4) src = s5;
		if (index_1 == 5) src = s6;
		if (index_1 == 6) src = s7;
		if (index_1 == 7) src = s8;


		if (index_2 == 0) dst = s1;
		if (index_2 == 1) dst = s2;
		if (index_2 == 2) dst = s3;
		if (index_2 == 3) dst = s4;
		if (index_2 == 4) dst = s5;
		if (index_2 == 5) dst = s6;
		if (index_2 == 6) dst = s7;
		if (index_2 == 7) dst = s8;

		double p = mm.transitionProbability(src, dst);
		//cout << "P(" << index_1+1 << "," << index_2+1 << "): " << p << endl;
		if (p < 0 || p > 1) {
			probWithinRange = false;
			break;
		}
	}

	ASSERT_TRUE((cond1 && cond2 && cond3 && cond4 && probWithinRange),
								("Can calculate transition probability"));
}
