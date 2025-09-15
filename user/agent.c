#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define SHM_NAME "/dev/uio0"
#define SHM_SIZE (8 * 1024 * 1024) // 8MB

int fd;
void *shm_ptr;

unsigned long get_threshold() {
    FILE *f;
    unsigned long low, pressure, high;

    f = fopen("/proc/sys/net/ipv4/tcp_mem", "r");
    if (fscanf(f, "%lu %lu %lu", &low, &pressure, &high) != 3) {
        perror("fscanf tcp_mem");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    fclose(f);

    return pressure;
}

unsigned long get_pages() {
    FILE *f;
    unsigned long used_pages;

    f = fopen("/proc/net/sockstat", "r");
    char buf[256];
    while (fgets(buf, sizeof(buf), f)) {
        if (sscanf(buf, "TCP: inuse %*d orphan %*d tw %*d alloc %*d mem %lu", &used_pages) == 1)
            break;
    }
    fclose(f);

    return used_pages;
}


void shm_start() {
    fd = open(SHM_NAME, O_RDWR);

    if (fd < 0) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_WARNING, "fd: %d, SHM_SIZE: %d", fd, SHM_SIZE);

    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 4096);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_WARNING, "SHM opened. mapped to %p", shm_ptr);
}

void shm_end() {
    munmap(shm_ptr, SHM_SIZE);
    close(fd);
}


int main() {
    long pagesize;
    unsigned long threshold, used_pages, used_bytes;
    
    shm_start();

    pagesize = sysconf(_SC_PAGESIZE);

    // (1)
    threshold = get_threshold();
    memcpy(shm_ptr, &threshold, sizeof(threshold));
    syslog(LOG_WARNING, "wrote TCP threshold: %lu", threshold);

    // (2)
    while(1) {
        used_pages = get_pages();
        used_bytes = used_pages * pagesize;
        memcpy(shm_ptr + sizeof(threshold), &used_bytes, sizeof(used_bytes));
        syslog(LOG_WARNING, "wrote TCP used pages: %lu", used_pages);
        sleep(3); // sleep for 3 sec
    }
    
    shm_end();

    return 0;
}
