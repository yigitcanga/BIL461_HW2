#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>


/*

Bu clienti bir görmezden gel..

*/



#define MAX_PAGES 64
#define FRAME_SIZE 512

struct msg_buffer {
    long msg_type;
    int process_id;
    char file_name[100];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Process name> <File name>\n", argv[0]);
        return 1;
    }

    int process_id = getpid();
    struct msg_buffer message;
    message.msg_type = 1;
    message.process_id = process_id;
    strcpy(message.file_name, argv[2]);

    key_t key = ftok("scheduler.c", 'A');
    int msgid = msgget(key, 0666 | IPC_CREAT);

    msgsnd(msgid, &message, sizeof(message), 0);
    msgrcv(msgid, &message, sizeof(message), process_id, 0);

    printf("Process %d: Page table and memory allocated for file %s\n", process_id, message.file_name);

    while (1) {
        int page_num;
        printf("Process %d: Enter page number (-1 to exit): ", process_id);
        scanf("%d", &page_num);

        if (page_num == -1) {
            break;
        }

        // Send request for a specific page
        message.msg_type = 1;
        msgsnd(msgid, &message, sizeof(message), 0);

        // Simulate receiving the page content from the scheduler
        msgrcv(msgid, &message, sizeof(message), process_id, 0);
        if (page_num < MAX_PAGES && message.process_id == process_id) {
            char *page_data = (char *)shmat(shmid, 0, 0);
            printf("Process %d: Page %d content: %.*s\n", process_id, page_num, FRAME_SIZE, page_data + message.page_table[page_num].frame_num * FRAME_SIZE);
            shmdt(page_data);
        } else {
            printf("Process %d: Invalid page number or process ID\n", process_id);
        }
    }

    return 0;
}
