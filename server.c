#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#define MAX_PAGES 64
#define FRAME_SIZE 512
#define TOTAL_FRAMES 1000

struct page_table_entry {
    int frame_num;
    int valid;
};

struct msg_buffer {
    long msg_type;
    int process_id;
    char file_name[100];
};

int main() {
    int shmid = shmget(IPC_PRIVATE, TOTAL_FRAMES * FRAME_SIZE, IPC_CREAT | 0666);
    char *main_memory = (char *)shmat(shmid, 0, 0);

    struct msg_buffer message;
    key_t key = ftok("scheduler.c", 'A');
    int msgid = msgget(key, 0666 | IPC_CREAT);

    while (1) {
        msgrcv(msgid, &message, sizeof(message), 1, 0);

        printf("Scheduler: Received request from Process %d for file %s\n", message.process_id, message.file_name);

        struct page_table_entry page_table[MAX_PAGES];
        FILE *file = fopen(message.file_name, "rb");
        if (!file) {
            perror("Scheduler: File open failed");
            continue;
        }

        int pages_needed = 0;
        while (!feof(file)) {
            if (pages_needed >= MAX_PAGES) {
                printf("Scheduler: Maximum pages reached for Process %d\n", message.process_id);
                break;
            }

            int frame_num = -1;
            if (pages_needed < TOTAL_FRAMES) {
                frame_num = pages_needed;
            } else {
                printf("Scheduler: Not enough frames for Process %d\n", message.process_id);
                break;
            }

            fread(main_memory + frame_num * FRAME_SIZE, FRAME_SIZE, 1, file);

            page_table[pages_needed].frame_num = frame_num;
            page_table[pages_needed].valid = 1;

            printf("Scheduler: Page %d assigned to Frame %d for Process %d\n", pages_needed, frame_num, message.process_id);

            pages_needed++;
        }

        fclose(file);

        // Simulate responding back to the user process
        message.msg_type = message.process_id;
        msgsnd(msgid, &message, sizeof(message), 0);
    }

    shmdt(main_memory);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
