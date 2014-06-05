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
 * markov_model.h
 *
 *  Created on: Jan 18, 2012
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *
 *  Modified on: Nov 2, 2013
 *  Author: Subrata Mitra
 *  Contact: mitra4@purdue.edu
 */

#ifndef MARKOV_MODEL_H_
#define MARKOV_MODEL_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "debugging.h"
#include "model_components.h"
#include "backtrace.h"
#include "utilities.h"

#include <pthread.h>

#include <map>
#include <set>
#include <utility>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
static inline void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63)
{
    fprintf(out, "stack trace:\n");

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
	fprintf(out, "  <empty, possibly corrupt>\n");
	return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++)
    {
	char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

	// find parentheses and +address offset surrounding the mangled name:
	// ./module(function+0x15c) [0x8048a6d]
	for (char *p = symbollist[i]; *p; ++p)
	{
	    if (*p == '(')
		begin_name = p;
	    else if (*p == '+')
		begin_offset = p;
	    else if (*p == ')' && begin_offset) {
		end_offset = p;
		break;
	    }
	}

	if (begin_name && begin_offset && end_offset
	    && begin_name < begin_offset)
	{
	    *begin_name++ = '\0';
	    *begin_offset++ = '\0';
	    *end_offset = '\0';

	    // mangled name is now in [begin_name, begin_offset) and caller
	    // offset in [begin_offset, end_offset). now apply
	    // __cxa_demangle():

	    int status;
	    char* ret = abi::__cxa_demangle(begin_name,
					    funcname, &funcnamesize, &status);
	    if (status == 0) {
		funcname = ret; // use possibly realloc()-ed string
		fprintf(out, "  %s : %s+%s\n",
			symbollist[i], funcname, begin_offset);
	    }
	    else {
		// demangling failed. Output function name as a C function with
		// no arguments.
		fprintf(out, "  %s : %s()+%s\n",
			symbollist[i], begin_name, begin_offset);
	    }
	}
	else
	{
	    // couldn't parse the line? print the whole line.
	    fprintf(out, "  %s\n", symbollist[i]);
	}
    }

    free(funcname);
    free(symbollist);
}

class Test_MarkovModel;
template<class T> class ProbabilityMatrixIterator;
template<class T> class MarkovModelIterator;

template<class T>
class EdgeInfo {
public:
    const T* edgeSource;
    const T* edgeDestination;
    EdgeAnnotation edgeAnnotation;
    EdgeInfo(const T* src, const T* dst, EdgeAnnotation ea):edgeSource(src),edgeDestination(dst),edgeAnnotation(ea)
    {
    }

};

template<class T>
class ProbabilityMatrix {
private:
	std::map < T, std::map< T, EdgeAnnotation > > transitions;
	T lastState;

public:
	// Returns TRUE if a similar transition has been seen before;
	// Return FALSE otherwise.
	bool updateMatrix(const T &src, const T &dst, const EdgeAnnotation & edgeAnnot)
	{
		bool ret = false;
		typename std::map < T, std::map< T, EdgeAnnotation > >::iterator it =
				transitions.find(src);

		if (it == transitions.end()) {
			// Source was not found in the table
			std::map< T, EdgeAnnotation > newMap;
			newMap.insert(typename std::pair<T, EdgeAnnotation>(dst, edgeAnnot));
			transitions.insert(
					std::pair<T, std::map< T, EdgeAnnotation > >(src, newMap));
		} else {
			typename std::map< T, EdgeAnnotation >::iterator jt =
					it->second.find(dst);
			if (jt == it->second.end()) {
				// Destination was not found
				it->second.insert(std::pair<T, EdgeAnnotation>(dst, edgeAnnot));
			} else {
			       EdgeAnnotation oldAnnot = jt->second;
			       oldAnnot = oldAnnot + edgeAnnot;
			       it->second.erase(jt);
				(it->second).insert(std::pair<T,EdgeAnnotation>(dst,oldAnnot)); // just increase the values
				ret = true;
			}
		}
		// Update last state
		lastState = dst;

		return ret;
	}

	double probability(const T &src, const T &dst) const
	{
		typename std::map < T, std::map< T, EdgeAnnotation > >::const_iterator it =
				transitions.find(src);
		// If state is unknown, probability is zero
		if (it == transitions.end())
			return 0;

		typename std::map< T, EdgeAnnotation >::const_iterator jt =
				it->second.find(dst);
		if (jt == it->second.end())
			return 0;

		unsigned long long totalNumTransitions = 0;
		typename std::map< T, EdgeAnnotation >::const_iterator i;
		for (i = it->second.begin(); i != it->second.end(); i++)
		{
			EdgeAnnotation ea = i->second;
			totalNumTransitions += ea.getTransition();

		}
		EdgeAnnotation annot = jt->second;
		unsigned long long numTransitions = annot.getTransition();
		return (double)(numTransitions)/(double)(totalNumTransitions);
	}

	unsigned long transitionCount(const T &src, const T &dst) const
        {
                typename std::map < T, std::map< T, EdgeAnnotation > >::const_iterator it =
                                transitions.find(src);
                // If state is unknown, probability is zero
                if (it == transitions.end())
                        return 0;

                typename std::map< T, EdgeAnnotation >::const_iterator jt =
                                it->second.find(dst);
                if (jt == it->second.end())
                        return 0;

                unsigned long numTransitions = (jt->second).getTransition();
                return numTransitions;
        }

        EdgeAnnotation getEdgeAnnotation(const T &src, const T &dst) const
        {
		EdgeAnnotation ea;
                typename std::map < T, std::map< T, EdgeAnnotation > >::const_iterator it =
                                transitions.find(src);
                // If state is unknown, probability is zero
                if (it == transitions.end())
                        return ea;

                typename std::map< T, EdgeAnnotation >::const_iterator jt =
                                it->second.find(dst);
                if (jt == it->second.end())
                        return ea;

                return jt->second;
        }

	void updateTransitionCountTaskMap(unsigned int rank)
	{
		typename std::map < T, std::map< T, EdgeAnnotation > > update_map;
		
		typename std::map < T, std::map< T, EdgeAnnotation > >::iterator it_beg =  transitions.begin();
                typename std::map < T, std::map< T, EdgeAnnotation > >::iterator it_end =  transitions.end();

                for(;it_beg != it_end;it_beg++)
 		{
			T srcState = it_beg->first;
                 	std::map< T, EdgeAnnotation >  updated_transitions;
                        typename std::map< T, EdgeAnnotation >::iterator jt_beg = it_beg->second.begin();
                        typename std::map< T, EdgeAnnotation >::iterator jt_end = it_beg->second.end();
                        for (;jt_beg != jt_end; jt_beg++) 
			{
			       T thisState = jt_beg->first;
                               EdgeAnnotation oldAnnot = jt_beg->second;
			       EdgeAnnotation newAnnot = oldAnnot;
			       unsigned long transCount = oldAnnot.getTransition();
			       newAnnot.insertIterationCountForTask(transCount,rank);
			       updated_transitions.insert(pair<T,EdgeAnnotation>(thisState,newAnnot));
                        }
			update_map.insert(pair<T,map< T, EdgeAnnotation > >(srcState,updated_transitions));
                }

		transitions = update_map;

	}

	T getLastState()
	{
		return lastState;
	}
	void setLastState(T& state)
	{
		//this function should be only called while copying MM
		lastState = state;
	}

	friend class Test_MarkovModel;
	friend class ProbabilityMatrixIterator<T>;
};

/**
 * Helps to iterate over a probability matrix.
 * This is useful when we want to iterate over all the transitions of
 * a Markov model.
 *
 */
template<class T>
class ProbabilityMatrixIterator
{
private:
	const ProbabilityMatrix<T> *pMatrix;
	typename std::map < T, std::map< T, EdgeAnnotation > >::const_iterator it;
	typename std::map < T, EdgeAnnotation >::const_iterator jt;
public:
	ProbabilityMatrixIterator<T> (const ProbabilityMatrix<T> *p) :
		pMatrix(p) {};

	// First transition
    void firstTrans()
    {
    	it = pMatrix->transitions.begin();
    	jt = it->second.begin();
    };

    // Next transition
    void nextTrans()
    {
    	jt++;
    	if (jt == it->second.end()) {
    		it++;
    		if (it != pMatrix->transitions.end())
    			jt = it->second.begin();
    	}
    };

    // Are we done iterating?
    bool isDone() const
    {
    	return (it == pMatrix->transitions.end());
    };

    EdgeInfo<T> currentTrans()   
    {
         EdgeAnnotation edgeAnnote(jt->second);
	 EdgeInfo<T> edg(&(it->first), &(jt->first),edgeAnnote);
	return edg;
    };
};


template<class T>
class MarkovModel {
private:
	typedef typename
			boost::adjacency_list <
			boost::setS,
			boost::vecS,
			boost::directedS,
			T // All vertices are of type T
			> Graph;
	typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;

	Graph graph;
	ProbabilityMatrix<T> matrix;
	std::map<T, Vertex > states;
	std::map<Vertex, T > statesInv; // inverse table of states
	std::set<Vertex> visitedVertices; // set of states to detect cycles
	std::vector<Vertex> vertexSequence;
	std::vector< std::vector<Vertex> > paths;

	typedef boost::adjacency_list<> tc_type;
	tc_type *transitiveClosure;
	bool useClosure;

	pthread_mutex_t mutex;

	Vertex addVertex(const T &s)
	{
		Vertex ret;
		typename std::map<T, Vertex>::const_iterator it = states.find(s);
		if (it == states.end()) {
			ret = boost::add_vertex(s, graph);
			states.insert(std::pair<T, Vertex>(s,ret));
			statesInv.insert(std::pair<Vertex,T>(ret,s));
		} else {
			ret = it->second;
		}

		return ret;
	};

	bool vertexIsKnown(const Vertex &v)
	{
		typename std::set<Vertex>::const_iterator it = visitedVertices.find(v);
		bool ret = (it == visitedVertices.end()) ? false : true;
		if (!ret) {
			visitedVertices.insert(v);
			vertexSequence.push_back(v);
		}
		return ret;
	};

	void eliminateFromKnownSet(const Vertex &v)
	{
		typename std::set<Vertex>::iterator it = visitedVertices.find(v);
		if (it != visitedVertices.end()) {
			visitedVertices.erase(it);
			vertexSequence.pop_back();
		}
	};

	// Recursive Depth-First Search
	void DFSearch(const Vertex &v, const Vertex &u)
	{
		static int recurCount = 0;
		recurCount++;
		if(recurCount > 1000)
		{
			return; //do not go into infinite recursion!
		}
		//cout << "dfs" << statesInv[v].getId() << "," << statesInv[u].getId() << endl;
		if (vertexIsKnown(v))
			return; // cycle is detected

		if (v == u) {
			paths.push_back(vertexSequence); // save path
			eliminateFromKnownSet(v);
			return;
		}

		//std::cout << "node: " << graph[v].getString() << std::endl;
		typename boost::graph_traits<Graph>::adjacency_iterator ai, ai_end;
		for (tie(ai, ai_end) = adjacent_vertices(v, graph); ai != ai_end; ++ai)
			DFSearch(*ai, u);

		eliminateFromKnownSet(v);
	};

	std::vector<double> vectorOfProbabilities()
	{
		// -- print path -------
		/*for (size_t i = 0; i < paths.size(); ++i) {
			for (size_t j=0; j < paths[i].size(); ++j) {
				Vertex x = paths[i][j];
				std::cout << graph[x].getString();
				if (j != paths[i].size()-1)
					std::cout << " --> ";
			}
			std::cout << std::endl;
		}*/
		// ---------------------

		std::vector<double> prob;
		for (size_t i = 0; i < paths.size(); ++i) {
			Vertex x = paths[i][0];
			double tmpProb = 1;
			for (size_t j=1; j < paths[i].size(); ++j) {
				Vertex y = paths[i][j];

				typename std::map<Vertex,T>::const_iterator it =
						statesInv.find(x);
				typename std::map<Vertex,T>::const_iterator jt =
						statesInv.find(y);

				tmpProb *= matrix.probability(it->second, jt->second);
				x = y;
			}
			prob.push_back(tmpProb);
		}

		return prob;
	};

	bool sourceCanReachDestination(const Vertex &src, const Vertex &dst)
	{
		// Create transitive closure only one time (if pointer is NULL)
		if (!transitiveClosure) {
			transitiveClosure = new tc_type();
			boost::transitive_closure(graph, *transitiveClosure);
		}

		bool ret = false;
		typename boost::graph_traits<tc_type>::adjacency_iterator ai, ai_end;
		for (tie(ai, ai_end) = adjacent_vertices(src, *transitiveClosure);
				ai != ai_end; ++ai)
		{
			if (*ai==dst) {
				ret = true;
				break;
			}
		}

		return ret;
	};

public:
	MarkovModel(bool flag=false) : transitiveClosure(NULL), useClosure(flag)
	{
		pthread_mutex_init(&mutex, NULL);
	};

	void addEdge(const T &src, const T &dst, const EdgeAnnotation & edgeAnnot )
	{
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		Vertex x = addVertex(src);
		Vertex y = addVertex(dst);

		if (!matrix.updateMatrix(src, dst, edgeAnnot))
			boost::add_edge(x, y, graph);

		// Invalidate transitive closure when adding an edge
		if (useClosure && (transitiveClosure != NULL)) {
			delete transitiveClosure;
			transitiveClosure = NULL;
		}

		ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
	};

	void updateEdgeTransitionCountTaskMap(unsigned int rank) 
	{
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		matrix.updateTransitionCountTaskMap(rank);
		ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
	}

	// Calculates the probabilities of going from source to destination
	// by evaluating all the possible paths.
	// It returns the largest probability.
	double largestPathProbability(const T &src, const T &dst)
	{
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		typename std::map<T, Vertex >::const_iterator it = states.find(src);
		typename std::map<T, Vertex >::const_iterator jt = states.find(dst);

		// If a state is not found the path probability is 0
		if (it == states.end() || jt == states.end() || src==dst) {
			ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
			return 0;
		}

		// Clear stateful data structures
		visitedVertices.clear();
		vertexSequence.clear();
		paths.clear();
                
		if (useClosure) {
			if (sourceCanReachDestination(it->second, jt->second))
				DFSearch(it->second, jt->second);
		} else {
			DFSearch(it->second, jt->second);
		}

		std::vector<double> prob = vectorOfProbabilities();
		double ret = (prob.size()==0)?
				0 : *(std::max_element(prob.begin(), prob.end()));
		ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
		return ret;
	};

	// Calculates the probabilities of going from source to destination
	// by evaluating all the possible paths.
	// It returns the sum of all probabilities. (it should be <= 1.0)
	double transitionProbability(const T &src, const T &dst)
	{
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		typename std::map<T, Vertex >::const_iterator it = states.find(src);
		typename std::map<T, Vertex >::const_iterator jt = states.find(dst);

		// If a state is not found the path probability is 0
		if (it == states.end() || jt == states.end() || src==dst) {
			cout << "could not found: " << src.getId() << "--" << dst.getId(); 
			ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
			return 0;
		}

		// Clear stateful data structures
		visitedVertices.clear();
		vertexSequence.clear();
		paths.clear();
		
		//cout << "use closure:" <<useClosure <<":" << endl;

		if (useClosure) {
			if (sourceCanReachDestination(it->second, jt->second))
				DFSearch(it->second, jt->second);
		} else {
			DFSearch(it->second, jt->second);
		}

		std::vector<double> prob = vectorOfProbabilities();
		double ret=0;
		for (size_t i=0; i < prob.size(); ++i)
			ret += prob[i];
		ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
		return ret;
	};

	/**
	 * Calculates the direct probability of going from source
	 * to destination. It does not evaluate all possible paths; only
	 * the direct path.
	 */
	double directProbability(const T &src, const T &dst)
	{
		double ret=0;
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		ret = matrix.probability(src, dst);
		ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
		return ret;
	}

	EdgeAnnotation getEdgeAnnotation(const T& src, const T& dst)
	{
	    EdgeAnnotation ea;
            ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
            ea = matrix.getEdgeAnnotation(src, dst);
            ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
            return ea;
	}

        unsigned long getIterationCountForEdgeAndProcess(Edge& edg, unsigned int proc)
        {
              EdgeAnnotation ea = getEdgeAnnotation(edg.getSourceState(), edg.getDestinationState());
	      return ea.getIterationCountForTask(proc);
        }

	T getLastState()
	{
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		T ret = matrix.getLastState();
		ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
		return ret;
	}
        void setLastState(T& state)
        {
                ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
                matrix.setLastState(state);
                ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
        }

	friend class Test_MarkovModel;
	friend class MarkovModelIterator<T>;

       void copyMarkovModel(MarkovModel<T>* copy)
        {
	   ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
          ProbabilityMatrixIterator<T> it(&(this->matrix));
          //MarkovModelIterator<State> it(original);
          for (it.firstTrans(); !it.isDone(); it.nextTrans())
          {
                EdgeInfo<State> e = it.currentTrans();
                Edge edge(*(e.edgeSource), *(e.edgeDestination));
                EdgeAnnotation annot(e.edgeAnnotation);
                copy->addEdge(edge.getSourceState(), edge.getDestinationState(),annot);
           }
	  
           //T last_state = this->getLastState();
	   T last_state = (this->matrix).getLastState();
           copy->setLastState(last_state);
	   ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");

         }

};

template<class T>
class MarkovModelIterator
{
private:
	ProbabilityMatrixIterator<T> it;

public:
	MarkovModelIterator<T> (const MarkovModel<T> *m) : it(&(m->matrix)) {};

	// First transition
    void firstTrans()
    {
    	it.firstTrans();
    };

    // Next transition
    void nextTrans()
    {
    	it.nextTrans();
    };

    // Are we done iterating?
    bool isDone() const
    {
    	return it.isDone();
    };

    //std::pair<const T*, const T*> currentTrans() const
    EdgeInfo<T> currentTrans() 
    {
    	return it.currentTrans();
    };
};

#endif /* MARKOV_MODEL_H_ */
