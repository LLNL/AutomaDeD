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
 * io_utilities.C
 *
 *  Created on: Jun 16, 2013
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@llnl.gov
 *        Modified on: Nov 2, 2013
 *         *    Author: Subrata Mitra
 *          *    Contact: mitra4@purdue.edu
 *
 */

#include "io_utilities.h"
#include "utilities.h"
#include "markov_model.h"
#include "model_components.h"

#include <string>
#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <mpi.h>
#include "backtrace.h"

using namespace std;

void getUniqueFileName(char *fileName, const char *extension)
{
	// Get rank
	int rank; 
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank);
	char rankBuff[64];
	sprintf(rankBuff, "%d", rank);
  
	// Get hostname
	char name[MAX_HOSTNAME_SIZE];
	if (gethostname(name, MAX_HOSTNAME_SIZE) != 0)
		handleError("in writeDumpFile(): could not get hostname");

	fileName[0] = '\0';
	strcat(fileName, "AUT-rank_");
	strcat(fileName, rankBuff);
	strcat(fileName, "-");
	strcat(fileName, name);

	// Get time and date
	char dateTime[100];
	dateTime[0] = '\0';
	time_t rawtime;
	struct tm * timeinfo;
	time( &rawtime );
	timeinfo = localtime( &rawtime );
	sprintf(dateTime, "-%d-%d-%d-%d-%d-%d.%s", timeinfo->tm_year + 1900,
			timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour,
			timeinfo->tm_min, timeinfo->tm_sec, extension);

	// Get final file name
	strcat(fileName, dateTime);
}

string writeFile(const string &data, const char *extension)
{
	char fileName[512];
	getUniqueFileName(fileName, extension);

	// Open file and write data
	ofstream fd(fileName);
	if (fd.is_open()) {
		fd << data;
		fd.close();
	} else {
		handleError("in writeDumpFile(): could not open file");
	}

	return string(fileName);
}

void dumpStateFactory(StateFactory& sf)
{
	cout << "\n This is from StateFactory dump \n" << endl;
        string data("");
	map< State , string >::iterator startIter = sf.stateToNameTableStartIter();
	map< State , string >::iterator endIter = sf.stateToNameTableEndIter();

	for(;startIter != endIter; startIter++)
	{
            State s = (*startIter).first;
	    string name = (*startIter).second;
	    data += s.getString() + "," + name + "\n";
	}
	writeFile(data,"csv");

}
void dumpStateFactoryWithResolvedName(StateFactory& sf)
{
        string fileData("");
        map< State , string >::iterator startIter = sf.stateToNameTableStartIter();
        map< State , string >::iterator endIter = sf.stateToNameTableEndIter();

        for(;startIter != endIter; startIter++)
        {
                State s = (*startIter).first;
                string name = "";
                sf.findAndGetName(name, s);
		//cout << s.getId() << "   " << name << endl;
                // eliminate first '|'
                name.erase(0,1);
                vector<string> tokens;
                Tokenize(name, tokens, "|");
                string setOfLines("");
		//cout << "token size: " << tokens.size() << endl;
                for (size_t j=0; j < tokens.size(); ++j) {
			
			if((s.getId() >= 45) && (s.getId() < 92))
			{
			//cout << j << ":  :"  << tokens[j] <<  endl;
                        FileAndFunction f = Backtrace::findFileAndFunctionFromObject(tokens[j]);
				//cout << "here 1" << endl;
                        string line = f.fileNameAndLine.substr(0, f.fileNameAndLine.size()-1);
				//cout << "here 2" << endl;
                        setOfLines += line + "|" + f.functionName;
                        if (f.fromTool)
                                setOfLines += "|*\n";
                        else
                                setOfLines += "\n";
				//cout << "here 3" << endl;
		       }
                }
		//cout << s.getId() << setOfLines <<  endl;

                // get state id
                char stateId[5];
                itoa((int)(s.getId()), stateId);

                fileData += string(stateId) + string(",") + setOfLines;
       }

       writeFile(fileData,"csv");
}

void dumpMarkovModelPerRankAsCSV(MarkovModel<State>* mm)
{
    string data("");
    State lastState = mm->getLastState();	
    data += string("LAST_STATE:") + lastState.getString() + string("\n");
    MarkovModelIterator<State> it(mm);
    for (it.firstTrans(); !it.isDone(); it.nextTrans()) {
                EdgeInfo<State> edge = it.currentTrans();
                State src = *(edge.edgeSource);
                State dst = *(edge.edgeDestination);
                EdgeAnnotation ea = edge.edgeAnnotation;
                stringstream ss_iter;
                map<unsigned long,RangeSetPtr> iterCountMap = ea.getIterationTaskMap();
                map<unsigned long,RangeSetPtr> :: iterator taskStart = iterCountMap.begin(), taskEnd = iterCountMap.end();
                for(;taskStart != taskEnd; taskStart++)
                {
                    unsigned long i_count = (*taskStart).first;
                    RangeSetPtr taskSetPtr = (*taskStart).second;
                    ss_iter << "," << i_count << ",";
                    string taskSetString = taskSetPtr->toString();
                    ss_iter << taskSetString; ;

                }
                unsigned long num_transitions = ea.getTransition();
                stringstream ss_trans;
                ss_trans<< num_transitions;
                data += src.getString() + string(",") + dst.getString();
                double p = mm->directProbability(src, dst);
                char buff[128];
                sprintf(buff, "%f", p);
                data += string(",") + string(buff) + string(",") + string(ss_trans.str()) + string(ss_iter.str());
                data += string("\n");
        }

        //data += string("}\n");
        writeFile(data, "graph_csv");

}
void dumpMarkovModel(MarkovModel<State>* mm)
{
	cout << "MM is:" << mm << endl;
	string data("");
	data += string("digraph finite_state_machine {\n");
	data += string("\trankdir=TB;\n");
	data += string("\tnode [shape = circle];\n");

	MarkovModelIterator<State> it(mm);
	for (it.firstTrans(); !it.isDone(); it.nextTrans()) {
		//pair<const State *, const State*> edge = it.currentTrans();
		EdgeInfo<State> edge = it.currentTrans();
		//State src = *(edge.first);
		//State dst = *(edge.second);
		State src = *(edge.edgeSource);
		State dst = *(edge.edgeDestination);
		EdgeAnnotation ea = edge.edgeAnnotation;
#if 0
		GaussianEstimator estimator;
                vector<long double> memVals = ea.getMemoryValues();
		vector<long double>::iterator vecStart = memVals.begin(), vecEnd = memVals.end();
		for(;vecStart != vecEnd; vecStart++)
		{
		    long double val = (*vecStart);
		    cout << "val = " << val << endl;
		    estimator.addValue(val);

		}
		DistributionPtr distPtr = estimator.estimate();
                vector<long double> params(distPtr->getParameters());
		long double mu = params[0];
		long double sigma = params[1];
		stringstream ss_mu;
		ss_mu << mu;
		stringstream ss_sigma;
		ss_sigma << sigma;
#endif
		stringstream ss_iter;
	       map<unsigned long,RangeSetPtr> iterCountMap = ea.getIterationTaskMap();
               map<unsigned long,RangeSetPtr> :: iterator taskStart = iterCountMap.begin(), taskEnd = iterCountMap.end(); 
                for(;taskStart != taskEnd; taskStart++)
                {
                    unsigned long i_count = (*taskStart).first;
                    RangeSetPtr taskSetPtr = (*taskStart).second;
                    ss_iter << "iter = " << i_count << " :  ";
		    string taskSetString = taskSetPtr->toString();
		    ss_iter << taskSetString << " | " ;

                }
		unsigned long num_transitions = ea.getTransition();
		//cout << " \n \n ** " << num_transitions << " \n \n *** \n";
		stringstream ss_trans;
		ss_trans<< num_transitions;
		data += string("\t") + src.getString() + string(" -> ") +
				dst.getString();
		double p = mm->directProbability(src, dst);
		char buff[128];
		sprintf(buff, "%f", p);
		data += string(" [ label = \"") + string(buff) + string("\" ],[ transitions = ") + string(ss_trans.str()) + 
				string(" ], [ ") + string(ss_iter.str())  + string(" ];\n");
		data += string(";\n");
	}

	data += string("}\n");
	writeFile(data, "dot");
}
void dumpGraph(Graph_Edges*  ge)
{
        if(!ge)
                return;

        string data("");
        data += string("digraph finite_state_machine {\n");
        data += string("\trankdir=TB;\n");
        data += string("\tnode [shape = circle];\n");

        map<string,EdgeInfoContainer > :: iterator ii = ge->edge_map.begin(), jj=ge->edge_map.end();
        for(; ii != jj; ii++)
        {
                EdgeInfoContainer eif = (*ii).second;
                string srcStr = (eif.theEdge.getSourceState()).getString();
                string dstStr = (eif.theEdge.getDestinationState()).getString();
                 
                data += string("\t") + srcStr + string(" -> ") +
                                dstStr;
                data += string(";\n");
		stringstream ss_iter;
               map<unsigned long,RangeSetPtr> iterCountMap = eif.annotation.getIterationTaskMap();
               map<unsigned long,RangeSetPtr> :: iterator taskStart = iterCountMap.begin(), taskEnd = iterCountMap.end();
                for(;taskStart != taskEnd; taskStart++)
                {
                    unsigned long i_count = (*taskStart).first;
                    RangeSetPtr taskSetPtr = (*taskStart).second;
                    ss_iter << "iter = " << i_count << " :  ";
                    string taskSetString = taskSetPtr->toString();
                    ss_iter << taskSetString << " | " ;

                }
                unsigned long num_transitions = eif.annotation.getTransition();
                //cout << " \n \n ** " << num_transitions << " \n \n *** \n";
                stringstream ss_trans;
                ss_trans<< num_transitions;
                char buff[128];
                //data += string(" [ label = ")  + string(ss_trans.str()) +
                  //            string(" ], [ ") + string(ss_iter.str())  + string(" ];\n");
        }

        data += string("}\n");
        writeFile(data, "graph_dot");
}
vector<string> executeShellCommand(const string &command)
{
	vector<string> ret;
	FILE *output;
	output = popen(command.c_str(), "r");
#if STATE_TRACKER_DEBUG
	if (!output)
		handleError("in executeShellCommand(): could not open pipe");
#endif
	size_t n = 512;
	char *lineptr;
	lineptr = (char *)xmalloc(n + 1);
	while (getline(&lineptr, &n, output) != -1) {
		ret.push_back( lineptr );
	}
	pclose(output);
	free(lineptr);

	return ret;
}
