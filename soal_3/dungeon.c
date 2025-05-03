// ===== dungeon.c =====
// Bagian a–g: RPC + multithread + ENTER + SHOW_STATS + SHOP + INVENTORY + BATTLE + Error Handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

// Prototipe & extern dari shop.c
typedef struct {
    char   name[32];
    int    price;
    int    damage;
    char   passive[64];
    int    crit_chance;
    int    passive_chance;
} Weapon;
extern Weapon weaponList[];
extern int NUM_WEAPONS;
extern int playerGold;
extern int equippedIndex;
extern int owned[];
int  handleShop(char*, size_t);
int  buyWeapon(int, char*, size_t);
int  handleInventory(char*, size_t);
int  equipWeapon(int, char*, size_t);
void getPlayerStats(char*, size_t, int);

#define PORT     8080
#define BACKLOG  10
#define MAXBUF   1024

// Utility: render health bar width=20
void render_healthbar(char *buf, size_t sz, int cur, int max) {
    int barW   = 20;
    int filled = (cur*barW)/max;
    strncat(buf, "[", sz-strlen(buf)-1);
    for (int i=0; i<filled; i++) strncat(buf,"=",sz-strlen(buf)-1);
    for (int i=filled; i<barW; i++) strncat(buf," ",sz-strlen(buf)-1);
    strncat(buf, "] ", sz-strlen(buf)-1);
    char tmp[32];
    snprintf(tmp,sizeof(tmp),"%d/%d HP\n",cur,max);
    strncat(buf,tmp,sz-strlen(buf)-1);
}

// Handler tiap client
void *handle_client(void *arg) {
    int client = *(int*)arg;
    free(arg);

    char buf[MAXBUF], sendbuf[2048];
    ssize_t n;
    int kills = 0;

    while ((n = read(client, buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';

        // a) ENTER
        if (strncmp(buf, "ENTER", 5) == 0) {
            snprintf(sendbuf, sizeof(sendbuf),
                     "SERVER: Welcome to The Lost Dungeon!\n");

        // b) SHOW_STATS
        } else if (strncmp(buf, "SHOW_STATS",10)==0) {
            getPlayerStats(sendbuf, sizeof(sendbuf), kills);

        // c) SHOP
        } else if (strncmp(buf, "SHOP",4)==0) {
            if (handleShop(sendbuf, sizeof(sendbuf))) {
                write(client, sendbuf, strlen(sendbuf));
                // tunggu input angka pilihan
                n = read(client, buf, sizeof(buf)-1);
                if (n<=0) break;
                buf[n]='\0';
                int choice = atoi(buf);
                buyWeapon(choice, sendbuf, sizeof(sendbuf));
            }

        // d) INVENTORY
        } else if (strncmp(buf, "INVENTORY",9)==0) {
            if (handleInventory(sendbuf, sizeof(sendbuf))) {
                write(client, sendbuf, strlen(sendbuf));
                n = read(client, buf, sizeof(buf)-1);
                if (n<=0) break;
                buf[n]='\0';
                int idx = atoi(buf);
                equipWeapon(idx, sendbuf, sizeof(sendbuf));
            }

        // f+g) BATTLE MODE
        } else if (strncmp(buf, "BATTLE",6)==0) {
            int enemy_max = rand()%151 + 50;
            int enemy_cur = enemy_max;
            ssize_t bn;

            // Header battle
            snprintf(sendbuf, sizeof(sendbuf),
                "=== BATTLE STARTED ===\nEnemy appeared with:\n");
            render_healthbar(sendbuf,sizeof(sendbuf),enemy_cur,enemy_max);
            strncat(sendbuf,"Type 'attack' or 'exit'.\n> ",
                    sizeof(sendbuf)-strlen(sendbuf)-1);
            write(client, sendbuf, strlen(sendbuf));

            // Loop battle
            while ((bn=read(client,buf,sizeof(buf)-1))>0) {
                buf[bn]='\0';
                if (strncmp(buf,"exit",4)==0) {
                    write(client,"You fled the battle.\n",21);
                    break;
                }
                if (strncmp(buf,"attack",6)==0) {
                    extern Weapon weaponList[];
                    extern int equippedIndex;
                    extern int playerGold;

                    Weapon *w = &weaponList[equippedIndex];

                    // damage: base + rand up to +50%
                    int base=w->damage;
                    int var = rand()%(base/2+1);
                    int dmg = base+var;
                    // critical?
                    int isCrit = (rand()%100)<w->crit_chance;
                    if (isCrit) dmg*=2;
                    // passive?
                    int isPassive = w->passive_chance>0 &&
                                    (rand()%100)<w->passive_chance;
                    enemy_cur -= dmg;

                    char *p = sendbuf;
                    size_t left = sizeof(sendbuf);
                    p += snprintf(p,left,
                        "You dealt %d damage%s%s!\n\n",
                        dmg,
                        isCrit?" (CRITICAL)":"",
                        isPassive?" (Passive activated!)":"");
                    left = sizeof(sendbuf)-(p-sendbuf);

                    if (enemy_cur>0) {
                        p += snprintf(p,left,
                          "=== ENEMY STATUS ===\nEnemy health: ");
                        left = sizeof(sendbuf)-(p-sendbuf);
                        render_healthbar(p,left,enemy_cur,enemy_max);
                        strncat(sendbuf,"> ",
                                sizeof(sendbuf)-strlen(sendbuf)-1);
                    } else {
                        int reward = rand()%101 + 50;
                        playerGold += reward;
                        p += snprintf(p,left,
                            "You defeated the enemy!\n\n"
                            "=== REWARD ===\nYou earned %d gold!\n\n"
                            "=== NEW ENEMY ===\nEnemy health: ",
                            reward);
                        left = sizeof(sendbuf)-(p-sendbuf);
                        enemy_max = rand()%151 + 50;
                        enemy_cur = enemy_max;
                        render_healthbar(p,left,enemy_cur,enemy_max);
                        strncat(sendbuf,"> ",
                                sizeof(sendbuf)-strlen(sendbuf)-1);
                    }
                    write(client, sendbuf, strlen(sendbuf));
                    continue;
                }
                // Unknown in battle
                write(client,"Unknown battle command.\n> ",27);
            }

        // h) Invalid RPC
        } else {
            snprintf(sendbuf, sizeof(sendbuf),
                     "SERVER: Invalid command. Please try again.\n");
        }

        // Kirim kalau bukan SHOP/INVENTORY/BATTLE yang sudah kirim sendiri
        write(client, sendbuf, strlen(sendbuf));
    }

    close(client);
    return NULL;
}

int main() {
    srand(time(NULL));

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd<0) { perror("socket"); exit(1); }

    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
    if (bind(server_fd,(void*)&addr,sizeof(addr))<0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd,BACKLOG)<0) {
        perror("listen"); exit(1);
    }
    printf("SERVER: Listening on port %d...\n",PORT);

    while (1) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int *c = malloc(sizeof(int));
        *c = accept(server_fd,(void*)&cli,&len);
        if (*c<0) { free(c); continue; }
        pthread_t tid;
        pthread_create(&tid,NULL,handle_client,c);
        pthread_detach(tid);
    }
    close(server_fd);
    return 0;
}
