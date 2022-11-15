#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

// nc -ltv 8080
// ps -a
// netstat -ano
// ps -all

void print_instructions()
{
    //Open instruction manual
    FILE *f;
    f = fopen("instructions.txt", "r");

    //If open errors
    if(f == NULL){
        printf("Whoops\n");
        return;
    }

    //Print out characters as they are read
    char c = fgetc(f);
    while (c != EOF){
        printf("%c", c);
        c = fgetc(f);
    }

    //Close instruction manual
    fclose(f);
}

void main()
{
    char send_msg[256];

    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket error\n");
        return;
    }

    struct sockaddr_in dest;
    int sockaddr_size = sizeof(struct sockaddr_in);
    memset(&dest, 0x00, sizeof(dest));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr *)&dest, sockaddr_size) < 0)
    {
        perror("Connection failure\n");
        return;
    }

    //Send inital message on connection
    send(sock, "Hello", 6, 0);

    //9KB
    //Shouldn't be larger than than 8KB but extra KB just in case
    char recv_msg[9216];
    memset(recv_msg, 0x00, sizeof(recv_msg));

    while (recv(sock, recv_msg, sizeof(recv_msg), 0) > 0)
    {
        //Print out the received message
        printf("%s\n", recv_msg);

        //If manual signal received, display instructions to player
        if(strncmp(recv_msg, "manual", 6) == 0){
            print_instructions();
        }

        memset(send_msg, 0x00, sizeof(send_msg));
        printf("Enter a value: ");
        fgets(send_msg, sizeof(send_msg), stdin);


        send(sock, send_msg, strlen(send_msg), 0);
        printf("\n");

        memset(recv_msg, 0x00, sizeof(recv_msg));
    }
    close(sock);
}
