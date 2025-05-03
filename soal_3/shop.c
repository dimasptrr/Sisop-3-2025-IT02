// ===== shop.c =====
// Bagian C: Weapon Shop & Inventory & Stats

#include <stdio.h>
#include <string.h>

typedef struct {
    char   name[32];
    int    price;
    int    damage;
    char   passive[64];
    int    crit_chance;     // % peluang critical hit
    int    passive_chance;  // % peluang passive aktif
} Weapon;

Weapon weaponList[] = {
    // name            price dmg passive                  crit% passive%
    {"Fists",            0,   5,  "-",                      10,    0},
    {"Iron Sword",     100,  15,  "-",                      15,    0},
    {"Flame Blade",    200,  25,  "Burns 2 dmg/turn",      20,   30},
    {"Frost Axe",      300,  30,  "Slows enemy 50%",       20,   25},
    {"Thunder Hammer", 500,  40,  "25% stun chance",       25,   20},
    {"Shadow Dagger",  400,  35,  "Crit boost next hit",   30,   40}
};
int NUM_WEAPONS = sizeof(weaponList)/sizeof(Weapon);

// Global state pemain
int playerGold    = 500;
int equippedIndex = 0;
int owned[6]      = {1,0,0,0,0,0};  // owned[0]=1 (Fists) otomatis

// a–c) Tampilkan daftar shop
// return 1 jika server harus menunggu choice berikutnya
int handleShop(char *response, size_t maxlen) {
    char tmp[128];
    snprintf(response, maxlen,
        "=== WEAPON SHOP ===\nYour Gold: %d\n\nAvailable Weapons:\n",
        playerGold);
    for (int i = 0; i < NUM_WEAPONS; i++) {
        snprintf(tmp, sizeof(tmp),
            "%d. %s (Price:%d, Dmg:%d%s%s)%s\n",
            i+1,
            weaponList[i].name,
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
    return 1;
}

// c) Proses pembelian
int buyWeapon(int choice, char *response, size_t maxlen) {
    if (choice < 1 || choice > NUM_WEAPONS) {
        snprintf(response, maxlen, "Invalid weapon choice.\n");
        return 0;
    }
    Weapon *w = &weaponList[choice-1];
    if (owned[choice-1]) {
        snprintf(response, maxlen, "You already own %s.\n", w->name);
        return 0;
    }
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

// d) Tampilkan inventory & prompt equip
int handleInventory(char *response, size_t maxlen) {
    char tmp[128];
    snprintf(response, maxlen, "=== YOUR INVENTORY ===\n");
    for (int i = 0; i < NUM_WEAPONS; i++) {
        if (!owned[i]) continue;
        snprintf(tmp, sizeof(tmp),
            "[%d] %s (Dmg:%d%s%s)%s\n",
            i,
            weaponList[i].name,
            weaponList[i].damage,
            weaponList[i].passive[0] ? ", Passive: " : "",
            weaponList[i].passive,
            i==equippedIndex ? " (EQUIPPED)" : ""
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

// b & e) Tampilkan stats (base + weapon + passive)
void getPlayerStats(char *response, size_t maxlen, int kills) {
    Weapon *w = &weaponList[equippedIndex];
    int totalDmg = w->damage;
    snprintf(response, maxlen,
        "=== PLAYER STATS ===\n"
        "Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d\n",
        playerGold, w->name, totalDmg, kills);
    if (w->passive[0]) {
        strncat(response, "Passive: ", maxlen - strlen(response) - 1);
        strncat(response, w->passive, maxlen - strlen(response) - 1);
        strncat(response, "\n", maxlen - strlen(response) - 1);
    }
}
