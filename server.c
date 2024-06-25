#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define LOCKOUT_TIME 30
#define MAX_ATTEMPTS 3

typedef struct {
    int id;
    int occupied;
    char password[BUFFER_SIZE];
    char content[BUFFER_SIZE];
    int failed_attempts;
    time_t lockout_end_time;
    int in_use;
} Locker;

Locker *lockers;
int num_lockers;
int password_length;
pthread_mutex_t lockers_mutex = PTHREAD_MUTEX_INITIALIZER;

void save_lockers_to_file() {
    FILE *file = fopen("lockers.dat", "wb");
    if (file != NULL) {
        fwrite(lockers, sizeof(Locker), num_lockers, file);
        fclose(file);
    }
}

void load_lockers_from_file() {
    FILE *file = fopen("lockers.dat", "rb");
    if (file != NULL) {
        fread(lockers, sizeof(Locker), num_lockers, file);
        fclose(file);
    }
}

void* client_handler(void* socket_desc) {
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    while ((read_size = recv(sock, client_message, BUFFER_SIZE, 0)) > 0) {
        client_message[read_size] = '\0';

        if (strncmp(client_message, "LIST", 4) == 0) {
            pthread_mutex_lock(&lockers_mutex);
            strcpy(response, "***** 사물함 관리 시스템입니다. *****\n");
            strcat(response, "--------------------------------------\n사물함 내역\n--------------------------------------\n");
            for (int i = 0; i < num_lockers; i++) {
                char status[BUFFER_SIZE];
                if (lockers[i].occupied) {
                    snprintf(status, BUFFER_SIZE, "사물함 %d : 사용 중\n", i + 1);
                } else {
                    snprintf(status, BUFFER_SIZE, "사물함 %d : 비어 있음\n", i + 1);
                }
                strcat(response, status);
            }
            strcat(response, "------------------------------------\n");
            pthread_mutex_unlock(&lockers_mutex);
            send(sock, response, strlen(response), 0);
        } else if (strncmp(client_message, "CHECK", 5) == 0) {
            int locker_id;
            sscanf(client_message, "CHECK %d", &locker_id);
            locker_id--;

            pthread_mutex_lock(&lockers_mutex);
            if (locker_id < 0 || locker_id >= num_lockers) {
                strcpy(response, "사물함 번호가 잘못되었습니다.\n");
            } else if (lockers[locker_id].occupied) {
                strcpy(response, "이미 사용 중인 사물함입니다.\n");
            } else {
                strcpy(response, "비어 있는 사물함입니다.\n");
            }
            pthread_mutex_unlock(&lockers_mutex);

            send(sock, response, strlen(response), 0);
        } else if (strncmp(client_message, "ACCESS", 6) == 0) {
            int locker_id;
            char password[BUFFER_SIZE];
            char content[BUFFER_SIZE];

            sscanf(client_message, "ACCESS %d %s %s", &locker_id, content, password);
            locker_id--;

            pthread_mutex_lock(&lockers_mutex);
            if (locker_id < 0 || locker_id >= num_lockers) {
                strcpy(response, "사물함 번호가 잘못되었습니다.\n");
            } else if (lockers[locker_id].occupied) {
                strcpy(response, "이미 사용 중인 사물함입니다.\n");
            } else if (time(NULL) < lockers[locker_id].lockout_end_time) {
                int lockout_remaining = (int)(lockers[locker_id].lockout_end_time - time(NULL));
                snprintf(response, BUFFER_SIZE, "사물함이 잠겼습니다. %d초 후에 다시 시도하세요.\n", lockout_remaining);
            } else {
                if (lockers[locker_id].failed_attempts >= MAX_ATTEMPTS) {
                    lockers[locker_id].failed_attempts = 0; // Reset failed attempts after lockout
                }
                if (lockers[locker_id].occupied && strcmp(lockers[locker_id].password, password) != 0) {
                    lockers[locker_id].failed_attempts++;
                    if (lockers[locker_id].failed_attempts >= MAX_ATTEMPTS) {
                        lockers[locker_id].lockout_end_time = time(NULL) + LOCKOUT_TIME;
                        snprintf(response, BUFFER_SIZE, "시도가 너무 많습니다. 사물함이 %d초 동안 잠깁니다.\n", LOCKOUT_TIME);
                    } else {
                        strcpy(response, "접근을 거부합니다. 비밀번호가 틀렸습니다.\n");
                    }
                } else {
                    lockers[locker_id].occupied = 1;
                    strcpy(lockers[locker_id].password, password);
                    strcpy(lockers[locker_id].content, content);
                    lockers[locker_id].failed_attempts = 0;
                    save_lockers_to_file();
                    strcpy(response, "사물함에 정상적으로 보관되었습니다.\n");
                }
            }
            pthread_mutex_unlock(&lockers_mutex);

            send(sock, response, strlen(response), 0);
        } else if (strncmp(client_message, "RELEASE", 7) == 0) {
            int locker_id;
            char password[BUFFER_SIZE];

            sscanf(client_message, "RELEASE %d %s", &locker_id, password);
            locker_id--;

            pthread_mutex_lock(&lockers_mutex);
            if (locker_id < 0 || locker_id >= num_lockers) {
                strcpy(response, "사물함 번호가 잘못되었습니다.\n");
            } else if (time(NULL) < lockers[locker_id].lockout_end_time) {
                int lockout_remaining = (int)(lockers[locker_id].lockout_end_time - time(NULL));
                snprintf(response, BUFFER_SIZE, "사물함이 잠겨 있습니다. %d초 후에 다시 시도하세요.\n", lockout_remaining);
            } else {
                if (lockers[locker_id].failed_attempts >= MAX_ATTEMPTS) {
                    lockers[locker_id].failed_attempts = 0; // Reset failed attempts after lockout
                }
                if (!lockers[locker_id].occupied) {
                    strcpy(response, "사물함이 이미 비어 있습니다.\n");
                } else if (strcmp(lockers[locker_id].password, password) != 0) {
                    lockers[locker_id].failed_attempts++;
                    if (lockers[locker_id].failed_attempts >= MAX_ATTEMPTS) {
                        lockers[locker_id].lockout_end_time = time(NULL) + LOCKOUT_TIME;
                        snprintf(response, BUFFER_SIZE, "시도가 너무 많습니다. 사물함이 %d초 동안 잠깁니다.\n", LOCKOUT_TIME);
                    } else {
                        strcpy(response, "비밀번호가 틀렸습니다.\n");
                    }
                } else {
                    lockers[locker_id].occupied = 0;
                    memset(lockers[locker_id].password, 0, sizeof(lockers[locker_id].password));
                    memset(lockers[locker_id].content, 0, sizeof(lockers[locker_id].content));
                    lockers[locker_id].failed_attempts = 0;
                    save_lockers_to_file();
                    strcpy(response, "사물함이 성공적으로 비워졌습니다.\n");
                }
            }
            pthread_mutex_unlock(&lockers_mutex);

            send(sock, response, strlen(response), 0);
        } else if (strncmp(client_message, "CHANGE", 6) == 0) {
            int locker_id;
            char old_password[BUFFER_SIZE];
            char new_password[BUFFER_SIZE];

            sscanf(client_message, "CHANGE %d %s %s", &locker_id, old_password, new_password);
            locker_id--;

            pthread_mutex_lock(&lockers_mutex);
            if (locker_id < 0 || locker_id >= num_lockers) {
                strcpy(response, "사물함 번호가 잘못되었습니다.\n");
            } else if (!lockers[locker_id].occupied) {
                strcpy(response, "사물함이 비어 있습니다. 비밀번호를 변경할 수 없습니다.\n");
            } else if (time(NULL) < lockers[locker_id].lockout_end_time) {
                int lockout_remaining =

                (int)(lockers[locker_id].lockout_end_time - time(NULL));
                snprintf(response, BUFFER_SIZE, "사물함이 잠겨 있습니다. %d초 후에 다시 시도하세요.\n", lockout_remaining);
            } else {
                if (lockers[locker_id].failed_attempts >= MAX_ATTEMPTS) {
                    lockers[locker_id].failed_attempts = 0; // Reset failed attempts after lockout
                }
                if (strcmp(lockers[locker_id].password, old_password) != 0) {
                    lockers[locker_id].failed_attempts++;
                    if (lockers[locker_id].failed_attempts >= MAX_ATTEMPTS) {
                        lockers[locker_id].lockout_end_time = time(NULL) + LOCKOUT_TIME;
                        snprintf(response, BUFFER_SIZE, "시도가 너무 많습니다. 사물함이 %d초 동안 잠깁니다.\n", LOCKOUT_TIME);
                    } else {
                        strcpy(response, "비밀번호가 틀렸습니다.\n");
                    }
                } else {
                    strcpy(lockers[locker_id].password, new_password);
                    lockers[locker_id].failed_attempts = 0;
                    save_lockers_to_file();
                    strcpy(response, "비밀번호가 성공적으로 변경되었습니다.\n");
                }
            }
            pthread_mutex_unlock(&lockers_mutex);

            send(sock, response, strlen(response), 0);
        }
    }

    if (read_size == 0) {
        puts("클라이언트 연결 종료");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv 실패");
    }

    free(socket_desc);
    return 0;
}

int main(int argc, char* argv[]) {
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    printf("사물함 개수를 입력하세요: ");
    scanf("%d", &num_lockers);
    printf("비밀번호 길이를 입력하세요: ");
    scanf("%d", &password_length);

    lockers = (Locker*)malloc(sizeof(Locker) * num_lockers);
    for (int i = 0; i < num_lockers; i++) {
        lockers[i].id = i;
        lockers[i].occupied = 0;
        lockers[i].failed_attempts = 0;
        lockers[i].lockout_end_time = 0;
        lockers[i].in_use = 0;
        memset(lockers[i].password, 0, sizeof(lockers[i].password));
        memset(lockers[i].content, 0, sizeof(lockers[i].content));
    }

    load_lockers_from_file();

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("소켓을 생성할 수 없습니다.");
    }
    puts("소켓 생성됨");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind 실패입니다. 오류가 발생했습니다.");
        return 1;
    }
    puts("bind 완료");

    listen(socket_desc, 3);

    puts("연결 대기 중...");
    c = sizeof(struct sockaddr_in);

    while ((client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c))) {
        puts("연결이 수락되었습니다.");

        pthread_t client_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&client_thread, NULL, client_handler, (void*)new_sock) < 0) {
            perror("스레드를 생성할 수 없습니다.");
            return 1;
        }

        puts("클라이언트 핸들러가 할당되었습니다.");
    }

    if (client_sock < 0) {
        perror("accept 실패");
        return 1;
    }

    return 0;
}
