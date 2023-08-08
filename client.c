#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

void sendToScheduler(char* processName, char* page_table[], int page_index);

#define PAGE_SIZE 512 
#define MSG_SIZE 512


/*
action_type
0-Register
1-Read
2-Delete
3-End
4-terminate
*/
struct msg_buffer {
    long msg_type;
    char process_name[25];
    char file_content[PAGE_SIZE];
    int action_type;
    int frame_index;
    int frame_length;
};

void sendToScheduler(char* processName, char* page_table[], int page_index) {
   
    key_t server_key = ftok("/tmp", 0);

    int server_mq_id = msgget(server_key, 0);

    if (server_mq_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    struct msg_buffer message;
    message.msg_type = 1;
    message.action_type = 0;//register
    message.frame_length = page_index;

    strcpy(message.process_name, processName);
    for (int i = 0; i < page_index; i++) {        
        
        memcpy(message.file_content, page_table[i], sizeof(message.file_content)-1);

        message.file_content[PAGE_SIZE-1] = '\0';

        message.frame_index = i;

        if (msgsnd(server_mq_id, &message, sizeof(message),0) == -1) {
        
            perror("msgsnd failed");
            exit(1);
        }
        printf("one time done\n");
    }

    message.action_type = 3;
    // page table'ın bittiğini belirten işaret gönderme
    if (msgsnd(server_mq_id, &message, sizeof(message) , 0) == -1) {
        perror("msgsnd failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <processName> <fileName>\n", argv[0]);
        return 1;
    }

    char *processName = argv[1];
    char *filename = argv[2];

    //remove message queue
    key_t client_key = ftok(processName, 0);
    int client_mq_id = msgget(client_key, 0);
    msgctl(client_mq_id, IPC_RMID, NULL);

    FILE *file;
    char line[PAGE_SIZE];
    char *page_table[PAGE_SIZE] = {""};
    int page_index = 0;

    file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error opening the file.\n");
        return 1;
    }

    size_t bytes_read = fread(line, sizeof(char), sizeof(line), file);
    if (bytes_read == 0) {
        printf("Error reading the file.\n");
        return 1;
    }

    while (bytes_read > 0) {
        page_table[page_index] = strdup(line);
        page_index++;
        bytes_read = fread(line, sizeof(char), sizeof(line), file);
    }
    

    fclose(file);

    printf("Page table of %s:\n", processName);
    printf("%d\n",page_index);
    sendToScheduler(argv[1], page_table, page_index);
    printf("Page table sent to scheduler.\n");
    // page table unutma
    for (int i = 0; i < page_index; i++) {
        free(page_table[i]);
    }

    int page_number;
    client_mq_id = msgget(client_key, 0666 | IPC_CREAT);
    if (client_mq_id == -1) {
        perror("msgget failed");
        exit(1);
    }


             
    key_t server_key = ftok("/tmp", 0);
    int server_mq_id = msgget(server_key, 0);
    if (server_mq_id == -1) {
        perror("msgget failed");
        exit(1);
    }
        
    struct msg_buffer message;
    message.msg_type = 1;
    message.action_type = 1;//read
    message.frame_index = page_number;
    strcpy(message.process_name, processName);
    while (1) {
        printf("Enter the page number (-1 to exit): ");
        scanf("%d", &page_number);
        if (page_number == -1) {

            message.action_type = 2;//delete
            message.frame_index = page_number;
            
            // page no yollama
            if (msgsnd(server_mq_id, &message, sizeof(message), 0) == -1) {
                perror("msgsnd failed");
                exit(1);
            }



            break;
        }


        

  


 


        
        message.action_type = 1;
        message.frame_index = page_number;
        // page no yollama
        if (msgsnd(server_mq_id, &message, sizeof(message), 0) == -1) {
            perror("msgsnd failed");
            exit(1);
        }

  
        // scheduler dan gelen yanıt
        if (msgrcv(client_mq_id, &message, sizeof(message), 0, 0) == -1) {
            perror("msgrcv failed");
            exit(1);
        }
     
        // gelen yanıtı basma
        printf("Response from scheduler: %s\n", message.file_content);
    }

    msgctl(client_mq_id, IPC_RMID, NULL);
    return 0;

}