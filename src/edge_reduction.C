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
 * edge_reduction.C
 *
 *  Created on: Jun 10, 2013
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 *         Modified on: Nov 2, 2013
 *          *    Author: Subrata Mitra
 *           *    Contact: mitra4@purdue.edu
 */

#include "edge_reduction.h"
#include "reduction.h"
#include "model_components.h"
#include "markov_model.h"
#include "debugging.h"
#include "range_set_table.h"

#include <set>
#include <mpi.h>

using namespace std;

void EdgeReduction::reduce(const MarkovModel<State> *m,
		StateFactory *f)
{
	// Update my temporal structures
	sFactory = f;
	mModel = m;
	int rank = mpiState.getProcessRank();

	int numProcesses = mpiState.getCommWorldSize();

	// Extract edges from the Markov model
	MarkovModelIterator<State> it(mModel);
	for (it.firstTrans(); !it.isDone(); it.nextTrans())
	{
		//pair<const State*, const State*> e = it.currentTrans();
		EdgeInfo<State> e = it.currentTrans();
		Edge edge(*(e.edgeSource), *(e.edgeDestination));
		EdgeAnnotation annot(e.edgeAnnotation);
		EdgeInfoContainer eic = EdgeInfoContainer(edge,annot); 
		map<Edge,EdgeInfoContainer>::iterator pos = edges.find(edge);
		if(pos != edges.end())
		{
		        EdgeAnnotation oldAnnotation = (pos->second).annotation;
			EdgeAnnotation newAnnotation = oldAnnotation + annot;
			edges.erase(pos);
		        eic = EdgeInfoContainer(edge,newAnnotation);	
		}
		edges.insert(pair<Edge,EdgeInfoContainer>(edge,eic));
	}
	//f->print(rank);
	//cout << "==================== end of reduce print ================== " << endl;
	//printEdges(rank);
	// Perform reduction using template
	Reducer<EdgeReduction> *red = new BinomialReducer<EdgeReduction>;
	red->reduce(0, numProcesses, rank, this);

	delete red;
}

int EdgeReduction::packed_size(MPI_Comm comm) const
{
	int size=0;

	// Size for factory
	size += sFactory->packed_size(comm);

	// Number of edges
	int tmp;
	PMPI_Pack_size(1, MPI_UNSIGNED, comm, &tmp);
	size += tmp;
       // cout << "Edge reduction with states pack_size: " << size << "  number of edges: " << edges.size()  <<endl;

	// Size for all the edges
	map<Edge,EdgeInfoContainer>::const_iterator it;
	for (it = edges.begin(); it != edges.end(); it++) {
		size += (it->second).packed_size(comm);
	}
        //cout << "Edge reduction final pack_size: " << size <<endl;

	return size;
}

void EdgeReduction::pack(void *buf, int bufsize, int *position,
		MPI_Comm comm) const
{
	if (edges.size() == 0) {
		handleError("EdgeReduction::pack(): "
				"cannot pack an empty edge list.");
	}

	// Pack state factory
	sFactory->pack(buf, bufsize, position, comm);

	// Pack number of edges
	unsigned int s = static_cast<unsigned int>(edges.size());
	PMPI_Pack((void *)&s, 1, MPI_UNSIGNED, buf, bufsize, position, comm);

	// Pack edges
	map<Edge,EdgeInfoContainer>::const_iterator it;
	for (it = edges.begin(); it != edges.end(); it++) {
		(it->second).pack(buf, bufsize, position, comm);
	}
}

FactoryAndEdges EdgeReduction::unpack(void *buf, int bufsize,
		int *position, MPI_Comm comm)
{
	FactoryAndEdges ret;

	StateFactory remoteFactory =
			StateFactory::unpack(buf, bufsize, position, comm);
	ret.sFactory = remoteFactory;

	unsigned int numEdges;
	PMPI_Unpack(buf, bufsize, position, &numEdges, 1, MPI_UNSIGNED, comm);

	map<Edge,EdgeInfoContainer> rcvdEdges;
	for (unsigned int i=0; i < numEdges; ++i) {
		EdgeInfoContainer e = EdgeInfoContainer::unpack(buf, bufsize, position, comm);
		rcvdEdges.insert(pair<Edge,EdgeInfoContainer>(e.theEdge,e));
	}
	ret.edges = rcvdEdges;

	return ret;
}

void EdgeReduction::send(int receiver) const
{
	// Send buffer size
	int buff_size = packed_size(comm);
	PMPI_Send((void *)(&buff_size), 1, MPI_INT, receiver, mpiState.getTag(), comm);

	// allocate buffer
	//void *buff = xmalloc((unsigned)buff_size);
	char buff[buff_size];
	int position = 0;

	// Send edge list
	pack(buff, buff_size, &position, comm);

	PMPI_Send(buff, buff_size, MPI_PACKED, receiver, mpiState.getTag(), comm);

	//free(buff);
}

void EdgeReduction::receive(int sender)
{
	MPI_Status status;

	// Receive buffer size
	int buff_size;
	PMPI_Recv((void *)(&buff_size), 1, MPI_INT, sender, mpiState.getTag(), comm, &status);

	// allocate buffer
	//void *buff = xmalloc((unsigned)buff_size);
	char buff[buff_size];
	int position = 0;

	// Receive edge list
	PMPI_Recv(buff, buff_size, MPI_PACKED, sender, mpiState.getTag(), comm, &status);

	FactoryAndEdges recvObject = unpack(buff, buff_size, &position, comm);

	// Update local data
	updateLocalData(recvObject);


	//free(buff);
}

void EdgeReduction::updateLocalData(const FactoryAndEdges &recvData)
{
	map<Edge,EdgeInfoContainer>::iterator it;
	map<Edge,EdgeInfoContainer> rcvEdges = recvData.edges;
	StateFactory remFactory = recvData.sFactory; // remote state factory
	int rank = mpiState.getProcessRank();
	for (it = rcvEdges.begin(); it != rcvEdges.end(); ++it) {
		EdgeInfoContainer edgeInfoCont = it->second;
		Edge recvEdge = edgeInfoCont.theEdge;
                EdgeAnnotation recvAnnot = edgeInfoCont.annotation;
		string src;
		//cout <<"*** Source state is " << recvEdge.getSourceState().getString() << endl;
		//remFactory.print(rank);
		if (!remFactory.findAndGetName(src, recvEdge.getSourceState())) {
			handleError("in EdgeReduction::updateLocalData: "
					"could not find src state");
		}

		string dst;
		if (!remFactory.findAndGetName(dst, recvEdge.getDestinationState())) {
			handleError("in EdgeReduction::updateLocalData: "
					"could not find dst state");
		}

		// Find local names for states
		State srcState;
		State dstState;
		if (! sFactory->findAndGetState(srcState, src)) {
			srcState = sFactory->createState(src);
		}
		if (! sFactory->findAndGetState(dstState, dst)) {
			dstState = sFactory->createState(dst);
		}

		Edge newEdge(srcState, dstState);
		EdgeAnnotation newAnnot = recvAnnot;
		EdgeInfoContainer eic = EdgeInfoContainer(newEdge,newAnnot);
		map<Edge,EdgeInfoContainer>::iterator pos = edges.find(newEdge);
		if(pos != edges.end())
		{
		   newAnnot = (pos->second).annotation + recvAnnot;

#if 0
		   string info = "";
		   if(((pos->second).annotation).iterationTaskMap.size() >= 1)
		   {
			info += " old map " + ((pos->second).annotation).iterationTaskMap.begin()->second->toString();
		   }
		   if(recvAnnot.iterationTaskMap.size() >= 1)
		   {
			   info += " rcvd " + recvAnnot.iterationTaskMap.begin()->second->toString();
		   }
		   if(newAnnot.iterationTaskMap.size() >= 1)
		   {
			info += " added " + newAnnot.iterationTaskMap.begin()->second->toString() ;
		   }
		   
		   cout << "for rank " << rank << " old map " << info << endl;
		    //cout << "**rank " << rank << " edge is : " << newEdge.getSourceState().getString() <<  " - " << newEdge.getDestinationState().getString() << " old anot " << (pos->second).annotation.getTransition() << " rcvd anot " << recvAnnot.getTransition() << endl;
#endif
		   edges.erase(pos);
		}
		eic = EdgeInfoContainer(newEdge,newAnnot);
		edges.insert(pair<Edge, EdgeInfoContainer>(newEdge,eic));
	}
}

map<Edge,EdgeInfoContainer> EdgeReduction::getEdges() const
{
	return edges;
}
#if 0
void EdgeReduction::printEdges(int forRank)
{
   int rank = mpiState.getProcessRank();
   if((forRank != -1) && (rank != forRank))
   {
     return;
   }
   map<Edge,EdgeInfoContainer>::iterator startEdge = edges.begin(), endEdge = edges.end();
   for(; startEdge != endEdge; startEdge++)
   {
       Edge e = (startEdge->second).theEdge;
       EdgeAnnotation ea = (startEdge->second).annotation;
       cout << "For rank=" << rank  <<" The edge is : " << e.getSourceState().getString() <<  " - " << e.getDestinationState().getString() << " Transitions: " << ea.getTransition() << endl;
       vector<long double> memVals = ea.getMemoryValues();
                vector<long double>::iterator vecStart = memVals.begin(), vecEnd = memVals.end();
                for(;vecStart != vecEnd; vecStart++)
                {
                    long double val = (*vecStart);
                    cout << "rank = " << rank << " val = " << val << endl;

                }
   }
}
#endif
