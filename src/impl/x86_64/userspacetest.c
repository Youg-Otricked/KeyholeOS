#include "userspacetest.h"
void user_function() {
    // this will run in ring 3
    // can't do anything except syscall
    while(1) {}
}