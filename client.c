#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void list_lockers(int sock) {
    char message[BUFFER_SIZE] = "LIST";
    char server_reply[BUFFER_SIZE];

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("전송 실패");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        perror("수신 실패");
        return;
    }

    printf("사물함 목록:\n%s\n", server_reply);
}

int check_locker_status(int sock, int locker_id) {
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];

    sprintf(message, "CHECK %d", locker_id);

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("전송 실패");
        return 1;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        perror("수신 실패");
        return 1;
    }

    if (strcmp(server_reply, "이미 사용 중인 사물함입니다.\n") == 0) {
        puts(server_reply);
        return 1;
    }

    return 0;
}

void access_locker(int sock) {
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];
    int locker_id;
    char content[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    char confirm_password[BUFFER_SIZE];

    while (1) {
        printf("사물함 번호를 입력하세요: ");
        if (scanf("%d", &locker_id) != 1) {
            printf("잘못된 입력입니다. 다시 시도하세요.\n");
            while (getchar() != '\n');
            continue;
        }

        if (check_locker_status(sock, locker_id) == 0) {
            break;
        } else {
            printf("다른 사물함을 선택하시겠습니까? (Y/N): ");
            char choice;
            scanf(" %c", &choice);
            if (choice == 'N' || choice == 'n') {
                return;
            }
        }
    }

    printf("내용을 입력하세요: ");
    getchar();
    fgets(content, BUFFER_SIZE, stdin);
    content[strcspn(content, "\n")] = '\0';
   
    printf("새 비밀번호를 설정하세요: ");
    fgets(password, BUFFER_SIZE, stdin);
    password[strcspn(password, "\n")] = '\0';

    printf("비밀번호를 확인하세요: ");
    fgets(confirm_password, BUFFER_SIZE, stdin);
    confirm_password[strcspn(confirm_password, "\n")] = '\0';

    if (strcmp(password, confirm_password) != 0) {
        printf("비밀번호가 일치하지 않습니다. 다시 시도하세요.\n");
        return;
    }

    snprintf(message, BUFFER_SIZE, "ACCESS %d %s %s", locker_id, content, password);

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("전송 실패");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        perror("수신 실패");
        return;
    }

    printf("%s\n", server_reply);
}

void release_locker(int sock) {
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];
    int locker_id;
    char password[BUFFER_SIZE];

    printf("사물함 번호를 입력하세요: ");
    if (scanf("%d", &locker_id) != 1) {
        printf("잘못된 입력입니다. 다시 시도하세요.\n");
        while (getchar() != '\n');
        return;
    }

    printf("비밀번호를 입력하세요: ");
    getchar();
    fgets(password, BUFFER_SIZE, stdin);
    password[strcspn(password, "\n")] = '\0';

    snprintf(message, BUFFER_SIZE, "RELEASE %d %s", locker_id, password);

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("전송 실패");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        perror("수신 실패");
        return;
    }

    printf("%s\n", server_reply);
}

void change_password(int sock) {
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];
    int locker_id;
    char old_password[BUFFER_SIZE];
    char new_password[BUFFER_SIZE];
    char confirm_password[BUFFER_SIZE];

    printf("사물함 번호를 입력하세요: ");
    if (scanf("%d", &locker_id) != 1) {
        printf("잘못된 입력입니다. 다시 시도하세요.\n");
        while (getchar() != '\n');
        return;
    }

    printf("기존 비밀번호를 입력하세요: ");
    getchar();
    fgets(old_password, BUFFER_SIZE, stdin);
    old_password[strcspn(old_password, "\n")] = '\0';

    printf("새 비밀번호를 입력하세요: ");
    fgets(new_password, BUFFER_SIZE, stdin);
    new_password[strcspn(new_password, "\n")] = '\0';

    printf("새 비밀번호를 확인하세요: ");
    fgets(confirm_password, BUFFER_SIZE, stdin);
    confirm_password[strcspn(confirm_password, "\n")] = '\0';

    if (strcmp(new_password, confirm_password) != 0) {
        printf("비밀번호가 일치하지 않습니다. 다시 시도하세요.\n");
        return;
    }

    snprintf(message, BUFFER_SIZE, "CHANGE %d %s %s", locker_id, old_password, new_password);

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("전송 실패");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        perror("수신 실패");
        return;
    }

    printf("%s\n", server_reply);
}

int main() {
    int sock;
    struct sockaddr_in server;
    int choice;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("소켓을 생성할 수 없습니다.");
        return 1;
    }
    puts("소켓 생성이 완료되었습니다.");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("연결 실패입니다. 오류가 발생했습니다.");
        return 1;
    }

    puts("연결됨\n");

    while (1) {
        list_lockers(sock);
        printf("1. 물건 보관하기\n");
        printf("2. 물건 꺼내기\n");
        printf("3. 비밀번호 바꾸기\n");
        printf("4. 나가기\n");
        printf("선택하세요: ");
        if (scanf("%d", &choice) != 1) {
            printf("잘못된 입력입니다. 다시 선택하세요.\n");
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                access_locker(sock);
                break;
            case 2:
                release_locker(sock);
                break;
            case 3:
                change_password(sock);
                break;
            case 4:
                close(sock);
                return 0;
            default:
                printf("잘못된 선택입니다.\n");
                break;
        }
    }

    close(sock);
    return 0;
}

