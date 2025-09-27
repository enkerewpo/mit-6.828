#include "kernel/types.h"
#include "user/user.h"


const int NUM_WORKERS = 15; // how many primes numbers we have < 280 ?

void primes() {
    int p[2]; // one-way pipe, parent -> child, 0-R, 1-W
    pipe(p);
    int pid = fork();
    if (pid == 0) {
        // child process
        
    }
    else {
        // parent process
        
    }
}

int main(int argc, char *argv[]) {

// p = get a number from left neighbor
// print p
// loop:
//     n = get a number from left neighbor
//     if (p does not divide n)
//         send n to right neighbor

    // step 1, create total of 15 workers (processes) and each one uses pipe to communicate with the left neighbor
    // which is exactly the parent process
    primes();

    exit(0);
}