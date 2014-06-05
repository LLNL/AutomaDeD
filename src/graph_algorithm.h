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
 * Author: Subrata Mitra
 * mitra4@purdue.edu
 Strongly Connected Component
 * Algorithm : Tarjan's Algorithm
 * Order : O( V+E )
 */

#ifndef GRAPH_LOOP
#define GRAPH_LOOP

#include<stdio.h>
#include<string.h>
#include<vector>
#include<list>
#include<map>
#include<set>
#include<stack>
#include<algorithm>
#include <queue>
#include<iostream>
#include <boost/functional/hash.hpp>
#include "model_components.h"
#include "graph_model.h"
using namespace std;

//typedef State Node;
#if 0
std::size_t hash_value(const State & n)
{
	boost::hash<unsigned int> hasher;
	return hasher(n.getId());
}
#endif

class loopAnalysis
{
private:
static list<State> path; // stack of nodes (vertices) in current path
static map<State,bool> blocked; //vertex: blocked from search?
static map<State,list<State> > B; // graph portions that yield no elementary circuit
static list< list<State>* >* result ;  // list to accumulate the circuits found
static map<State,set<size_t>* > nodeLoopMap;
static map<size_t,set<Edge>* > loopEdgeMap;
static map<Edge,set<size_t>* > edgeLoopMap;
//static map<size_t,Edge> loopCharecteristicEdgeMap;
static map<Edge,Edge> backEdgeCharecteristicEdgeMap;
static map<size_t,State> loopEntryNodeMap;
static map<size_t,Edge> backEdgeMap;
static map<size_t, map<unsigned int,unsigned long> > resolvedLoopIterationMap;
static State blockAndGetMinimumNode(map<State, map<State,bool> >& component);
static void  unblock(State& thisnode);
static bool circuit(State& thisnode, State& startnode, map<State,map<State,bool> >& component);

static State getListMinimum(list<State> * lst);
static list<State>* getMinSCC(list< list<State>* >* sccList);
static bool checkAndPopulateForEntryNode(State& src,size_t& loopHash);
static void populateCharacteristicEdgeInformation();
static set<State> getSetOfHeaders(set<Edge>& backEdgeSet);
static void populateCommonHeaderBackEdgeMap(set<Edge>&,set<State>&,map<State,list<Edge> >&);
public:
static Graph_Adjacency* graph_adj; 
loopAnalysis(Graph_Adjacency* ga);
static void setGraph(Graph_Adjacency* ga){ graph_adj = ga; } ;
static list< list<State>* >*  findAllLoops(map<State, map<State,bool> >& sourceNodeMap);
static list< list<State>* >* findAllLoopsWithBoost(map<State, map<State,bool> >& sourceNodeMap);

static void populateAllLoopInformation();
static void populateBackEdgeInfo(size_t& theloop, Edge& backedge);
static void getCharacteristicEdges(size_t& loop1, size_t& loop2, Edge & e1, Edge & e2);
//static set<size_t> getCommonLoops(State & A, State & B);
static map<State, list<Edge> > getCommonMergedLoopsBackEdges(State & A, State & B);
static set<Edge> getBackEdgesForCommonLoops(State & A, State & B);
static set<Edge> getCommonBackEdges(set<size_t>* loopSet);
static set<size_t>* getListOfLoops(State & s);
static Edge getCharacteristicEdge(size_t& theloop,bool& found);
static Edge getCharacteristicEdge(Edge& backE, bool& found);
static Edge getBackEdge(size_t& theloop);
static State getLoopEntryState(size_t& theloop);
static int getHopCountFromLoopEntry(Edge& , State&);
static bool loopLexicographicComparator(Edge&, Edge&);
static list<Edge> getLexicographicOrderdedLoop(set<Edge>& );
static size_t getLengthOfSmallestLoop(set<size_t>* loopSet);
//static void resolveLoopIterationsFromLinearEquations(size_t& ,set<size_t>& ,unsigned int ,unsigned int);
static unsigned long getLoopIterations(size_t& loop, unsigned int proc);
};

#endif
