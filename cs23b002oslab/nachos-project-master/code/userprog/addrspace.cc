// addrspace.cc
//      Routines to manage address spaces (executing user programs).
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"
#include "synch.h"

//----------------------------------------------------------------------
// SwapHeader
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
#ifdef RDATA
    noffH->readonlyData.size = WordToHost(noffH->readonlyData.size);
    noffH->readonlyData.virtualAddr = WordToHost(noffH->readonlyData.virtualAddr);
    noffH->readonlyData.inFileAddr = WordToHost(noffH->readonlyData.inFileAddr);
#endif
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//----------------------------------------------------------------------

AddrSpace::AddrSpace() {
    executable = NULL;
    codeSize = codeVirtAddr = codeFileAddr = 0;
    initDataSize = initDataVirtAddr = initDataFileAddr = 0;
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace(char *fileName)
//      Demand paging: set up page table with all pages invalid.
//      Pages are loaded on demand when PageFaultException occurs.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(char *fileName) {
    NoffHeader noffH;
    unsigned int size;

    executable = kernel->fileSystem->Open(fileName);
    if (executable == NULL) {
        DEBUG(dbgFile, "\n Error opening file.");
        return;
    }

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // save segment info for use in LoadPage later
    codeSize      = noffH.code.size;
    codeVirtAddr  = noffH.code.virtualAddr;
    codeFileAddr  = noffH.code.inFileAddr;
    initDataSize      = noffH.initData.size;
    initDataVirtAddr  = noffH.initData.virtualAddr;
    initDataFileAddr  = noffH.initData.inFileAddr;

    kernel->addrLock->P();

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
           UserStackSize;
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

    // set up page table — all pages invalid, no physical memory allocated yet
    pageTable = new TranslationEntry[numPages];
    for (unsigned int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = FALSE;
        pageTable[i].use          = FALSE;
        pageTable[i].dirty        = FALSE;
        pageTable[i].readOnly     = FALSE;
    }

    kernel->addrLock->V();
    // DO NOT delete executable — LoadPage needs it later
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
    for (unsigned int i = 0; i < numPages; i++) {
        if (pageTable[i].valid)
            kernel->gPhysPageBitMap->Clear(pageTable[i].physicalPage);
    }
    delete[] pageTable;
    if (executable != NULL)
        delete executable;
}

//----------------------------------------------------------------------
// AddrSpace::LoadPage
//      Handle a page fault for virtual page vpn.
//      Allocates a physical frame, zeroes it, loads data from the
//      executable if this page overlaps code or initData segments,
//      then marks the page valid.
//----------------------------------------------------------------------

bool AddrSpace::LoadPage(int vpn) {
    if (vpn < 0 || (unsigned int)vpn >= numPages)
        return false;
    if (pageTable[vpn].valid)
        return true;  // already loaded

    kernel->addrLock->P();

    int ppn = kernel->gPhysPageBitMap->FindAndSet();
    if (ppn == -1) {
        kernel->addrLock->V();
        DEBUG(dbgAddr, "No free physical pages for vpn " << vpn);
        return false;
    }

    // zero out the frame
    bzero(&(kernel->machine->mainMemory[ppn * PageSize]), PageSize);

    // update page table entry
    pageTable[vpn].physicalPage = ppn;
    pageTable[vpn].valid        = TRUE;
    pageTable[vpn].use          = FALSE;
    pageTable[vpn].dirty        = FALSE;

    kernel->addrLock->V();

    int virtAddr = vpn * PageSize;

    // load from code segment if this page overlaps it
    if (codeSize > 0) {
        int codeEnd = codeVirtAddr + codeSize;
        if (virtAddr < codeEnd && (virtAddr + (int)PageSize) > codeVirtAddr) {
            int offset   = virtAddr - codeVirtAddr;
            int memOff   = 0;
            int fileAddr = codeFileAddr + offset;
            int copySize = PageSize;
            if (offset < 0) {
                memOff    = -offset;
                fileAddr += memOff;
                copySize -= memOff;
            }
            int remaining = (codeFileAddr + codeSize) - fileAddr;
            if (copySize > remaining) copySize = remaining;
            if (copySize > 0)
                executable->ReadAt(
                    &(kernel->machine->mainMemory[ppn * PageSize + memOff]),
                    copySize, fileAddr);
        }
    }

    // load from initData segment if this page overlaps it
    if (initDataSize > 0) {
        int dataEnd = initDataVirtAddr + initDataSize;
        if (virtAddr < dataEnd && (virtAddr + (int)PageSize) > initDataVirtAddr) {
            int offset   = virtAddr - initDataVirtAddr;
            int memOff   = 0;
            int fileAddr = initDataFileAddr + offset;
            int copySize = PageSize;
            if (offset < 0) {
                memOff    = -offset;
                fileAddr += memOff;
                copySize -= memOff;
            }
            int remaining = (initDataFileAddr + initDataSize) - fileAddr;
            if (copySize > remaining) copySize = remaining;
            if (copySize > 0)
                executable->ReadAt(
                    &(kernel->machine->mainMemory[ppn * PageSize + memOff]),
                    copySize, fileAddr);
        }
    }

    DEBUG(dbgAddr, "Loaded vpn " << vpn << " into ppn " << ppn);
    return true;
}

//----------------------------------------------------------------------
// AddrSpace::Execute
//----------------------------------------------------------------------

void AddrSpace::Execute() {
    kernel->currentThread->space = this;
    this->InitRegisters();
    this->RestoreState();
    kernel->machine->Run();
    ASSERTNOTREACHED();
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++) machine->WriteRegister(i, 0);

    machine->WriteRegister(PCReg, 0);
    machine->WriteRegister(NextPCReg, 4);
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}

//----------------------------------------------------------------------
// AddrSpace::Translate
//----------------------------------------------------------------------

ExceptionType AddrSpace::Translate(unsigned int vaddr, unsigned int *paddr,
                                   int isReadWrite) {
    TranslationEntry *pte;
    int pfn;
    unsigned int vpn    = vaddr / PageSize;
    unsigned int offset = vaddr % PageSize;

    if (vpn >= numPages)
        return AddressErrorException;

    pte = &pageTable[vpn];

    if (!pte->valid)
        return PageFaultException;   // page not loaded yet — trigger fault handler

    if (isReadWrite && pte->readOnly)
        return ReadOnlyException;

    pfn = pte->physicalPage;

    if (pfn >= NumPhysPages) {
        DEBUG(dbgAddr, "Illegal physical page " << pfn);
        return BusErrorException;
    }

    pte->use = TRUE;
    if (isReadWrite) pte->dirty = TRUE;

    *paddr = pfn * PageSize + offset;

    ASSERT((*paddr < MemorySize));

    return NoException;
}