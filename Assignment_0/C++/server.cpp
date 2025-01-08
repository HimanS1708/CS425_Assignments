#include <WinSock2.h>
#include <WS2tcpip.h>
#include <shlwapi.h>
#include <tchar.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <filesystem>

#define BUFFER_SIZE 1024

namespace fs = std::filesystem;

void log_request(const std::string& clientIp, const std::string& request, int statusCode){
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::ctime(&now);
    timestamp.pop_back();
    std::ofstream logFile("server.log", std::ios::app);
    std::string logEntry = "["+timestamp+"] ["+clientIp+"] "+request+std::to_string(statusCode)+"\n";
    std::cout<<logEntry;
    if(logFile.is_open()) logFile << logEntry;
}

void send_response(int acceptSocket, int statusCode, const std::string& statusMessage, const std::string& content, const std::string& contentType = "text/plain"){
    std::ostringstream response;
    response<<"HTTP/1.1 "<<statusCode<<" "<<statusMessage<<"\r\n";
    response<<"Content-Type: "<<contentType<<"\r\n";
    response<<"Content-Length: "<<content.size()<<"\r\n\r\n";
    response<<content;
    
    send(acceptSocket, response.str().c_str(), response.str().size(), 0);
}

bool ends_with(const std::string& value, const std::string& ending) {
    if (ending.size() > value.size()) {
        return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string get_mime_type(const std::string& fullPath){
    if(ends_with(fullPath, ".html")) return "text/html";
    if(ends_with(fullPath, ".css")) return "text/css";
    if(ends_with(fullPath, ".js")) return "application/javascript";
    if(ends_with(fullPath, ".png")) return "image/png";
    if(ends_with(fullPath, ".jpg") || ends_with(fullPath, ".jpeg")) return "image/jpeg";
    return "application/octet-stream";
}

std::string read_file(const std::string& fullPath){
    std::ifstream file(fullPath, std::ios::binary);
    if(!file.is_open()) return "";
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void send_file(int acceptSocket, const std::string& fullPath){
    std::string content = read_file(fullPath);
    if(content.empty()){
        send_response(acceptSocket, 404, "Not Found", "<h1>404 Not Found</h1>", "text/html");
    }
    else{
        send_response(acceptSocket, 200, "OK", content, get_mime_type(fullPath));
    }
}

void send_directory_listing(int acceptSocket, const std::string& fullPath){
    std::ostringstream content;
    content << "<html><body><h1>Directory Listing</h1><ul>";

    try {
        for (const auto& entry : fs::directory_iterator(fullPath)) {
            content << "<li>"<< entry.path().filename().string() << "</li>";
        }
    } catch (const fs::filesystem_error& e) {
        content << "<li>Error reading directory: " << e.what() << "</li>";
    }

    content << "</ul></body></html>";
    send_response(acceptSocket, 200, "OK", content.str(), "text/html");
}

void handle_request(int acceptSocket,const std::string& directory){
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int byteCount = recv(acceptSocket, buffer, BUFFER_SIZE, 0);

    std::string request(buffer);
    std::istringstream requestStream(request);
    std::string method, path, version;
    requestStream >> method >> path >> version;

    std::string clientIp = inet_ntoa(((sockaddr_in*)(&acceptSocket))->sin_addr);

    if(method != "GET"){
        log_request(clientIp, request, 405);
        send_response(acceptSocket, 405, "Method Not Allowed", "<h1>405 Method Not Allowed</h1>", "text/html");
        return;
    }

    std::string fullPath = directory+path;
    struct _stat fileStat;
    if(_stat(fullPath.c_str(), &fileStat)==0){
        if(fileStat.st_mode & _S_IFDIR){
            send_directory_listing(acceptSocket, fullPath);
        }
        else{
            send_file(acceptSocket, fullPath);
        }
        log_request(clientIp, request, 200);
    }
    else{
        send_response(acceptSocket, 404, "Not Found", "<h1>404 Not Found</h1>", "text/html");
        log_request(clientIp, request, 404);
    }
}

int main(int argc, char* argv[]){
    if(argc!=3){
        std::cerr<<"Usage: "<<argv[0]<<" <port> <directory>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string directory = argv[2];

    WSADATA wsadata;
    int wsaerr;
    WORD wVersionRequested=MAKEWORD(2,2);
    wsaerr = WSAStartup(wVersionRequested, &wsadata);

    if(wsaerr!=0){
        std::cout<<"The winsock dll not found\n";
        return 0;
    }
    else{
        std::cout<<"The winsock dll found\n";
        std::cout<<"Status: "<<wsadata.szSystemStatus<<"\n";
    }

    SOCKET serverSocket = INVALID_SOCKET;
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(serverSocket == INVALID_SOCKET){
        std::cout<<"Error at socket(): "<<WSAGetLastError()<<"\n";
        WSACleanup();
        return 0;
    }
    else{
        std::cout<<"socket() is OK!\n";
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
    service.sin_port = htons(port);
    if(bind(serverSocket, (sockaddr*)&service, sizeof(service)) == SOCKET_ERROR){
        std::cout<<"bind() failed: "<<WSAGetLastError()<<"\n";
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    else{
        std::cout<<"bind() is OK\n";
    }

    if(listen(serverSocket, 1) == SOCKET_ERROR){
        std::cout<<"listen(): Error listening on socket "<<WSAGetLastError()<<"\n";
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    else{
        std::cout<<"Server started on port "<<port<<", serving files from "<<directory<<"\n";
    }

    while(1){
        SOCKET acceptSocket;
        acceptSocket = accept(serverSocket, NULL, NULL);
        if(acceptSocket == INVALID_SOCKET){
            std::cout<<"accept() failed: "<<WSAGetLastError()<<"\n";
            continue;
        }

        handle_request(acceptSocket, directory);
        closesocket(acceptSocket);
    }
    
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}