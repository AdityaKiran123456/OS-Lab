#include "pipe.h"

Pipe::Pipe() {
    readPos  = 0;
    writePos = 0;
    lock     = new Lock("pipeLock");
    notEmpty = new Semaphore("pipeNotEmpty", 0);
    notFull  = new Semaphore("pipeNotFull", PIPE_BUFFER_SIZE);
}

Pipe::~Pipe() {
    delete lock;
    delete notEmpty;
    delete notFull;
}

int Pipe::Write(char* buf, int count) {
    for (int i = 0; i < count; i++) {
        notFull->P();
        lock->Acquire();
        buffer[writePos] = buf[i];
        writePos = (writePos + 1) % PIPE_BUFFER_SIZE;
        lock->Release();
        notEmpty->V();
    }
    return count;
}

int Pipe::Read(char* buf, int count) {
    for (int i = 0; i < count; i++) {
        notEmpty->P();
        lock->Acquire();
        buf[i] = buffer[readPos];
        readPos = (readPos + 1) % PIPE_BUFFER_SIZE;
        lock->Release();
        notFull->V();
    }
    return count;
}
