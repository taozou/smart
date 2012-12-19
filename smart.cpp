//#include <sstream>
//#include <memory>
#include <cstdio>
#include <cstring>
#include <mpi.h>

static const int KB = 1024;
static const int MB = KB * 1024;
static const char bucketName[100] = "scanspeed";



bool readAll = false;

static std::string getKey( int i, int objectMB )
{
    std::stringstream tmp;
    tmp << i << "/" << objectMB << "mb" ;
    //tmp << objectMB << "mb/" << (offset + i);
    return tmp.str();
}

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
    
    if (rank == 0)
    {
        Aggregator.run(inputcount, NULL);
    }
    else if (rank <= sCount)
    {
        Selector.run(low,high, upRank);
    }
    else if (rank <= sCount + aCount)
    {
        Aggregator.run(inputcount, upRank);
    }
    else
    {
        MPI::Finalize();
        return 1;
    }

    
    
    
    
    

    
    int unitSize = objectMB * MB / size / connectionCount;
    int base = objectMB * MB / size * rank;
    
    Stopwatch total;
    //fprintf(stderr, "%d %d %d\n", rank, base, unitSize);
    for ( int i = 0; i < connectionCount; ++i )
    {
        cons[i] = new S3Connection(config);
        buf[i] = new unsigned char[ unitSize ];
        //buf[i] = new unsigned char[ objectMB * MB ];
        memset(buf[i], 0, unitSize);
    }

    //get
    Stopwatch stopwatch;
    if (rank == 0) std::cout << connectionCount << " connection(s): \n";

    
    MPI::COMM_WORLD.Barrier();
    total.start();
    stopwatch.start();
    
    for ( int i = 0; i < connectionCount; ++i )
    {
        //fprintf(stderr, "size %d; offset %d\n", unitSize, unitSize * i + base);
        cons[i]->pendGet( &asyncMans[i % asyncManCount],
                bucketName, getKey(key, objectMB).c_str(), buf[i], unitSize, unitSize * i + base);
    }

    for ( int i = 0; i < connectionCount; ++i )
        cons[i]->completeGet();
    
    double time = total.elapsed();
    double bandwidth = 1000.0 * objectMB / size / stopwatch.elapsed();
    std::cerr << rank << ": " << bandwidth << "MiB/s\n";
    
    MPI::COMM_WORLD.Barrier();
    
    if (rank == 0)
        std::cerr << "Total time: " << time << "ms" << "\n";
    
    MPI::Finalize();
  
    return 0;
}
