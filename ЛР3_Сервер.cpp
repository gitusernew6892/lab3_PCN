#include <iostream>
#include <fstream>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 54000
#define BUFFER_SIZE 4096

int main() {

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket =
        socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr));

    std::cout << "UDP Server running...\n";

    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);

    int mode = 0;

    recvfrom(serverSocket,
        (char*)&mode,
        sizeof(mode),
        0,
        (sockaddr*)&clientAddr,
        &clientLen);

    char filename[260]{};

    recvfrom(serverSocket,
        filename,
        sizeof(filename),
        0,
        (sockaddr*)&clientAddr,
        &clientLen);

    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        closesocket(serverSocket);
        return 0;
    }

    char buffer[BUFFER_SIZE];

    
    if (mode == 1) {

        while (true) {

            int bytes =
                recvfrom(serverSocket,
                    buffer,
                    BUFFER_SIZE,
                    0,
                    (sockaddr*)&clientAddr,
                    &clientLen);

            if (bytes <= 0)
                break;

            
            if (strcmp(buffer, "END") == 0)
                break;

            file.write(buffer, bytes);
        }
    }

    
    else if (mode == 2) {

        int parts = 0;

        recvfrom(serverSocket,
            (char*)&parts,
            sizeof(parts),
            0,
            (sockaddr*)&clientAddr,
            &clientLen);

        for (int i = 0; i < parts; i++) {

            int chunkSize = 0;

            recvfrom(serverSocket,
                (char*)&chunkSize,
                sizeof(chunkSize),
                0,
                (sockaddr*)&clientAddr,
                &clientLen);

            int received = 0;

            while (received < chunkSize) {

                int bytes =
                    recvfrom(serverSocket,
                        buffer,
                        BUFFER_SIZE,
                        0,
                        (sockaddr*)&clientAddr,
                        &clientLen);

                file.write(buffer, bytes);

                received += bytes;
            }
        }

        
        recvfrom(serverSocket,
            buffer,
            BUFFER_SIZE,
            0,
            (sockaddr*)&clientAddr,
            &clientLen);
    }

    file.close();

    std::cout << "Saved: " << filename << "\n";

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}