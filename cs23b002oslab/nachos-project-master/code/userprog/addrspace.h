// addrspace.h
//      Data structures to keep track of executing user programs
//      (address spaces).
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize 1024

class AddrSpace {
   public:
    AddrSpace();
    AddrSpace(char *fileName);
    ~AddrSpace();
    void Execute();
    void SaveState();
    void RestoreState();
    ExceptionType Translate(unsigned int vaddr, unsigned int *paddr, int mode);
    bool LoadPage(int vpn);

   private:
    TranslationEntry *pageTable;
    unsigned int numPages;
    void InitRegisters();

    OpenFile *executable;

    // fields copied from NoffHeader after reading — avoids including noff.h here
    int codeSize, codeVirtAddr, codeFileAddr;
    int initDataSize, initDataVirtAddr, initDataFileAddr;
};

#endif  // ADDRSPACE_H