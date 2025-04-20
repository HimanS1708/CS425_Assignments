# TCP Handshake Assignment

This project implements the client-side logic of a simplified TCP three-way handshake using raw sockets in C++. The goal is to craft and send custom TCP packets (SYN, ACK) manually using raw socket APIs to connect with a provided server that completes the handshake.

---

## Assignment Features

**Implemented:**
- **SYN Packet Generation:**  
  - Constructs a valid TCP SYN packet with the required sequence number (`200`).
  - Populates all relevant fields in the IP and TCP headers.
  
- **SYN-ACK Parsing:**  
  - Waits for a server response and verifies flags (`SYN`, `ACK`).
  - Ensures the `ack_seq` from the server is as expected (`201`).

- **Final ACK Packet Transmission:**  
  - Sends the final ACK with sequence number `600` and acknowledgment number `server_seq + 1`.
  - Completes the 3-step TCP handshake.

**Not Implemented:**
- **TCP Option Fields or Advanced Features:**  
  - Only core TCP fields are used; optional TCP features are ignored.

---

## Design Decisions

- **Raw Sockets & IP_HDRINCL Usage:**  
  - The client uses raw sockets to bypass OS-managed TCP/IP stack behavior.
  - `IP_HDRINCL` is set so the client can manually build the IP header.

- **Checksum Calculation:**  
  - A header is used with the TCP segment to compute the checksum as required by the TCP protocol.

- **Static Sequence Numbers for Evaluation:**  
  - Fixed values (`200`, `400`, `600`) are used for `seq` and `ack_seq` as required by the server’s logic.

- **Retry Mechanism:**  
  - If a SYN-ACK is not received within 2 seconds, the SYN is resent (up to 3 times).
  
---

## Implementation

### High-Level Overview

The project is organized around a class that encapsulates the handshake logic:

- **`send_syn()`**  
  - Constructs and transmits the initial SYN packet.

- **`receive_syn_ack()`**  
  - Waits for the server’s SYN-ACK and extracts the sequence number.

- **`send_ack()`**  
  - Constructs and transmits the final ACK with a sequence number of `600`.

- **`perform_handshake()`**  
  - Orchestrates the full three-way handshake and ensures sequencing is valid.

### Code Flow Diagram

Below is a simplified flow representing the handshake:

```sequence
    A[Start Client] --> B[Send SYN (SEQ=200)]  
    B --> C[Wait for SYN-ACK]  
    C --> D{Valid Response?}  
    D -- Yes --> E[Extract SEQ=400, ACK=201]  
    E --> F[Send ACK (SEQ=600, ACK=401)]  
    F --> G[Handshake Complete]  
    D -- No --> H[Resend SYN up to 3 times]  
    H --> C
```

---

## Restrictions

- **Localhost Only:**  
  - The implementation assumes both client and server are running on the same host (`127.0.0.1`).
- **No Dynamic Packet Lengths or Payloads:**  
  - No support for options or variable TCP header lengths.

---

## Challenges

- **Checksum Accuracy:**  
  - Building the pseudo-header and ensuring alignment was crucial and error-prone.
- **Raw Socket Limitations:**  
  - Root privileges were required to run the client, and OS interference had to be minimized.

---

## Contribution of Each Member
### **Team Members**
- Daksh Kumar Singh-220322
- Himanshu Shekhar-220454
- Swayamsidh Pradhan-221117

The work was equally distributed among all the three team members, with each member contributing equally to the total effort through collaborative implementation, testing, and debugging of the project.

---

## Sources Referred

*   **TCP Raw Socket References:**  
    *   Linux Raw Socket Programming articles on GeeksForGeeks

---

## Declaration

* We hereby declare that we did not indulge in plagiarism and that this assignment represents our original work.

---

## Feedback

* For any queries or feedback, feel free to reach out via email. We appreciate your input and are happy to assist with any questions!