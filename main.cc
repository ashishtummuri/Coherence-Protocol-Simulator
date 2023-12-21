/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;
#include "cache.h"

int main(int argc, char *argv[])
{
    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MODIFIED_MSI 1:DRAGON*/
    char *fname        = (char *) malloc(20);
    fname              = argv[6];

    printf("===== 506 Personal information =====\n");
    // print out Personal Info here
    std::cout << "Name: Ashish Tummuri" << std::endl;
    std::cout << "Unity ID: atummur" << std::endl;
    std::cout << "ECE492 Students?: NO" << std::endl;

    printf("===== 506 SMP Simulator configuration =====\n");
    // print out simulator configuration here
    std::cout << "L1_SIZE: " << cache_size << std::endl;
    std::cout << "L1_ASSOC: " << cache_assoc << std::endl;
    std::cout << "L1_BLOCKSIZE: " << std::dec<< blk_size << std::endl;
    std::cout << "NUMBER OF PROCESSORS: " << num_processors << std::endl;
    if (protocol == 0){
        std::cout << "COHERENCE PROTOCOL: " << "MSI" << std::endl;}
    else if (protocol == 1){
        std::cout << "COHERENCE PROTOCOL: " << "Dragon" << std::endl;}
    else {
        std::cout << "COHERENCE PROTOCOL: " << "Wrong COHERENCE PROTOCOL!" << std::endl;exit(0);}
    std::cout << "TRACE FILE:  " << fname << std::endl;
    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));

    for(ulong i = 0; i < num_processors; i++) {
        //check this line
        // if(protocol ==  0) {
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
        // }
    }
    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
ulong proc;
char op;
ulong addr;

// int line = 1;
while (fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF) {
#ifdef _DEBUG
    // printf("Processing line: %d\n", line);
#endif

    // Check whether a copy exists for MSI and Dragon protocols
    cacheArray[proc]->copyexist = 0;
    for (int i = 0; i < num_processors; i++) {
        if ((i != proc) && (cacheArray[i]->findLine(addr))) {
            cacheArray[proc]->copyexist = 1;
            // break; 
        }
    }
    switch (protocol) {
        case 0:
            cacheArray[proc]->MODIFIED_MSIAccess(proc, num_processors, addr, op, protocol, cacheArray);
            break;
        case 1:
            cacheArray[proc]->DRAGONAccess(proc, num_processors, addr, op, protocol, cacheArray);
            break;
        default:
            // Handle unexpected protocol value
            fprintf(stderr, "Unknown protocol: %lu\n", protocol);
            break;
    }

    // line++; // Increment line counter after processing each line
}

fclose(pFile); // Close the file after processing all lines


//********************************//
//print out all caches' statistics //
//********************************//
for (int i = 0; i < num_processors; i++)
{
    printf("============ Simulation results (Cache %d) ============\n", i);
    cacheArray[i]->printStats(protocol);
}
}
