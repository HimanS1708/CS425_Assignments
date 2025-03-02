// Include Stuff
#include <mutex>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <iostream>

// Define macros
#define BUFFER_SIZE 1024
#define BACKLOG 10

namespace fs = std::filesystem;

int INVALID_SOCKET = -1;
int SOCKET_ERROR = -1;

// Define global variables
std::mutex cout_mutex;
std::mutex global_mutex;

std::unordered_map<int, std::mutex> group_mutex;
std::unordered_map<int, std::mutex> client_send_mutexes;
std::unordered_map<int, std::mutex> client_recv_mutexes;

std::map<std::string, std::string> Passwords;
std::map<std::string, int> client_socket;
std::map<std::string, int> logged_in;
std::map<std::string, std::set<std::string>> group;
std::map<std::string, int> group_id;

// [sender, receiver, message]
std::queue<std::tuple<std::string, std::string, std::string>> msgs;
int group_count = 0;

// Print the server logs
void server_logs(std::string log)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Server Logs: " << log << "\n";
}

// Handle user messages
void handle_messages(std::string username, char *buffer)
{
    std::string message = buffer;
    std::string word = "";
    int id = client_socket[username];
    // read first word
    std::stringstream ss(message);
    ss >> word;

    if (word == "/msg")
    {
        // receiver , msg
        std::string receiver, msg;
        ss >> receiver;
        getline(ss, msg);
        msg = msg.substr(1);
        server_logs("Message from " + username + " to " + receiver + ": " + msg);
        std::lock_guard<std::mutex> lock(global_mutex);
        msgs.push({username, receiver, msg});
    }
    else if (word == "/create_group")
    {
        std::string group_name;
        ss >> group_name;
        {
            std::lock_guard<std::mutex> lock(global_mutex);
<<<<<<< HEAD
            if(group.count(group_name)){
                std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
=======
            if (group.count(group_name))
            {
                std::lock_guard<std::mutex> lock(client_mutexes[id]);
>>>>>>> 76a6057c0cf5c1e2ef51f8517b23356bef875afb
                std::string response = "Group " + group_name + " already exists";
                send(client_socket[username], response.c_str(), response.size(), 0);
                return;
            }
            group[group_name].insert(username);
            group_id[group_name] = group_count++;
        }
        server_logs("Group " + group_name + " created by " + username);

        {
            std::lock_guard<std::mutex> lock(client_send_mutexes[id]);

            std::string response = "Group " + group_name + " created";
            send(client_socket[username], response.c_str(), response.size(), 0);
        }
    }
    else if (word == "/join_group")
    {
        std::string group_name;
        ss >> group_name;

        // Check if the group exists
<<<<<<< HEAD
        if(!group_id.count(group_name)){
            std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
=======
        if (!group_id.count(group_name))
        {
            std::lock_guard<std::mutex> lock(client_mutexes[id]);
>>>>>>> 76a6057c0cf5c1e2ef51f8517b23356bef875afb

            std::string response = "Group " + group_name + " does not exist";
            send(client_socket[username], response.c_str(), response.size(), 0);

            return;
        }

        // Handle group logic
        int id = group_id[group_name];
        {
            std::lock_guard<std::mutex> lock(group_mutex[id]);
            group[group_name].insert(username);
        }

        server_logs(username + " joined group " + group_name);

        {
            std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
            std::string response = "Joined group " + group_name;
            send(client_socket[username], response.c_str(), response.size(), 0);
        }
    }
    else if (word == "/group_msg")
    {
        std::string group_name, msg;
        ss >> group_name;
        int id = group_id[group_name];
        getline(ss, msg);
        msg = msg.substr(1);
        server_logs("Message from " + username + " to group " + group_name + ": " + msg);
        std::set<std::string> members;
        {
            std::lock_guard<std::mutex> lock(group_mutex[id]);
            for (auto member : group[group_name])
            {
                members.insert(member);
            }
        }

        std::lock_guard<std::mutex> lock(global_mutex);
        for (auto member : members)
        {
            if (member == username)
                continue;
            msgs.push({"Group " + group_name, member, msg});
        }
    }
    else if (word == "/leave_group")
    {
        std::string group_name;
        ss >> group_name;
        int group_exists = 0;
        int is_member = 0;
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            group_exists = group_id.count(group_name);
            is_member = group[group_name].count(username);
        }
        // Check if user is a member of the group or not
        if (!group_exists || !is_member)
        {
            std::lock_guard<std::mutex> lock(client_send_mutexes[id]);

            std::string response = "User " + username + " not a member of group " + group_name;
            send(client_socket[username], response.c_str(), response.size(), 0);

            return;
        }

        // Handle group leaving logic
        int id = -1;
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            id = group_id[group_name];
        }
        {
            std::lock_guard<std::mutex> lock(group_mutex[id]);
            group[group_name].erase(username);
        }
        server_logs(username + " left group " + group_name);

        {
            std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
            std::string response = "Left group " + group_name;
            send(client_socket[username], response.c_str(), response.size(), 0);
        }
    }
    else if (word == "/broadcast")
    {
        std::string msg;
        getline(ss, msg);
        msg = msg.substr(1);
        server_logs("Broadcast message from " + username + ": " + msg);
        std::lock_guard<std::mutex> lock(global_mutex);
        for (auto [client, logged_in] : logged_in)
        {
            if (client == username)
                continue;
            if (logged_in == 1)
            {
                msgs.push({"BROADCAST " + username, client, msg});
            }
        }
    }
    else
    {
        std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
        std::string response = "Invalid command";
        send(client_socket[username], response.c_str(), response.size(), 0);
    }
}

// Handle requests from the client
void handle_client_messages(std::string username)
{
    int acceptSocket = client_socket[username];
    char buffer[BUFFER_SIZE];
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        {
            std::lock_guard<std::mutex> lock(client_recv_mutexes[acceptSocket]);
            int bytes_received = recv(acceptSocket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0)
            {
                server_logs("Disconnected from client " + username);
                client_send_mutexes.erase(acceptSocket);
                client_recv_mutexes.erase(acceptSocket);
                close(acceptSocket);
                {
                    std::lock_guard<std::mutex> lock(global_mutex);
                    logged_in[username] = 0;
                }

                std::lock_guard<std::mutex> lock(global_mutex);
                client_socket.erase(username);
                return;
            }
        }
        handle_messages(username, buffer);
    }
}

// Handle the joining of a client
void handle_client(std::string username)
{
    int acceptSocket = client_socket[username];

    // send messages about other participants
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        for (auto [client, logged_in] : logged_in)
        {
            if (logged_in == 1 && client != username)
            {
                // take the lock
                std::string message = client + " has joined the chat";
                std::string message_to_send = "\n" + message;
                int id = client_socket[username];
                server_logs("Sending message to " + username + ": " + message);
                std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
                send(acceptSocket, message_to_send.c_str(), message_to_send.size(), 0);
            }
        }
    }

    // create a thread to handle messages from this client
    std::thread handle_client_messages_thread(handle_client_messages, username);
    handle_client_messages_thread.detach();
}

// Authenticate the client
void authenticate_client(int acceptSocket)
{
    char user_prompt[BUFFER_SIZE];
    char password_prompt[BUFFER_SIZE];
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];
    int id = acceptSocket;
    memset(username, 0, BUFFER_SIZE);
    memset(password, 0, BUFFER_SIZE);
    strcpy(user_prompt, "Enter username: ");
    strcpy(password_prompt, "Enter password: ");

    {
        std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
        int bytes_sent = send(acceptSocket, user_prompt, strlen(user_prompt), 0);
        if (bytes_sent < 0)
        {
            perror("send() failed (username prompt)");
            client_send_mutexes.erase(acceptSocket);
            client_recv_mutexes.erase(acceptSocket);
            close(acceptSocket);
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(client_recv_mutexes[id]);
        memset(username, 0, BUFFER_SIZE);
        int bytes_received = recv(acceptSocket, username, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            server_logs("Disconnected from client at socket " + std::to_string(acceptSocket) + " (username)");
            client_send_mutexes.erase(acceptSocket);
            client_recv_mutexes.erase(acceptSocket);
            close(acceptSocket);
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
        int bytes_sent = send(acceptSocket, password_prompt, strlen(password_prompt), 0);
        if (bytes_sent < 0)
        {
            perror("send() failed (password prompt)");
            client_send_mutexes.erase(acceptSocket);
            client_recv_mutexes.erase(acceptSocket);
            close(acceptSocket);
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(client_recv_mutexes[id]);
        memset(password, 0, BUFFER_SIZE);
        int bytes_received = recv(acceptSocket, password, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            server_logs("Disconnected from client at socket " + std::to_string(acceptSocket) + " (password)");
            client_send_mutexes.erase(acceptSocket);
            client_recv_mutexes.erase(acceptSocket);
            close(acceptSocket);
            return;
        }
    }

    server_logs("Username: " + std::string(username) + " Password: " + std::string(password));
    if (Passwords.find(username) == Passwords.end() || Passwords[username] != password || logged_in[username] == 1)
    {
        server_logs("Authentication failed for " + std::string(username));
        std::string response = "Authentication failed";
        id = acceptSocket;
        std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
        send(acceptSocket, response.c_str(), response.size(), 0);
        client_send_mutexes.erase(acceptSocket);
        client_recv_mutexes.erase(acceptSocket);
        close(acceptSocket);
        return;
    }
    else
    {
        // take the lock
        server_logs("Authentication successful for " + std::string(username));
        {
            std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
            std::string response = "Welcome to the server " + std::string(username) + "!";
            send(acceptSocket, response.c_str(), response.size(), 0);
        }
        server_logs("Welcome " + std::string(username) + "!");
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            logged_in[username] = 1;
        }

        // if authentication is successful, start the client thread
        client_socket[username] = acceptSocket;
        handle_client(username);
    }
}

// Handle message pushing to each client
void push_messages()
{
    while (true)
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        std::queue<std::tuple<std::string, std::string, std::string>> afk_queue;
        while (!msgs.empty())
        {
            auto [sender, receiver, message] = msgs.front();
            msgs.pop();
            std::string msg = "[" + sender + "]: " + message;

            {
                if (logged_in[receiver] == 1)
                {
                    int id = client_socket[receiver];
                    // take the lock
                    server_logs("Sending message from " + sender + " to " + receiver + ": " + message);
                    std::lock_guard<std::mutex> lock(client_send_mutexes[id]);
                    send(client_socket[receiver], msg.c_str(), msg.size(), 0);
                }
                else
                {
                    // push to the back of the queue
                    afk_queue.push({sender, receiver, message});
                }
            }
        }
        while (!afk_queue.empty())
        {
            msgs.push(afk_queue.front());
            afk_queue.pop();
        }
    }
}

int main()
{
    int port = 12345;

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
        std::string username = str.substr(0, pos);
        std::string password = str.substr(pos + 1);
        Passwords[username] = password;
    }

    // Create the server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Error at socket(): Creation Failed " << "\n";
        return 0;
    }
    std::cout << "socket() is OK!\n";

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSocket, (sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR)
    {
        std::cout << "bind() failed" << "\n";
        close(serverSocket);
        return 0;
    }
    std::cout << "bind() is OK\n";

    if (listen(serverSocket, BACKLOG) == SOCKET_ERROR)
    {
        std::cout << "listen(): Error listening on socket " << "\n";
        close(serverSocket);
        return 0;
    }
    std::cout << "Server started on port " << port << "\n";

    std::thread push_dms_thread(push_messages);
    push_dms_thread.detach();

    while (1)
    {
        server_logs("Waiting for client connection...");
        sockaddr clientaddress;
        __socklen_t addressLength = sizeof(clientaddress);
        int acceptSocket = accept(serverSocket, (sockaddr *)&clientaddress, &addressLength);
        if (acceptSocket == INVALID_SOCKET)
        {
            close(serverSocket);
            exit(0);
        }

        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Connected to : " << inet_ntoa(((sockaddr_in *)&clientaddress)->sin_addr) << " " << "on socket" << " " << acceptSocket << "\n";
        client_send_mutexes[acceptSocket];
        client_recv_mutexes[acceptSocket];
        std::thread authenticate_client_thread(authenticate_client, acceptSocket);
        authenticate_client_thread.detach();
    }
    return 0;
}
