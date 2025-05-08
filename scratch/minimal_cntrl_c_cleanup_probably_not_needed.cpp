#include <csignal>  // for signal()
#include <atomic>

// add this global flag
std::atomic<bool> g_shutdown_requested(false);

// add the signal handler
void handle_sigint(int) {
    g_shutdown_requested = true;
    printf("\n[INFO] Ctrl+C received. Shutting down...\n");
}

// register it in main() before the threads start
signal(SIGINT, handle_sigint);

// update the infinite loops in your threads like this:
while (!g_shutdown_requested)
{
    // ...
}

// clean up after the join() in main
close(sock);  // inside your broadcaster thread or at end of main if passed back

// add a shutdown queue or print a final summary

/*

    matters less for UDP - really matters for TCP.
    
    this is udp. so...
    broadcaster thread creates the socket once, uses it forever. No resource leak, no reconnection.
    It’s totally fine if I don’t close it, though it’s nice practice to close if I ever break the loop programmatically.
    
    might matter for:
    
    Prevent resource leakage in select(), poll(), etc
    I might fill up file descriptors over time
    Multisocket systems	Can run out of port bindings or ephemeral ports
    
    to check that:
        - check for open file descriptors: `lsof -p <pid>`
        - check for max number of file descriptors per process: `ulimit -n`
        - watch for "too many open files" errors (leaking sockets), probs not important for this test
    
    yeah just putting this in here cause I wanna see it later
*/
