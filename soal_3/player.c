// ===== player.c =====
// Bagian a–h: Client RPC + menu + error handling

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT   8080
#define IP     "127.0.0.1"
#define MAXBUF 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock<0) { perror("socket"); exit(1); }

    struct sockaddr_in srv = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    if (inet_pton(AF_INET,IP,&srv.sin_addr)<=0) {
        perror("inet_pton"); exit(1);
    }
    if (connect(sock,(void*)&srv,sizeof(srv))<0) {
        perror("connect"); exit(1);
    }

    char sendbuf[MAXBUF], recvbuf[MAXBUF];
    ssize_t n;

    // a) ENTER
    strcpy(sendbuf,"ENTER\n");
    write(sock,sendbuf,strlen(sendbuf));
    n = read(sock,recvbuf,MAXBUF-1);
    if (n>0) {
        recvbuf[n]='\0';
        printf("%s",recvbuf);
    }

    // Main Menu
    while (1) {
    	printf("           />____________________________________\n");
	printf("          / /				       |_\\\n");
    	printf(" [########[]					_|\\\n");
	printf("	  \\ \\__________________________________|_/\n");
 	printf("           \\>\n");
	printf(" ________________________________________________ \n");
	printf("|	         THE LOST DUNGEON                |\n");
	printf("+------------------------------------------------+\n");
    	printf("|  [1] Show Player Stats                         |\n");
    	printf("|  [2] Shop (Buy Weapons)                        |\n");
    	printf("|  [3] View Inventory & Equip Weapons            |\n");
    	printf("|  [4] Battle Mode                               |\n");
    	printf("|  [5] Exit Game                                 |\n");
    	printf("+------------------------------------------------+\n");
    	printf("  Choose an option (1-5): ");
        fflush(stdout);

        int pilihan;
        if (scanf("%d",&pilihan)!=1) {
            while(getchar()!='\n');
            printf("Invalid option. Please try again.\n");
            continue;
        }
        while(getchar()!='\n');

        // Map & Error Handling menu (h)
        switch(pilihan) {
            case 1: strcpy(sendbuf,"SHOW_STATS\n");    break;
            case 2: strcpy(sendbuf,"SHOP\n");          break;
            case 3: strcpy(sendbuf,"INVENTORY\n");     break;
            case 4: strcpy(sendbuf,"BATTLE\n");        break;
            case 5: strcpy(sendbuf,"EXIT\n");          break;
            default:
                printf("Invalid option. Please try again.\n");
                continue;
        }

        // 1 & 5 & default-simple commands
        write(sock,sendbuf,strlen(sendbuf));
        if (pilihan==5) { printf("Keluar dari game. Bye!\n"); break; }

        // 2) SHOP flow
        if (pilihan==2) {
            // terima shop list + prompt
            n = read(sock,recvbuf,MAXBUF-1);
            recvbuf[n]='\0'; printf("%s",recvbuf);
            // input choice
            printf("> "); fflush(stdout);
            fgets(sendbuf,MAXBUF,stdin);
            write(sock,sendbuf,strlen(sendbuf));
            // confirm purchase
            n = read(sock,recvbuf,MAXBUF-1);
            recvbuf[n]='\0'; printf("%s",recvbuf);
            continue;
        }

        // 3) INVENTORY flow
        if (pilihan==3) {
            n = read(sock,recvbuf,MAXBUF-1);
            recvbuf[n]='\0'; printf("%s",recvbuf);
            printf("> "); fflush(stdout);
            fgets(sendbuf,MAXBUF,stdin);
            write(sock,sendbuf,strlen(sendbuf));
            n = read(sock,recvbuf,MAXBUF-1);
            recvbuf[n]='\0'; printf("%s",recvbuf);
            continue;
        }

        // 4) BATTLE flow
        if (pilihan==4) {
            // header hingga prompt
            while ((n=read(sock,recvbuf,MAXBUF-1))>0) {
                recvbuf[n]='\0'; printf("%s",recvbuf);
                if (strstr(recvbuf,"> ")) break;
            }
            // loop battle
            while (1) {
                if (!fgets(sendbuf,MAXBUF,stdin)) break;
                if (sendbuf[strlen(sendbuf)-1]!='\n')
                    strcat(sendbuf,"\n");
                write(sock,sendbuf,strlen(sendbuf));
                // baca hingga server prompt lagi atau flight
                while ((n=read(sock,recvbuf,MAXBUF-1))>0) {
                    recvbuf[n]='\0'; printf("%s",recvbuf);
                    if (strstr(recvbuf,"> ") ||
                        strstr(recvbuf,"fled") ||
                        strstr(recvbuf,"earned")) break;
                }
                if (strncmp(sendbuf,"exit",4)==0) break;
            }
            continue;
        }

        // 1) SHOW_STATS & other simple responses
        n = read(sock,recvbuf,MAXBUF-1);
        if (n<=0) break;
        recvbuf[n]='\0';
        printf("%s",recvbuf);
    }

    close(sock);
    return 0;
}
