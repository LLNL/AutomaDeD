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
#include<vector>
#include<list>
#include<map>
#include<set>
#include<algorithm>
#include<sstream>
#include "graph_algorithm.h"
#include "model_components.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/bind.hpp>
 #include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/transpose_graph.hpp>
#include <boost/algorithm/string.hpp>

#include "config.h"


using namespace std;
Graph_Adjacency* loopAnalysis::graph_adj = 0;
list<State> loopAnalysis::path = list<State>(); // stack of nodes (vertices) in current path
map<State,bool> loopAnalysis::blocked = map<State,bool>(); //vertex: blocked from search?
map<State,list<State> > loopAnalysis::B = map<State,list<State> >(); // graph portions that yield no elementary circuit
list< list<State>* >* loopAnalysis::result = 0;  // list to accumulate the circuits found
map<State,set<size_t>* > loopAnalysis::nodeLoopMap = map<State,set<size_t>* >();
map<size_t,set<Edge>* > loopAnalysis::loopEdgeMap = map<size_t,set<Edge>* >();
map<Edge,set<size_t>* > loopAnalysis::edgeLoopMap = map<Edge,set<size_t>* >();
//map<size_t,Edge> loopAnalysis::loopCharecteristicEdgeMap = map<size_t,Edge>();
map<Edge,Edge> loopAnalysis::backEdgeCharecteristicEdgeMap = map<Edge,Edge>();
map<size_t,State> loopAnalysis::loopEntryNodeMap = map<size_t,State>();
map<size_t,Edge> loopAnalysis::backEdgeMap = map<size_t,Edge>();
map<size_t, map<unsigned int,unsigned long> > loopAnalysis::resolvedLoopIterationMap = 
                                                  map<size_t, map<unsigned int,unsigned long> >();

int circuitReqCount = 0; 

static bool sccSortFunction (list<State>*  lst1 ,list<State>*  lst2) 
{
	return (lst1->size() < lst2->size());
}

static std::size_t getLoopHash(list<State> & nodeList)
{
        //ordering matters!
                 std::size_t seed = 0;
                 list<State>::iterator iterBeg = nodeList.begin();
                 list<State>::iterator iterEnd = nodeList.end();
                 for(;iterBeg != iterEnd;iterBeg++)
                 {
                    unsigned int id = (*iterBeg).getId();
                    boost::hash_combine(seed, id);
                 }
                 return seed;
}
/* magic numbers from http://www.isthe.com/chongo/tech/comp/fnv/ */
static const size_t InitialFNV = 2166136261U;
static const size_t FNVMultiple = 16777619;

/* Fowler / Noll / Vo (FNV) Hash */
size_t myhash(const string &s)
{
    size_t hash = InitialFNV;
    for(size_t i = 0; i < s.length(); i++)
    {
        hash = hash ^ (s[i]);       /* xor  the low 8 bits */
        hash = hash * FNVMultiple;  /* multiply by the magic number */
    }
    return hash;
}


typedef                 boost::adjacency_list <
                        boost::setS,
                        boost::vecS,
                        boost::bidirectionalS,
                        State
                        > Graph;
        typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
        typedef boost::graph_traits<Graph> GraphTraits;
        typedef GraphTraits::edge_descriptor boost_edge;

        Graph graph;
        std::map<State, Vertex > states;
        std::map<Vertex, State > statesInv; // inverse table of states
        std::set<Vertex> visitedVertices; // set of states to detect cycles
        std::vector<Vertex> vertexSequence;
        std::vector< std::vector<Vertex> > paths;
        typedef std::map<Vertex, Vertex> VertexVertexMap;

 VertexVertexMap dominatorTree_;
  Vertex entry_;


list<State>* findNaturalLoops(boost_edge be);
void  printLoop(list<Vertex>& l);

  Vertex addVertex(const State &s)
        {
                Vertex ret;
                std::map<State, Vertex>::const_iterator it = states.find(s);
                if (it == states.end()) {
                        ret = boost::add_vertex(s, graph);
                        if(s.getId() == 1)
                        {
                             entry_ = ret;
                         }
                        states.insert(std::pair<State, Vertex>(s,ret));
                        statesInv.insert(std::pair<Vertex,State>(ret,s));
                } else {
                        ret = it->second;
                }

                return ret;
        }


 void addEdge(const State &src, const State &dst)
        {
                Vertex x = addVertex(src);
                Vertex y = addVertex(dst);

                boost::add_edge(x, y, graph);

        }


void createBoostGraph(map<State, map<State,bool> >& sourceNodeMap)
{
  map<State, map<State,bool> > :: iterator mapStart = sourceNodeMap.begin();
   map<State, map<State,bool> > :: iterator mapEnd = sourceNodeMap.end();
   for(; mapStart != mapEnd; mapStart++){
        State s = mapStart->first;
        map<State,bool> dstMap = mapStart->second;
        map<State,bool>::iterator destStart = dstMap.begin(), destEnd = dstMap.end();
        for(;destStart != destEnd; destStart++)
        {
             State d = destStart->first;
             addEdge(s,d);
        }
   }
}



 VertexVertexMap& getDominatorTree()
 {
     if (!dominatorTree_.empty())
         return dominatorTree_;

   boost::associative_property_map<std::map<Vertex, Vertex> > domTreePredMap(dominatorTree_);
 
    // Here we use the algorithm in boost::graph to build a map from each node to its immediate dominator.
    boost::lengauer_tarjan_dominator_tree(graph, entry_, domTreePredMap);
     return dominatorTree_;
 }

 
 std::vector<boost_edge> getAllBackEdges()
 {
     std::vector<boost_edge> backEdges;
 
     // If the dominator tree is not built yet, build it now.
     getDominatorTree();
 
     BOOST_FOREACH (const boost_edge& e, boost::edges(graph))
     {
         Vertex src = boost::source(e, graph);
         Vertex tar = boost::target(e, graph);
 
         //Vertex v = *(dominatorTree.find(src));
         VertexVertexMap::const_iterator iter = dominatorTree_.find(src);
         while (iter != dominatorTree_.end())
         {
             if (iter->second == tar)
             {
                 backEdges.push_back(e);
		 //extra...
		 //list<Vertex> lv = findNaturalLoops(e);
		 //printLoop(lv);
		 //end extra..
                 break; // break the while loop
             }
             iter = dominatorTree_.find(iter->second);
         }
    }
 
     return backEdges;
 }

void  printLoop(list<State>* l)
{
        list<State> :: iterator beg = l->begin();
        list<State> :: iterator end = l->end();
        cout << "BOOST FULL LOOP: " ;
        for(;beg != end; beg++)
        {
                State s = (*beg);
                cout << s.getId() << "," ;
        }
        cout << endl;

}

void  printEdges(vector<boost_edge>& bes)
{
	vector<boost_edge> :: iterator beg = bes.begin(); 
	vector<boost_edge> :: iterator end = bes.end();
        for(;beg != end; beg++)
	{
		boost_edge e = (*beg);
		Vertex v1 = boost::source(e, graph);
		Vertex v2 = boost::target(e, graph);
		State src = statesInv[v1];
		State dst = statesInv[v2];
                cout << "BOOST Back edge is: " << src.getId() << " -- " << dst.getId() << endl;
	}
}

std::vector<Vertex> getAllLoopHeaders()
 {
     std::vector<boost_edge> backEdges = getAllBackEdges();
     std::vector<Vertex> headers;
     BOOST_FOREACH (boost_edge e, backEdges)
         headers.push_back(boost::target(e, graph));
     return headers;
} 

void insertInLoop(Vertex n,list<Vertex>& vstk, list<State>* lList, set<Vertex>& lSet )
{
  if(lSet.find(n) == lSet.end())
  {
          State s = statesInv[n];
	  lList->push_back(s);
	  lSet.insert(n);

	  vstk.push_back(n);
  }
}

list<State>* findNaturalLoops(boost_edge be)
{
  list<Vertex>  vstack;
  list<State>*  loopList = new list<State>;
  set<Vertex> loopSet;
   Vertex head = boost::target(be, graph);
   Vertex bSource = boost::source(be, graph);

    State headState = statesInv[head];
   loopList->push_back(headState);
   loopSet.insert(head);

   insertInLoop(bSource,vstack,loopList,loopSet);
   while(1)
   {
	   if(vstack.size() == 0)
	   {
		   break;
	   }

	   Vertex v = vstack.back();
	   if(v == head)
	   {
		   break;
	   }
	   vstack.pop_back();

           Graph::in_edge_iterator in_begin, in_end;
           for (boost::tie(in_begin, in_end) = in_edges(v,graph); in_begin != in_end; ++in_begin)
           {   
	       Vertex p = boost::source(*in_begin, graph);
	       insertInLoop(p,vstack,loopList,loopSet);
	       //std::cout << source(*in_begin,graph) << std::endl;
           }

   }
   //loopList->pop_front(); we need head node to be both on front and back of the list
   loopList->push_back(headState);
   loopList->reverse();
   return loopList;


}

list< list<State>* >*  loopAnalysis::findAllLoopsWithBoost(map<State, map<State,bool> >& sourceNodeMap) 
{
  list< list<State>* >* retList = new list< list<State>* >;
 
  createBoostGraph(sourceNodeMap);
  vector<boost_edge> bes =  getAllBackEdges();

  vector<boost_edge>::iterator begIter = bes.begin(), endIter = bes.end();
  for(;begIter != endIter; begIter++)
  {
	  boost_edge be = (*begIter);
	  list<State>* loopList = findNaturalLoops(be);
	  //printLoop(loopList);
	  retList->push_back(loopList);
  }


  return retList;
  //printEdges(bes);
}

static std::size_t getLoopHash_1(list<State> & nodeList)
{
                  stringstream ss;
                 list<State>::iterator iterBeg = nodeList.begin();
                 list<State>::iterator iterEnd = nodeList.end();
                 for(;iterBeg != iterEnd;iterBeg++)
                 {
                    unsigned int id = (*iterBeg).getId();
                    ss<<id;
                 }
                 string fullList = ss.str();
		 size_t hash = myhash(fullList);
		 return hash;
}
static list< list<State>* >* getSCCFromGraph(map<State, map<State,bool> >& sourceNodeMap)
{
  map<State,int> preorder;
  map<State,int> lowlink;  
  map<State,bool> scc_found;
  stack<State> scc_stack;
  list<list<State>* >* scc_list = new list<list<State>* >;
  int    i=0;//     # Preorder counter  
  map<State, map<State,bool> > :: iterator mapStart = sourceNodeMap.begin();
  map<State, map<State,bool> > :: iterator mapEnd = sourceNodeMap.end();
  for(;mapStart != mapEnd; mapStart++)
  {
        State source = mapStart->first;
        if(scc_found.find(source) == scc_found.end())
	{
            stack<State> nodeStack;
	    nodeStack.push(source);
            while (nodeStack.empty() == false) {
                State v = nodeStack.top();
                if(preorder.find(v) == preorder.end()){
                    i=i+1;
                    preorder[v]=i;
		}
                bool done= true;
		map<State, map<State,bool> > :: iterator vPos = sourceNodeMap.find(v);
		map<State,bool> v_nbrs;
		if(vPos != mapEnd)
		{
		    v_nbrs = vPos->second;
		}

		map<State,bool> :: iterator nbr_start = v_nbrs.begin();
		map<State,bool> :: iterator nbr_end = v_nbrs.end();
                for(;nbr_start != nbr_end ; nbr_start++){
		    State w = nbr_start->first;
                    if(preorder.find(w) == preorder.end()){
                        nodeStack.push(w);
                        done= false;
                        break;
		    }
		}
                if (done){
                    lowlink[v]=preorder[v];
		    nbr_start = v_nbrs.begin();
		    for(;nbr_start != nbr_end ; nbr_start++){
			State w = nbr_start->first;
                        if(scc_found.find(w) == scc_found.end()){
                            if(preorder[w]>preorder[v]){
                                lowlink[v]=std::min(lowlink[v],lowlink[w]);
			    }
                            else{
                                lowlink[v]= std::min(lowlink[v],preorder[w]);
			    }
			}
		    }
                    nodeStack.pop();
                    if (lowlink[v]==preorder[v]) {
                        scc_found[v]= true;
                        list<State>* scc = new list<State>;
			scc->push_back(v);
                        while ((scc_stack.empty() == false) && ( preorder[scc_stack.top()]> preorder[v])){
                            State k=scc_stack.top();
                            scc_stack.pop();
                            scc_found[k]=true;
                            scc->push_back(k);
			}
			if(scc->size() > 1){
                           scc_list->push_back(scc);
			}
		    }
                    else{
                        scc_stack.push(v);
		    }
		}
	    }
	}
  }

  scc_list->sort(sccSortFunction);
  return scc_list;
}

map<State, map<State,bool> > getSubgraph(State s, list<State>* nodeList, map<State, map<State,bool> >&  parentGraph, bool useLimitNodes)
{
	set<State> nodeSet;
	if(useLimitNodes)
	{
		list<State> ::iterator listStart = nodeList->begin();
		list<State> ::iterator listEnd = nodeList->end();
		for(; listStart != listEnd; listStart++)
		{
			nodeSet.insert(*listStart);
		}
	}
	
	
	//get a subgraph oof nodes >= s
	map<State, map<State,bool> > subGraph;
	map<State, map<State,bool> > :: iterator mapStart = parentGraph.begin();
        map<State, map<State,bool> > :: iterator mapEnd = parentGraph.end();
	for(;mapStart != mapEnd; mapStart++)
	{
		State source = mapStart->first;
		if(!useLimitNodes)
		{
		  if(source < s)
		  {
		    continue;	
		  }
		}
		else
		{
			if(nodeSet.find(source) == nodeSet.end())
			{
				continue;
			}
		}
		map<State,bool> neighbors = mapStart->second;

		bool hasNbr = false;
		map<State,bool> :: iterator nbr_start = neighbors.begin();
		map<State,bool> :: iterator nbr_end = neighbors.end();
                map<State,bool> neighborMap; 
		for(; nbr_start != nbr_end; nbr_start++)
		{
			State destination = nbr_start->first;
			if(!useLimitNodes)
			{
				if(destination < s)
				{
					continue;
				}
			}
			else
			{
				if(nodeSet.find(destination) == nodeSet.end())
                        	{
                                	continue;
                        	}
			}
			
			
                        hasNbr = true;
			neighborMap[destination] = true;
			
		}

		if(hasNbr)
	        {
			subGraph[source] = neighborMap;

		}
	}

	return subGraph;
	
}


loopAnalysis::loopAnalysis(Graph_Adjacency* ga)
{
  graph_adj = ga;
  result = new list< list<State>* >;  // list to accumulate the circuits found
}

State loopAnalysis::blockAndGetMinimumNode(map<State, map<State,bool> >& component)
{
  map<State, map<State,bool> > :: iterator compStart = component.begin();
  map<State, map<State,bool> > :: iterator compEnd = component.end();
  State minNode = compStart->first;
  for(; compStart != compEnd; compStart++)
  {
       State source = (*compStart).first;
       if(source < minNode)
        {
           minNode = source;
        }
        blocked[source] = false;
	list<State> lstsrc;
		B[source] = lstsrc;
		map<State,bool>  :: iterator destStart = (compStart->second).begin();
		map<State,bool>  :: iterator destEnd = (compStart->second).end();
		for(;destStart != destEnd;destStart++)
		{
			State destination = (*destStart).first;
			if(destination < minNode)
			{
			  minNode = destination;
			}
			blocked[destination] = false;
			list<State> lstdst;
			B[destination] = lstdst;
		}

	   }
	   return minNode;
	}

void  loopAnalysis::unblock(State& thisnode){
		//Recursively unblock and remove nodes from B[thisnode].
		if(blocked[thisnode]){
		    blocked[thisnode] = false;
		    while( B[thisnode].empty() == false){
			State anode = B[thisnode].back();
			B[thisnode].pop_back();
			unblock(anode);
            }
        }
   }

bool loopAnalysis::circuit(State& thisnode, State& startnode, map<State,map<State,bool> >& component){
	circuitReqCount++;
	if(circuitReqCount > 490)
	{
	 cout << "In circuit recursion num: " << circuitReqCount << endl;
	}
        bool closed = false;   // set to True if elementary path is closed
	//cout << "thisnode " << thisnode << endl;
        path.push_back(thisnode);
        blocked[thisnode] = true;
        map<State,bool> :: iterator startIter = component[thisnode].begin();
        map<State,bool> :: iterator endIter = component[thisnode].end();
        for(;startIter != endIter; startIter++){
            State nextnode = startIter->first;      // direct successors of thisnode
            if (nextnode == startnode){
                list<State>* temp = new list<State>;
                temp->insert(temp->end(), path.begin(), path.end());
                temp->push_back(startnode);
                result->push_back(temp);
                closed = true;
            }                 
            else if((blocked.find(nextnode) == blocked.end()) || (false == blocked[nextnode]) ) {
	       if(circuitReqCount < 500){  // prevent infinite recursion
	       //if(true){  // prevent infinite recursion
                  if(circuit(nextnode, startnode, component)) {
                      closed = true;
                  }             
               }
	    }                 
        }                     
        if(closed){           
            unblock(thisnode);
        }                     
        else{                 
            startIter = component[thisnode].begin();
            endIter = component[thisnode].end();
            for(;startIter != endIter; startIter++) { 
                State nextnode = startIter->first;
                list<State> stk = B[nextnode];
		list<State>::iterator stkBeg = stk.begin();
		list<State>::iterator stkEnd = stk.end();
		bool foundThisNode = false;
		for(;stkBeg != stkEnd; stkBeg++)
		{
		   if(thisnode == (*stkBeg))
		   {
                       foundThisNode = true;
		       break;
		   }	
		}
                if(false == foundThisNode) { // TODO: use set for speedup?
                    B[nextnode].push_back(thisnode);
                }             
            }                 
        }                     
        path.pop_back(); //remove thisnode from path
        return closed;
} 

State loopAnalysis::getListMinimum(list<State> * lst)
{
   list<State> :: iterator ls =  lst->begin();
   list<State> :: iterator le =  lst->end();
   State item = (*ls);
   for(;ls != le; ls++)
   {
	   if((*ls) < item)
	   {
		   item = (*ls);
	   }
   }

   return item;

}
list<State>* loopAnalysis::getMinSCC(list< list<State>* >* sccList)
{
    map<State,list<State >* > sccorder;
    list< list<State>* > :: iterator ls =  sccList->begin();
    list< list<State>* > :: iterator le =  sccList->end();
    for(; ls != le;ls++)
    {
	    State n = getListMinimum((*ls));
      sccorder[n] = (*ls);
    }
    map<State,list<State>* > :: iterator mapBeg = sccorder.begin();
    list<State>* retlst = mapBeg->second;
    return retlst;


}

list< list<State>* >*  loopAnalysis::findAllLoops(map<State, map<State,bool> >& sourceNodeMap) {
    
 /*
   Returns a list of loops/circuits, where each circuit is a list of nodes, with the first 
    and last node being the same.
    
    References
    ----------
    
    The implementation follows pp. 79-80 of:
    .. [1] Finding all the elementary circuits of a directed graph.
       D.B. Johnson
       SIAM Journal on Computing 4, no. 1: 77-84.
       http://dx.doi.org/10.1137/0204007
    
**/



   map<State, map<State,bool> > :: iterator mapStart = sourceNodeMap.begin();
   map<State, map<State,bool> > :: iterator mapEnd = sourceNodeMap.end();
    // Johnson's algorithm requires some ordering of the vertices which is there in the map
    for(; mapStart != mapEnd; mapStart++){
        State s = mapStart->first;
       // Find the strong component K with least vertex (i.e. node) 
       // in the subgraph induced by s and its following nodes.
	 map<State, map<State,bool> > subgraph = getSubgraph(s,NULL,sourceNodeMap,false);
 	//subgraph = sourceStateMap;
        list< list<State>* >* strongcomp = getSCCFromGraph(subgraph);
 	if(strongcomp->size() > 0)
	{
	 //list< list<State>* > :: iterator sccBegin = strongcomp->begin();
	 //list<State>* lstPtr = (*sccBegin);
	 list<State>* lstPtr = getMinSCC(strongcomp);
	 map<State, map<State,bool> > component = getSubgraph(s,lstPtr,sourceNodeMap,true);
         if(component.size() > 0){
	    
            State startnode = blockAndGetMinimumNode(component); 
            circuitReqCount = 0;
	    circuit(startnode, startnode, component);
	 }
         else{
            break;
	 }
	}
	else{
           break;
	}
	
    }
    
    return result;
}

static void giveMeHops(map<Node,map<Node,bool> >& destBasedMap, State& s, State& matchState, int& hops, bool& error)
{
    //cout << "gimeme hops called with s= " << s.getId() << " and match= " << matchState.getId() << endl;
    map<Node,map<Node,bool> > :: iterator nodePos =  destBasedMap.find(s); 
    if(nodePos != destBasedMap.end())
    {
	    hops++;
	    map<Node,bool> incomingMap = nodePos->second;
	    map<Node,bool> :: iterator firstEntry = incomingMap.begin();
	    if(firstEntry == incomingMap.end())
	    {
		    // something is wrong!!
		    cout << s.getId() << " not in incomingMap" << endl;
		    error = true;
		    return;
	    }
	    Node srcNode = firstEntry->first;
	    //it is interesting to NOTE: why always taking the first entry works?
	    //BEacuse a loop-node can have two kind of incoming edges:
	    //1) from a predecessor P->S
	    //2) a backedge B->S
	    //where S is the current node
	    //Now since all the nodes are numbered in increasing order, B.id > P.id always
	    //In the map container, entries will be ordered according to < operator of Edge class
	    //which compares source (S) first and then (P vs B) and will always get P->S before B->S
	    //Thus first is always P->S ... and we can go up to predecessor to count hops upto loop entry
	    if(srcNode == matchState)
	    {
		    //success!!
		    error = false;
		    return;
	    }
	    return giveMeHops(destBasedMap,srcNode,matchState,hops,error);
    }
    else
    {
	    //something is wrong!!
	    cout << s.getId() << " not in dest based map" << endl;
	    error = true;
	    return;
    }
}
State loopAnalysis::getLoopEntryState(size_t& theloop)
{
	return loopEntryNodeMap[theloop];
}
int loopAnalysis::getHopCountFromLoopEntry(Edge& backEdge, State& currentState) 
{
     //State loopEntryState = getLoopEntryState(theloop);
     
     //The head(destination) of backedge is the loop entry point for that loop by definition!!

     State loopEntryState = backEdge.getDestinationState();

     // This is tricky...we might want to count hop forward from loopEntryState to currentState.. but there might be many exit points!!
     // traverse backwards from currentNode to loopEntryPoint to get the hopCount

     //cout << "Loop entry: " << loopEntryState.getId() << " current state: " << currentState.getId() << endl;

     if(loopEntryState == currentState)
     {
	     //if it itself is the loopEntry point. Had a bug for not having this in paradise hang!
	     return 0;
     }

     int hopCount = 0;
     bool error;
     giveMeHops(graph_adj->nodeDestMap,currentState,loopEntryState,hopCount,error);
     if(error == false)
     {
	     return hopCount;
     }
    
     return -1;
     
}

bool loopAnalysis::checkAndPopulateForEntryNode(State& src,size_t& loopHash)
{  
  //Assuming loop finding algorithm always gives the entry point as the first node. Otherwise try to find the backedge.. 	
  loopEntryNodeMap.insert(pair<size_t,State>(loopHash,src));
  return true;
#if 0
  //TODO: THis function is wrong!! Fix me!!
  map<Node,map<Node,bool> > :: iterator nodePos =  (graph_adj->nodeDestMap).find(src); 
  if(nodePos != (graph_adj->nodeDestMap).end())
  {
      // this map contains information about number of INCOMING edges to this node
     // IMPORTANT: For reducible loops. there will be only one node with multiple incoming edges. that node is the entry 
     map<Node,bool> srcMap = nodePos->second;
     if(srcMap.size() > 1)
     {
	     // more than one incoming edge.. this is the loop entry
	     // For reducible loops. there will be only one node with multiple incoming edges. that node is the entry
	     loopEntryNodeMap.insert(pair<size_t,State>(loopHash,src));
	     return true;
     }
     else
     {
	     return false;
     }
  }

  return false;
#endif
}

void loopAnalysis::populateAllLoopInformation() //input about graph info
{
   map<State, map<State,bool> > sourceNodeMap = graph_adj->adjacencyListSimple;

   list<list<State>* >* allLoops = NULL;
   
   bool useJohnson = false;
   bool r = false;
   r = AUTConfig::getBoolParameter("AUT_USE_JOHNSON", useJohnson);
   if(!r || !useJohnson)
   {
       allLoops = findAllLoopsWithBoost(sourceNodeMap);
   }
    else
   {
       allLoops = findAllLoops(sourceNodeMap);
   }
   //cout << "------ NUMBER OF TOTAL LOOPS : " << allLoops->size() << endl;
   list<list<State>* >::iterator loopStart = allLoops->begin(), loopEnd = allLoops->end();
   int i = 0;
   for(;loopStart != loopEnd; loopStart++)
   {
	   i++;
	   list<State>* nodeList = (*loopStart);
	   size_t loopHash = getLoopHash(*nodeList);
	   //size_t loopHash = getLoopHash_1(*nodeList);
	   set<Edge>* set_edge = new set<Edge>;
	   list<State> :: iterator nodeStart = nodeList->begin(), nodeEnd = nodeList->end();
	   State startState = (*nodeStart);
	   bool foundEntryNode = false;
	   bool backedgeFound = false;
	   for(;nodeStart != nodeEnd;nodeStart++)
	   {
		State src = (*nodeStart);
		set<size_t>* loopSet;
		map<State,set<size_t>* > :: iterator nodePos = nodeLoopMap.find(src);
		if(nodePos != nodeLoopMap.end())
		{
			loopSet = nodePos->second;
		}
		else
		{
			loopSet = new set<size_t>;
                	nodeLoopMap[src] = loopSet;
		}
		loopSet->insert(loopHash);
                if(!foundEntryNode)
		{
		  foundEntryNode = checkAndPopulateForEntryNode(src,loopHash);
		}

		nodeStart++;
		if(nodeStart != nodeEnd)
		{
		    State dst = (*nodeStart);
		    Edge e(src,dst);
	             //cout << " creating edge" << src.getId() << ":"  << dst.getId() <<endl;
		    set_edge->insert(e);

                    set<size_t>* setOfLoops;
                    map<Edge,set<size_t>* > :: iterator edgePos = edgeLoopMap.find(e);
                    if(edgePos != edgeLoopMap.end())
                    {
                        setOfLoops = edgePos->second;
                    }
                    else
                    {
                        setOfLoops = new set<size_t>;
                        edgeLoopMap[e] = setOfLoops;
                    }
                    setOfLoops->insert(loopHash);
		    if((!backedgeFound))
		    {
		       if(e.getDestinationState() == loopEntryNodeMap[loopHash]) // this check is necessary for short cut path. see test 6
		       {
                         //this is a backedge
		         backedgeFound = true;
		         populateBackEdgeInfo(loopHash,e);
			 // since we are considering the loops with common backedges as the same loop, so in that case
			 // backedges can be treated as characteristic edge
			 backEdgeCharecteristicEdgeMap[e] = e;
			 //cout << "Inserted backedge" << src.getId() << " and " << dst.getId() << endl;
		       }
		    }
				    
				    
		}
		nodeStart--;
	   }
#if 0
	   map<size_t,set<Edge>* > :: iterator lPos = loopEdgeMap.find(loopHash);
	   if(lPos != loopEdgeMap.end())
	   {
		   cout << "Hash collission for " << loopHash << endl;
                   nodeStart = nodeList->begin(), nodeEnd = nodeList->end();
		   cout << "states:" ;
                   for(;nodeStart != nodeEnd;nodeStart++)
                   {
                      State src = (*nodeStart);
		      cout << src.getId() << "," ;
		   }
		   cout << endl;
	   }
#endif
	   loopEdgeMap[loopHash] = set_edge;

   }

}

void loopAnalysis::populateBackEdgeInfo(size_t& theloop, Edge& backedge)
{
    //cout << "Back edge " << backedge.getSourceState().getId() << ":" << backedge.getDestinationState().getId() << endl;
	backEdgeMap.insert(make_pair(theloop,backedge));
}

Edge loopAnalysis::getBackEdge(size_t & theloop)
{
   return backEdgeMap[theloop];
}

Edge loopAnalysis::getCharacteristicEdge(Edge& backE, bool& found)
{
   //In this case characteristic Edge IS the backedge..so we will always find the charedge which is the backedge...
   found = false;
   Edge retedge;
   //Edge retedge = backEdgeCharecteristicEdgeMap[backE];
   map<Edge,Edge>::iterator pos = backEdgeCharecteristicEdgeMap.find(backE);
   if(pos != backEdgeCharecteristicEdgeMap.end())
   {
	   retedge = pos->second;
	   found = true;
   }
   return retedge;
}

void loopAnalysis::getCharacteristicEdges(size_t& loop1, size_t& loop2, Edge & e1, Edge & e2)
{
    set<Edge>* set1 = loopEdgeMap[loop1];
    set<Edge>* set2 = loopEdgeMap[loop2];
    set<Edge> :: iterator setStart = set1->begin();
    set<Edge> :: iterator setEnd = set1->end();
    for(;setStart != setEnd; setStart++ )
    {
         set<Edge> :: iterator setPos = set2->find((*setStart));
	 if(setPos == set2->end())
	 {
                e1 = (*setStart);
		break;
	 }
    }

    setStart = set2->begin();
    setEnd = set2->end();
    for(;setStart != setEnd; setStart++ )
    {                 
         set<Edge> :: iterator setPos = set1->find((*setStart));
         if(setPos == set2->end())
         {            
                e2 = (*setStart);
                break;
         }
    }
}
static set<Edge> list2Set(list<Edge>& edgeList)
{
	set<Edge> retSet;
	list<Edge> :: iterator listStart = edgeList.begin();
	list<Edge> :: iterator listEnd = edgeList.end();
	for(; listStart != listEnd; listStart++)
	{
		retSet.insert(*listStart);
	}
	return retSet;

}
set<Edge> loopAnalysis::getBackEdgesForCommonLoops(State & A, State & B)
{
	//cout << "src : " << A.getId() << " dst : " << B.getId() << endl;
	map<State, list<Edge> > setOfLoopHeaders = getCommonMergedLoopsBackEdges(A,B);

	set<Edge> usefulBackEdges;

	map<State, list<Edge> >::iterator headerStart = setOfLoopHeaders.begin();
	map<State, list<Edge> >::iterator headerEnd = setOfLoopHeaders.end();
	for(; headerStart != headerEnd; headerStart++)
	{
           set<Edge> commonSet;
	   size_t longest_loop;
	   size_t maxLength = 0;
	   list<Edge> backEdges = headerStart->second;
	   set<Edge> backEdgeUniqueSet = list2Set(backEdges);
	   //cout << "HEADER : " << (headerStart->first).getId() << " num of backEdges " << backEdgeUniqueSet.size() << endl;
	   if(backEdgeUniqueSet.size() > 1)
	   {
	     list<Edge> :: iterator listStart = backEdges.begin();
	     list<Edge> :: iterator listEnd = backEdges.end();
	     for(; listStart != listEnd; listStart++)
	     {
		   Edge backE = (*listStart);
		   //cout << "PRINT backedges :  " << backE.getSourceState().getId() << " - " << backE.getDestinationState().getId() << endl;

		   set<size_t>* loopSet = edgeLoopMap[backE];
		   set<size_t> :: iterator loopBegin = loopSet->begin();
		   set<size_t> :: iterator loopEnd = loopSet->end();
		   for(; loopBegin != loopEnd; loopBegin++)
		   {
			   size_t loop = (*loopBegin);
                           set<Edge>* edgeSet = loopEdgeMap[loop];
			   if(maxLength < edgeSet->size())
			   {
				   maxLength = edgeSet->size();
				   longest_loop = loop;
			   }
			   if(commonSet.empty())
			   {
				   set_intersection(edgeSet->begin(),edgeSet->end(),edgeSet->begin(),edgeSet->end(),
						                 std::inserter(commonSet,commonSet.begin()));
			   }
			   else
			   {
				   set<Edge> temp;
				   set_intersection(edgeSet->begin(),edgeSet->end(),commonSet.begin(),commonSet.end(),
						                    std::inserter(temp,temp.begin()));
				   commonSet = temp;
			   }
		   }
	     }
             //cout << " size of common edge : " <<  commonSet.size() << endl;

             Edge effectiveBackE = getBackEdge(longest_loop);
	     usefulBackEdges.insert(effectiveBackE);
	     Edge commonEdgeAsCharEdge = *(commonSet.begin());

	     //cout << "common Edge is :  " << commonEdgeAsCharEdge.getSourceState().getId() << " - " << commonEdgeAsCharEdge.getDestinationState().getId() << endl;  
             map<Edge,Edge>::iterator pos = backEdgeCharecteristicEdgeMap.find(effectiveBackE);
	     backEdgeCharecteristicEdgeMap.erase(pos);
	     backEdgeCharecteristicEdgeMap.insert(make_pair(effectiveBackE,commonEdgeAsCharEdge));
	   }
	   else
	   {
		   usefulBackEdges.insert(*(backEdges.begin()));
	   }

	}

	return usefulBackEdges;

}

void loopAnalysis::populateCommonHeaderBackEdgeMap(set<Edge>& backEdgeSet, set<State>& commonHeaders, 
		                                         map<State, list<Edge> >& setOfLoopHeaders)
{
    set<Edge>::iterator beBegin = backEdgeSet.begin();
    set<Edge>::iterator beEnd = backEdgeSet.end();
    for(; beBegin != beEnd; beBegin++)
    {
           State header = beBegin->getDestinationState();

	   //cout << beBegin->getSourceState().getId() << " - " << beBegin->getDestinationState().getId() << endl;
	   if(commonHeaders.find(header) != commonHeaders.end())
	   {
             map<State, list<Edge> >::iterator headerPos = setOfLoopHeaders.find(header);
             if(headerPos != setOfLoopHeaders.end())
             {
                     list<Edge> listWithSameHead = headerPos->second;
		     setOfLoopHeaders.erase(headerPos);
                     listWithSameHead.push_back(*beBegin);
                     setOfLoopHeaders.insert(make_pair(header, listWithSameHead));
             }
             else
             {
                     list<Edge> listWithSameHead;
                     listWithSameHead.push_back(*beBegin);
                     setOfLoopHeaders.insert(make_pair(header, listWithSameHead));
             }
	   }
    }
}

set<State> loopAnalysis::getSetOfHeaders(set<Edge>& backEdgeSet)
{
   set<State> headerSet;
   set<Edge>::iterator beBegin = backEdgeSet.begin();
   set<Edge>::iterator beEnd = backEdgeSet.end();
   for(; beBegin != beEnd; beBegin++)
   {
	   State header = beBegin->getDestinationState();
	   headerSet.insert(header);
   }
   return headerSet;
}

map<State, list<Edge> > loopAnalysis::getCommonMergedLoopsBackEdges(State & A, State & B)
{
    map<State, list<Edge> > setOfLoopHeaders;

    set<size_t>* loopset1 = getListOfLoops(A);
    set<size_t>* loopset2 = getListOfLoops(B);

    if(loopset1 && loopset2)
    {
      //the reason behind finding a backedge here is it will merge multiple loops in the CFG created
      ////from a single source code loop... almost all the cases such loops will have a common backedge
      //in only one case two loops in CFG creted from same loop will have two different CFG..
      set<Edge> set1 = getCommonBackEdges(loopset1); 
      set<Edge> set2 = getCommonBackEdges(loopset2);

      set<State> intersect;
      set<State> Hset1 = getSetOfHeaders(set1);
      set<State> Hset2 = getSetOfHeaders(set2);
      
      set_intersection(Hset1.begin(),Hset1.end(),Hset2.begin(),Hset2.end(),
     		                      std::inserter(intersect,intersect.begin()));

      populateCommonHeaderBackEdgeMap(set1,intersect,setOfLoopHeaders);
      populateCommonHeaderBackEdgeMap(set2,intersect,setOfLoopHeaders);

      return setOfLoopHeaders;
    }
    
    return setOfLoopHeaders;
}

list<Edge> loopAnalysis::getLexicographicOrderdedLoop(set<Edge>& loopSet)
{
    list<Edge> lexicoList;
    set<Edge>::iterator begIter = loopSet.begin();
    set<Edge>::iterator endIter = loopSet.end();
    for(;begIter != endIter; begIter++)
    {
        Edge back_edge = (*begIter);
        lexicoList.push_back(back_edge);
    }
    lexicoList.sort(loopAnalysis::loopLexicographicComparator);
    return lexicoList;
}

set<Edge> loopAnalysis::getCommonBackEdges(set<size_t>* loopSet)
{
	set<Edge> retSet;
	set<size_t> :: iterator loopStart = loopSet->begin();
	set<size_t> :: iterator loopEnd = loopSet->end();
        
	for(; loopStart != loopEnd; loopStart++)
	{
	    size_t thisLoop = (*loopStart);
	    Edge bckEdg = getBackEdge(thisLoop);
            retSet.insert(bckEdg);             
	}

	return retSet;
}
set<size_t>* loopAnalysis::getListOfLoops(State & s)
{
	map<State,set<size_t>* > :: iterator nodePos = nodeLoopMap.find(s);
	if(nodePos == nodeLoopMap.end())
	{
		return NULL;
	}
	else
	{
		//cout << "for " << s.getId() <<endl;
		return nodePos->second;
	}
}

size_t loopAnalysis::getLengthOfSmallestLoop(set<size_t>* loopSet)
{
       size_t minLength = 999999;
       set<size_t> :: iterator loopStart = loopSet->begin();
       set<size_t> :: iterator loopEnd = loopSet->end();
       for(;loopStart != loopEnd; loopStart++)
       {
	       size_t loop = (*loopStart);
               map<size_t,set<Edge>* > :: iterator lPos = loopEdgeMap.find(loop);
               size_t loop_length = lPos->second->size();
	       if(minLength > loop_length )
	       {
		       minLength = loop_length;
	       }
       }

       return minLength;
}

bool loopAnalysis::loopLexicographicComparator(Edge& backEdge1, Edge& backEdge2)
{
   //Edge backEdge1 = getBackEdge(loop1);
   //Edge backEdge2 = getBackEdge(loop2);
   //loop whos head of backedge goes to a lower numbered State, might be the outer loop
   State s1 = backEdge1.getDestinationState();
   State s2 = backEdge2.getDestinationState();
   if(s1.getId() < s2.getId())
   {
      return true;
   }
   // This means there is a tie... bothe loops has exactly same entry point..backedge going to same point
   //To resolve this, for now calculate the over all size of loop in terms of states in it
   //TODO: we need to find a better algorithm for this case..
   map<Edge, set<size_t>* > :: iterator loopsIterator1 = edgeLoopMap.find(backEdge1); //all the loops which has same backedge is basicaaly created from same source code loop
   map<Edge, set<size_t>* > :: iterator loopsIterator2 = edgeLoopMap.find(backEdge2);
   set<size_t>* loopSet1 = loopsIterator1->second;
   set<size_t>* loopSet2 = loopsIterator2->second;

   size_t length1 = getLengthOfSmallestLoop(loopSet1);
   size_t length2 = getLengthOfSmallestLoop(loopSet2);

   return (length1 > length2);
   
}


unsigned long loopAnalysis::getLoopIterations(size_t& loop, unsigned int proc)
{ 
    map<size_t, map<unsigned int,unsigned long> > :: iterator loopPos = resolvedLoopIterationMap.find(loop);
    if(loopPos != resolvedLoopIterationMap.end())
    {
        map<unsigned int,unsigned long> procIterMap = loopPos->second;
        map<unsigned int,unsigned long>::iterator procPos = procIterMap.find(proc);
	if(procPos != procIterMap.end())
	{
          return procPos->second;
	}
    }
    return 0;
}

