#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_ORDERS 100
#define MAX_STRING 100
#define SHM_NAME "/rushgo_shared"

typedef struct {
    int id; 
    char nama_penerima[MAX_STRING];
    char alamat[MAX_STRING];
    char layanan[MAX_STRING]; 
    int sudah_dikirim; 
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int jumlah_order;
} SharedData;

void log_pengiriman(char *user_name, Order *order) {
    FILE *log = fopen("delivery.log", "a");
    if (log) {
        time_t now = time(NULL);  
        struct tm *tm_info = localtime(&now);  
        char waktu[32];
        strftime(waktu, sizeof(waktu), "%d/%m/%Y %H:%M:%S", tm_info);  // Format waktu
        fprintf(log, "[%s] [AGENT %s] %s package delivered to %s in %s\n", 
                waktu, user_name, order->layanan, order->nama_penerima, order->alamat);
        fclose(log);
    }
}

void pengiriman_reguler(char *user_name, SharedData *shared_data) {
    int found = 0;
    for (int i = 0; i < shared_data->jumlah_order; i++) {
        Order *order = &shared_data->orders[i];
        if (strcasecmp(order->layanan, "Reguler") == 0 && order->sudah_dikirim == 0) {
            if (order->sudah_dikirim == 0) {
                order->sudah_dikirim = 1;

                log_pengiriman(user_name, order);

                printf("Order Reguler dengan nama penerima %s telah dikirim oleh agent %s.\n", 
                       order->nama_penerima, user_name);
                found = 1;
            }
            break;
        }
    }

    if (!found) {
        printf("Tidak ditemukan order Reguler yang belum dikirim.\n");
    }
}

void cek_status_pesanan(char *nama, SharedData *shared_data) {
    int found = 0;
    for (int i = 0; i < shared_data->jumlah_order; i++) {
        Order *order = &shared_data->orders[i];
        if (strcasecmp(order->nama_penerima, nama) == 0) {
            if (order->sudah_dikirim == 1) {
                printf("Status for %s: Delivered by Agent %s\n", order->nama_penerima, "Agent X"); // Bisa diubah sesuai agent
            } else {
                printf("Status for %s: Pending\n", order->nama_penerima);
            }
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Order dengan nama %s tidak ditemukan.\n", nama);
    }
}

void list_semua_pesanan(SharedData *shared_data) {
    printf("Daftar Pesanan:\n");
    for (int i = 0; i < shared_data->jumlah_order; i++) {
        Order *order = &shared_data->orders[i];
        printf("ID: %d, Nama Penerima: %s, Layanan: %s, Status: %s\n", 
               order->id, order->nama_penerima, order->layanan, 
               order->sudah_dikirim == 1 ? "Delivered" : "Pending");
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Membaca file CSV dan menyimpan data ke shared memory...\n");

        FILE *file = fopen("delivery_order.csv", "r");
        if (!file) {
            perror("Gagal membuka file CSV");
            return 1;
        }

        int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("Gagal membuat shared memory");
            return 1;
        }

        ftruncate(shm_fd, sizeof(SharedData));

        SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            perror("Gagal mmap");
            return 1;
        }

        char buffer[256];
        fgets(buffer, sizeof(buffer), file); 

        int i = 0;
        while (fgets(buffer, sizeof(buffer), file) && i < MAX_ORDERS) {
            sscanf(buffer, "%99[^,],%99[^,],%99[^\n]",
                   shared_data->orders[i].nama_penerima,
                   shared_data->orders[i].alamat,
                   shared_data->orders[i].layanan);

            shared_data->orders[i].id = i + 1;  
            shared_data->orders[i].sudah_dikirim = 0;  
            i++;
        }
        shared_data->jumlah_order = i;

        printf("Berhasil menyimpan %d order ke shared memory.\n", i);

        fclose(file);
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else if (argc == 3 && strcmp(argv[1], "-deliver") == 0) {
        char *user_name = argv[2];
        
        int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("Gagal membuka shared memory");
            return 1;
        }

        SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            perror("Gagal mmap");
            return 1;
        }

        int found = 0;
        for (int i = 0; i < shared_data->jumlah_order; i++) {
            Order *order = &shared_data->orders[i];
            if (strcasecmp(order->nama_penerima, user_name) == 0 && order->sudah_dikirim == 0) {
                if (order->sudah_dikirim == 0) {
                    order->sudah_dikirim = 1;

                    log_pengiriman(user_name, order);

                    printf("Order Reguler dengan nama penerima %s telah dikirim oleh agent %s.\n", 
                           order->nama_penerima, user_name);
                    found = 1;
                }
                break;
            }
        }

        if (!found) {
            printf("Tidak ditemukan order dengan nama penerima %s atau order sudah dikirim.\n", user_name);
        }

        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else if (argc == 3 && strcmp(argv[1], "-status") == 0) {
        char *nama = argv[2];

        int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
        if (shm_fd == -1) {
            perror("Gagal membuka shared memory");
            return 1;
        }

        SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            perror("Gagal mmap");
            return 1;
        }

        cek_status_pesanan(nama, shared_data);

        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else if (argc == 2 && strcmp(argv[1], "-list") == 0) {
        int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
        if (shm_fd == -1) {
            perror("Gagal membuka shared memory");
            return 1;
        }

        SharedData *shared_data = mmap(NULL, sizeof(SharedData), PROT_READ, MAP_SHARED, shm_fd, 0);
        if (shared_data == MAP_FAILED) {
            perror("Gagal mmap");
            return 1;
        }

        list_semua_pesanan(shared_data);

        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else {
        fprintf(stderr, "Usage: %s [-deliver [Nama]] [-status [Nama]] [-list]\n", argv[0]);
        return 1;
    }

    return 0;
}
