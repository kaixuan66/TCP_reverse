#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <winsock2.h>
#include <random> 
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4996)

int totalBlocks; // �ܿ���
std::string total; // ������������

// ��ʼ��Э�����͵�ö��
enum Initialization {
    IniType = 1,//����
    IniN = 0,//����
};

// ��ת����Э�����͵�ö��
enum reverseRequest {
    RType = 3,
    RLength = 0,
    RData
};

// ���ͳ�ʼ����Ϣ
void sendMessage(SOCKET clientSocket, Initialization type, int blockNum) {
    std::string message = std::to_string(type) + std::to_string(blockNum);
    send(clientSocket, message.c_str(), message.length(), 0);
}

// ���ͷ�ת������Ϣ
void RsendMessage(SOCKET clientSocket, reverseRequest type, int blockNum, const std::string& content) {
    std::string message = std::to_string(type) + std::to_string(blockNum) + content;
    send(clientSocket, message.c_str(), message.length(), 0);
}

// ���շ�������Ϣ
void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;
    int currentBlock = 0; // ��ǰ����

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        std::string receivedMessage(buffer, 0, bytesReceived);
        if (!receivedMessage.empty()) {
            // ������4��ͷ����Ϣ
            if (receivedMessage[0] == '4') {
                receivedMessage = receivedMessage.substr(1);

                size_t pos = receivedMessage.find_first_not_of("0123456789");
                if (pos != std::string::npos) {
                    std::string numberStr = receivedMessage.substr(0, pos);
                    int extractedNumber = std::stoi(numberStr);
                    std::cout << "��ǰ��Ϣ�Ŀ��� " << ++currentBlock << std::endl;
                    std::string remainingMessage = receivedMessage.substr(pos);
                    total.insert(0, remainingMessage);
                    std::cout << "��ת�������: " << remainingMessage << std::endl;
                    if (currentBlock == totalBlocks) {
                        std::cout << "�������� " << total << std::endl;
                    }
                }
            }
            // ������2��ͷ����Ϣ
            if (receivedMessage[0] == '2') {
                std::cout << "������ͬ����գ���Ҫ�����ܿ��� " << totalBlocks << std::endl;
            }
        }
    }

    if (bytesReceived == 0) {
        std::cerr << "�������Ѿ��Ͽ�" << std::endl;
    }
    else {
        std::cerr << "�����ѹرջ�������" << std::endl;
    }
}

// �����ļ����ݸ�������
void sendMessages(SOCKET clientSocket, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "���ļ�ʧ��: " << filename << std::endl;
        return;
    }

    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (fileContent.empty()) {
        std::cerr << "�ļ�Ϊ��" << std::endl;
        return;
    }

    // ������ɿ��С
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(5, 15);
    const int chunkSize = dis(gen);

    totalBlocks = (fileContent.size() + chunkSize - 1) / chunkSize;
    sendMessage(clientSocket, IniType, totalBlocks);

    for (int blockNum = 0; blockNum < totalBlocks; ++blockNum) {
        size_t start = blockNum * chunkSize;
        size_t end = std::min<size_t>(start + chunkSize, fileContent.size());
        size_t length = end - start;
        std::string chunk = fileContent.substr(start, end - start);
        RsendMessage(clientSocket, RType, length, chunk);
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // ���Ʒ����ٶ�
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData); // ��ʼ��Winsock��

    // ��ʾ�û����� IP ��ַ�Ͷ˿ں�
    std::string ipAddress;
    unsigned short port;
    std::cout << "������������� IP ��ַ��";
    std::cin >> ipAddress;
    std::cout << "������������Ķ˿ںţ�";
    std::cin >> port;

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "�����׽���ʧ��: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // ���÷�������ַ�ṹ
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    serverAddr.sin_port = htons(port);

    // ���ӵ�������
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "����ʧ��: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
     
    // �������ͺͽ����߳�
    std::thread recvThread(receiveMessages, clientSocket);
    std::thread sendThread(sendMessages, clientSocket, "test.txt");

    recvThread.join();
    sendThread.join();

    // �ر��׽��ֺ�����Winsock��
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
