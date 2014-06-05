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

#include <mpi.h>
#include <iostream>
#include <unistd.h>

/*
 *
 */

using namespace std;

void fault(int rank,int faultforRank)
{
	if (rank == faultforRank)
	{
		cout << "Process " << rank << " hangs." << endl;
		sleep(60*60*24); /* sleep for one day */
	}
}

int main(int argc, char *argv[])
{
	int rank, i;
	int buffer[512];

	/* Init MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	for(int k=0;k<9;k++)
	{
	   for(int m = 0; m < 9; m++)
	   {
             MPI_Pcontrol(1);
             MPI_Pcontrol(1);
	     if((k ==4) && (m==6))
	     {
	       fault(rank,3);
	     }
	     //cout << "rank " << rank << endl;
	     if(rank == 5)
	     {
		     fault(rank,5);
		     continue;
	     }
             MPI_Pcontrol(1);
	     MPI_Barrier(MPI_COMM_WORLD);
	   }
	   MPI_Pcontrol(1);
	   MPI_Barrier(MPI_COMM_WORLD);
	   
	}

	MPI_Finalize();
	return 0;
}
