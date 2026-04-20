#ifndef PIPE_H
#define PIPE_H

#include "synch.h"

#define PIPE_BUFFER_SIZE 256


class Pipe {
  public:
    Pipe();
    ~Pipe();
    int Read(char* buf, int count);
    int Write(char* buf, int count);

  private:
    char buffer[PIPE_BUFFER_SIZE];
    int readPos;
    int writePos;

    Lock*      lock;
    Semaphore* notEmpty; 
    Semaphore* notFull;   
};

#endif
