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
