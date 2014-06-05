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
 * model_components.C
 *
 *  Created on: Dec 9, 2011
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

#include "model_components.h"
#include "debugging.h"
#include "utilities.h"
#include "backtrace.h"

#include <stdio.h>
#include <pthread.h>
#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <mpi.h>

using namespace std;

/*-----------------------------------------------------------------*/
/*-- State class methods                                           */
/*-----------------------------------------------------------------*/

string State::getString() const
{
	char str[50];
	sprintf(str, "%d", (int)(id));
	string ret(str);
	return ret;
}

State State::getStateFromString(const string & stateStr)
{
	unsigned int id = atoi(stateStr.c_str());
	State ret(id);
	return ret;
}

bool State::operator < (const State &state) const
{
	return (id < state.id);
}

bool State::operator == (const State &state) const
{
	return (id == state.id);
}

bool State::operator != (const State &state) const
{
	return (!(id == state.id));
}

int State::packed_size(MPI_Comm comm) const
{
	int size = 0;

	// Size of id
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &size);

	return size;
}

// Pack it via MPI
void State::pack(void *buf, int bufsize, int *position, MPI_Comm comm) const
{
	// Pack id
	PMPI_Pack((void *)&id, 1, MPI_UNSIGNED, buf, bufsize, position, comm);
}

// Unpack it via MPI
State State::unpack(void *buf, int bufsize, int *position, MPI_Comm comm)
{
	unsigned int i;
	PMPI_Unpack(buf, bufsize, position, &i, 1, MPI_UNSIGNED, comm);
	State ret(i);
	return ret;
}

/*-----------------------------------------------------------------*/
/*-- StateFactory class methods                                    */
/*-----------------------------------------------------------------*/
State StateFactory::createState(const string &name)
{
	ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
	State ret;
	map<string, State>::const_iterator it = nameToStateTable.find(name);
	if (it == nameToStateTable.end()) {
		unsigned int tmpId =
				static_cast<unsigned int>(nameToStateTable.size()+1);
		State s(tmpId);
		nameToStateTable.insert(pair<string, State>(name, s));
		stateToNameTable.insert(pair<State, string>(s, name));
		ret = s;
	} else {
		ret = it->second;
	}

	ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
	return ret;
}

bool StateFactory::findAndGetState(State &state, const string &name)
{
	ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
	bool ret = false;
	map<string, State>::const_iterator it = nameToStateTable.find(name);
	if (it != nameToStateTable.end()) {
		state = it->second;
		ret = true;
	}

	ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
	return ret;
}

bool StateFactory::findAndGetName(string &name, const State &state)
{
	ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
	bool ret = false;
	map<State, string>::const_iterator it = stateToNameTable.find(state);
	if (it != stateToNameTable.end()) {
		name = it->second;
		ret = true;
	}

	ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
	return ret;
}

int StateFactory::packed_size(MPI_Comm comm) const
{
	int size=0, tmp=0;

	PMPI_Pack_size(1, MPI_INT, comm, &tmp); // number of elements in table
	size += tmp;

	map<State, string>::const_iterator it;
	for (it = stateToNameTable.begin(); it != stateToNameTable.end(); it++) {
		size += it->first.packed_size(comm); // state size

		PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp); // number of characters
		size += tmp;

		int strSize = static_cast<int>(it->second.size());
		PMPI_Pack_size(strSize, MPI_CHAR, comm, &tmp); // string
		size += tmp;
	}

	return size;
}

void StateFactory::pack(void *buf, int bufsize,
		int *position, MPI_Comm comm) const
{
	// pack number of elements
	unsigned int n = static_cast<unsigned int>(stateToNameTable.size());
	PMPI_Pack((void *)&n, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

	map<State, string>::const_iterator it;
	for (it = stateToNameTable.begin(); it != stateToNameTable.end(); it++) {
		it->first.pack(buf, bufsize, position, comm); // pack state

		// pack string name
		int num = static_cast<unsigned int>(it->second.size());
		PMPI_Pack((void *)&num, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

		PMPI_Pack((void *)(it->second.c_str()), num, MPI_CHAR,
				buf, bufsize, position, comm);
	}
}

StateFactory StateFactory::unpack(void *buf, int bufsize,
		int *position, MPI_Comm comm)
{
	unsigned int n = 0;
	PMPI_Unpack(buf, bufsize, position, &n, 1, MPI_UNSIGNED, comm);

	StateFactory sFactory;

	for (unsigned int i = 0; i < n; ++i) {
		// unpack state
		State state = State::unpack(buf, bufsize, position, comm);

		// unpack string
		unsigned int strSize;
		PMPI_Unpack(buf, bufsize, position, &strSize, 1, MPI_UNSIGNED, comm);

		char buffer[strSize+1];
		buffer[0] = '\0';
		PMPI_Unpack(buf, bufsize, position, (void *)buffer,
				strSize, MPI_CHAR, comm);
		buffer[strSize] = '\0'; // make sure the string is null-terminated
		string name(buffer);

		// update tables
		sFactory.nameToStateTable.insert(pair<string, State>(name, state));
		sFactory.stateToNameTable.insert(pair<State, string>(state, name));
	}

	return sFactory;
}

void StateFactory::print(int rank)
{
      map<State, string>::const_iterator it;
      for (it = stateToNameTable.begin(); it != stateToNameTable.end(); it++) {
           cout << " For rank = " << rank  <<" State = " << ((*it).first).getString() << " Name = " << (*it).second << endl;

	}
}

/*void StateFactory::dump(const char *path, int id)
{
	char buff[50];
	sprintf(buff, "%d", id);
	string timeStr(buff);

	string file_name = string(path) +
			string("/trace_") + string("table_") + timeStr;
	if (file_name.size() == 0)
		handleError("in StateFactory::dump(): the file name is empty.");

	FILE *file;

	// Check that at least one transition exists.
	if (stateToNameTable.size() <= 0)
		handleError("in StateFactory::dump(): "
				"cannot write an empty table into a file.");

	// Opening file to output dot graph file
	file = fopen(file_name.c_str(), "w");
    if (file == (FILE *)NULL) {
    	string msg = string("in StateFactory::dump(): could not open file ") +
    			file_name;
    	handleError(msg.c_str());
	}

    map<State, string>::const_iterator it;
    for (it = stateToNameTable.begin(); it != stateToNameTable.end(); it++) {
    	string callpath = it->second.c_str();
    	//callpath = convertAddressesToCodeLines(callpath);
    	fprintf(file, "%s,%s\n", it->first.getString().c_str(),
    			callpath.c_str());
    }

	fclose(file);
}*/

/*string StateFactory::convertAddressesToCodeLines(const string &callpath)
{
	// Split address and create a string of addresses
	// separated by spaces
	vector<string> tokens;
	Tokenize(callpath, tokens, "|");
	string spaceSeparated = "";
	for (size_t i=0; i < tokens.size(); i++)
		spaceSeparated = spaceSeparated + " " + tokens[i];

	string codeLines = Backtrace::convertAddrToLine(spaceSeparated);
	// split by '\n'
	tokens.clear();
	char buf[1];
	buf[0] = '\n';
	Tokenize(codeLines, tokens, buf);

	string ret = "";
	for (size_t i=0; i < tokens.size(); i++)
		ret = ret + "|" + tokens[i];

	//cout << "RETURN=" << ret << endl;
	//cout << "CALLPATH=" << callpath << endl;

	return ret;
}*/

void StateFactory::addEntry(const State &state, const string &name)
{
	ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
	stateToNameTable.insert(pair<State, string>(state, name));
	nameToStateTable.insert(pair<string, State>(name, state));
	ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
}

size_t StateFactory::getNumberOfElements()
{
	ASSERT(pthread_mutex_lock(&mutex) == 0, "Lock obtained.");
	size_t ret = stateToNameTable.size();
	ASSERT(pthread_mutex_unlock(&mutex) == 0, "Lock released.");
	return ret;
}

/*-------------------------------------------------------------------*/
/*-- Edge class functions                                          --*/
/*-------------------------------------------------------------------*/

Edge& Edge::operator=(const Edge & edge) {
	source = edge.source;
	destination =  edge.destination;

	return *this;
}

void Edge::setSourceState(const State &src)
{
	source = src;
}

void Edge::setDestinationState(const State &dst)
{
	destination = dst;
}

const State Edge::getSourceState() const
{
	return source;
}

const State Edge::getDestinationState() const
{
	return destination;
}
EdgeAnnotation::EdgeAnnotation(const EdgeAnnotation & edge)
{
  transition_count = edge.transition_count;
  iterationTaskMap = edge.iterationTaskMap;
}

EdgeAnnotation& EdgeAnnotation::operator=(const EdgeAnnotation& ea)
{
    transition_count = ea.transition_count;
    iterationTaskMap = ea.iterationTaskMap;
    return(*this);
}
const EdgeAnnotation EdgeAnnotation::operator+ (const EdgeAnnotation& ea) const
{
     EdgeAnnotation newEdgeAnnot(*this);
     unsigned long total = transition_count + ea.transition_count;
     newEdgeAnnot.transition_count = total;

     map<unsigned long,RangeSetPtr > :: const_iterator iterCountBeg = (ea.iterationTaskMap).begin();
     map<unsigned long,RangeSetPtr > :: const_iterator iterCountEnd = (ea.iterationTaskMap).end();
     for(;iterCountBeg != iterCountEnd; iterCountBeg++)
     { 
           unsigned long iter_count = (*iterCountBeg).first;
	   RangeSetPtr taskSet = (*iterCountBeg).second;
	   newEdgeAnnot.insertIterationCountForTask(iter_count,taskSet);

     }
     return newEdgeAnnot;
}

void EdgeAnnotation::insertIterationCountForTask(unsigned long count, RangeSetPtr taskSet)
{

	map<unsigned long,RangeSetPtr > :: iterator pos = iterationTaskMap.find(count);
	if(pos == iterationTaskMap.end())
	{       
		RangeSetPtr rsPtr(new RangeSet(*taskSet));
		iterationTaskMap.insert(pair<unsigned long,RangeSetPtr >(count,rsPtr));
	}
	else
	{
		RangeSetPtr prevTaskSet = pos->second;
		string info;
                //info = "prev:" + prevTaskSet->toString() + "new:" + taskSet->toString();

		(*prevTaskSet) += (*taskSet);
		
		//info = "result:" + prevTaskSet->toString();
                //cout << " count "<< count << " for " << info << endl;
		iterationTaskMap.insert(pair<unsigned long,RangeSetPtr >(count,prevTaskSet));
	}
}

void EdgeAnnotation::insertIterationCountForTask(unsigned long count, unsigned int task)
{
	RangeSetPtr rsPtr(new RangeSet(task));
        map<unsigned long,RangeSetPtr > :: iterator pos = iterationTaskMap.find(count);
        if(pos == iterationTaskMap.end())
        {
                iterationTaskMap.insert(pair<unsigned long,RangeSetPtr >(count,rsPtr));
        }
        else
        {
                RangeSetPtr prevTaskSet = pos->second;
		string info;
		//info = "prev:" + prevTaskSet->toString() + "new:" + rsPtr->toString();
                (*prevTaskSet) += (*rsPtr);
		//info = "result:" + prevTaskSet->toString();
	   	//cout << " count "<< count << " for " << info << endl; 
                iterationTaskMap.insert(pair<unsigned long,RangeSetPtr >(count,prevTaskSet));
        }
}

unsigned long EdgeAnnotation::getIterationCountForTask(unsigned int task)
{
	std::map<unsigned long,RangeSetPtr> :: iterator taskStart = iterationTaskMap.begin();
	std::map<unsigned long,RangeSetPtr> :: iterator taskEnd = iterationTaskMap.end();
	for(; taskStart != taskEnd; taskStart++)
	{
		RangeSetPtr rsptr = taskStart->second;
		bool isTaskPresent = rsptr->isPresent(task);
		if(isTaskPresent)
		{
			return taskStart->first;
		}
	}
	return 0;
}
RangeSetPtr EdgeAnnotation::getTaskSetForIterationCount(unsigned long count, bool& found)
{
	map<unsigned long,RangeSetPtr > :: iterator pos = iterationTaskMap.find(count);
	if(pos == iterationTaskMap.end())
	{
	     found = false;
	     return RangeSetPtr();
	}
	else
	{
		found = true;
		return pos->second;
	}

}

void EdgeAnnotation::setTransition(unsigned long trans)
{
  transition_count = trans;
}
#if 0
EdgeAnnotation EdgeAnnotation::createEdgeAnnotation(SnapShot srcSnap, SnapShot dstSnap)
{
     //long double memDelta = (long double)(long long)(dstSnap.memory - srcSnap.memory);
     EdgeAnnotation ea(1);
     //ea.insertMemoryValues(memDelta);
     return ea;
}
#endif
const unsigned long EdgeAnnotation::getTransition() const
{
  return transition_count;
}

int EdgeAnnotation::packed_size(MPI_Comm comm) const
{
        int size=0;

        PMPI_Pack_size(1, MPI_UNSIGNED_LONG, comm, &size); // number of transition
	int count = 0;
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &count); // number of entries in transition map
	size += count;
	unsigned int numCounters = (unsigned int)(iterationTaskMap.size());
        int mapSize = 0;
        PMPI_Pack_size(numCounters, MPI_UNSIGNED_LONG, comm, &mapSize); // size of keys in transition map
        size += mapSize;
	std::map < unsigned long,RangeSetPtr > :: const_iterator taskStart = this->iterationTaskMap.begin();
	std::map < unsigned long,RangeSetPtr > :: const_iterator taskEnd = this->iterationTaskMap.end();
	for(;taskStart != taskEnd; taskStart++)
	{
             size += taskStart->second->packed_size(comm);
	}

        return size;
}

void EdgeAnnotation::pack(void *buf, int bufsize,
                int *position, MPI_Comm comm) const
{
        // pack number of elements
        PMPI_Pack((void *)&transition_count, 1, MPI_UNSIGNED_LONG, buf, bufsize, position, comm);


        unsigned int numValues = (unsigned int)(iterationTaskMap.size());
        PMPI_Pack((void *)&numValues, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

	map < unsigned long,RangeSetPtr > :: const_iterator taskStart = iterationTaskMap.begin(), taskEnd = iterationTaskMap.end();
        for(;taskStart != taskEnd; taskStart++)
        {    
		unsigned long counter = taskStart->first;        
                PMPI_Pack((void *)&counter, 1, MPI_UNSIGNED_LONG, buf, bufsize, position, comm);
		taskStart->second->pack(buf,bufsize,position,comm);
        }
}

EdgeAnnotation EdgeAnnotation::unpack(void *buf, int bufsize,
                int *position, MPI_Comm comm)
{
        unsigned long numTrans = 0;
        PMPI_Unpack(buf, bufsize, position, &numTrans, 1, MPI_UNSIGNED_LONG, comm);

        EdgeAnnotation ea(numTrans);


	unsigned int numValues = 0;
        PMPI_Unpack(buf, bufsize, position, &numValues, 1, MPI_UNSIGNED, comm);

        for (unsigned int i = 0; i < numValues; ++i) {
                // unpack long double
                unsigned long counter;
                PMPI_Unpack(buf, bufsize, position, &counter, 1, MPI_UNSIGNED_LONG, comm);
		RangeSet rs = RangeSet::unpack(buf,bufsize,position,comm);
		RangeSetPtr ptr(new RangeSet(rs));
		ea.insertIterationCountForTask(counter,ptr);
        }

        return ea;
}

bool Edge::operator < (const Edge &e) const
{
	bool ret = true;

	if (source < e.source) { // is x < y?
		ret = true;
	} else if (source == e.source) { // is x == y?
		if (destination < e.destination)
			ret = true;
		else
			ret = false;
	} else {
		ret = false;
	}

	return ret;
}

bool Edge::operator == (const Edge &e) const
{
	bool ret = (source == e.source && destination == e.destination) ?
			true : false;
	return ret;
}

bool Edge::operator != (const Edge &e) const
{
	return !(this->operator ==(e));
}

int Edge::packed_size(MPI_Comm comm) const
{
	int size=0;

	size += source.packed_size(comm);
	size += destination.packed_size(comm);

	return size;
}

void Edge::pack(void *buf, int bufsize, int *position, MPI_Comm comm) const
{
	source.pack(buf, bufsize, position, comm);
	destination.pack(buf, bufsize, position, comm);
}

Edge Edge::unpack(void *buf, int bufsize, int *position, MPI_Comm comm)
{
	State source = State::unpack(buf, bufsize, position, comm);
	State destination = State::unpack(buf, bufsize, position, comm);

	Edge e(source, destination);
	return e;
}

EdgeInfoContainer& EdgeInfoContainer::operator=(const EdgeInfoContainer& eic)
{
    theEdge = eic.theEdge;
    annotation = eic.annotation;
    return (*this);
}

bool EdgeInfoContainer::operator < (const EdgeInfoContainer & eic) const
{
      bool ret = (theEdge < eic.theEdge) ? true : false;
}

bool EdgeInfoContainer::operator == (const EdgeInfoContainer & edgeContainer) const
{
        bool ret = (theEdge == edgeContainer.theEdge) ?
                        true : false;
        return ret;
}
bool EdgeInfoContainer::operator != (const EdgeInfoContainer & eic) const
{
         return !(this->operator ==(eic));
}

int EdgeInfoContainer::packed_size(MPI_Comm comm) const
{
        int size=0;

        size += theEdge.packed_size(comm);
        //cout << "Edge pack size : "  << size << endl;
        size += annotation.packed_size(comm);
        //cout << "Annot return pack size : "  << size << endl;
        return size;
}

void EdgeInfoContainer::pack(void *buf, int bufsize, int *position, MPI_Comm comm) const
{
        theEdge.pack(buf, bufsize, position, comm);
        annotation.pack(buf, bufsize, position, comm);
}

EdgeInfoContainer EdgeInfoContainer::unpack(void *buf, int bufsize, int *position, MPI_Comm comm)
{
        Edge edg = Edge::unpack(buf, bufsize, position, comm);
        EdgeAnnotation annot = EdgeAnnotation::unpack(buf, bufsize, position, comm);

        EdgeInfoContainer eic(edg,annot);
        return eic;
}


