#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
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

const int BUFFER_SIZE = 4096;

std::atomic<bool> shuttingDown(false);
std::thread *supervisorThread;

std::deque<std::thread*> runningWorkers;
std::mutex runningWorkersMutex;

std::vector<int> tcpSocketFds;
std::mutex tcpSocketFdsMutex;

std::string sockaddr2string(const sockaddr_in *client_addr) {
  char buffer[INET_ADDRSTRLEN];
  const char *hostaddrp = inet_ntop(client_addr->sin_family, &(client_addr->sin_addr), buffer, INET_ADDRSTRLEN);

  if (hostaddrp == nullptr) {
    std::cerr << "Failed on inet_ntoa.\n";
  }

  return std::string(hostaddrp);
}

int startTCPBroadcastServer(int listenPort) {
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

      while (true) {
        char buffer[BUFFER_SIZE];
        int bytes = recv(client_socket, &buffer, BUFFER_SIZE, 0);

        if (bytes == 0) {
          break;
        } else if (bytes < 0) {
          std::cerr << "[TCP] Something went wrong while receiving data!.\n";
          break;
        } else {
          std::cout << "[TCP] Received " << bytes << " bytes from " << clientName << "\n";

          std::vector<int> socketFdsSnapshot;

          tcpSocketFdsMutex.lock();
          socketFdsSnapshot = tcpSocketFds;
          tcpSocketFdsMutex.unlock();

          for (int socketFd : socketFdsSnapshot) {
            if (send(socketFd, &buffer, bytes, 0) < 0) {
              std::cerr << "[TCP] Fail to send to" << socketFd <<"\n";
            }
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

int startUDPEchoServer(int listenPort) {
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(listenPort);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  int server_socket;
  if ((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    std::cerr << "[UDP] Fail to create socket\n";
    return -1;
  }

  if ((bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address))) < 0) {
    std::cerr << "[UDP] Fail to bind socket\n";
    return -1;
  }

  std::cout << "[UDP] Socket is listening on " << listenPort << "\n";

  while (true) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    int bytes = recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &client_address_len);

    std::cout << "[UDP] Received " << bytes << " bytes from " << sockaddr2string(&client_address) << "\n";

    sendto(server_socket, buffer, bytes, 0, (struct sockaddr *)&client_address, client_address_len);
  }

  close(server_socket);

  return 0;
}

int startHTTPServer(int listenPort) {
  int server_socket;
  int client_socket;

  struct sockaddr_in server_address;
  struct sockaddr_in client_address;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    std::cerr << "[HTTP] Opening socket failed.\n";
    return 1;
  }

  int optval = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  bzero((char *) &server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(listenPort);
  if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
    std::cerr << "[HTTP] Bind socket failed\n";
    return 1;
  }

  if (listen(server_socket, SOMAXCONN) < 0) {
    std::cerr << "[HTTP] Listen socket failed\n";
    return 1;
  }

  std::cout << "[HTTP] Socket is listening on " << listenPort << "\n";

  socklen_t clientlen = sizeof(client_address);

  while (true) {
      client_socket = accept(server_socket, (struct sockaddr*)&client_address, &clientlen);

      if (client_socket < 0) {
        std::cerr << "[HTTP] Accept client socket failed.\n";
        continue;
      }

      runningWorkersMutex.lock();

      runningWorkers.push_back(new std::thread([client_address, client_socket]() {
        const char* helloResponse = "HTTP/1.1 200 OK\r\n\r\nHello\n";
        const char* notFoundResponse = "HTTP/1.1 404 Not Found\r\n\r\nNot Found\n";

        std::cout << "[HTTP] Received request from " << sockaddr2string(&client_address) << "\n";

        char buffer[BUFFER_SIZE];
        int bytes = recv(client_socket, &buffer, sizeof(buffer), 0);
        int sentBytes;

        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 1000));

        if (!strncmp(buffer, "GET / ", 6)) {
          sentBytes = send(client_socket, helloResponse, strlen(helloResponse), 0);
        } else {
          sentBytes = send(client_socket, notFoundResponse, strlen(notFoundResponse), 0);
        }

        if (sentBytes < 0) {
          std::cerr << "[HTTP] Send response failed\n";
        }

        close(client_socket);
      }));

      runningWorkersMutex.unlock();
  }

  close(server_socket);

  return 0;
}

void supervisor() {
  while (true) {
    std::thread *worker = nullptr;

    runningWorkersMutex.lock();

    if (runningWorkers.size() > 0) {
      worker = runningWorkers.front();
      runningWorkers.pop_front();
    }

    if (!worker && shuttingDown) {
      break;
    }

    runningWorkersMutex.unlock();

    if (worker) {
      worker->join();
      delete worker;
    } else {
      sleep(1);
    }
  }
}

void signalHandler(int signalNumber) {
  std::cerr << "[SIGNAL] Received " << signalNumber << "\n";

  shuttingDown = true;

  runningWorkersMutex.lock();
  std::cerr << "[SIGNAL] " << runningWorkers.size() + 1 << " workers are running, waiting for them finish ...\n";
  runningWorkersMutex.unlock();

  if (supervisorThread->joinable()) {
    supervisorThread->join();
  }

  exit(0);
}

int main(int argc, char **argv) {
  srand(time(nullptr));

  std::thread threadTcp(startTCPBroadcastServer, 4000);
  std::thread threadUdp(startUDPEchoServer, 4000);
  std::thread threadHTTP(startHTTPServer, 3000);

  supervisorThread = new std::thread(supervisor);

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);

  threadTcp.join();
  threadUdp.join();
  threadHTTP.join();

  return 0;
}
