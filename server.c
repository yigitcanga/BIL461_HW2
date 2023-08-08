#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#define PAGE_TABLE_SIZE 128
#define FRAME_SIZE 512
#define TOTAL_FRAMES 1000

/*Continious Allocation*/
struct page_table_entry
{
    int frame_num;
    int start_index;
    char process_name[25];
};

/*
action_type
0-Register
1-Read
2-Delete
3-End
4-terminate
*/
struct msg_buffer
{
    long msg_type;
    char process_name[25];
    char file_content[FRAME_SIZE];
    int action_type;
    int frame_index;
    int frame_length;
};

int main()
{

    printf("Server Started..\n");
    
    // remove message queue
    
    key_t server_key = ftok("/tmp", 0);
    int msgid = msgget(server_key, 0);
    msgctl(msgid, IPC_RMID, NULL);
    
    char *main_memory = (char *)malloc(TOTAL_FRAMES * FRAME_SIZE);
    struct page_table_entry page_table[PAGE_TABLE_SIZE] ={{0}};
    
    int entry_count = 0;
    
    if (main_memory == NULL)
    {
        perror("main_memory allocation failed");
        exit(1);
    }
    
    // initilize server

    msgid = msgget(server_key, IPC_CREAT | 0666);

    if (msgid == -1)
    {
        perror("msgget failed");
        exit(1);
    }

    char process_name[25];
    char *file_content;
    int frame_length = 0;
    int page_index = 0;
    
    struct msg_buffer message;
    message.action_type = 0;
    while (message.action_type != 4)
    {

        while (message.action_type != 4)
        {
            
            if (msgrcv(msgid, &message, sizeof(message), 0, 0) == -1)
            {
                perror("msgrcv failed");
                exit(1);
            }
            

            if (message.action_type == 0)
            {
                strcpy(process_name, message.process_name);
                if (entry_count >= PAGE_TABLE_SIZE)
                {
                    printf("Page table is full\n");
                    continue;
                }

                frame_length = message.frame_length;
                
                // try to find empty frame
                // Continious allocation
                int frame_start_index = -1;
                printf("HERE3 ===>\n");
                for (int i = 0; i <= TOTAL_FRAMES - frame_length; i++)
                {
                    
                    if (page_table[i].start_index == 0)
                    {
                        frame_start_index = i;
                        break;
                    }

                    

                }
                printf("HERE3.5 ===>\n");
                if (frame_start_index == -1)
                {
                    printf("No empty frame\n");
                    continue;
                }
          
                // Register to page table
                for (int i = frame_start_index; i < frame_start_index + frame_length; i++)
                {
                    page_table[i].start_index = frame_start_index;
                    page_table[i].frame_num = i - frame_start_index; // Use a unique identifier for each frame
                    strcpy(page_table[i].process_name, process_name);
                }
                printf("HERE2 ===>");
                entry_count += frame_length;

                int frame_number = page_index;

                if (frame_number >= 0 && frame_number < TOTAL_FRAMES)
                {
                    size_t offset = frame_number * FRAME_SIZE;
                    char *frame_ptr = main_memory + offset;
                    strcpy(frame_ptr, message.file_content);
                }
                else
                {
                    printf("Invalid frame number.\n");
                }

                page_index++;
                printf("\nRegistering %s : %d\n", process_name, page_index);
                continue;
            }
            else if (message.action_type == 1)
            { // read

                printf("\nReading %s : %d\n", process_name, page_index);
                page_index = message.frame_index;
                strcpy(process_name, message.process_name);
                char frame_content[FRAME_SIZE];
                memcpy(frame_content, main_memory + (page_index * FRAME_SIZE), FRAME_SIZE);
                frame_content[FRAME_SIZE - 1] = '\0';

                //check if frame is allocated by the process name and frame index
                int found = 0;
                for (int i = 0; i < PAGE_TABLE_SIZE; i++)
                {
                    if (page_table[i].frame_num == page_index && strcmp(page_table[i].process_name, process_name) == 0)
                    {
                        found = 1;
                        break;
                    }
                }
                
                // send message to client

                message.msg_type = 1;
                message.action_type = 1;

                key_t client_key = ftok(message.process_name, 0);
                int client_mq_id = msgget(client_key, 0);
                if (client_mq_id == -1)
                {
                    perror("msgget failed");
                    exit(1);
                }

                
               
                if (!found)
                {
                    printf("Frame is not allocated by the process\n");
                    strncpy(message.file_content, "Frame is not allocated by the process", sizeof(message.file_content) - 1);
                    message.file_content[sizeof(message.file_content) - 1] = '\0';
                    printf("%s", message.file_content);
                }
                else
                {
                    strncpy(message.file_content, frame_content, sizeof(message.file_content) - 1);
                    message.file_content[sizeof(message.file_content) - 1] = '\0';
                    printf("%s", message.file_content);
                }


                message.msg_type = 1;
                // sleep(0.1);
                if (msgsnd(client_mq_id, &message, sizeof(message), 0) == -1)
                {
                    perror("msgsnd failed");
                    exit(1);
                }
            }
            else if (message.action_type == 2)
            { // delete

                printf("\nDeleting %s : %d\n", process_name, page_index);
                strcpy(process_name, message.process_name);
                // remove the enrty from page table
                for (int i = 0; i < PAGE_TABLE_SIZE; i++)
                {
                    if (strcmp(page_table[i].process_name, process_name) == 0)
                    {
                        // remove the enrty from page table

                        page_table[i].frame_num = -1;
                        page_table[i].start_index = -1;
                        strcpy(page_table[i].process_name, "");
                        entry_count--;
                        break;
                    }
                }
            }
            else
            { // end
                strcpy(process_name, message.process_name);

                printf("Registed %s with page count of %d\n", process_name, page_index);

                break;
            }
        }

        printf("Page Index: %d\n", page_index);
    }

    printf("Server Terminated..\n");
    msgctl(msgid, IPC_RMID, NULL);
    free(main_memory);
    return 0;
}
