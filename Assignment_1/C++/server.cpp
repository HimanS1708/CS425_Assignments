#include <bits/stdc++.h>
#include <mutex>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <sstream>
#include <resolv.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <filesystem>

#define BUFFER_SIZE 1024
#define BACKLOG 10
using namespace std;
namespace fs = std::filesystem;

int INVALID_SOCKET = -1;
int SOCKET_ERROR = -1;

std::mutex cout_mutex;

// for client sockets
std::mutex client_mutex;
// for msg q
std::mutex message_mutex;

map<string, string> Passwords;
map<string, int> client_socket;
map<string, int> logged_in;
// sender , receiver, message
queue<tuple<string, string, string>> msgs;

void server_logs(std::string log)
{
    // log the server logs,  take cout mutex
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Server Logs: " << log << "\n";
}

void handle_messages(string username, char *buffer)
{
    std::string message = buffer;
    string word = "";
    // read first word
    std::stringstream ss(message);
    ss >> word;
    if (word == "/msg")
    {
        // receiver , msg
        string receiver, msg;
        ss >> receiver;
        getline(ss, msg);
        msg = msg.substr(1);
        server_logs("Message from " + username + " to " + receiver + ": " + msg);
        std::lock_guard<std::mutex> lock(message_mutex);
        msgs.push({username, receiver, msg});
    }
}

void handle_client_messages(string username)
{
    int acceptSocket = client_socket[username];
    char buffer[BUFFER_SIZE];
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(acceptSocket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Disconnected from client." << std::endl;
            close(acceptSocket);
            logged_in[username] = 0;
            client_socket.erase(username);
            return;
        }
        else
        {
            // std::lock_guard<std::mutex> lock(cout_mutex);
            // std::cout << buffer << std::endl;
        }
        handle_messages(username, buffer);
    }
}

void handle_client(string username)
{
    int acceptSocket = client_socket[username];
    char buffer[BUFFER_SIZE];
    // send messages about other participants
    for (auto [client, logged_in] : logged_in)
    {
        if (logged_in == 1 and client != username)
        {
            // take the lock
            std::string message = client + " has joined the chat";
            server_logs("Sending message to " + username + ": " + message);
            std::lock_guard<std::mutex> lock(client_mutex);
            send(acceptSocket, message.c_str(), message.size(), 0);
        }
    }

    // create a thread to handle messages from this client
    std::thread handle_client_messages_thread(handle_client_messages, username);
    handle_client_messages_thread.detach();
}

void broadcast_message(const std::string &message)
{
    for (auto &client : client_socket)
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        send(client.second, message.c_str(), message.size(), 0);
    }
}

void authenticate_client(int acceptSocket)
{
    char user_prompt[BUFFER_SIZE];
    char password_prompt[BUFFER_SIZE];
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    memset(username, 0, BUFFER_SIZE);
    memset(password, 0, BUFFER_SIZE);
    strcpy(user_prompt, "Enter username: ");
    strcpy(password_prompt, "Enter password: ");
    if (1)
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        send(acceptSocket, user_prompt, strlen(user_prompt), 0);
    }
    if (1)
    {

        std::lock_guard<std::mutex> lock(client_mutex);
        memset(username, 0, BUFFER_SIZE);
        recv(acceptSocket, username, BUFFER_SIZE, 0);
    }
    if (1)
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        send(acceptSocket, password_prompt, strlen(password_prompt), 0);
    }
    if (1)
    {
        std::lock_guard<std::mutex> lock(client_mutex);
        memset(password, 0, BUFFER_SIZE);
        recv(acceptSocket, password, BUFFER_SIZE, 0);
    }
    server_logs("Username: " + std::string(username) + " Password: " + std::string(password));
    if (Passwords.find(username) == Passwords.end() || Passwords[username] != password || logged_in[username] == 1)
    {
        server_logs("Authentication failed for " + std::string(username));
        string response = "Authentication failed";
        std::lock_guard<std::mutex> lock(client_mutex);
        send(acceptSocket, response.c_str(), response.size(), 0);
        close(acceptSocket);
        return;
    }
    else
    {
        // take the lock
        server_logs("Authentication successful for " + std::string(username));
        if (1)
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            std::string response = "Welcome to the server " + std::string(username);
            send(acceptSocket, response.c_str(), response.size(), 0);
        }
        server_logs("Welcome " + std::string(username) + "!");
        logged_in[username] = 1;
        // if authentication is successful, start the client thread
        client_socket[username] = acceptSocket;
        handle_client(username);
    }
}

void push_dms()
{
    while (true)
    {
        std::lock_guard<std::mutex> lock(message_mutex);
        while (!msgs.empty())
        {
            auto [sender, receiver, message] = msgs.front();
            msgs.pop();
            std::string msg = sender + ": " + message + "\n";
            if (receiver == "all")
            {
                broadcast_message(msg);
            }
            else
            {
                if (logged_in[receiver] == 1)
                {
                    // take the lock
                    server_logs("Sending message from " + sender + " to " + receiver + ": " + message);
                    std::lock_guard<std::mutex> lock(client_mutex);
                    send(client_socket[receiver], msg.c_str(), msg.size(), 0);
                }
                else
                {
                    // push to the back of the queue
                    msgs.push({sender, receiver, message});
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    int port = 12345;
    std::string directory = "static";

    // read from users.txt
    std::ifstream usersFile("users.txt");
    if (!usersFile.is_open())
    {
        std::cerr << "Failed to open users.txt" << std::endl;
        return 0;
    }
    std::string str;
    while (usersFile >> str)
    {
        // username:password
        auto pos = str.find(':');
        if (pos == std::string::npos)
        {
            std::cerr << "Invalid line in users.txt: " << str << std::endl;
            return 0;
        }
        string username = str.substr(0, pos);
        string password = str.substr(pos + 1);
        Passwords[username] = password;
    }

    int serverSocket = INVALID_SOCKET;

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, NULL, 1);
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Error at socket(): Creation Failed " << "\n";
        return 0;
    }
    else
    {
        std::cout << "socket() is OK!\n";
    }

    // cout << serverSocket << "\n";

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSocket, (sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        std::cout << "bind() failed: " << "\n";
        close(serverSocket);
        return 0;
    }
    else
    {
        std::cout << "bind() is OK\n";
    }

    if (listen(serverSocket, BACKLOG) == SOCKET_ERROR)
    {
        std::cout << "listen(): Error listening on socket " << "\n";
        close(serverSocket);
        return 0;
    }
    else
    {
        std::cout << "Server started on port " << port << "\n";
    }

    std::thread push_dms_thread(push_dms);
    push_dms_thread.detach();

    while (1)
    {
        server_logs("Waiting for client connection...");
        int acceptSocket = INVALID_SOCKET;
        sockaddr clientaddress;
        __socklen_t addressLength = sizeof(clientaddress);
        acceptSocket = accept(serverSocket, (sockaddr *)&clientaddress, &addressLength);
        if (acceptSocket == INVALID_SOCKET)
        {
            close(serverSocket);
            exit(0);
        }
        else
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Connected to : " << inet_ntoa(((sockaddr_in *)&clientaddress)->sin_addr) << " " << "on socket" << " " << acceptSocket << "\n";
        }
        std::thread authenticate_client_thread(authenticate_client, acceptSocket);
        authenticate_client_thread.detach();
    }
    return 0;
}

// disconnecting client sometimes causes the server to crash
// sometimes authentication fails for no reason, some thread problem
// while displaying all users logged in, the client clubs multiple packets together and displays them together as one message ? If we can implement a blocking send.....
// implement broadcast and group chat
// another issue... make separate client_mutex locks for different client sockets.
