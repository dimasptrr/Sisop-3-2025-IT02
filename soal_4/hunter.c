#include "shm_common.h"

void register_hunter(struct SystemData *system_data) {
    if (system_data->num_hunters >= MAX_HUNTERS) {
        printf("Sistem penuh. Tidak bisa menambahkan hunter baru.\n");
        return;
    }

    char username[50];
    printf("Masukkan username baru: ");
    scanf("%s", username);

    // Cek apakah username sudah digunakan
    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, username) == 0) {
            printf("Username sudah digunakan.\n");
            return;
        }
    }

    // Tambahkan hunter baru ke sistem
    struct Hunter *new_hunter = &system_data->hunters[system_data->num_hunters];
    strcpy(new_hunter->username, username);
    new_hunter->level = 1;
    new_hunter->exp = 0;
    new_hunter->atk = 10;
    new_hunter->hp = 100;
    new_hunter->def = 5;
    new_hunter->banned = 0;

    // Buat shared memory pribadi untuk hunter ini
    key_t hunter_key = ftok("/tmp", 'H' + system_data->num_hunters); // key unik per hunter
    int shmid = shmget(hunter_key, sizeof(struct Hunter), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Gagal membuat shared memory hunter");
        exit(1);
    }

    struct Hunter *personal_data = (struct Hunter *) shmat(shmid, NULL, 0);
    memcpy(personal_data, new_hunter, sizeof(struct Hunter));
    shmdt(personal_data);

    new_hunter->shm_key = hunter_key;

    system_data->num_hunters++;

    printf("Registrasi berhasil! Selamat datang, %s\n", username);

    printf("DEBUG: Terdaftar %s. Total sekarang: %d\n", new_hunter->username, system_data->num_hunters);

}

void login_hunter(struct SystemData *system_data) {
    char username[50];
    printf("Masukkan username: ");
    scanf("%s", username);

    for (int i = 0; i < system_data->num_hunters; i++) {
        if (strcmp(system_data->hunters[i].username, username) == 0) {
            if (system_data->hunters[i].banned) {
                printf("Hunter ini telah dibanned!\n");
                return;
            }

            // Hubungkan ke shared memory hunter
            key_t key = system_data->hunters[i].shm_key;
            int shmid = shmget(key, sizeof(struct Hunter), 0666);
            if (shmid < 0) {
                perror("Gagal mengakses data hunter");
                return;
            }

            struct Hunter *data = (struct Hunter *) shmat(shmid, NULL, 0);

            printf("\nLogin berhasil!\n");
            printf("Halo, %s!\n", data->username);
            printf("Level: %d | EXP: %d | HP: %d | ATK: %d | DEF: %d\n",
                   data->level, data->exp, data->hp, data->atk, data->def);

            // Tambahkan logika lain di sini (contoh: menu dungeon, dll.)
            shmdt(data);
            return;
        }
    }

    printf("Hunter tidak ditemukan.\n");
}

int main() {
    // Ambil shared memory dari sistem utama
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), 0666);
    if (shmid < 0) {
        perror("Gagal terhubung ke sistem (jalankan system terlebih dahulu)");
        exit(1);
    }

    struct SystemData *system_data = (struct SystemData *) shmat(shmid, NULL, 0);

    printf("1. Registrasi\n2. Login\n3. Keluar\nPilihan: ");
    int pilihan;
    scanf("%d", &pilihan);
	

    while(1){
    if (pilihan == 1) {
        register_hunter(system_data);
	break;
    } else if (pilihan == 2) {
        login_hunter(system_data);
	break;
    } else if (pilihan == 3) {
        printf("Berhasil keluar");
	break;
    } else {
	printf("invalid");
	break;
    }
}

    shmdt(system_data);
    return 0;
}

