/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
   INVALID = 0,
   VALID, //Shared
   CLEAN,
   DIRTY, // Modified
   EXCLUSIVE,
   SHARED_MODIFIED,
   SHARED_CLEAN

};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
 
public:
   cacheLine()                { tag = 0; Flags = 0; }
   ulong getTag()             { return tag; }
   ulong getFlags()           { return Flags;}
   ulong getSeq()             { return seq; }
   void setSeq(ulong Seq)     { seq = Seq;}
   void setFlags(ulong flags) {  Flags = flags;}
   void setTag(ulong a)       { tag = a; }
   void invalidate()          { tag = 0; Flags = INVALID; } //useful function
   bool isValid()             { return ((Flags) != INVALID); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //******///
   //add coherence counters here///
   //******///
   ulong interventions, invalidations, memory_trans;
   ulong flushes, busrdx_trans, busupd_trans;

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)   { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk));}
   
public:
   ulong currentCycle;  
   int busrd, busrdx, busupgr, busupd;
   bool copyexist;
     
   Cache(int,int,int);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM()     {return readMisses;} 
   ulong getWM()     {return writeMisses;} 
   ulong getReads()  {return reads;}       
   ulong getWrites() {return writes;}
   ulong getWB()     {return writeBacks;}
   ulong getInterventions(){ return interventions; }
   ulong getInvalidations(){ return invalidations; }
   ulong getFlushes(){ return flushes; }
   ulong getBusRdX(){ return busrdx_trans; }
   ulong getBusUpd(){ return busupd_trans; }
   
   void writeBack(ulong) {writeBacks++; memory_trans++;}
   void MODIFIED_MSIAccess(int, int, ulong,uchar, ulong, Cache **);
   void DRAGONAccess(int, int, ulong,uchar, ulong, Cache **);
   void printStats(int protocol);
   void updateLRU(cacheLine *);

   //******///
   //add other functions to handle bus transactions///
   //******///
   void BusRdX(bool, ulong);
   void BusUpdate(bool, ulong);
   void MSIBusRd(bool, ulong);
   void DragonBusRd(bool, ulong);
   void MODIFIED_MSIBusRd(bool, ulong);
   void MODIFIED_MSI_BusRdx(bool, ulong);

};

#endif
