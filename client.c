#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define PAGE_TABLE_SIZE 128 // 512 bayt / 4 (4-byte giriş)

void sendToScheduler(const char* processName, char* page_table[], int page_index) {
    key_t server_key = ftok("/tmp", 0);
    int server_mq_id = msgget(server_key, 0);
    if (server_mq_id == -1) {
        perror("msgget failed");
        exit(1);
    }

    // process name gönderme
    if (msgsnd(server_mq_id, processName, strlen(processName) + 1, 0) == -1) {
        perror("msgsnd failed");
        exit(1);
    }

    // page table gönderme
    for (int i = 0; i < page_index; i++) {
        if (msgsnd(server_mq_id, page_table[i], strlen(page_table[i]) + 1, 0) == -1) {
            perror("msgsnd failed");
            exit(1);
        }
    }

    // page tableın bittiğini belirten işaret gönderme
    if (msgsnd(server_mq_id, "END_MARKER", strlen("END_MARKER") + 1, 0) == -1) {
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

    FILE *file;
    char line[256];
    char *page_table[PAGE_TABLE_SIZE] = {0};
    int page_index = 0;

    file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error opening the file.\n");
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {      // dosyadaki verileri satır satır okuma
        int line_length = strlen(line);

        if (line_length > 0 && line[line_length - 1] == '\n') {
            line[line_length - 1] = '\0';
        }
        page_table[page_index] = strdup(
                line);                      // her satırı page table'a kopyalama#include <stdio.h>

    }

    fclose(file);

    sendToScheduler(argv[1], page_table, page_index);

    // page table unutma
    for (int i = 0; i < page_index; i++) {
        free(page_table[i]);
    }

    int page_number;

    while (1) {
        printf("Enter the page number (-1 to exit): ");
        scanf("%d", &page_number);
        if (page_number == -1) {
            break;
        }

        key_t server_key = ftok("/tmp", 0);
        int server_mq_id = msgget(server_key, 0);
        if (server_mq_id == -1) {
            perror("msgget failed");
            exit(1);
        }

        long mtype = getpid();
        char mtext[MSG_SIZE];
        snprintf(mtext, MSG_SIZE, "%d", page_number);

        // page no yollama
        if (msgsnd(server_mq_id, &mtext, sizeof(mtext), 0) == -1) {
            perror("msgsnd failed");
            exit(1);
        }

        // scheduler dan gelen yanıt
        if (msgrcv(server_mq_id, &mtext, sizeof(mtext), mtype, 0) == -1) {
            perror("msgrcv failed");
            exit(1);
        }

        // gelen yanıtı basma
        printf("Response from scheduler: %s\n", mtext);
    }

        return 0;

}