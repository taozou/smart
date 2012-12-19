#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mpi.h>
#include "selector.h"
#include "aggregator.h"

char bucketName[100] = "scanspeed";

int main( int argc, char **argv )
{
    MPI::Init(argc, argv);
    int rank = MPI::COMM_WORLD.Get_rank();
    int size = MPI::COMM_WORLD.Get_size();

    int sCount = -1;
    int aCount = 0;
    int keyHigh = -1;
    
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-s"))
        {
            sCount = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-a"))
        {
            aCount = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-k"))
        {
            keyHigh = atoi(argv[++i]);
        }
    }
    
    if (sCount == -1)
    {
         if (rank == 0)
            fprintf(stderr, "smart [-s SelectorCount] [-a AggregatorCount(0)] [-k KeyRange 0-k(s)]\n");
         MPI::Finalize();
         return 1;
    }
    
    if (aCount + sCount + 1 > size)
    {
        if (rank == 0)
            fprintf(stderr, "SelectorCount + AggregatorCount + 1 > Total Nodes\n");
        MPI::Finalize();
        return 1;          
    }

    if (keyHigh == -1)
        keyHigh = sCount;
    
    int perCount = keyHigh / size;
    
    if (perCount * size < keyHigh)
        ++perCount;
    
    int idLow = perCount * rank;
    int idHigh = idLow + perCount;
    
    if (idHigh > keyHigh)
        idHigh = keyHigh;
    
    //assume aCount <= sCount 
    int perAggr = sCount / aCount;
    if (perAggr * aCount < sCount)
        ++perAggr;
    
    if (rank == 0)
    {
        aggregator a;
        a.run(aCount, -1);
    }
    else if (rank <= sCount)
    {
        int idA = (rank - 1) / perAggr + 1;
        selector s;
        s.init(bucketName);
        s.run(idLow, idHigh, idA + sCount);
    }
    else if (rank <= sCount + aCount)
    {
        aggregator a;
        if ( ((rank - sCount) * perAggr) > sCount)
            a.run( sCount - (rank - sCount -1 ) * perAggr, 0);
        else
            a.run( perAggr, 0 );
    }

    MPI::Finalize();
    return 0;
    
    //Stopwatch stopwatch;
    
    
    //MPI::COMM_WORLD.Barrier();
    
    return 0;
}
