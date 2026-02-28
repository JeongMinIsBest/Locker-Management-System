# 🔒 Real-time Locker Management System
A system that manages **locker usage in real time** based on access by multiple users. Through **network communication between clients and a server**, the system supports locker usage, password setup and modification, and secure unlocking.
<br/>

## ✏️ Project Overview
👩‍💻 **Planning & Development**: Jeongmin Lim  
🗓 **Development Period**: June 10, 2024 – June 17, 2024 (8 days)  
📚 **Project Title**: Linux Programming Final Project (Spring 2024)
<br/>

## 🖱️ How to Run
1. Compile the server program with pthread support
```
gcc -o server server.c -lpthread
```
  
2. Run the server
```
./server
```

3. Compile the client program for server connection and data transmission
```
gcc -o client client.c
```

4. Run the client
```
./client
```
<br/>

## 📈 Program Flow

### 1. Server Execution (`server.c`)
- Input the number of lockers and password conditions  
- Load or create the locker state file  
- Create a server socket and wait for client connections  
</br>

### 2. Client Execution (`client.c`)
- Connect to the server and enter commands  
</br>

### 3. Feature Execution
- View locker list  
- Store items and retrieve items  
- Change locker password  
</br>

### 4. Save Locker State and Terminate
- Previous locker state is preserved when the program is restarted  
<br/>

## 💡 Locker System Features

### 1. Locker Count & Password Condition Setup
- Users can view available and occupied lockers  
- The locker list is received from the server and displayed on the client screen  
</br>

### 2. Item Storage and Retrieval
- Clients can select a locker to store items  
- A password must be set when storing an item  
- Items can only be retrieved by entering the correct password  
- If the password is entered incorrectly **three times**, the locker is locked for **30 seconds**  
</br>

### 3. Password Setup and Modification
- Users can change the locker password after entering the current password  
</br>

## 🚨 Key System Highlights

### ✅ Concurrent Client Handling with Multithreading
- Uses **pthread** to allow multiple clients to connect to the server simultaneously  
- Synchronization is required to prevent data conflicts when multiple requests occur  
</br>

### ✅ Persistent Locker Data Storage
- Locker states are saved using a `lockers.dat` file  
- The server loads this file on startup to restore the previous state  
</br>

### ✅ Real-time Locker Status Updates
- The server responds with the latest locker status for each client request  
- When a user selects a locker, the server checks its current state and returns the result  
</br>
