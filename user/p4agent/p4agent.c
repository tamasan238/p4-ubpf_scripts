#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdbool.h>

#define ENCRYPT
// #define MEASURE_ENC

// #define USE_CHACHAPOLY
#define USE_AES

#ifdef ENCRYPT
#include <stdint.h>
#include <wolfssl/options.h>
#ifdef USE_CHACHAPOLY
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#endif
#ifdef USE_AES
#include <wolfssl/wolfcrypt/aes.h>
#endif
#include <wolfssl/wolfcrypt/random.h>
#endif

#ifdef MEASURE_ENC
#include <time.h>
#endif

#define SHM_NAME "/dev/uio0"
#define SHM_SIZE (8 * 1024 * 1024) // 8MB

#define VM_AREA 0                                  // 
#define META_AREA (VM_AREA + 2 * 1024 * 1024)      // Start at 2MB
#define PACKETS_AREA (META_AREA + 2 * 1024 * 1024) // Start at 4MB

int fd;
void *shm_ptr;

/* META_AREA */
typedef struct
{
    long long ovs_thread_id;
    int p4runtime_id;
    long long packet_count;
} Connection;

#define MAX_CONNECTIONS 32
// #define MAX_CONNECTIONS 8
#define SHM_SESSION_TABLE META_AREA
#define SHM_TABLE_IS_LOCKED (SHM_SESSION_TABLE + sizeof(Connection) * MAX_CONNECTIONS)

Connection *session;
unsigned long long passed_packets_tx_offset, passed_packets_rx_offset,
    passed_packets, passed_packets_rx, passed_packets_tx;
/* end */

/* PACKETS_AREA */
#define SHM_SIZE_DP_PACKET_2 64
#define SHM_SIZE_PACKET 64
#define SHM_SIZE_RESULT 32
#define SHM_SIZE_FLAGS 32
#define SHM_SIZE_PER_PACKET (SHM_SIZE_DP_PACKET_2 + SHM_SIZE_PACKET + SHM_SIZE_RESULT + SHM_SIZE_FLAGS)

#define SHM_FLAG_PACKETS (PACKETS_AREA + SHM_SIZE_PER_PACKET - SHM_SIZE_FLAGS)
#define SHM_FLAG_RESULTS (SHM_FLAG_PACKETS + 1)
#define SHM_FLAG_HOW_MANY_PACKETS (SHM_FLAG_PACKETS + 2) // use only first packet in batch
/* end */

#ifdef ENCRYPT
WC_RNG rng;
unsigned char ciphertext[256];
#ifdef USE_CHACHAPOLY
unsigned char key[CHACHA20_POLY1305_AEAD_KEYSIZE] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
};
unsigned char iv[CHACHA20_POLY1305_AEAD_IV_SIZE];
unsigned char authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE];
#endif
#ifdef USE_AES
Aes aes;
const unsigned char key[AES_256_KEY_SIZE] = {
0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
};

unsigned char iv[12];
unsigned char authTag[16];
#endif

void init_wolf() {
    wc_InitRng(&rng);
#ifdef USE_AES
    wc_AesGcmSetKey(
        &aes,
        key,
        sizeof(key)
    );
#endif
}

int encrypt_message(const unsigned char* plaintext, unsigned int len) {
    int ret = 0;

#ifdef MEASURE_ENC
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif

    wc_RNG_GenerateBlock(&rng, iv, sizeof(iv));
#ifdef USE_CHACHAPOLY
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
#endif
#ifdef USE_AES
    ret = wc_AesGcmEncrypt(
        &aes,
        ciphertext,
        plaintext,
        len,
        iv,
        sizeof(iv), 
        authTag,
        sizeof(authTag),
        NULL,
        0
    );
#endif
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

    session = (Connection *)(shm_ptr + SHM_SESSION_TABLE);
}

void get_nic_stat(bool is_init){
    FILE *fp;
    char header[1024];
    char values[1024];
    char *h[64];
    char *v[64];
    int hc = 0;
    int vc = 0;
    unsigned long long in_receives = 0;
    unsigned long long out_requests = 0;

    fp = fopen("/proc/net/snmp", "r");
    if (!fp)
        exit(1);

    while (fgets(header, sizeof(header), fp)) {
        if (strncmp(header, "Ip:", 3) == 0) {
            if (!fgets(values, sizeof(values), fp))
                break;

            char *p = strtok(header, " \n");
            while (p && hc < 64) {
                h[hc++] = p;
                p = strtok(NULL, " \n");
            }

            p = strtok(values, " \n");
            while (p && vc < 64) {
                v[vc++] = p;
                p = strtok(NULL, " \n");
            }

            for (int i = 0; i < hc && i < vc; i++) {
                if (strcmp(h[i], "InReceives") == 0)
                    in_receives = strtoull(v[i], NULL, 10);
                else if (strcmp(h[i], "OutRequests") == 0)
                    out_requests = strtoull(v[i], NULL, 10);
            }
            break;
        }
    }

    fclose(fp);

    if(is_init){
        passed_packets_rx_offset = in_receives;
        passed_packets_tx_offset = out_requests;
    } else {
        passed_packets_rx = in_receives - passed_packets_rx_offset;
        passed_packets_tx = out_requests - passed_packets_tx_offset;
        passed_packets = passed_packets_rx + passed_packets_tx;
    }
}

void init_nic_stat(){
    get_nic_stat(true);

    syslog(LOG_WARNING, "offset_rx: %llu, offset_tx: %llu", 
        passed_packets_rx_offset, passed_packets_tx_offset);
}

void shm_end() {
    munmap(shm_ptr, SHM_SIZE);
    close(fd);
}

void check_p4_execution() {
    int i;
    unsigned long long executions = 0;

    for(i=0; i<MAX_CONNECTIONS; i++){
        executions += session[i].packet_count;
    }

    get_nic_stat(false);

    double diff = ((double)executions - (double)passed_packets) /
           (double)passed_packets * 100.0;

    syslog(LOG_WARNING, "diff: %.2f %%, P4 executed: %llu, packet proccessed: %llu", 
        diff, executions, passed_packets);
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
    init_wolf();
    init_nic_stat();

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
        
        check_p4_execution();

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