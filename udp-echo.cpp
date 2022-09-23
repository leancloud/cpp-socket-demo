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
#include <type_traits>
#include <unistd.h>

#include "demo.h"

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
