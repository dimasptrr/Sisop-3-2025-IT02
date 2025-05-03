# Sisop-3-2025-IT02

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
