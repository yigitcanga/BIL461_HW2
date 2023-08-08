#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SIZE 512
#define MSG_SIZE 256

struct msg_buffer {
    long msg_type;
    char process_name[25];
    char file_content[PAGE_SIZE];
    int frame_index;
};

int main() {
    key_t server_key = ftok("/tmp", 0);
    int server_mq_id = msgget(server_key, IPC_CREAT | 0666);
    if (server_mq_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    struct msg_buffer message;

    while (1) {
        if (msgrcv(server_mq_id, &message, sizeof(message) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv failed");
            exit(1);
        }

        if (strcmp(message.process_name, "\0") == 0) {
            printf("End of communication.\n");
            break;
        }

        printf("Received message from client:\n");
        printf("Process Name: %s\n", message.process_name);
        printf("File Content: %s\n", message.file_content);
        printf("Frame Index: %d\n", message.frame_index);
    }

    // Clean up
    msgctl(server_mq_id, IPC_RMID, NULL);

    return 0;
}
