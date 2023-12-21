/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
#include <iomanip>
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   // reads = readMisses = writes = 0; 
   // writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
   reads = readMisses = writes = writeMisses = 0; 
   writeBacks = currentCycle = 0;
   memory_trans= interventions = invalidations = flushes = 0;
   busrd = busrdx = busrdx_trans= busupd_trans = 0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
      }
   }        
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::MODIFIED_MSIAccess(int processor, int num_processors, ulong addr,uchar op, ulong protocol, Cache **cacheArray)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
   busrd = busrdx = busupd = 0;

   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      memory_trans++;
      if(op == 'w') writeMisses++;
      else readMisses++;

      cacheLine *newline = fillLine(addr);
      if(op == 'w')
      {
         newline->setFlags(DIRTY);    
         busrdx = 1;
         busrdx_trans++;
      } 
      if(op == 'r')
      {
          newline->setFlags(CLEAN);
          busrd = 1;
      }
      
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w') 
         line->setFlags(DIRTY);
   }

   for (int i= 0; i<num_processors; i++)
   {
      if(i != processor)
      {
         cacheArray[i]->MODIFIED_MSIBusRd(busrd,addr);
         cacheArray[i]->MODIFIED_MSI_BusRdx(busrdx,addr);
      }
   }
}

void Cache::MODIFIED_MSIBusRd(bool a, ulong addr)
{
   if (a == 1){
      cacheLine *line = findLine(addr);
      if (line != NULL)
      {
         if (line ->getFlags() == DIRTY)
         {
            flushes++;
            interventions++;
            line->setFlags(INVALID);
            memory_trans++;
            writeBacks++;
         }
         if (line->getFlags() == CLEAN)
         {
            line->setFlags(INVALID);
            interventions++;
         }
      }
   }
}

void Cache::MODIFIED_MSI_BusRdx(bool a, ulong addr)
{
   if (a == 1){
      cacheLine *line = findLine(addr);
      if (line != NULL)
      {
         if (line ->getFlags() == DIRTY)
         {
            flushes++;
            interventions++;
            line->setFlags(INVALID);
            memory_trans++;
            writeBacks++;
         }
         if (line->getFlags() == CLEAN)
         {
            line->setFlags(INVALID);
            interventions++;
         }
      }
   }
}

void Cache::DRAGONAccess(int processor, int num_processors, ulong addr, uchar op, ulong protocol, Cache **cacheArray){
   currentCycle++;
   busrd = busrdx = busupd = 0;

   if(op == 'w') writes++;
   else reads++;

   cacheLine *line = findLine(addr);
   if(line == NULL)
   {
      memory_trans++;

      if(op == 'w') writeMisses++;
      else readMisses++;

      cacheLine *newline = fillLine(addr);

      if(op == 'w')
      {
         newline->setFlags(DIRTY);
         busrd = 1;
         if(copyexist)
         {
            newline-> setFlags(SHARED_MODIFIED);
            busupd = 1;
            busupd_trans++;
         }
         else
         {
            newline->setFlags(DIRTY);
         }
      }
      if(op == 'r')
      {
         if(copyexist)
         {
            newline->setFlags(SHARED_CLEAN);
            busrd = 1;

         }
         else
         {
            newline->setFlags(EXCLUSIVE);
            busrd = 1;
         }
      }
   }
   else
   {
      updateLRU(line);
      if(op == 'w')
      {
         if(line->getFlags() == EXCLUSIVE)
         {
            line->setFlags(DIRTY);
         }
         if(line->getFlags() == SHARED_CLEAN)
         {
            if(copyexist)
            {
               line-> setFlags(SHARED_MODIFIED);
               busupd = 1;
               // busupd_trans++;

            }
            else
            {
               line-> setFlags(DIRTY);
               busupd = 1;
               busupd_trans++;

            }
         }
         if(line->getFlags() == SHARED_MODIFIED)
         {
            if(copyexist)  
            {
               busupd = 1;
               busupd_trans++;
            }
            else 
            {
               line->setFlags(DIRTY);
               busupd = 1; 
               busupd_trans++;

            }
         }
      }
   }
   for(int i=0; i< num_processors;i++)
   {
      if(i != processor)
      {
         cacheArray[i]->DragonBusRd(busrd, addr);
         cacheArray[i]->BusUpdate(busupd, addr);
      }
   }
}

void Cache::DragonBusRd(bool a, ulong addr){
   if(a){
      cacheLine *line = findLine(addr);
      if(line != NULL)
      {
         if(line->getFlags() == DIRTY)
         {
            line->setFlags(SHARED_MODIFIED);
            interventions++;
            // flushes++;
         }
         if(line->getFlags() == EXCLUSIVE)
         {
            line->setFlags(SHARED_CLEAN);
            interventions++;

         }
         if(line->getFlags() == SHARED_MODIFIED)
         {
            flushes++;
            writeBacks++;
            memory_trans++;
         }

      }
   }
}

void Cache::BusUpdate(bool a, ulong addr){
   if(a){
      cacheLine *line = findLine(addr);
      if(line)
      {
         if(line->getFlags() == SHARED_MODIFIED)
            line->setFlags(SHARED_CLEAN);
         
      }
   }
}
/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getFlags() == DIRTY || (victim->getFlags() == SHARED_MODIFIED)) 
   {
      writeBack(addr);
   }
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int protocol)
{ 
    // print out simulator results here
    std::cout << "01. number of reads:                            "<< getReads() << "\n";
    std::cout << "02. number of read misses:                      "<< getRM() << "\n";
    std::cout << "03. number of writes:                           "<< getWrites() << "\n";
    std::cout << "04. number of write misses:                     "<< getWM() << "\n";
    std::cout << "05. total miss rate:                            "<< std::fixed << std::setprecision(2) 
         << (static_cast<double>(getRM() + getWM()) * 100 / (getReads() + getWrites())) << "%" <<"\n";
    std::cout << "06. number of writebacks:                       "<< getWB() << "\n";
    std::cout << "07. number of memory transactions:              "<< memory_trans<< "\n";
      if(protocol == 0)
    std::cout << "08. number of invalidations:                    "<< getInterventions() <<"\n";
      else 
    std::cout << "08. number of interventions:                    "<< getInterventions() <<"\n";
    std::cout << "09. number of flushes:                          "<<  getFlushes()  << "\n";
   if(protocol == 0)
    std::cout << "10. number of BusRdX:                           "<< getBusRdX() <<"\n";
   else 
    std::cout << "10. number of Bus Transactions(BusUpd):         "<< getBusUpd() <<"\n";
    
}

