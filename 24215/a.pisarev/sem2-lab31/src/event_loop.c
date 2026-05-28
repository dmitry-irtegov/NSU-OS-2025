#include "event_loop.h"
#include "client.h"
#include "upstream.h"
#include "utils.h"

struct pollfd pollfds[MAX_FDS];
int nfds = 0;
volatile sig_atomic_t running = 1;

void event_loop_init(int listen_fd) {
    for (int i = 0; i < MAX_FDS; i++) {
        pollfds[i].fd = -1;
        pollfds[i].events = 0;
    }
    pollfds[0].fd = listen_fd;
    pollfds[0].events = POLLIN;
    nfds = 1;
    
}

void event_loop_run(void) {
    while (running) {
        int ready = poll(pollfds, MAX_FDS, 1000);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        for (int i = 0; i < MAX_FDS; i++) {
            if (pollfds[i].fd < 0) continue;
            
            int fd = pollfds[i].fd;
            short revents = pollfds[i].revents;
            
            if (fd == pollfds[0].fd && (revents & POLLIN)) {
                client_accept(fd);
                continue;
            }
            
            int ci = fd_to_idx_lookup(fd, FD_TYPE_CLIENT, fd_to_index);
            if (ci >= 0) {
                client_handle_event(ci, revents);
                continue;
            }
            
            int ui = fd_to_idx_lookup(fd, FD_TYPE_UPSTREAM, fd_to_index);
            if (ui >= 0) {
                upstream_handle_event(ui, revents);
                continue;
            }
        }
    }
}

void event_loop_cleanup(void) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (pollfds[i].fd >= 0) {
            safe_close(pollfds[i].fd);
            fd_unregister(pollfds[i].fd, fd_to_index);
        }
    }
}
