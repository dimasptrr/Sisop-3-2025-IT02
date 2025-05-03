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

// ========================== Sub Soal A: Log Pengiriman ==========================
void log_pengiriman(char *user_name, Order *order) {
    // Tulis log sekali saja untuk setiap pengiriman
    FILE *log = fopen("delivery.log", "a");
    if (log) {
        time_t now = time(NULL);  // Mendapatkan waktu saat ini
        struct tm *tm_info = localtime(&now);  // Mengonversi waktu ke struct tm
        char waktu[32];
        strftime(waktu, sizeof(waktu), "%d/%m/%Y %H:%M:%S", tm_info);  // Format waktu
        fprintf(log, "[%s] [AGENT %s] %s package delivered to %s in %s\n", 
                waktu, user_name, order->layanan, order->nama_penerima, order->alamat);
        fclose(log);
    }
}

// ========================== Sub Soal C: Pengiriman Bertipe Reguler ==========================
void pengiriman_reguler(char *user_name, SharedData *shared_data) {
    // Pengiriman untuk order bertipe Reguler (sub soal C)
    int found = 0;
    for (int i = 0; i < shared_data->jumlah_order; i++) {
        Order *order = &shared_data->orders[i];
        if (strcasecmp(order->layanan, "Reguler") == 0 && order->sudah_dikirim == 0) {
            // Tandai order sudah dikirim hanya jika belum dikirim sebelumnya
            if (order->sudah_dikirim == 0) {
                order->sudah_dikirim = 1;

                // Log pengiriman dilakukan hanya sekali
                log_pengiriman(user_name, order);

                // Tampilkan pesan pengiriman
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

// ========================== Sub Soal D: Cek Status Pesanan ==========================
void cek_status_pesanan(char *nama, SharedData *shared_data) {
    // Cek status pesanan berdasarkan nama penerima
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

// ========================== Sub Soal E: List Semua Pesanan ==========================
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
    // Handle perintah dari command line
    if (argc == 1) {
        // Jika tidak ada argumen, hanya baca file CSV dan simpan ke shared memory
        printf("Membaca file CSV dan menyimpan data ke shared memory...\n");

        FILE *file = fopen("delivery_order.csv", "r");
        if (!file) {
            perror("Gagal membuka file CSV");
            return 1;
        }

        // Membuat shared memory
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

        // Baca file CSV
        char buffer[256];
        fgets(buffer, sizeof(buffer), file); // skip header

        int i = 0;
        while (fgets(buffer, sizeof(buffer), file) && i < MAX_ORDERS) {
            // Baca setiap baris dalam CSV
            sscanf(buffer, "%99[^,],%99[^,],%99[^\n]",
                   shared_data->orders[i].nama_penerima,
                   shared_data->orders[i].alamat,
                   shared_data->orders[i].layanan);

            shared_data->orders[i].id = i + 1;  // ID otomatis berdasarkan urutan
            shared_data->orders[i].sudah_dikirim = 0;  // Setel status belum dikirim
            i++;
        }
        shared_data->jumlah_order = i;

        printf("Berhasil menyimpan %d order ke shared memory.\n", i);

        fclose(file);
        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else if (argc == 3 && strcmp(argv[1], "-deliver") == 0) {
        // Jika argumen adalah -deliver [Nama]
        char *user_name = argv[2];
        
        // Membuka shared memory
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

        // Cari order dengan nama penerima sesuai dengan input
        int found = 0;
        for (int i = 0; i < shared_data->jumlah_order; i++) {
            Order *order = &shared_data->orders[i];
            if (strcasecmp(order->nama_penerima, user_name) == 0 && order->sudah_dikirim == 0) {
                // Tandai order sudah dikirim hanya jika belum dikirim sebelumnya
                if (order->sudah_dikirim == 0) {
                    order->sudah_dikirim = 1;

                    // Log pengiriman dilakukan hanya sekali
                    log_pengiriman(user_name, order);

                    // Tampilkan pesan pengiriman
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
        // Jika argumen adalah -status [Nama]
        char *nama = argv[2];

        // Membuka shared memory untuk cek status pesanan
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

        // Cek status berdasarkan nama
        cek_status_pesanan(nama, shared_data);

        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else if (argc == 2 && strcmp(argv[1], "-list") == 0) {
        // ========================== Sub Soal E: List Semua Pesanan ==========================
        // Jika argumen adalah -list
        // Membuka shared memory untuk list semua pesanan
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

        // Menampilkan daftar pesanan
        list_semua_pesanan(shared_data);

        munmap(shared_data, sizeof(SharedData));
        close(shm_fd);
    } else {
        fprintf(stderr, "Usage: %s [-deliver [Nama]] [-status [Nama]] [-list]\n", argv[0]);
        return 1;
    }

    return 0;
}
