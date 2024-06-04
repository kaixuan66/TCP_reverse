#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")// 指定链接到Ws2_32.lib库
#pragma warning(disable: 4996)

// 定义协议类型的枚举
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

// 反转字符串函数
std::string reverseString(const std::string& str) {
    std::string reversedStr(str.rbegin(), str.rend());
    return reversedStr;
}

// 处理客户端连接的函数
void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int blocksToReceive = 0; // 用于记录要接收的块数
    while (true) {
        // 接收客户端消息
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::string receivedMessage(buffer, 0, bytesReceived);

            if (!receivedMessage.empty()) {
                // 处理以1开头的消息，表示要接收的块数
                if (receivedMessage[0] == '1') {
                    blocksToReceive = std::stoi(receivedMessage.substr(1));
                    receivedMessage = std::to_string(AgreeType);
                    std::cout << "需要处理的块数: " << blocksToReceive << std::endl;
                    send(clientSocket, receivedMessage.c_str(), receivedMessage.length(), 0);// 发送响应消息
                }
                // 处理以3开头的消息，表示要反转的字符串
                else if (receivedMessage[0] == '3') {
                    receivedMessage = receivedMessage.substr(1);
                    size_t pos = receivedMessage.find_first_not_of("0123456789"); // 查找第一个非数字字符的位置
                    if (pos != std::string::npos) {
                        std::string numberStr = receivedMessage.substr(0, pos); // 提取整数部分
                        int length = std::stoi(numberStr); // 转换为整数
                        std::cout << "接收到的长度: " << length << std::endl;
                        receivedMessage = receivedMessage.substr(pos); // 去除整数部分
                        std::cout << "接收的信息为: " << receivedMessage << std::endl;
                        std::string reversedMessage = reverseString(receivedMessage); // 反转内容
                        std::string reply = std::to_string(RType) + std::to_string(length) + reversedMessage;
                        send(clientSocket, reply.c_str(), reply.length(), 0);
                        ++blocksToReceive; // 块数加1
                    }
                }
            }

            // 如果块数处理完，跳出循环
            if (--blocksToReceive <= 0)
                break;
        }
        else {//没有接收到数据跳出循环
            break;
        }
    }
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;// 初始化Winsock库  定义WSADATA结构，用于存储Winsock库的信息
    WSAStartup(MAKEWORD(2, 2), &wsaData);//初始化Winsock2.2版本

    // 创建服务器套接字
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "套接字创建失败" << WSAGetLastError() << std::endl;// 输出错误信息
        WSACleanup();// 清理Winsock库
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;// 设置IP地址为任意
    serverAddr.sin_port = htons(11111);

    // 绑定服务器套接字
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "绑定失败" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // 使服务器进入监听状态
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "监听失败" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "服务器正在监听端口11111" << std::endl;

    // 接受客户端连接并创建新线程处理每个连接
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "接受失败" << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "客户端成功连接" << std::endl;

        std::thread clientThread(handleClient, clientSocket);// 创建新线程处理客户端连接
        clientThread.detach(); // 分离线程，让它自己执行
    }

    // 关闭服务器套接字
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
