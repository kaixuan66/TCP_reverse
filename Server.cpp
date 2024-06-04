#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")// ָ�����ӵ�Ws2_32.lib��
#pragma warning(disable: 4996)

// ����Э�����͵�ö��
enum agree
{
    AgreeType = 2
};
enum reverseAnswer
{
    RType = 4,
    RLength = 0,
    RData
};

// ��ת�ַ�������
std::string reverseString(const std::string& str) {
    std::string reversedStr(str.rbegin(), str.rend());
    return reversedStr;
}

// ����ͻ������ӵĺ���
void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int blocksToReceive = 0; // ���ڼ�¼Ҫ���յĿ���
    while (true) {
        // ���տͻ�����Ϣ
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::string receivedMessage(buffer, 0, bytesReceived);

            if (!receivedMessage.empty()) {
                // ������1��ͷ����Ϣ����ʾҪ���յĿ���
                if (receivedMessage[0] == '1') {
                    blocksToReceive = std::stoi(receivedMessage.substr(1));
                    receivedMessage = std::to_string(AgreeType);
                    std::cout << "��Ҫ����Ŀ���: " << blocksToReceive << std::endl;
                    send(clientSocket, receivedMessage.c_str(), receivedMessage.length(), 0);// ������Ӧ��Ϣ
                }
                // ������3��ͷ����Ϣ����ʾҪ��ת���ַ���
                else if (receivedMessage[0] == '3') {
                    receivedMessage = receivedMessage.substr(1);
                    size_t pos = receivedMessage.find_first_not_of("0123456789"); // ���ҵ�һ���������ַ���λ��
                    if (pos != std::string::npos) {
                        std::string numberStr = receivedMessage.substr(0, pos); // ��ȡ��������
                        int length = std::stoi(numberStr); // ת��Ϊ����
                        std::cout << "���յ��ĳ���: " << length << std::endl;
                        receivedMessage = receivedMessage.substr(pos); // ȥ����������
                        std::cout << "���յ���ϢΪ: " << receivedMessage << std::endl;
                        std::string reversedMessage = reverseString(receivedMessage); // ��ת����
                        std::string reply = std::to_string(RType) + std::to_string(length) + reversedMessage;
                        send(clientSocket, reply.c_str(), reply.length(), 0);
                        ++blocksToReceive; // ������1
                    }
                }
            }

            // ������������꣬����ѭ��
            if (--blocksToReceive <= 0)
                break;
        }
        else {//û�н��յ���������ѭ��
            break;
        }
    }
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;// ��ʼ��Winsock��  ����WSADATA�ṹ�����ڴ洢Winsock�����Ϣ
    WSAStartup(MAKEWORD(2, 2), &wsaData);//��ʼ��Winsock2.2�汾

    // �����������׽���
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "�׽��ִ���ʧ��" << WSAGetLastError() << std::endl;// ���������Ϣ
        WSACleanup();// ����Winsock��
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;// ����IP��ַΪ����
    serverAddr.sin_port = htons(11111);

    // �󶨷������׽���
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "��ʧ��" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // ʹ�������������״̬
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "����ʧ��" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "���������ڼ����˿�11111" << std::endl;

    // ���ܿͻ������Ӳ��������̴߳���ÿ������
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "����ʧ��" << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "�ͻ��˳ɹ�����" << std::endl;

        std::thread clientThread(handleClient, clientSocket);// �������̴߳���ͻ�������
        clientThread.detach(); // �����̣߳������Լ�ִ��
    }

    // �رշ������׽���
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
