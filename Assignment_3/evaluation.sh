#!/bin/bash

echo "[+] Compiling server..."
g++ server.cpp -o server
echo "[+] Compiling client..."
g++ client.cpp -o client

echo "[+] Running server in background (requires sudo)..."
sudo ./server > server_output.txt 2>&1 &
SERVER_PID=$!
echo "[+] Server started with PID $SERVER_PID"
sleep 5  # Give the server time to start

echo "[+] Running client..."
sudo ./client > client_output.txt 2>&1

# Wait for client to finish
sleep 2

# Kill the server process
echo "[+] Killing server..."
sudo kill $SERVER_PID

# Output logs
echo ""
echo "==== SERVER OUTPUT ===="
cat server_output.txt
echo ""
echo "==== CLIENT OUTPUT ===="
cat client_output.txt

# Clean up
rm -f server client server_output.txt client_output.txt

echo "[+] Test completed."
