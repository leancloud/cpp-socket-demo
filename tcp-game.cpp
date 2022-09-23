#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "demo.h"

std::vector<int> tcpSocketFds;
std::mutex tcpSocketFdsMutex;

std::atomic<int> lastNumber(1);

const std::string rule("you must send the next number but skip numbers which multiples of 3 or includes the digit 3.");

bool numberContainsDigit(int number, int digit) {
  while (number != 0) {
    if (number % 10 == digit) {
      return true;
    }

    number /= 10;
  }

  return false;
}

int nextSafeNumber(int from) {
  while (int current = ++from) {
    if (current % 3 != 0 && !numberContainsDigit(current, 3)) {
      return current;
    }
  }
}

int startTCPGameServer(int listenPort) {
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket < 0) {
    std::cerr << "[TCP] Socket cannot be created!\n";
    return -2;
  }

  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(listenPort);

  inet_pton(AF_INET, "0.0.0.0", &server_address.sin_addr);

  if (bind(server_socket, (sockaddr *)&server_address, sizeof(server_address)) < 0) {
    std::cerr << "[TCP] Could not bind socket\n";
    return -3;
  }

  if (listen(server_socket, SOMAXCONN) < 0) {
    std::cerr << "[TCP] Socket cannot be switched to listen mode!\n";
    return -4;
  }

  std::cout << "[TCP] Socket is listening on " << listenPort << "\n";

  while (true) {
    sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int client_socket;
    if ((client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_size)) < 0) {
      std::cerr << "[TCP] Connections cannot be accepted.\n";
      return -5;
    }

    std::cout << "[TCP] A connection is accepted now.\n";

    runningWorkersMutex.lock();

    runningWorkers.push_back(new std::thread([client_addr, client_addr_size, client_socket](){
      std::string clientName = sockaddr2string(&client_addr);

      tcpSocketFdsMutex.lock();
      tcpSocketFds.push_back(client_socket);
      tcpSocketFdsMutex.unlock();

      std::cout << "[TCP] " << clientName << " connected...\n";

      std::ostringstream broadcast;
      broadcast << ">> " << tcpSocketFds.size() << " players online, last number is " << lastNumber << ", " << rule << "\n";
      std::string message = broadcast.str();

      if (send(client_socket, message.c_str(), message.size(), 0) < 0) {
        std::cerr << "[TCP] Fail to send to" << client_socket <<"\n";
      }

      while (true) {
        char buffer[BUFFER_SIZE];
        int bytes = recv(client_socket, &buffer, BUFFER_SIZE, 0);

        if (bytes == 0) {
          break;
        } else if (bytes < 0) {
          std::cerr << "[TCP] Something went wrong while receiving data!.\n";
          break;
        } else {
          int number = atoi(buffer);

          std::cout << "[TCP] Received " << number << " from " << clientName << "\n";

          bool needDisconnect = false;
          std::ostringstream broadcast;

          if (number == nextSafeNumber(lastNumber)) {
            lastNumber = number;
            broadcast << ">> " << tcpSocketFds.size() << " players online, " << clientName << " sent " << lastNumber << ".\n";
          } else {
            needDisconnect = true;
            std::string message(">> you are lost\n");

            if (send(client_socket, message.c_str(), message.size(), 0) < 0) {
              std::cerr << "[TCP] Fail to send to" << client_socket <<"\n";
            }

            broadcast << ">> " << clientName << " lost, " << tcpSocketFds.size() - 1 << " players online.\n";
          }

          std::vector<int> socketFdsSnapshot;

          tcpSocketFdsMutex.lock();
          socketFdsSnapshot = tcpSocketFds;
          tcpSocketFdsMutex.unlock();

          for (int socketFd : socketFdsSnapshot) {
            std::string message = broadcast.str();

            if (send(socketFd, message.c_str(), message.size(), 0) < 0) {
              std::cerr << "[TCP] Fail to send to" << socketFd <<"\n";
            }
          }

          if (needDisconnect) {
            break;
          }
        }
      }

      tcpSocketFdsMutex.lock();
      tcpSocketFds.erase(std::remove(tcpSocketFds.begin(), tcpSocketFds.end(), client_socket), tcpSocketFds.end());
      tcpSocketFdsMutex.unlock();

      close(client_socket);
      std::cout << "[TCP] " << clientName << " disconnected.\n";
    }));

    runningWorkersMutex.unlock();
  }

  close(server_socket);

  return 0;
}
