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
   * Created on: Nov 2, 2013
   * Author: Subrata Mitra
   * Contact: mitra4@purdue.edu
   */

#ifndef GRAPH_DATA_MODEL_H_
#define GRAPH_DATA_MODEL_H_


//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "debugging.h"
#include "model_components.h"

#include <pthread.h>

#include <map>
#include <set>
#include <utility>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <sstream>

#include <iostream>
#include <vector>
#include <cstdio>
#include <fstream>
#include <functional>
using namespace std;


class Node:public State
{
	public:
	string label;
	bool isMultiColored;
	Node(unsigned int i);
	Node(unsigned int i,string l,bool isMulti);
	Node();
	Node(const Node&);
	Node(const State&);
        bool operator == (const Node &n) const;
        bool operator != (const Node &n) const;
        Node& operator=(const Node & n);
        bool operator < (const Node &node) const;

};

class Graph_Edges
{
  public:
        map<string,Node> node_map;
        map<string,EdgeInfoContainer> edge_map;
        Graph_Edges(){};
        void add_edges(EdgeInfoContainer& eif);
        //void print(bool onlyDiff = false);
};

class Graph_Adjacency
{ 
  private:
  map<Node,bool> boundaryNodes; //where different tasks are waiting, we do not want to compress these nodes..
public:
  map<Node,map<EdgeInfoContainer,bool> > nodeSourceMap;
  map<Node,map<Node,bool> > nodeDestMap;
  map<State,map<State,bool> > adjacencyListSimple;

  void addEdge(const State&, const State&, const EdgeAnnotation&);
  void addBoundaryNode(const State&);
  EdgeAnnotation getEdgeAnnotation(const State &src, const State &dst) const;
  unsigned long getIterationCountForEdgeAndProcess(Edge& edg, unsigned int proc);
  Node followChainInGraph( Node currentNode, map<Node,bool>* visited, map<Node,bool>& allreadyTraversed, EdgeAnnotation& );
  void compressGraph();
  void eliminateSmallLoops(Node currentnode,map<Node,bool>* visited);
  void removeFromNodeDestMap(Node& , Node& );
  void addToNodeDestMap(Node& , Node& );

  void DFSUtil(Node node,map<Node,bool>* visited,bool followChain);
  void DFS(Node startNode,bool followChain); //bool flag to decide whether we are in chain compression mode or loop elimination mode
  
  Graph_Edges* convert_To_Graph_Edge_Format();

};

#endif /*DATA_MODEL_H_*/
