#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>

#define ENCRYPT
#define MEASURE_ENC

#ifdef ENCRYPT
#include <stdint.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/random.h>
#endif

#ifdef MEASURE_ENC
#include <time.h>
#endif

#define SHM_NAME "/dev/uio0"
#define SHM_SIZE (8 * 1024 * 1024) // 8MB

int fd;
void *shm_ptr;

#ifdef ENCRYPT
WC_RNG rng;
unsigned char key[CHACHA20_POLY1305_AEAD_KEYSIZE] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
};
unsigned char iv[CHACHA20_POLY1305_AEAD_IV_SIZE];
unsigned char authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE];
unsigned char ciphertext[256];

void init_rng() {
    wc_InitRng(&rng);
}

int encrypt_message(const unsigned char* plaintext, unsigned int len) {
    int ret = 0;

#ifdef MEASURE_ENC
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif

    wc_RNG_GenerateBlock(&rng, iv, sizeof(iv));
    ret =  wc_ChaCha20Poly1305_Encrypt(
        key,
        iv,
        NULL,
        0,
        plaintext,
        len,
        ciphertext,
        authTag
    );
#ifdef MEASURE_ENC
    clock_gettime(CLOCK_MONOTONIC, &end);
    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long total_nanoseconds = seconds * 1000000000L + nanoseconds;
    syslog(LOG_DEBUG, "暗号化所要時間: %ld[ns] (%ld)", nanoseconds, start.tv_nsec);
#endif

    if(ret == 0){
        unsigned char* ptr = (unsigned char*)shm_ptr;

        memcpy(ptr, &len, sizeof(len));
        ptr += sizeof(len);

        memcpy(ptr, iv, sizeof(iv));
        ptr += sizeof(iv);

        memcpy(ptr, ciphertext, sizeof(ciphertext));
        ptr += sizeof(ciphertext);

        memcpy(ptr, authTag, sizeof(authTag));
    }
    return ret;
}
#endif

#define FOR_DEBUG
#ifdef FOR_DEBUG

unsigned long get_threshold() {
    return ULONG_MAX;
}

#else

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

#endif

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


void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
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

#ifdef ENCRYPT
int main() {
    daemonize();

    long pagesize;
    unsigned long threshold, used_pages, used_bytes;
    uint64_t combined;
    unsigned char data[8];
    int ret = 0;
    
    shm_start();
    init_rng();

    pagesize = sysconf(_SC_PAGESIZE);

    // (1)
    threshold = get_threshold();
    syslog(LOG_WARNING, "got TCP threshold: %lu", threshold);
    // memcpy(shm_ptr, &threshold, sizeof(threshold));

    // (2)
    while(1) {
        used_pages = get_pages();
        used_bytes = used_pages * pagesize;
        syslog(LOG_WARNING, "got TCP used bytes: %lu", used_bytes);
        // memcpy(shm_ptr + sizeof(threshold), &used_bytes, sizeof(used_bytes));

        combined = ((uint64_t)threshold << 32) | used_bytes;
        for (int i = 0; i < 8; i++) {
            data[7 - i] = (combined >> (i * 8)) & 0xFF;
        }
        ret = encrypt_message(data, sizeof(data));
        if(ret != 0){
            syslog(LOG_WARNING, "encryption failed. err: %d", ret);
        }
        
        sleep(3); // sleep for 3 sec
    }
    
    shm_end();

    return 0;
}

#else

int main() {
    daemonize();

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
        syslog(LOG_WARNING, "wrote TCP used bytes: %lu", used_bytes);
        sleep(3); // sleep for 3 sec
    }
    
    shm_end();

    return 0;
}
#endif