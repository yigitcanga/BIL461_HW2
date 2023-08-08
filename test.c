#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SIZE 512
#define MSG_SIZE 256

/*
action_type
0-Register
1-Read
2-Delete
3-End
*/
struct msg_buffer {
    long msg_type;
    char process_name[25];
    char file_content[PAGE_SIZE];
    int action_type;
    int frame_index;
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <processName>\n", argv[0]);
        return 1;
    }

    char *processName = argv[1];
    
    key_t server_key = ftok("/tmp", 0);
    int server_mq_id = msgget(server_key, 0);
    if (server_mq_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    struct msg_buffer message;
    strcpy(message.process_name, processName);
    message.msg_type = 1;

    // Simulate sending multiple messages to the server
    for (int i = 0; i < 3; i++) {
        snprintf(message.file_content, sizeof(message.file_content), "File content for frame %d", i);
        message.frame_index = i;

        if (msgsnd(server_mq_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("msgsnd failed");
            exit(1);
        }
        printf("Sent message to server.\n");
    }

    // Indicate end of communication
    strcpy(message.process_name, "\0");
    if (msgsnd(server_mq_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("msgsnd failed");
        exit(1);
    }
    printf("Sent end of communication message.\n");

    return 0;
}
