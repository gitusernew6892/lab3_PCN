#include <iostream>
#include <fstream>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commdlg.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comdlg32.lib")

#define PORT 54000
#define BUFFER_SIZE 4096

std::string selectFile() {
    char file[MAX_PATH] = {};

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
        return std::string(file);

    return "";
}

std::string getName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    return path.substr(pos + 1);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    std::string path = selectFile();
    if (path.empty()) return 0;

    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;

    int mode;
    std::cout << "Mode (1-stream, 2-parts): ";
    std::cin >> mode;

    sendto(sock,
        (char*)&mode,
        sizeof(mode),
        0,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr));

    std::string name = getName(path);

    sendto(sock,
        name.c_str(),
        name.size() + 1,
        0,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr));

    char buffer[BUFFER_SIZE];

    
    if (mode == 1) {

        while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {

            int bytes = file.gcount();

            sendto(sock,
                buffer,
                bytes,
                0,
                (sockaddr*)&serverAddr,
                sizeof(serverAddr));
        }
    }

    
    else {

        int parts;
        std::cout << "Enter parts count: ";
        std::cin >> parts;

        sendto(sock,
            (char*)&parts,
            sizeof(parts),
            0,
            (sockaddr*)&serverAddr,
            sizeof(serverAddr));

        file.seekg(0, std::ios::end);
        int size = file.tellg();
        file.seekg(0);

        int chunk = size / parts;
        int offset = 0;

        for (int i = 0; i < parts; i++) {

            int curSize =
                (i == parts - 1)
                ? size - offset
                : chunk;

            sendto(sock,
                (char*)&curSize,
                sizeof(curSize),
                0,
                (sockaddr*)&serverAddr,
                sizeof(serverAddr));

            int sent = 0;

            while (sent < curSize) {

                int toRead =
                    (curSize - sent > BUFFER_SIZE)
                    ? BUFFER_SIZE
                    : curSize - sent;

                file.read(buffer, toRead);

                int bytes = file.gcount();

                sendto(sock,
                    buffer,
                    bytes,
                    0,
                    (sockaddr*)&serverAddr,
                    sizeof(serverAddr));

                sent += bytes;
            }

            offset += curSize;
        }
    }

    
    const char endMsg[] = "END";

    sendto(sock,
        endMsg,
        sizeof(endMsg),
        0,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr));

    closesocket(sock);
    WSACleanup();

    return 0;
}