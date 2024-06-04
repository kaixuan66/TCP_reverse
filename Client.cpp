#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <winsock2.h>
#include <random> 
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4996)

int totalBlocks; // 总块数
std::string total; // 保存最终内容

// 初始化协议类型的枚举
enum Initialization {
    IniType = 1,//类型
    IniN = 0,//块数
};

// 反转请求协议类型的枚举
enum reverseRequest {
    RType = 3,
    RLength = 0,
    RData
};

// 发送初始化消息
void sendMessage(SOCKET clientSocket, Initialization type, int blockNum) {
    std::string message = std::to_string(type) + std::to_string(blockNum);
    send(clientSocket, message.c_str(), message.length(), 0);
}

// 发送反转请求消息
void RsendMessage(SOCKET clientSocket, reverseRequest type, int blockNum, const std::string& content) {
    std::string message = std::to_string(type) + std::to_string(blockNum) + content;
    send(clientSocket, message.c_str(), message.length(), 0);
}

// 接收服务器消息
void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;
    int currentBlock = 0; // 当前块数

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        std::string receivedMessage(buffer, 0, bytesReceived);
        if (!receivedMessage.empty()) {
            // 处理以4开头的消息
            if (receivedMessage[0] == '4') {
                receivedMessage = receivedMessage.substr(1);

                size_t pos = receivedMessage.find_first_not_of("0123456789");
                if (pos != std::string::npos) {
                    std::string numberStr = receivedMessage.substr(0, pos);
                    int extractedNumber = std::stoi(numberStr);
                    std::cout << "当前信息的块数 " << ++currentBlock << std::endl;
                    std::string remainingMessage = receivedMessage.substr(pos);
                    total.insert(0, remainingMessage);
                    std::cout << "反转后的内容: " << remainingMessage << std::endl;
                    if (currentBlock == totalBlocks) {
                        std::cout << "最终内容 " << total << std::endl;
                    }
                }
            }
            // 处理以2开头的消息
            if (receivedMessage[0] == '2') {
                std::cout << "服务器同意接收，需要处理总块数 " << totalBlocks << std::endl;
            }
        }
    }

    if (bytesReceived == 0) {
        std::cerr << "服务器已经断开" << std::endl;
    }
    else {
        std::cerr << "连接已关闭或发生错误" << std::endl;
    }
}

// 发送文件内容给服务器
void sendMessages(SOCKET clientSocket, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "打开文件失败: " << filename << std::endl;
        return;
    }

    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (fileContent.empty()) {
        std::cerr << "文件为空" << std::endl;
        return;
    }

    // 随机生成块大小
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
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 控制发送速度
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData); // 初始化Winsock库

    // 提示用户输入 IP 地址和端口号
    std::string ipAddress;
    unsigned short port;
    std::cout << "请输入服务器的 IP 地址：";
    std::cin >> ipAddress;
    std::cout << "请输入服务器的端口号：";
    std::cin >> port;

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "创建套接字失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 配置服务器地址结构
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    serverAddr.sin_port = htons(port);

    // 连接到服务器
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "连接失败: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
     
    // 创建发送和接收线程
    std::thread recvThread(receiveMessages, clientSocket);
    std::thread sendThread(sendMessages, clientSocket, "test.txt");

    recvThread.join();
    sendThread.join();

    // 关闭套接字和清理Winsock库
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
