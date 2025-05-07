# Sisop-3-2025-IT02

# Soal Ketiga
- **Server**: `image_server.c`  
- **Client**: `image_client.c`

#### ➡️ Kode image_server.c
```c
void daemonize() {
  pid_t pid, sid;

  pid = fork();

  if (pid < 0)
    exit(EXIT_FAILURE);
  if (pid > 0)
    exit(EXIT_SUCCESS);

  umask(0);

  sid = setsid();
  if (sid < 0)
    exit(EXIT_FAILURE);

  if ((chdir("/home/kali/modul3/soal_1/")) < 0)
    exit(EXIT_FAILURE);

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}
```
#### ➡️ daemon
- Fungsi ini digunakan supaya program server bisa berjalan di background (sebagai daemon).
- Langkah sederhananya:
    - fork() ➔ membuat proses baru, proses lama keluar.
    - setsid() ➔ pisah dari terminal, jadi proses bebas.
    - chdir() ➔ pindah ke folder kerja soal.
    - close() ➔ menutup input/output supaya server tidak muncul di terminal.
- Tujuan:
    - Agar server bisa jalan otomatis tanpa tampilan di terminal, seperti service di Linux.
 
```c
void write_log(const char *source, const char *action, const char *info) {
  FILE *log_file = fopen("server.log", "a");
  if (!log_file) return;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

  fprintf(log_file, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
  fclose(log_file);
}
```

#### ➡️ Fungsi write_log ini dipakai untuk mencatat:
- Perintah `decrypt`
- Perintah `download`
- Perintah `exit`
- Perintah yang tidak valid (invalid command)

#### 🎯 **Tujuan:**
Supaya aktivitas server bisa dicek/dilacak kapan saja dengan membuka file `server.log`.

```c
void parse_buffer(char *buffer, char *command, char *data) {
  char temp[BUFFER_SIZE];
  strncpy(temp, buffer, BUFFER_SIZE);
  temp[BUFFER_SIZE - 1] = '\0';

  char *token = strtok(temp, "-");
  if (token != NULL) {
      strcpy(command, token);

      token = strtok(NULL, "-");
      if (token != NULL) {
          strcpy(data, token);
      } else {
          strcpy(data, "");
      }

  } else {
      strcpy(command, "");
      strcpy(data, "");
  }

  command[strcspn(command, "\r\n")] = 0;
  data[strcspn(data, "\r\n")] = 0;
}
```

#### ➡️ Fungsi parse_buffer
- Fungsi parse_buffer berfungsi untuk memecah string input (buffer) menjadi dua bagian: command dan data, yang dipisahkan oleh tanda hubung (-). Fungsi ini bekerja dengan cara:
    - Menyalin string buffer ke variabel sementara (temp) untuk menghindari perubahan pada buffer asli.
    - Menggunakan strtok() untuk memisahkan string berdasarkan tanda -.
    - Menyimpan bagian pertama sebagai command dan bagian kedua (jika ada) sebagai data.
    - Jika tidak ada tanda -, maka command dan data akan menjadi string kosong.
    - Menghapus karakter newline atau carriage return (\r\n) pada akhir command dan data.
- Fungsi ini membantu untuk menangani dan memecah perintah dari input yang berformat tertentu.



```c
void write_log(const char *source, const char *action, const char *info) {
  FILE *log_file = fopen("server.log", "a");
  if (!log_file) return;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

  fprintf(log_file, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
  fclose(log_file);
}
```
```c
void handle_decrypt(int client_fd, const char *data) {
  int len = strlen(data);

  char reversed[BUFFER_SIZE];
  for (int i = 0; i < len; i++) {
    reversed[i] = data[len - i - 1];
  }
  reversed[len] = '\0';

  unsigned char byte_array[BUFFER_SIZE];
  int byte_len = 0;
  for (int i = 0; i < len; i += 2) {
    char byte_str[3] = { reversed[i], reversed[i+1], 0 }; // 2 char + null
    byte_array[byte_len++] = (unsigned char)strtol(byte_str, NULL, 16);
  }

  time_t now = time(NULL);
  char filename[64];
  snprintf(filename, sizeof(filename), "database/%ld.jpeg", now);

  FILE *fp = fopen(filename, "wb");
  if (fp) {
    fwrite(byte_array, 1, byte_len, fp);
    fclose(fp);
  }

  char response[BUFFER_SIZE];
  snprintf(response, sizeof(response), "Saved %d bytes to %s", byte_len, filename);
  write(client_fd, response, strlen(response));
}
```
#### ➡️ fungsi handle_decrypt
- Fungsi ini menangani proses dekripsi data yang diterima dari klien:
    - Membalik urutan string hex: String hex yang diterima dibalik urutannya karakter per karakter (bukan per byte).
    - Konversi string hex menjadi array byte: Setelah dibalik, setiap pasangan karakter hex (misalnya FF) dikonversi menjadi byte menggunakan strtol() dan disimpan dalam array byte_array.
    - Simpan ke file dengan nama timestamp: Data byte yang dihasilkan disimpan ke dalam file .jpeg di direktori database/ dengan nama file yang dihasilkan dari timestamp saat itu (misalnya       1234567890.jpeg).
    - Kirim respon ke klien: Setelah file disimpan, fungsi mengirimkan pesan ke klien berisi jumlah byte yang disimpan dan nama file yang digunakan.
```c
void bytes_to_hex(const unsigned char *bytes, size_t len, char *hex_output) {
    const char hex_digits[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; ++i) {
        hex_output[i * 2] = hex_digits[(bytes[i] >> 4) & 0x0F];
        hex_output[i * 2 + 1] = hex_digits[bytes[i] & 0x0F];
    }
    hex_output[len * 2] = '\0'; // Null-terminate
}
```
#### ➡️ fungsi bytes_to_hex
- Fungsi ini mengonversi array byte menjadi string hex:
    - Setiap byte diubah menjadi dua karakter hex menggunakan lookup table (hex_digits), dan hasilnya disimpan dalam hex_output.
    - Fungsi ini mengonversi setiap byte ke dua digit hex dan menambahkan null-terminator di akhir string.
- Kedua fungsi bekerja bersama untuk mengonversi data hex menjadi byte dan menyimpannya sebagai file gambar, serta mengonversi kembali data byte menjadi representasi hex saat perlu.

```c
void handle_invalid(int client_fd) {
  char *msg = "Invalid Command\n";
  write(client_fd, msg, strlen(msg));
}
```
#### ➡️ fungsi handle_invalid
- Mengirimkan pesan "Invalid Command" kepada klien jika perintah yang diterima tidak valid.

```c
void run_rpc_server() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len;
  char buffer[BUFFER_SIZE];
  char command[BUFFER_SIZE];
  char data[BUFFER_SIZE];

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 5) < 0) {
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  while (1) {
    addr_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
      continue;
    }

    memset(buffer, 0, BUFFER_SIZE);
    read(client_fd, buffer, BUFFER_SIZE);

    memset(command, 0, BUFFER_SIZE);
    memset(data, 0, BUFFER_SIZE);
    parse_buffer(buffer, command, data);

    if (strcmp(command, "ping") == 0) {
      handle_ping(client_fd);
    } else if (strcmp(command, "decrypt") == 0) {
      handle_decrypt(client_fd, data);
      write_log("Client", "DECRYPT", "Text Data");

    } else if (strcmp(command, "download") == 0) {
      handle_download(client_fd,data);
      write_log("Client", "DOWNLOAD", "Text Data");

    } else if (strcmp(command, "exit") == 0) {
      close(client_fd);
      write_log("Client", "EXIT", "Client Requested Exit");

    } else {
      handle_invalid(client_fd);
      write_log("Client", "INVALID", buffer);
    }

    close(client_fd);
  }

  close(server_fd);
}
```
#### ➡️ fungsi run_rpc_server
- Fungsi ini mengatur server RPC yang menerima koneksi dari klien dan mengeksekusi perintah yang diterima.
    - Membuat socket server: Menggunakan socket() untuk membuat server dengan tipe koneksi TCP (SOCK_STREAM).
    - Mengikat socket: Menggunakan bind() untuk mengikat socket ke alamat IP dan port tertentu.
    - Mendengarkan koneksi: Menggunakan listen() untuk menunggu koneksi dari klien.
    - Menerima koneksi klien: Menggunakan accept() untuk menerima koneksi dari klien.
    - Membaca data perintah: Menerima data dari klien menggunakan read().
    - Memisahkan command dan data: Menggunakan parse_buffer() untuk memisahkan perintah (command) dan data (data).
- **Menangani perintah:**
    - Jika perintah adalah ping, memanggil handle_ping().
    - Jika perintah adalah decrypt, memanggil handle_decrypt() dan mencatat log.
    - Jika perintah adalah download, memanggil handle_download() dan mencatat log.
    - Jika perintah adalah exit, menutup koneksi dan mencatat log.
    - Jika perintah tidak valid, memanggil handle_invalid() dan mencatat log.
  - **Menutup koneksi:** Setelah menangani perintah, koneksi dengan klien ditutup menggunakan close().
- Fungsi ini memastikan server menerima dan merespons berbagai perintah dari klien, serta mencatat aktivitas ke dalam log.

```c
int main() {
  daemonize();

  if (mkdir("database", 0755) == -1 && errno != EEXIST) {
    perror("mkdir failed");
    exit(EXIT_FAILURE);
  }

  FILE *log_file = fopen("server.log", "a");
  if (!log_file) {
    perror("Failed to create/open server.log");
    exit(EXIT_FAILURE);
  }
  fclose(log_file);

  run_rpc_server();
  return 0;
}
```
#### ➡️ fungsi main:
- Fungsi utama yang menjalankan server RPC sebagai daemon.
- daemonize(): Mengubah proses menjadi daemon, sehingga berjalan di latar belakang.
- Membuat folder database/:
    - Menggunakan mkdir() untuk membuat folder tempat file hasil decrypt disimpan.
    - Jika folder sudah ada (EEXIST), lanjut tanpa error.
- Membuka (atau membuat) file log server.log:
    - Jika gagal membuka/menulis file log, program akan keluar.
- Menjalankan server:
    - Memanggil run_rpc_server() untuk memulai server dan menangani perintah dari klien.
**Kesimpulan singkat:** Fungsi main menyiapkan lingkungan (folder dan log), lalu menjalankan server daemon yang siap menerima koneksi.

# Soal Ketiga
# Lost Dungeon

**Lost Dungeon** adalah game RPG terminal berbasis client‑server yang sepenuhnya ditulis dalam bahasa C.  
Komponen utamanya:

- **Server**: `dungeon.c`  
- **Client**: `player.c`  
- **Modul Shop & Stats**: `shop.c`

Setiap file saling terhubung lewat “RPC manual” (perintah teks) dan socket TCP. Berikut penjelasan tiap fitur (soal A–H) dengan potongan kode asli.

---

## A. Entering the Dungeon

**Tujuan:** Server (`dungeon.c`) menerima banyak koneksi client dan memproses perintah `"ENTER"`. Client (`player.c`) mengirim perintah tersebut dan mencetak sambutan.

### Kode Server (`dungeon.c`)

```c
// … inside handle_client() …
if (strncmp(buf, "ENTER", 5) == 0) {
    snprintf(sendbuf, sizeof(sendbuf),
             "SERVER: Welcome to The Lost Dungeon!\n");
    write(client, sendbuf, strlen(sendbuf));
}
```
- Multithreaded: setiap accept() → pthread_create(handle_client)
- RPC manual: client kirim "ENTER\n" → server balas sambutan

### Kode Client (`player.c`)
```c
// a) ENTER
strcpy(sendbuf, "ENTER\n");
write(sock, sendbuf, strlen(sendbuf));

n = read(sock, recvbuf, MAXBUF-1);
recvbuf[n] = '\0';
printf("%s", recvbuf);
```
- Client connect() ke server dan menulis "ENTER\n".
- Membaca response dan mencetak ke layar.

## B. Sightseeing (Main Menu)
```c
// di dalam loop utama:
printf(" ________________________________________________ \n");
printf("|                 THE LOST DUNGEON                |\n");
printf("+------------------------------------------------+\n");
printf("|  [1] Show Player Stats                         |\n");
printf("|  [2] Shop (Buy Weapons)                        |\n");
printf("|  [3] View Inventory & Equip Weapons            |\n");
printf("|  [4] Battle Mode                               |\n");
printf("|  [5] Exit Game                                 |\n");
printf("+------------------------------------------------+\n");
printf("  Choose an option (1-5): ");
fflush(stdout);
```
- Main Menu dicetak setiap iterasi.
- Setelah pilihan dibaca, client mengirim perintah RPC sesuai mapping (`"SHOW_STATS"`, `"SHOP"`, dll.).

## C. Status Check (Show Player Stats)
Tujuan: Opsi `1. Show Player Stats` menampilkan Gold, Equipped Weapon, Base Damage, Kills, dan Passive jika ada.

### Kode Modul Stats (`shop.c`)
```c
void getPlayerStats(char *response, size_t maxlen, int kills) {
    Weapon *w = &weaponList[equippedIndex];
    snprintf(response, maxlen,
        "=== PLAYER STATS ===\n"
        "Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d\n",
        playerGold, w->name, w->damage, kills);
    if (w->passive[0]) {
        strncat(response, "Passive: ", maxlen - strlen(response) - 1);
        strncat(response, w->passive, maxlen - strlen(response) - 1);
        strncat(response, "\n", maxlen - strlen(response) - 1);
    }
}
```

### Kode Server Memanggil Stat (`dungeon.c`)
```c
} else if (strncmp(buf, "SHOW_STATS", 10) == 0) {
    getPlayerStats(sendbuf, sizeof(sendbuf), kills);
    write(client, sendbuf, strlen(sendbuf));
}
```

### Kode Client Menerima Stat (`player.c`)
```c
case 1:
    strcpy(sendbuf, "SHOW_STATS\n");
    write(sock, sendbuf, strlen(sendbuf));
    n = read(sock, recvbuf, MAXBUF-1);
    recvbuf[n] = '\0';
    printf("%s", recvbuf);
    break;
```

## D. Weapon Shop

**Deskripsi Soal D:**  
Pemain dapat pergi ke toko senjata dan membeli salah satu dari 5+ senjata yang tersedia. Beberapa senjata memiliki efek _passive_. Setelah opsi **Shop** dipilih, client mengirim `SHOP`, server menampilkan daftar senjata (nama, harga, damage, passive) lalu menunggu nomor pilihan untuk memproses pembelian.

### 1. Client – `player.c`

```c
// player.c, di dalam switch(pilihan):
case 2:  // Shop (Buy Weapons)
    strcpy(sendbuf, "SHOP\n");
    write(sock, sendbuf, strlen(sendbuf));

// Terima daftar shop + prompt
n = read(sock, recvbuf, MAXBUF-1);
recvbuf[n] = '\0';
printf("%s", recvbuf);

// Input nomor senjata
printf("> "); fflush(stdout);
fgets(sendbuf, MAXBUF, stdin);
write(sock, sendbuf, strlen(sendbuf));

// Baca konfirmasi pembelian
n = read(sock, recvbuf, MAXBUF-1);
recvbuf[n] = '\0';
printf("%s", recvbuf);
continue;
```
- Client mengirim "SHOP\n".
- Client menampilkan daftar yang dikirim server, kemudian membaca nomor senjata.
- Client menulis pilihan kembali ke server dan mencetak hasil buyWeapon().

### 2. Modul Shop - `shop.c`
```c
// shop.c

// a–c) Menampilkan daftar shop
int handleShop(char *response, size_t maxlen) {
    snprintf(response, maxlen,
        "=== WEAPON SHOP ===\nYour Gold: %d\n\nAvailable Weapons:\n",
        playerGold);
    for (int i = 0; i < NUM_WEAPONS; i++) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
            "%d. %s (Price:%d, Dmg:%d%s%s)%s\n",
            i+1, weaponList[i].name,
            weaponList[i].price,
            weaponList[i].damage,
            weaponList[i].passive[0] ? ", Passive: " : "",
            weaponList[i].passive,
            owned[i] ? " (OWNED)" : ""
        );
        strncat(response, tmp, maxlen - strlen(response) - 1);
    }
    strncat(response,
        "\nChoose weapon to buy (enter number): ",
        maxlen - strlen(response) - 1);
    return 1;  // server akan tunggu input berikutnya
}

// c) Logika pembelian
int buyWeapon(int choice, char *response, size_t maxlen) {
    if (choice < 1 || choice > NUM_WEAPONS || owned[choice-1]) {
        snprintf(response, maxlen, "Invalid weapon choice.\n");
        return 0;
    }
    Weapon *w = &weaponList[choice-1];
    if (playerGold < w->price) {
        snprintf(response, maxlen, "Not enough gold to buy %s.\n", w->name);
        return 0;
    }
    playerGold -= w->price;
    owned[choice-1] = 1;
    equippedIndex  = choice-1;
    snprintf(response, maxlen, "You bought and equipped %s!\n", w->name);
    return 0;
}
```
- `handleShop()` membangun string daftar senjata dan mengembalikan `1` untuk menandakan server harus membaca input pilihan.
- `buyWeapon()` memvalidasi pilihan, memotong `playerGold`, menandai senjata sebagai owned, dan mengubah `equippedIndex`.

### 3. Server - `dungeon.c`
```c
// dungeon.c, di dalam handle_client():
} else if (strncmp(buf, "SHOP", 4) == 0) {
    if (handleShop(sendbuf, sizeof(sendbuf))) {
        write(client, sendbuf, strlen(sendbuf));
        // Baca nomor pilihan
        n = read(client, buf, sizeof(buf)-1);
        buf[n] = '\0';
        int choice = atoi(buf);
        buyWeapon(choice, sendbuf, sizeof(sendbuf));
    }
}
```
- Saat server menerima `"SHOP"`, ia memanggil `handleShop()`, mengirim daftar, lalu menunggu dan memproses `buyWeapon()` sesuai input.

## E. Handy Inventory

**Deskripsi Soal E:**
Pemain dapat membuka inventaris untuk melihat semua senjata yang dimiliki dan memilih salah satu yang akan dipakai. Passive ditampilkan jika ada, dan `SHOW_STATS` selanjutnya akan mencerminkan senjata yang dipakai.

### 1. Client - `player.c`
```c
// player.c, di dalam switch(pilihan):
case 3:  // View Inventory & Equip
    strcpy(sendbuf, "INVENTORY\n");
    write(sock, sendbuf, strlen(sendbuf));

    // Terima daftar inventaris + prompt
    n = read(sock, recvbuf, MAXBUF-1);
    recvbuf[n] = '\0';
    printf("%s", recvbuf);

    // Input index equip
    printf("> "); fflush(stdout);
    fgets(sendbuf, MAXBUF, stdin);
    write(sock, sendbuf, strlen(sendbuf));

    // Baca konfirmasi equip
    n = read(sock, recvbuf, MAXBUF-1);
    recvbuf[n] = '\0';
    printf("%s", recvbuf);
    continue;
```
- Client mengirim `"INVENTORY\n"`.
- Client menampilkan inventaris, mengirimkan index pilihan, lalu mencetak respon `equipWeapon()`.

### 2. Modul Shop - `shop.c`
```c
// shop.c

// d) Tampilkan inventaris
int handleInventory(char *response, size_t maxlen) {
    snprintf(response, maxlen, "=== YOUR INVENTORY ===\n");
    for (int i = 0; i < NUM_WEAPONS; i++) {
        if (!owned[i]) continue;
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
            "[%d] %s (Dmg:%d%s%s)%s\n",
            i, weaponList[i].name, weaponList[i].damage,
            weaponList[i].passive[0] ? ", Passive: " : "",
            weaponList[i].passive,
            i == equippedIndex ? " (EQUIPPED)" : ""
        );
        strncat(response, tmp, maxlen - strlen(response) - 1);
    }
    strncat(response,
        "\nChoose weapon to equip (enter index): ",
        maxlen - strlen(response) - 1);
    return 1;
}

// d) Proses equip
int equipWeapon(int choice, char *response, size_t maxlen) {
    if (choice < 0 || choice >= NUM_WEAPONS || !owned[choice]) {
        snprintf(response, maxlen, "Invalid equip choice.\n");
    } else {
        equippedIndex = choice;
        snprintf(response, maxlen, "You equipped %s!\n",
                 weaponList[choice].name);
    }
    return 0;
}
```
- `handleInventory()` mencetak semua senjata milik pemain dan menandai yang terpasang.
- `equipWeapon()` memvalidasi dan mengubah `equippedIndex`.

### 3. Server - `dungeon.c`
```c
// dungeon.c, di dalam handle_client():
} else if (strncmp(buf, "INVENTORY", 9) == 0) {
    if (handleInventory(sendbuf, sizeof(sendbuf))) {
        write(client, sendbuf, strlen(sendbuf));
        // Baca index equip
        n = read(client, buf, sizeof(buf)-1);
        buf[n] = '\0';
        int idx = atoi(buf);
        equipWeapon(idx, sendbuf, sizeof(sendbuf));
    }
}
```
- Saat server menerima `"INVENTORY"`, ia memanggil `handleInventory()`, mengirim daftar, lalu memproses `equipWeapon()` sesuai input client.

## F. Enemy Encounter (Battle Mode)

**Deskripsi Soal F:**  
Ketika pemain memilih **Battle Mode**, mereka akan bertemu musuh dengan health bar, dapat mengetik `attack` atau `exit`, dan melihat health bar musuh berubah setiap serangan. Jika darah musuh habis, pemain mendapat reward gold dan musuh baru muncul.

---

### 1. Client – `player.c`

```c
// player.c, case 4 di switch(pilihan):
case 4:
    // Kirim permintaan battle
    strcpy(sendbuf, "BATTLE\n");
    write(sock, sendbuf, strlen(sendbuf));

    // 1) Terima header battle hingga prompt "> "
    while ((n = read(sock, recvbuf, MAXBUF-1)) > 0) {
        recvbuf[n] = '\0';
        printf("%s", recvbuf);
        if (strstr(recvbuf, "> ")) break;
    }

    // 2) Loop menerima input 'attack' atau 'exit'
    while (1) {
        // Baca perintah user
        if (!fgets(sendbuf, MAXBUF, stdin)) break;
        if (sendbuf[strlen(sendbuf)-1] != '\n')
            strcat(sendbuf, "\n");
        write(sock, sendbuf, strlen(sendbuf));

        // 3) Terima response hingga prompt baru atau battle selesai
        while ((n = read(sock, recvbuf, MAXBUF-1)) > 0) {
            recvbuf[n] = '\0';
            printf("%s", recvbuf);
            if (strstr(recvbuf, "> ") ||
                strstr(recvbuf, "fled") ||
                strstr(recvbuf, "earned")) {
                break;
            }
        }
        // Jika user memilih exit, keluar loop battle
        if (strncmp(sendbuf, "exit", 4) == 0) break;
    }
    continue;
```
- Client menunggu prompt `"> "` dari server,
- Mengirim `attack\n` atau `exit\n`,
- Mencetak balasan server (damage, health bar, reward),
- Keluar jika `exit`.

### Server - `dungeon.c`
```c
// dungeon.c di handle_client(), menggantikan stub BATTLE:
} else if (strncmp(buf, "BATTLE", 6) == 0) {
    // Inisialisasi HP musuh random 50–200
    int enemy_max = rand() % 151 + 50;
    int enemy_cur = enemy_max;
    ssize_t bn;

    // Kirim header battle + health bar awal
    snprintf(sendbuf, sizeof(sendbuf),
        "=== BATTLE STARTED ===\n"
        "Enemy appeared with:\n");
    render_healthbar(sendbuf, sizeof(sendbuf), enemy_cur, enemy_max);
    strncat(sendbuf, "Type 'attack' or 'exit'.\n> ",
            sizeof(sendbuf)-strlen(sendbuf)-1);
    write(client, sendbuf, strlen(sendbuf));

    // Loop battle: baca perintah hingga 'exit'
    while ((bn = read(client, buf, sizeof(buf)-1)) > 0) {
        buf[bn] = '\0';

        if (strncmp(buf, "exit", 4) == 0) {
            write(client, "You fled the battle.\n", 21);
            break;
        }

        if (strncmp(buf, "attack", 6) == 0) {
            // … (logika attack & reward ada di bagian G) …
            // Setelah memproses, kirim updated health bar atau reward
            // Lihat bagian G untuk detail implementasi
        }

        // Perintah tak dikenal di battle
        write(client, "Unknown battle command.\n> ", 27);
    }
}
```
- Server meng-generate musuh baru dengan HP random,
- Memanggil `render_healthbar()` untuk visualisasi,
- Baca command `attack`/`exit`, atau menolak command lain.

## G. Other Battle Logic
**Deskripsi Soal G:**
Tingkatkan battle dengan:
1. Random HP musuh (50–200) dan random reward (50–150 gold).
2. Damage Equation: `damage = base + rand()%((base/2)+1)`.
3. Critical Hit: peluang sebesar `crit_chance` membuat damage ×2.
4. Passive: jika senjata punya `passive_chance`, tampilkan `"(Passive activated!)"`.

### Server – `dungeon.c` (lanjutan di blok `"attack"`)
```c
if (strncmp(buf, "attack", 6) == 0) {
    extern Weapon weaponList[];
    extern int equippedIndex;
    extern int playerGold;

    Weapon *w = &weaponList[equippedIndex];

    // 1) Damage random: base + [0..base/2]
    int base = w->damage;
    int var  = rand() % (base/2 + 1);
    int dmg  = base + var;

    // 2) Critical?
    int isCrit = (rand() % 100) < w->crit_chance;
    if (isCrit) dmg *= 2;

    // 3) Passive?
    int isPassive = (w->passive_chance > 0) &&
                    ((rand() % 100) < w->passive_chance);

    enemy_cur -= dmg;

    // 4) Bangun response string
    char *p = sendbuf;
    size_t left = sizeof(sendbuf);
    p += snprintf(p, left,
        "You dealt %d damage%s%s!\n\n",
        dmg,
        isCrit     ? " (CRITICAL)"          : "",
        isPassive  ? " (Passive activated!)" : "");
    left = sizeof(sendbuf) - (p - sendbuf);

    if (enemy_cur > 0) {
        // Tampilkan health bar sisa HP
        p += snprintf(p, left,
            "=== ENEMY STATUS ===\nEnemy health: ");
        left = sizeof(sendbuf) - (p - sendbuf);
        render_healthbar(p, left, enemy_cur, enemy_max);
        strncat(sendbuf, "> ", sizeof(sendbuf)-strlen(sendbuf)-1);
    } else {
        // Musuh mati: reward 50–150 gold
        int reward = rand() % 101 + 50;
        playerGold += reward;
        p += snprintf(p, left,
            "You defeated the enemy!\n\n"
            "=== REWARD ===\nYou earned %d gold!\n\n"
            "=== NEW ENEMY ===\nEnemy health: ",
            reward);
        left = sizeof(sendbuf) - (p - sendbuf);

        // Spawn musuh baru
        enemy_max = rand() % 151 + 50;
        enemy_cur = enemy_max;
        render_healthbar(p, left, enemy_cur, enemy_max);
        strncat(sendbuf, "> ", sizeof(sendbuf)-strlen(sendbuf)-1);
    }

    write(client, sendbuf, strlen(sendbuf));
    continue;  // kembali ke loop battle
}
```
- HP musuh dan reward di‐random.
- Damage bervariasi, ada chance critical (×2).
- Jika senjata punya passive, tampil string tambahan.
- Setelah musuh mati, langsung respawn musuh baru.

## H. Error Handling

**Deskripsi Soal H:**  
Menangani input atau perintah yang **tidak valid**, baik di sisi client (menu pilihan di `player.c`) maupun di sisi server (RPC command di `dungeon.c`).

---

### 1. Client – `player.c`

Pada loop **Main Menu**, jika pemain memasukkan angka di luar `1–5`, langsung tampilkan pesan error tanpa mengirim apa pun ke server:

```c
// player.c (di dalam switch(pilihan))
switch (pilihan) {
    case 1: strcpy(sendbuf, "SHOW_STATS\n");    break;
    case 2: strcpy(sendbuf, "SHOP\n");          break;
    case 3: strcpy(sendbuf, "INVENTORY\n");     break;
    case 4: strcpy(sendbuf, "BATTLE\n");        break;
    case 5: strcpy(sendbuf, "EXIT\n");          break;
    default:
        // H) Error Handling menu client
        printf("Invalid option. Please try again.\n");
        continue;  // kembali cetak menu, tidak kirim ke server
}
```
- `default` case menampilkan :
```
Invalid option. Please try again.
```
- Loop `continue` menjaga client tetap di menu utama.

### 2. Server - `dungeon.c`
Di dalam handler RPC, jika server menerima perintah teks yang tidak dikenali, kirimkan pesan error generik ke client:
```c
// dungeon.c (di akhir rangkaian if/else if)
} else {
    // H) Error Handling RPC server
    snprintf(sendbuf, sizeof(sendbuf),
             "SERVER: Invalid command. Please try again.\n");
}
// Kirim balasan jika bukan blok SHOP/INVENTORY/BATTLE yang sudah menulis sendiri
write(client, sendbuf, strlen(sendbuf));
```
- Pastikan blok `else` ini berada setelah semua kondisi `strncmp(..., "ENTER", ...), "SHOW_STATS", "SHOP", "INVENTORY", "BATTLE", "EXIT"`.
- Sehinnga server membalas dengan :
```
SERVER: Invalid command. Please try again.
```

## KESIMPULAN

Proyek **Lost Dungeon** menggabungkan mekanisme client‑server sederhana dengan gameplay RPG turn‑based di terminal. Berikut poin‑poin pentingnya:

1. **Modular & Terpisah**  
   - `dungeon.c` sebagai **server**: menangani koneksi banyak client (multithreading), parsing perintah RPC, dan logika utama (ENTER, SHOW_STATS, SHOP, INVENTORY, BATTLE, Error Handling).  
   - `shop.c` sebagai modul **Shop & Inventory**: mengelola daftar senjata, pembelian, inventarisasi, dan status pemain (gold, equipped weapon, passive).  
   - `player.c` sebagai **client**: antar muka menu interaktif ASCII‑art, mengirim perintah ke server, dan menampilkan hasil—dengan validasi input di sisi client.

2. **Flow Sesi Permainan**  
   - **A**: Client mengirim `ENTER`, server menyambut dengan pesan pembuka.  
   - **B**: Menu “The Lost Dungeon” ditampilkan, memberi pilihan fitur.  
   - **C**: `SHOW_STATS` menampilkan gold, weapon, damage, kills, dan passive.  
   - **D**: `SHOP` menampilkan senjata—pemain membeli dan langsung mengekuilkannya.  
   - **E**: `INVENTORY` melihat & mengganti senjata terpasang.  
   - **F–G**: `BATTLE` memulai encounter musuh dengan health bar, damage acak, critical & passive effect, reward gold, dan respawn musuh.  
   - **H**: Invalid input di client maupun server ditangani dengan pesan error yang jelas.

3. **Teknologi & Konsep Utama**  
   - **Socket TCP** untuk komunikasi client‑server.  
   - **Multithreading** (`pthread`) di server untuk melayani banyak client paralel.  
   - **RPC manual** via string commands (tanpa library RPC khusus).  
   - **Randomization** di battle (HP, damage variation, critical, passive, reward).  
   - **Modularisasi**: shop/inventory/stat logic terpisah di `shop.c` agar kode lebih terstruktur.
