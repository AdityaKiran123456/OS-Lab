#include "syscall.h"

int main() {
    // Unit: seconds
    PrintString("Before sleep\n");
    Sleep(2);  // sleep for 2 seconds
    PrintString("After sleep\n");
    Halt();
}
