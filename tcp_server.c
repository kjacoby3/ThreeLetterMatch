#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define WELCOME_MSG "Welcome to the Three Letter Match Game. Use the \"manual\" command to learn how to play!"
#define INVALID_SUB "ERROR: The substring must be 3 letters long. Please try again."
#define VALID_SUB "VALID SUBSTRING: "
#define START_GAME "\nEnter a word containing the substring to score points. Enter \"list\" to end the game."
#define GAME_OVER "GAME OVER\n Final Score: "
#define SET "The \"set\" command must be followed by a 3-letter substring"
#define INVALID_COMMAND "Sorry, that is not a valid command"
#define ALREADY_SET "Sorry, the substring is already set. Quit the game to reset the substring"
#define NOT_SET "There is no substring set. \nPlease use the \"set\" command to start playing"
#define SUBMIT "The \"submit\" command must be followed by word"
#define TOO_LONG "The word must be no larger than 32 characters long"
#define INCORRECT "This word didn't match\n"
#define CORRECT "Correct!\n"
#define SCORE "Current Score: "
#define ALREADY_GUESSED "Sorry, you already guessed this word. Please try again.\n"
#define FINAL_SCORE "\nYour final score was "
#define QUIT "You exited the game"
#define SUB "Your substring is: "
#define GUESSED "Correctly guessed words: \n"

//Splits a string (s2) at the index (n) and copies the remaining string (s1)
void strsplit(char s1[], char s2[], int n)
{
    int x;
    int len=(strlen(s2) - n);
    for (x=0; x<len; x++){
        s1[x]=s2[x+n];
    }
}

//Checks if given word exists in selected list
// Returns integers -1, 0, or 1
// Returns 1 if word found and guessed for the first time
// Returns 0 if word found but already guessed
// Returns -1 if word not found
int check_list(char list[][32], char word[], int length)
{
    for(int i=0; i<length; i++){
        //Used to print string comparisons
        // printf("%d, %ld, %s", strcmp(list[i] + 1, word), strlen(list[i]), list[i]);

        if(strncmp(list[i], word, strlen(list[i])) == 0){
            //Set first character in word to 1 to mark that word was used.
            char marked_word[sizeof(list[i])];
            marked_word[0] = '1';
            strncpy(marked_word + 1, list[i], strlen(list[i]));
            
            //Place marked word back into list
            strncpy(list[i], marked_word, strlen(list[i]) + 1);
            return 1;
        //Check if word is marked
        } else if (strcmp(list[i] + 1, word) == 0 && list[i][0] == '1') {
            return 0;
        }
    }
    //Return -1 if word wasn't found in list.
    return -1;
}

// Generates the list of words containing substring to avoid traversing file multiple times
// Returns length of list as int
int set_list(char list[][32], char match[])
{
    // Open the dictionary file
    FILE *f;
    f = fopen("words.txt", "r");
    char word[32];
    
    memset(word, 0x00, sizeof(word));

    // Initialize index
    // This value will be used to determine the length of the list
    int index = 0;

    // Retrieve each line in text file until end of file found
    while (fgets(word, sizeof(word), f) != NULL) {

        //Convert the current word to lowercase so that the substring is not case sensitive
        char lwr[strlen(word)];
        for(int i = 0; word[i]; i++){
            lwr[i] = tolower(word[i]);
        }

        //If the substring exists in the current line, copy the line into the list
        if(strstr(lwr, match) != NULL){
            strcpy(list[index++], word);
        }
        memset(word, 0x00, sizeof(word));
    }

    //Used to print out all solutions to substring (Cheat sheet)
    // for(int j=0; j<index;j++){
    //     printf("list[%d], %s", j, list[j]);
    // }

    //Close the dictionary file
    fclose(f);

    //Return the index as the length of the list
    return index;

}

void main()
{
    struct sockaddr_in server;
    struct sockaddr_in client;
    int sockaddr_size = sizeof(struct sockaddr_in);

    int sock;
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket error\n");
        return;
    }

    memset(&server, 0x00, sizeof(server));

    server.sin_family = AF_INET; // IPv4
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(8080);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Bind error");
        return;
    }

    listen(sock, 5);
    printf("listening....\n");

    char msg[256];
    memset(msg, 0x00, sizeof(msg));

    while (1)
    {
        int client_sock = accept(sock, (struct sockaddr *)&client, &sockaddr_size);
        printf("Connected to %s:%i\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        //New subprocess for each client that connects
        if(fork() == 0){
            close(sock);
            
            //Score is initialized for each player so they play separately
            char match[4];
            int score = 0;

            //List of the valid words that match the player's substring
            //Maximum length of 256
            char list[256][32];

            //Length of the list
            //Initialized to 0 while substring is not set
            int length = 0;

            memset(match, 0x00, sizeof(match));

            memset(msg, 0x00, sizeof(msg));
            while(recv(client_sock, msg, sizeof(msg) - 1, 0) > 0)
            {
                //Print out message received from client
                printf("%s\n", msg);
                printf("%s\n", match);

                //Send welcome message if default message is received
                if(strcmp("Hello", msg) == 0) {
                    sendto(client_sock,
                        WELCOME_MSG,
                        strlen(WELCOME_MSG),
                        0,
                        (struct sockaddr *)&client,
                        sockaddr_size);

                    memset(msg, 0x00, sizeof(msg));
                
                //Conditions for the manual command
                //Instructions for how to play
                } else if (strncasecmp("manual", msg, 6) == 0){
                    sendto(client_sock,
                        msg,
                        strlen(msg),
                        0,
                        (struct sockaddr *)&client,
                        sockaddr_size);

                    memset(msg, 0x00, sizeof(msg));

                //Conditions for the set command
                } else if(strncasecmp("set", msg, 3) == 0){

                    //Message if substring is already set
                    if(strlen(match) != 0){
                        sendto(client_sock,
                            ALREADY_SET,
                            strlen(ALREADY_SET),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);
                        
                        memset(msg, 0x00, sizeof(msg));
                    
                    //Message if no args provided with command
                    } else if(strlen(msg) == 4){
                        sendto(client_sock,
                            SET,
                            strlen(SET),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);
                        
                        memset(msg, 0x00, sizeof(msg));
                    
                    //Message if incorrect arg length
                    } else if(strlen(msg) != 8){
                        sendto(client_sock,
                            INVALID_SUB,
                            strlen(INVALID_SUB),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);
                        
                        memset(msg, 0x00, sizeof(msg));
                    } else {
                        //Message if substring doesn't contain only letters
                        for(int i = 4; i < 7; i++){
                            if(isalpha(msg[i]) == 0) {
                                sendto(client_sock,
                                    INVALID_SUB,
                                    strlen(INVALID_SUB),
                                    0,
                                    (struct sockaddr *)&client,
                                    sockaddr_size);

                                memset(msg, 0x00, sizeof(msg));
                                continue;
                            }
                        }

                        //Set matching substring
                        strsplit(match, msg, 4);

                        // Terminating bit is supposed to be ignored in the
                        // strstr() method but my function won't work without
                        // manually setting the last bit :(
                        match[3] = '\0';

                        //Set the list of matching words
                        //Set list length from value return by set_list()
                        length = set_list(list, match);

                        char start[256] = VALID_SUB;
                        strcat(start, match);
                        strcat(start, START_GAME);

                        //Message for successfully set substring
                        sendto(client_sock,
                            start,
                            strlen(start),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                        memset(start, 0x00, sizeof(start));
                    }
                //Conditions for the submit command
                } else if (strncasecmp("submit", msg, 6) == 0){
                    //Message if substring was not set yet
                    if(strlen(match) == 0){
                        sendto(client_sock,
                            NOT_SET,
                            strlen(NOT_SET),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                    //Message if no args provided with command
                    } else if (strlen(msg) == 8) {
                        sendto(client_sock,
                            SUBMIT,
                            strlen(SUBMIT),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                    //Message if arg too long
                    } else if (strlen(msg) > 40) {
                        sendto(client_sock,
                            TOO_LONG,
                            strlen(TOO_LONG),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                    //Correct use of submit command
                    } else {
                        //String for submitted word
                        char submit[32];

                        //String for player's score
                        char s[4];

                        //String for message sent to player
                        char update[128];

                        //Split the "submit" off of the received message
                        //Set remaining string to the submit string
                        strsplit(submit, msg, 7);

                        //Check list for the submitted word
                        int check = check_list(list, submit, length);

                        //If word found and unused
                        if(check == 1){
                            //Increase score
                            score++;
                            sprintf(s, "%d", score);

                            //Build message string
                            strcat(update, CORRECT);
                            strcat(update, SCORE);
                            strcat(update, s);

                            //Send message
                            sendto(client_sock,
                                update,
                                strlen(update),
                                0,
                                (struct sockaddr *)&client,
                                sockaddr_size);

                            memset(msg, 0x00, sizeof(msg));
                            memset(update, 0x00, sizeof(update));
                            memset(s, 0x00, sizeof(s));
                            memset(submit, 0x00, sizeof(submit));
                        
                        //If word not found
                        } else if(check == -1){
                            //Decrease score
                            score--;
                            sprintf(s, "%d", score);

                            //Build message string
                            strcat(update, INCORRECT);
                            strcat(update, SCORE);
                            strcat(update, s);

                            //Send message
                            sendto(client_sock,
                                update,
                                strlen(update),
                                0,
                                (struct sockaddr *)&client,
                                sockaddr_size);

                            memset(msg, 0x00, sizeof(msg));
                            memset(update, 0x00, sizeof(update));
                            memset(s, 0x00, sizeof(s));
                            memset(submit, 0x00, sizeof(submit));

                        //If 0 is returned, word was already guessed.
                        } else {
                            sprintf(s, "%d", score);

                            //Build message string
                            strcat(update, ALREADY_GUESSED);
                            strcat(update, SCORE);
                            strcat(update, s);

                            //Send message
                            sendto(client_sock,
                                update,
                                strlen(update),
                                0,
                                (struct sockaddr *)&client,
                                sockaddr_size);

                            memset(msg, 0x00, sizeof(msg));
                            memset(update, 0x00, sizeof(update));
                            memset(s, 0x00, sizeof(s));
                            memset(submit, 0x00, sizeof(submit));
                        }
                    }
                //Conditions for list command
                } else if (strncasecmp("list", msg, 4) == 0 && strlen(msg) == 5) {
                    //If substring not set yet
                    if(sizeof(match) == 0){
                        sendto(client_sock,
                            NOT_SET,
                            strlen(NOT_SET),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);
                        memset(msg, 0x00, sizeof(msg));
                    } else {

                        char s[4];
                        sprintf(s, "%d", score);

                        //Max size 8192 (256 * 32) + 4 + strlen(FINAL_SCORE)
                        char list_msg[8192 + strlen(s) + strlen(FINAL_SCORE)];

                        //Build list message
                        for(int i=0; i<=length; i++){
                            int len = strlen(list_msg);
                            snprintf(list_msg + len, sizeof(list_msg) - len, "%s", list[i]);
                        }

                        snprintf(list_msg + strlen(list_msg), sizeof(list_msg) - strlen(list_msg), "%s", FINAL_SCORE);
                        snprintf(list_msg + strlen(list_msg), sizeof(list_msg) - strlen(list_msg), "%s", s);

                        sendto(client_sock,
                            list_msg,
                            strlen(list_msg),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                        memset(list_msg, 0x00, sizeof(list_msg));
                        memset(match, 0x00, sizeof(match));
                        memset(list, 0x00, sizeof(list));
                        score = 0;
                    }
                //Conditions for quit command
                } else if (strncasecmp("quit", msg, 4) == 0 && strlen(msg) == 5) {
                    char quit_msg[256];
                    char s[4];
                    sprintf(s, "%d", score);

                    //Build quit message
                    strcat(quit_msg, QUIT);
                    strcat(quit_msg, FINAL_SCORE);
                    strcat(quit_msg, s);
                    strcat(quit_msg, "\n\n");
                    strcat(quit_msg, WELCOME_MSG);

                    sendto(client_sock,
                        quit_msg,
                        strlen(quit_msg),
                        0,
                        (struct sockaddr *)&client,
                        sockaddr_size);

                    memset(msg, 0x00, sizeof(msg));
                    memset(quit_msg, 0x00, sizeof(quit_msg));
                    memset(match, 0x00, sizeof(match));
                    memset(list, 0x00, sizeof(list));
                    score = 0;

                //Conditions for scoreboard command
                } else if (strncasecmp("scoreboard", msg, 10) == 0 && strlen(msg) == 11){
                    char s[4];
                    sprintf(s, "%d", score);

                    //Message if no substring set
                    if(strlen(match) == 0){
                        sendto(client_sock,
                            NOT_SET,
                            strlen(NOT_SET),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                    } else {
                        char score_msg[8192 + strlen(s) + strlen(SUB) + strlen(match) + strlen(GUESSED) + strlen(SCORE)];

                        //Build scoreboard message
                        snprintf(score_msg, sizeof(score_msg), "%s", GUESSED);
                        for(int i=0; i<=length; i++){
                            int len = strlen(score_msg);
                            if(list[i][0] == '1'){
                                snprintf(score_msg + len, sizeof(score_msg) - len, "\n%s", list[i] + 1);
                            }
                        }
                        snprintf(score_msg + strlen(score_msg), sizeof(score_msg) - strlen(score_msg), "%s %s\n%s %s\n", SUB, match, SCORE, s);

                        sendto(client_sock,
                            score_msg,
                            strlen(score_msg),
                            0,
                            (struct sockaddr *)&client,
                            sockaddr_size);

                        memset(msg, 0x00, sizeof(msg));
                        memset(score_msg, 0x00, sizeof(score_msg));
                        memset(s, 0x00, sizeof(s));
                    }
                
                //Condition for else (incorrect command)
                } else {
                    sendto(client_sock,
                        INVALID_COMMAND,
                        strlen(INVALID_COMMAND),
                        0,
                        (struct sockaddr *)&client,
                        sockaddr_size);

                    memset(msg, 0x00, sizeof(msg));
                }
            }
            
            close(client_sock);
            return;
        }

        close(client_sock);
    }
    close(sock);
}
