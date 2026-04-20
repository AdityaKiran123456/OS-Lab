#include "syscall.h"

int main() {
    int fd[2];
    char* msg = "hello from writer!";
    char readBuf[20];
    int i;

    if (Pipe(fd) == -1) {
        PrintString("Pipe creation failed\n");
        Halt();
    }

    Write(msg, 18, fd[1]);
    Read(readBuf, 18, fd[0]);

    readBuf[18] = '\0';
    PrintString("Read from pipe: ");
    PrintString(readBuf);
    PrintString("\n");

    Halt();
    return 0;
}