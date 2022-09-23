#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
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

#include "demo.h"

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
