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
 * model_components.h
 *
 *  Created on: Dec 9, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *        Modified on: Nov 2, 2013
 *         *    Author: Subrata Mitra
 *          *    Contact: mitra4@purdue.edu
 *
 */

#ifndef MODEL_COMPONENTS_H_
#define MODEL_COMPONENTS_H_

//#if HAVE_CONFIG_H
#include "statetracker-config.h"
//#endif

#include "range_set.h"
#include "range_set_table.h"
#include <pthread.h>
#include <mpi.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include "debugging.h"

using namespace std;

class StateFactory;

class State {
private:
	unsigned int id;
public:
	~State(){};

	bool operator < (const State &s) const;
	bool operator == (const State &s) const;
	bool operator != (const State &s) const;

	string getString() const;
	unsigned int getId() const
	{
	    return id;
	};
	static State getStateFromString(const string &stateStr);

	friend class StateFactory;

	// MPI Pack and Unpack functions
	int packed_size(MPI_Comm comm) const;
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;
	static State unpack(void *buf, int bufsize, int *position, MPI_Comm comm);

	State(unsigned int i) : id(i) {};
//protected:
	State() : id(0) {};
};

// All public functions are thread safe.
class StateFactory {
private:
	map< string , State > nameToStateTable;
	map< State , string > stateToNameTable;

	pthread_mutex_t mutex;

public:
	StateFactory()
	{
		pthread_mutex_init(&mutex, NULL);
	};

	State createState(const string &name);

	// False if state or name is not found
	bool findAndGetState(State &state, const string &name);
	bool findAndGetName(string &name, const State &state);

	size_t getNumberOfElements();

	map< State , string >::iterator stateToNameTableStartIter()
	{
		return stateToNameTable.begin();
	};
	map< State , string >::iterator stateToNameTableEndIter()
	{
		return stateToNameTable.end();
	};
	
        void print(int rank);

        void copyStateFactory(StateFactory* copy)
	{
		ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
		map< string , State >::iterator startName = (this->nameToStateTable).begin();
		map< string , State >::iterator endName = (this->nameToStateTable).end();
		for(;startName != endName; startName++)
		{
			(copy->nameToStateTable).insert(make_pair(startName->first,startName->second));
		}
	        
                map< State , string > :: iterator startState = (this->stateToNameTable).begin();
                map< State , string >:: iterator endState =  (this->stateToNameTable).end();
                for(;startState != endState; startState++)
                {
                        (copy->stateToNameTable).insert(make_pair(startState->first,startState->second));
                }
                ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");	

	}

	// Function used to build a state from a trace
	void addEntry(const State &state, const string &name);

	//void dump(const char *path, int id);

	//static string convertAddressesToCodeLines(const string &callpath);

	// MPI Pack and Unpack functions
	int packed_size(MPI_Comm comm) const;
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;
	static StateFactory unpack(void *buf, int bufsize,
			int *position, MPI_Comm comm);
};

#if 0
class SnapShot {

     protected:
     SnapShot():memory(0) {};
     public:
     SnapShot(State& s, unsigned long mem) : state(s), memory(mem) {} ;
     State state;
     unsigned long memory;
     SnapShot(const SnapShot& snap)
     {
          state = snap.state;
          memory = snap.memory;
     };
     SnapShot& operator=(const SnapShot& snap)
     {
         state = snap.state;
         memory = snap.memory;
        return (*this);
     };
};
#endif

class EdgeAnnotation {


        unsigned long  transition_count;
	protected:
		std::map<unsigned long,RangeSetPtr> iterationTaskMap;
        public:
        EdgeAnnotation() :transition_count(0) {};
        EdgeAnnotation(unsigned long transition) :transition_count(transition) {};
        EdgeAnnotation(const EdgeAnnotation & edge);
        EdgeAnnotation& operator=(const EdgeAnnotation& ea);
        const EdgeAnnotation operator+(const EdgeAnnotation & ea) const;
	
	map<unsigned long,RangeSetPtr> getIterationTaskMap()
	{
		return iterationTaskMap;
	};
	void insertIterationCountForTask(unsigned long count,unsigned int task);
	void insertIterationCountForTask(unsigned long count,RangeSetPtr taskSet);
	unsigned long getIterationCountForTask(unsigned int task);
	RangeSetPtr getTaskSetForIterationCount(unsigned long count,bool& found);

        void setTransition(unsigned long);
        const unsigned long getTransition() const;

        // MPI Pack and Unpack functions
        int packed_size(MPI_Comm comm) const;
        void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;
        static EdgeAnnotation unpack(void *buf, int bufsize,
                        int *position, MPI_Comm comm);
       
	friend class Test_Loopaware_Dependency;
};


class Edge {
private:
	State source;
	State destination;
public:

	Edge(const State &src, const State &dst) :
		source(src), destination(dst) {};

	Edge (const Edge & edge) :
		source(edge.source),
		destination(edge.destination)
	{};

	Edge() : source(), destination()
	{
	};

	Edge& operator=(const Edge & edge);

	void setSourceState(const State &src);
	void setDestinationState(const State &dst);

	const State getSourceState() const;
	const State getDestinationState() const;

	/**
	 * Less-than operator. Performs a comparison in the form x < y.
	 * @param edge Edge to compare to
	 * @return True if x's source state < y's source state.
	 * If both x and y source state are equal,
	 * returns true if x's destination state < y's destination state.
	 * Otherwise, returns false.
	 */
	bool operator < (const Edge & edge) const;

	bool operator == (const Edge & edge) const;
	bool operator != (const Edge & edge) const;

	int packed_size(MPI_Comm comm) const;
	void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;
	static Edge unpack(void *buf, int bufsize, int *position, MPI_Comm comm);
};

class EdgeInfoContainer {
        public:
                Edge theEdge;
                EdgeAnnotation annotation;
                EdgeInfoContainer(const State& src, const State& dst ) : theEdge(src,dst) {};
                EdgeInfoContainer(const Edge& e, const EdgeAnnotation& ea) : theEdge(e), annotation(ea) {};
                EdgeInfoContainer(const EdgeInfoContainer & other) : theEdge(other.theEdge), annotation(other.annotation) {};
                bool operator < (const EdgeInfoContainer & edgeContainer) const;
                EdgeInfoContainer& operator=(const EdgeInfoContainer& eic);
                bool operator == (const EdgeInfoContainer & edgeContainer) const;
                bool operator != (const EdgeInfoContainer & edgeContainer) const;
                int packed_size(MPI_Comm comm) const;
                void pack(void *buf, int bufsize, int *position, MPI_Comm comm) const;
                static EdgeInfoContainer unpack(void *buf, int bufsize, int *position, MPI_Comm comm);

};

#endif /* MODEL_COMPONENTS_H_ */
