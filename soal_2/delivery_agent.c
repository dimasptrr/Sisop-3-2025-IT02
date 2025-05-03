#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_ORDERS 100
#define MAX_STRING 100
#define SHM_NAME "/rushgo_shared"

typedef struct {
    int id;  // ID order
    char nama_penerima[MAX_STRING];
    char alamat[MAX_STRING];
    char layanan[MAX_STRING]; // Express / Reguler
    int sudah_dikirim; // 0 = belum, 1 = sudah
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int jumlah_order;
} SharedData;

SharedData *shared_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void tulis_log(const char *agent, const char *nama, const char *alamat) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char waktu[32];
    strftime(waktu, sizeof(waktu), "%d/%m/%Y %H:%M:%S", tm_info);

    fprintf(log, "[%s] [%s] Express package delivered to %s in %s\n",
            waktu, agent, nama, alamat);
    fclose(log);
}

void* agent_worker(void *arg) {
    char *agent_name = (char *)arg;

    while (1) {
        int found = 0;

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < shared_data->jumlah_order; i++) {
            Order *o = &shared_data->orders[i];
            // Menggunakan strcasecmp untuk perbandingan tanpa memperhatikan huruf kapital
            if (strcasecmp(o->layanan, "Express") == 0 && o->sudah_dikirim == 0) {
                o->sudah_dikirim = 1;
                printf("[%s] Mengirim order ID %d ke %s\n", agent_name, o->id, o->nama_penerima);
                tulis_log(agent_name, o->nama_penerima, o->alamat);
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&mutex);

        if (!found) break; // Tidak ada lagi order Express
        sleep(1); // Simulasi pengiriman
    }

    pthread_exit(NULL);
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Gagal membuka shared memory");
        return 1;
    }

    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Gagal mmap");
        return 1;
    }

    pthread_t agents[3];
    char *agent_names[3] = {"AGENT A", "AGENT B", "AGENT C"};

    for (int i = 0; i < 3; i++) {
        pthread_create(&agents[i], NULL, agent_worker, agent_names[i]);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(agents[i], NULL);
    }

    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);
    return 0;
}
