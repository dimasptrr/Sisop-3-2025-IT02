#include "shm_common.h"

const char *nama_dungeon[] = {
    "Double Dungeon", "Demon Castle", "Pyramid Dungeon", "Red Gate Dungeon",
    "Hunters Guild Dungeon", "Busan A-Rank Dungeon", "Insects Dungeon",
    "Goblins Dungeon", "D-Rank Dungeon", "Gwanak Mountain Dungeon",
    "Hapjeong Subway Station Dungeon"
};
#define TOTAL_DUNGEON_NAMA 11

// Deklarasi fungsi
void tampilkan_hunters(struct SystemData *system_data);
void buat_dungeon(struct SystemData *system_data);
void tampilkan_dungeons(struct SystemData *system_data);

int main() {
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Gagal membuat shared memory sistem");
        exit(EXIT_FAILURE);
    }

    struct SystemData *system_data = (struct SystemData *)shmat(shmid, NULL, 0);
    if (system_data == (void *)-1) {
        perror("Gagal attach shared memory");
        exit(EXIT_FAILURE);
    }

    // Inisialisasi jika belum pernah dilakukan
    if (system_data->initialized != 1) {
        system_data->num_hunters = 0;
        system_data->num_dungeons = 0;
        system_data->current_notification_index = 0;
        system_data->initialized = 1;
    }

    int pilihan;
    while (1) {
        printf("\n===== SISTEM HUNTER =====\n");
        printf("1. Tampilkan semua hunter\n");
        printf("2. Buat dungeon baru\n");
        printf("3. Tampilkan semua dungeon\n");
        printf("4. Keluar\n");
        printf("Pilih: ");
        scanf("%d", &pilihan);
        getchar(); // hapus newline

        if (pilihan == 1) {
            tampilkan_hunters(system_data);
        } else if (pilihan == 2) {
            buat_dungeon(system_data);
        } else if (pilihan == 3) {
            tampilkan_dungeons(system_data);
        } else if (pilihan == 4) {
            printf("Keluar dari sistem.\n");
            break;
        } else {
            printf("Pilihan tidak valid.\n");
        }
    }

    shmdt(system_data);
    return 0;
}

void tampilkan_hunters(struct SystemData *system_data) {
    printf("\n=== INFORMASI SEMUA HUNTER TERDAFTAR ===\n");
    if (system_data->num_hunters == 0) {
        printf("Belum ada hunter terdaftar.\n");
        return;
    }

    printf("%-3s %-15s %-6s %-6s %-6s %-6s %-6s %-10s\n",
           "No", "Username", "Level", "EXP", "ATK", "HP", "DEF", "Status");
    printf("-------------------------------------------------------------\n");

    for (int i = 0; i < system_data->num_hunters; i++) {
        struct Hunter *h = &system_data->hunters[i];
        printf("%-3d %-15s %-6d %-6d %-6d %-6d %-6d %-10s\n",
               i + 1, h->username, h->level, h->exp, h->atk, h->hp, h->def,
               h->banned ? "BANNED" : "ACTIVE");
    }
}

void buat_dungeon(struct SystemData *system_data) {
    if (system_data->num_dungeons >= MAX_DUNGEONS) {
        printf("Kapasitas dungeon penuh!\n");
        return;
    }

    srand(time(NULL) + rand()); // acak
    int idx = system_data->num_dungeons;
    struct Dungeon *d = &system_data->dungeons[idx];

    strcpy(d->name, nama_dungeon[idx % TOTAL_DUNGEON_NAMA]);
    d->min_level = idx * 2; // Level dungeon naik 2 per dungeon
    d->atk = rand() % 51 + 100;   // 100-150
    d->hp = rand() % 51 + 50;     // 50-100
    d->def = rand() % 26 + 25;    // 25-50
    d->exp = rand() % 151 + 150;  // 150-300

    // Buat shared memory dungeon
    key_t dungeon_key = ftok("/tmp", 'D' + idx);
    int dshm = shmget(dungeon_key, sizeof(struct Dungeon), IPC_CREAT | 0666);
    if (dshm == -1) {
        perror("Gagal membuat shared memory dungeon");
        return;
    }

    d->shm_key = dungeon_key;

    struct Dungeon *shared_d = (struct Dungeon *)shmat(dshm, NULL, 0);
    if (shared_d == (void *)-1) {
        perror("Gagal attach dungeon shm");
        return;
    }

    memcpy(shared_d, d, sizeof(struct Dungeon));
    shmdt(shared_d);

    system_data->num_dungeons++;
    printf("Dungeon \"%s\" level %d berhasil dibuat!\n", d->name, d->min_level);
}

void tampilkan_dungeons(struct SystemData *system_data) {
    printf("\n=== DAFTAR DUNGEON TERSEDIA ===\n");
        if (system_data->num_dungeons == 0) {
            printf("Belum ada dungeon dibuat.\n");
            return;
        }
    
    for (int i = 0; i < system_data->num_dungeons; i++) {
        struct Dungeon *d = &system_data->dungeons[i];
        printf("%d. %s\n", i + 1, d->name);
        printf("   - Level Minimum : %d\n", d->min_level);
        printf("   - EXP           : %d\n", d->exp);
        printf("   - ATK           : %d\n", d->atk);
        printf("   - HP            : %d\n", d->hp);
        printf("   - DEF           : %d\n", d->def);
        printf("   - Shared Key    : %d\n", d->shm_key);
    }
}
    

