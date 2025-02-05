# Features

## Features implemented

✔ **Multithreaded Client Handling** – Each client connection is handled in a **separate thread**, allowing multiple users to interact simultaneously.  
✔ **User Authentication** – Users must log in using credentials stored in `users.txt`.  
✔ **Private Messaging** – Users can send **direct messages** using the `/msg` command.  
✔ **Group Messaging** – Users can create, join, and leave **chat groups**.  
✔ **Broadcast Messaging** – Users can send messages to **all online users** using `/broadcast`.  
✔ **Asynchronous Message Processing** – Messages are queued and **processed in the background** to avoid blocking client requests.  
✔ **Thread-Safe Message Queue** – Ensures **no data races** when handling messages.  
✔ **Proper Client Disconnection Handling** – **Prevents server crashes** when a client disconnects unexpectedly.  
✔ **Mutex-Guarded Communication** – Prevents **race conditions** with per-client and per-group mutexes.  
✔ **Blocking Send Mechanism** – Ensures **TCP does not club multiple messages together**.  

---

## Features not implemented

❌ **Persistent Chat History** – Messages are **not stored permanently** after a restart.  
❌ **File Transfer Support** – Currently, only **text messages** are supported.  
❌ **User Registration** – Users must be manually added to `users.txt`.  
❌ **End-to-End Encryption** – Messages are sent **in plain text** over the network.  
❌ **Error Handling** – Some messages are not checked for **errors** and **null** values.  

---

# Design Decisions

- Used **`std::unordered_map<int, std::mutex>`** instead of a **fixed-size mutex array** to **dynamically manage active clients and groups**, making memory allocation more efficient.  
- Implemented a **background message processing thread** (`push_messages()`) to handle **delayed/offline messages**, ensuring **no messages are lost**.  
- Used **blocking `send()`** to ensure **each message is sent separately** over TCP and prevent **message clubbing**.  
- Implemented **separate send and receive mutexes (`client_send_mutexes` and `client_recv_mutexes`)** for each client to prevent **data corruption**.  
- Used **graceful client disconnection handling** by properly erasing **sockets and mutexes** when a user logs out.  

---

# Code Flow

1. **Server Starts**
   - Reads `users.txt` to load valid usernames and passwords.
   - Creates a **TCP server socket** and starts listening for connections.
   - Starts the **`push_messages()`** thread for processing queued messages.

2. **Client Connects**
   - A new socket is created using `accept()`.
   - A new thread is spawned to **authenticate the user** (`authenticate_client()`).
   - After authentication, the client is added to `client_socket`.

3. **Message Handling**
   - Clients can send commands such as `/msg`, `/group_msg`, `/broadcast`, etc.
   - Messages are **added to a queue (`msgs`)**.
   - The `push_messages()` thread **processes and sends queued messages** asynchronously.

4. **Client Disconnects**
   - Mutexes and socket references are **removed safely**.
   - The thread handling the client **terminates gracefully**.

---

# Testing

-

# Challenges faced

### **1. Race Conditions on Shared Data**
#### **Issue:**
- Multiple threads were accessing and modifying shared data structures (`msgs`, `client_socket`, `logged_in`) **without proper synchronization**, causing **data corruption and crashes**.
- **Example:**  
  - If one thread erased `client_socket[username]` while another thread was sending a message to the same user, the program would crash.

#### **Solution:**
- Implemented **`std::mutex` locks** around shared resources.
- Used **separate send and receive mutexes** (`client_send_mutexes`, `client_recv_mutexes`) to **prevent simultaneous access issues**.
- **Example Fix:**
  ```cpp
  {
      std::lock_guard<std::mutex> lock(global_mutex);
      client_socket.erase(username);  // Prevents another thread from accessing after deletion
  }
  ```

### **2. Managing Client Disconnections Gracefully**
#### **Issue:**
- If a client **disconnected abruptly**, their **socket might still be accessed** by other threads, leading to **segmentation faults**.
- The **server crashed randomly** when clients **disconnected during message transmission**.
- **Example Scenario:**
  - A client disconnects while the `push_messages()` thread is attempting to send them a message.
  - Another thread tries to access `client_socket[username]`, but the socket is already closed, causing a **segfault**.

#### **Solution:**
- **Properly erased all references** to the socket before closing it.
- **Ensured no thread accesses a deleted client socket** by:
  - Locking `global_mutex` before modifying `client_socket`.
  - Checking if the socket exists before sending data.

#### **Example Fix:**
```cpp
{
    std::lock_guard<std::mutex> lock(global_mutex);
    if (client_socket.find(username) != client_socket.end()) {
        close(client_socket[username]);  // Ensure socket is closed before erasing
        client_socket.erase(username);
    }
    logged_in[username] = 0;
}
```

### **3. Offline Message Handling (Ensuring Messages Are Not Lost)**
#### **Issue:**
- If a user was **offline**, messages sent to them were **lost**.
- There was no mechanism to **store and retry messages when the user reconnects**.
- Users expected to receive messages even if they temporarily disconnected.

#### **Solution:**
- Introduced a **temporary message queue (`afk_queue`)** to store messages **for offline users**.
- When an offline user reconnects, **their queued messages are delivered**.
- This prevents **message loss** when a user is not available.

#### **Example Fix:**
```cpp
std::queue<std::tuple<std::string, std::string, std::string>> afk_queue;
while (!msgs.empty()) {
    auto [sender, receiver, message] = msgs.front();
    msgs.pop();

    if (logged_in[receiver] == 1) {
        send(client_socket[receiver], message.c_str(), message.size(), 0);
    } else {
        afk_queue.push({sender, receiver, message});
    }
}
```

# Server Restrictions

While the chat server is optimized for performance and scalability, there are certain limitations to be aware of:

### **1️. Maximum Concurrent Clients**
- The maximum number of concurrent clients is **limited by system resources** (CPU & RAM).  
- The server does **not impose a hard limit** but will become slow if too many clients are connected.  

### **2️. Maximum Message Size**
- The server currently limits messages to **1024 bytes** (defined by `BUFFER_SIZE`).  
- Messages exceeding this limit **will be truncated or lost**.

### **3️. No Persistent Chat History**
- Messages are **not stored permanently** after a restart.  
- The server does **not save chat logs** for retrieval later.

### **4️. No Encryption (Plaintext Communication)**
- All messages are sent **in plaintext over the network**.  
- No **end-to-end encryption** or TLS security is implemented.  
- **Risk:** Messages can be intercepted if running on an **unsecured network**.

### **5. No User Registration**
- Users must be **manually added** to `users.txt`.  
- There is **no dynamic user creation** or password change feature.  

### **6. Single Server Instance**
- The chat server runs as a **single instance** and does **not support distributed scaling**.  
- **Risk:** A single point of failure; if the server crashes, all clients disconnect.

---

# Contribution
### **1. Team Members**
- Daksh Kumar Singh-220322
- Himanshu Shekhar-220454
- Swayamsidh Pradhan-221117

The work was equally distributed among all the three team members, with each member contributing 33.33% of the total effort through collaborative implementation, testing, and debugging of the project.

# Sources

The following references and materials were used in building this chat server:

1. **C++ Networking and Sockets**
   - Beej's Guide to Network Programming: [https://beej.us/guide/bgnet/](https://beej.us/guide/bgnet/)
   - Linux `socket` and `pthread` Documentation: [https://man7.org/linux/man-pages/](https://man7.org/linux/man-pages/)

2. **Thread Synchronization and Concurrency**
   - C++ `std::mutex` and `std::thread` Documentation: [https://en.cppreference.com/](https://en.cppreference.com/)
   - Effective Use of Mutexes in C++: [https://www.modernescpp.com/index.php/mutex](https://www.modernescpp.com/index.php/mutex)

3. **Message Queueing and Data Structures**
   - C++ `std::queue` for Message Handling: [https://en.cppreference.com/w/cpp/container/queue](https://en.cppreference.com/w/cpp/container/queue)
   - Managing Thread-Safe Data Structures: [https://en.cppreference.com/w/cpp/thread](https://en.cppreference.com/w/cpp/thread)

4. **Client-Server Communication Best Practices**
   - TCP/IP Socket Programming: [https://www.geeksforgeeks.org/socket-programming-cc/](https://www.geeksforgeeks.org/socket-programming-cc/)
   - Handling Multiple Clients with `select()` and `poll()`: [https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html](https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html)



# Declaration

This project is developed as part of the **IITK CS425 Computer Networks** course under **Prof. Aditya Vadapalli**.  
All code and documentation are intended for educational purposes only. Any external resources used have been appropriately referenced in the **Sources** section.  


# Feedback

For any queries or feedback, feel free to reach out via email. We appreciate your input and are happy to assist with any questions!

