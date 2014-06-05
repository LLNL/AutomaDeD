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
     *    Author: Subrata Mitra
     *     *    Contact: mitra4@purdue.edu
     */

#include <boost/graph/directed_graph.hpp> 
#include <iostream>
#include <map>
#include <vector>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>


#include "graph_model.h"

using namespace std;


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}



Node::Node(unsigned int i):State(i)
{
		label = "";
		isMultiColored = false;

}
Node::Node(unsigned int i,string l,bool isMulti):State(i)
{
  label = l;
  isMultiColored = isMulti;
}
Node::Node()
	{
		label = "";
		isMultiColored = false;

	}

Node::Node( const Node& other ):State(other) 
{
  label = other.label;
  isMultiColored = other.isMultiColored;
}
Node::Node( const State& other ):State(other)
{
	  label = "";
	    isMultiColored = false;
}
Node& Node::operator=(const Node & node)
 {
	 State::operator=(node);
         label = node.label;
         isMultiColored = node.isMultiColored;
          return *this;
}
bool Node::operator == (const Node &node) const
{
	 bool ret = State::operator==(node);
	 return ret;
}

bool Node::operator != (const Node &node) const
{
          return !(this->operator ==(node));
}

bool Node::operator < (const Node &node) const
{
          bool ret = State::operator<(node);
          return ret;
}


Node doCompressionAndClean(map<Node,map<EdgeInfoContainer,bool> >& nodeMap, Node currentNode, 
		map<Node,bool>& allreadyTraversed,EdgeAnnotation & lastAnnotation)
{
   // clean the redundant edges after compression. this are intermediate nodes in a chain which we are compressing
   // only the first and last node of this chain will be reinserted in the graph [outside the initial call to followChainInGraph() ]
   map<Node,bool>::iterator travStart =  allreadyTraversed.begin();
   map<Node,bool>::iterator travEnd =  allreadyTraversed.end();
   for(; travStart != travEnd; travStart++)
   {
     Node traversedNode = (*travStart).first;
     map<Node,map<EdgeInfoContainer,bool> >:: iterator nodePos = nodeMap.find(traversedNode);
     //it will have only one outgoing edge, map's second will have only one entry...
     map<EdgeInfoContainer,bool> edgeMap = nodePos->second;
     map<EdgeInfoContainer,bool> :: iterator edgemap_iter = edgeMap.begin();
     EdgeInfoContainer einfo = edgemap_iter->first;
     lastAnnotation = einfo.annotation;
     nodeMap.erase(nodePos);
   }
   return currentNode;
}


Node Graph_Adjacency::followChainInGraph( Node currentNode, map<Node,bool>* visited, 
		map<Node,bool>& allreadyTraversed, EdgeAnnotation & lastAnnotation )
{
  map<Node,map<EdgeInfoContainer,bool> > :: iterator pos = nodeSourceMap.find(currentNode);
  if(pos == nodeSourceMap.end()) 
  {
    // this means this node is not a source ..there is not outgoing edge from this node..or previously visited .. end here..!
    return doCompressionAndClean(nodeSourceMap,currentNode,allreadyTraversed, lastAnnotation);
  }

  if(boundaryNodes.find(currentNode) != boundaryNodes.end())
  {
	  //BoundaryNodes: nodes where different tasks are waiting after a hang... we do not want to compress those and loose information!
	  return doCompressionAndClean(nodeSourceMap,currentNode,allreadyTraversed, lastAnnotation);
  }
  map<EdgeInfoContainer,bool> adjMap = (*pos).second;

  
  map<Node,map<Node,bool> > :: iterator destpos = nodeDestMap.find(currentNode);
  int dest_count = 0;
  if((destpos) != nodeDestMap.end())
  {
     map<Node,bool> destMap = (*destpos).second;

    //this number tells how many nodes has an edge to this node or this nodes serves as destination for how many nodes...
    dest_count = destMap.size();
  }
  
  //cout << "destination count of current node: " << currentNode.getId() << "  " << dest_count << endl;
  // we will compress only the nodes which has single incoming or outgoing edge
  
  if((adjMap.size() == 1) && (dest_count <= 1) )  // first node will have o(zero) as count of destination!!
  {
    // this node has only one going edge...follow to compress... 
     allreadyTraversed[currentNode] = true;
     //this map has only one element. No need to iterate..
     map<EdgeInfoContainer,bool> :: iterator mapStart = adjMap.begin();

       Node node = (*mapStart).first.theEdge.getDestinationState();

       //we should skip visited nodes
       //check if next neighbor is already visisted... then stop and return...
       if(visited->find(node) != visited->end())
       {
          return doCompressionAndClean(nodeSourceMap,node,allreadyTraversed,lastAnnotation);
       }

       //if everything is good go ahead in recursion and compress more...!
       return followChainInGraph( node, visited,allreadyTraversed,lastAnnotation);
  }
  else
  {
     //this node has multiple outgoing edge.. we should stop here..compression will end here...
    return doCompressionAndClean(nodeSourceMap,currentNode,allreadyTraversed,lastAnnotation);
  }
}


void Graph_Adjacency::addEdge(const State& src,const State& dst,const EdgeAnnotation& ea)
{
    EdgeInfoContainer eif(Edge(src,dst),ea);
    map<Node,map<EdgeInfoContainer,bool> >:: iterator pos = nodeSourceMap.find(src);
    if(pos == nodeSourceMap.end())
    {
       map<EdgeInfoContainer,bool> adjacentNodeMap;
       adjacentNodeMap[eif] = true;
       nodeSourceMap[Node(src)] = adjacentNodeMap;

       map<State,bool> simpleNbrMap;
       simpleNbrMap[dst] = true;
       adjacencyListSimple[src] = simpleNbrMap;
    }
    else
    {
      map<EdgeInfoContainer,bool> adjacentNodeMap = (*pos).second;
      // just to make everything clean...remove the old entry and repopulate with clean stuff...
      nodeSourceMap.erase(pos);

      adjacentNodeMap[eif] = true;
      nodeSourceMap[Node(src)] = adjacentNodeMap;

      map<State,bool> simpleNbrMap = adjacencyListSimple[src];
      simpleNbrMap[dst] = true;
      adjacencyListSimple[src] = simpleNbrMap;
    }

    map<Node,map<Node,bool> >:: iterator posDest = nodeDestMap.find(dst);
    if(posDest == nodeDestMap.end())
    {
       map<Node,bool> adjacentNodeMap;
       adjacentNodeMap[Node(src)] = true;
       nodeDestMap[Node(dst)] = adjacentNodeMap;
    }
    else
    {
      map<Node,bool> adjacentNodeMap = (*posDest).second;
      adjacentNodeMap[Node(src)] = true;
      nodeDestMap[Node(dst)] = adjacentNodeMap;
    }
  }


void Graph_Adjacency::addBoundaryNode(const State& s)
{
  Node n(s);
  boundaryNodes[n] = true;
}
EdgeAnnotation Graph_Adjacency::getEdgeAnnotation(const State &src, const State &dst) const
{
      EdgeAnnotation ea;
      map<Node,map<EdgeInfoContainer,bool> >:: const_iterator sourcePosIter = nodeSourceMap.find(src);
      if (sourcePosIter == nodeSourceMap.end())
               return ea;

      
      map<EdgeInfoContainer,bool> dstMap = sourcePosIter->second;
      map<EdgeInfoContainer,bool>::const_iterator dstPos = dstMap.find(EdgeInfoContainer(src,dst)); 
      if (dstPos == dstMap.end())
                return ea;

      ea = (dstPos->first).annotation;

      return ea;
}
unsigned long Graph_Adjacency::getIterationCountForEdgeAndProcess(Edge& edg, unsigned int proc)
{
              EdgeAnnotation ea = getEdgeAnnotation(edg.getSourceState(), edg.getDestinationState());
              return ea.getIterationCountForTask(proc);
}
void Graph_Adjacency::compressGraph()
{

  map<Node,map<EdgeInfoContainer,bool> >:: iterator startIter = nodeSourceMap.begin();
  Node startNode = (*startIter).first;
  // this will traverse the graph and will try to compress...
  DFS(startNode,false);
  DFS(startNode,true);
  
  //repopulate this after compression
  adjacencyListSimple.clear();

  map<Node,map<EdgeInfoContainer,bool> >:: iterator begIter = nodeSourceMap.begin();
  map<Node,map<EdgeInfoContainer,bool> >:: iterator endIter = nodeSourceMap.end();
  for(; begIter != endIter; begIter++)
  {
    Node src = begIter->first;
    map<State,bool> simpleNbrMap;
    map<EdgeInfoContainer,bool> dstMap = begIter->second;
    map<EdgeInfoContainer,bool> :: iterator mapStart = dstMap.begin();
    map<EdgeInfoContainer,bool> :: iterator mapEnd = dstMap.end();
    for(;mapStart != mapEnd; mapStart++)
    {
        Node dst = (mapStart->first).theEdge.getDestinationState();
        simpleNbrMap[dst] = true;
    }
    adjacencyListSimple[src] = simpleNbrMap;
  }

}

void Graph_Adjacency::removeFromNodeDestMap(Node& currentnode, Node& neighborNode)
{
   map<Node, map<Node,bool> >::iterator dstNodeLoc = nodeDestMap.find(currentnode);
   if(dstNodeLoc == nodeDestMap.end())                    
   {                                                        
          // no outgoing edges from this node... return
         return;                              
   }
   map<Node,bool> dstMap = (*dstNodeLoc).second;
   map<Node,bool>::iterator posNeighbor = dstMap.find(neighborNode);
   if(posNeighbor != dstMap.end())
   {
     dstMap.erase(posNeighbor);

     //now replace the old map... we are not dealing with pointers... remember!!!
     nodeDestMap[currentnode] = dstMap;
   }
}
void Graph_Adjacency::addToNodeDestMap(Node& neighborsNeighbor, Node& currentnode)
{
   map<Node, map<Node,bool> >::iterator dstNodeLoc = nodeDestMap.find(neighborsNeighbor);
   if(dstNodeLoc == nodeDestMap.end())
   {
          // no outgoing edges from this node... return
         return;
   }
   map<Node,bool> dstMap = (*dstNodeLoc).second;
   dstMap.insert(make_pair(currentnode,true));
   nodeDestMap[neighborsNeighbor] = dstMap;
}
void Graph_Adjacency::eliminateSmallLoops(Node currentnode,map<Node,bool>* visited)
{
     //cout << "elimination node is " <<  currentnode.getId() <<  endl;
     map<Node, map<EdgeInfoContainer,bool> >::iterator srcNodeLoc = nodeSourceMap.find(currentnode);
     if(srcNodeLoc == nodeSourceMap.end())
     {
          // no outgoing edges from this node... return
       return;
     }
     if(boundaryNodes.find(currentnode) != boundaryNodes.end())
     {  
	     //this is a boundary node where tasks are waiting. We will not compress it.
	     return;
     }
     map<EdgeInfoContainer,bool> adjMap = (*srcNodeLoc).second;
     map<Node,bool> destMap;
     map<Node, map<Node,bool> >::iterator dstNodeLoc = nodeDestMap.find(currentnode);
     if(dstNodeLoc == nodeDestMap.end())
     {
          // no incoming edges from this node... return
           return;
     }
     else
     {
        destMap = dstNodeLoc->second;
     } 
     //small loops will have exactly one outgoing edge and two incoming edges and neighbor's next will point to this node
     if((adjMap.size() == 1) && (destMap.size() == 2))
     {
        map<EdgeInfoContainer,bool> :: iterator nodePos = adjMap.begin();
	Node neighborNode = (*nodePos).first.theEdge.getDestinationState();
        if(boundaryNodes.find(neighborNode) != boundaryNodes.end())
        {
             //this is a boundary node where tasks are waiting. We will not compress it.
             return;
        }
	//check if it is also pointing to the currentNode ..
	map<Node, map<EdgeInfoContainer,bool> >::iterator neighborLoc = nodeSourceMap.find(neighborNode);
	if(neighborLoc != nodeSourceMap.end()) 
	{
	   map<EdgeInfoContainer,bool> neighborAdjMap = (*neighborLoc).second;
           map<Node, map<Node,bool> >::iterator dstNeighborNodeLoc = nodeDestMap.find(neighborNode);
           if(dstNeighborNodeLoc == nodeDestMap.end())
           {
             // no outgoing edges from this node... return
             return;
           }
     	   map<Node,bool> destNeighborMap = dstNeighborNodeLoc->second;
           //we will only eliminate neighbors which has 1 or 2 ougoing edges..one of those fust be current node..
	   if(((neighborAdjMap.size() == 1) || (neighborAdjMap.size() == 2)) && (destNeighborMap.size() == 1))
	   {
             EdgeInfoContainer e(neighborNode,currentnode);
	     map<EdgeInfoContainer,bool>::iterator posNeighborMap = neighborAdjMap.find(e);
	     if(posNeighborMap != neighborAdjMap.end())
	     {
	         // this is a loop .. currentnode points to neighbornode and vice-versa..
		 //remove this reverse mapping edge..
		 neighborAdjMap.erase(posNeighborMap);
		 //TODO: actually we want to remove the whole loop along with neighbor altogether..
		 //To use proper iteration count, we will remove this annotation and will add another later
                 adjMap.erase(nodePos);
		// cout << " removing neighbor " << neighborNode.getId() << endl; 
		 if(neighborAdjMap.size() == 0)
		 { 
		     //no more edges from this node.. delete the entry from source map.. 
		     nodeSourceMap.erase(neighborLoc); 
		     removeFromNodeDestMap(currentnode,neighborNode);
		 }
		 else
		 {
	             posNeighborMap = neighborAdjMap.begin();
                     EdgeInfoContainer neighborsNextEdge = posNeighborMap->first;
		     EdgeAnnotation neighborsNextAnnotation = neighborsNextEdge.annotation;
		     //Just to be careful that we do not mess up with iteration counts after elimating small loops
		     //use the edgeAnnotation of the edge outside of this small-loop as the annotation after elimination
		     adjMap.insert(make_pair(
                            EdgeInfoContainer(Edge(currentnode,neighborNode),neighborsNextAnnotation),
                            true));
                     nodeSourceMap[currentnode] = adjMap;

		     //replace the old map with this updated map..
		     nodeSourceMap[neighborNode] = neighborAdjMap;
		      removeFromNodeDestMap(currentnode,neighborNode);
		      //addToNodeDestMap(neighborsNeighbor,currentnode);
		   
		 }

	     }
	   }
	}
       
     }

}

void Graph_Adjacency::DFSUtil(Node node,map<Node,bool>* visited, bool followChain)
{

  //bool followchain to decide whether this DFS is in followchain compression mode or loop elimination mode...
  if(visited->find(node) != visited->end())
  {
    return;
  }
  else
  {
    (*visited)[node] = true;
  }
  map<Node, map<EdgeInfoContainer,bool> >::iterator srcNodeLoc = nodeSourceMap.find(node);
  if(srcNodeLoc == nodeSourceMap.end())
  {
   // no outgoing edges from this node... return
    return;
  }
  map<EdgeInfoContainer,bool> adjMap = (*srcNodeLoc).second;
  if(adjMap.size() == 1)
  {  
      Node nextCandidateNode = node;
      if(followChain)
      {
      	//has only one outgoing edge...so we might compress.... try!! and compression will internally remove redundant edges
      	map<Node,bool> redundantEdgeDatas;
	EdgeAnnotation validAnnotation;
      	Node nextnode = followChainInGraph(node,visited,redundantEdgeDatas,validAnnotation);
      	if(nextnode != node)
      	{
        	//we were able to compress a chain .. insert new compressed edge in the graph...we have already removed redundant intermediate edges..
		map<EdgeInfoContainer,bool> newDest;
		newDest[EdgeInfoContainer(Edge(node,nextnode),validAnnotation)] = true;
		nodeSourceMap[node] = newDest;

		nextCandidateNode = nextnode;
      	}
      	else
      	{
        	 //it returned the same node:
	 	//possible cause: 1) multiple incoming edge
	 	//send next node for DFS.. 
	 	//NOTE: it can not be due to multiple outgoing edges or no outgoing edge because we already checked adjMap.size() == 1 above..

	 	map<EdgeInfoContainer,bool>::iterator mapStart = adjMap.begin();
	 	nextCandidateNode = (*mapStart).first.theEdge.getDestinationState();
		//cout << "node " << node.getId() << " next node " << nextCandidateNode.getId() << endl;
      	}
      }
     else
     {	
     		eliminateSmallLoops(node,visited);
         	map<EdgeInfoContainer,bool>::iterator mapStart = adjMap.begin();
		nextCandidateNode = (*mapStart).first.theEdge.getDestinationState();
		//cout << "node " << node.getId() << " next node " << nextCandidateNode.getId() << endl;
       
      }

      if(visited->find(nextCandidateNode) == visited->end())
      {
           DFSUtil(nextCandidateNode,visited,followChain);
      }
  }
  else
  {
     map<EdgeInfoContainer,bool>::iterator mapStart = adjMap.begin(), mapEnd = adjMap.end();
     for(;mapStart != mapEnd;mapStart++)
     {
        Node adjNode = (*mapStart).first.theEdge.getDestinationState();
	if(visited->find(adjNode) == visited->end())
	{
	  DFSUtil(adjNode,visited,followChain);
	}
     }

  }
}
void Graph_Adjacency::DFS(Node startNode,bool followChain)
{
  map<Node,bool>* visited = new map<Node,bool>;
  
  // Call the recursive helper function to print DFS traversal
  DFSUtil(startNode, visited,followChain);
  
  delete visited;
}

Graph_Edges* Graph_Adjacency::convert_To_Graph_Edge_Format()
{

  Graph_Edges* ge = new Graph_Edges();

  map<Node,map<EdgeInfoContainer,bool> >::iterator sourceStart = nodeSourceMap.begin(), sourceEnd = nodeSourceMap.end();
 for(;sourceStart != sourceEnd; sourceStart++)
 {
    Node node = (*sourceStart).first;

    map<EdgeInfoContainer,bool> adjMap = (*sourceStart).second;
    map<EdgeInfoContainer,bool>::iterator adjStart = adjMap.begin(), adjEnd = adjMap.end();
    for(;adjStart != adjEnd; adjStart++)
    {
        EdgeInfoContainer einfo = (*adjStart).first;
        ge->add_edges(einfo);
    }
 }

 return ge;
}
void Graph_Edges::add_edges(EdgeInfoContainer& eif)
        {
                string edge_key = (eif.theEdge.getSourceState()).getString() + "#" + (eif.theEdge.getDestinationState()).getString();
                edge_map.insert(make_pair(edge_key,eif));
                node_map[(eif.theEdge.getSourceState()).getString()] = eif.theEdge.getSourceState();
                node_map[(eif.theEdge.getDestinationState()).getString()] = eif.theEdge.getDestinationState();
        }

