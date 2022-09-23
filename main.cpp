#include <arpa/inet.h>
#include <atomic>
#include <cstdlib>
#include <iostream>
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

#include "demo.h"

std::atomic<bool> shuttingDown(false);
std::thread *supervisorThread;

std::deque<std::thread*> runningWorkers;
std::mutex runningWorkersMutex;

std::string sockaddr2string(const sockaddr_in *client_addr) {
  char buffer[INET_ADDRSTRLEN];
  const char *hostaddrp = inet_ntop(client_addr->sin_family, &(client_addr->sin_addr), buffer, INET_ADDRSTRLEN);

  if (hostaddrp == nullptr) {
    std::cerr << "Failed on inet_ntoa.\n";
  }

  return std::string(hostaddrp).append(":").append(std::to_string(client_addr->sin_port));
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

  std::thread threadTcp(startTCPGameServer, 4000);
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
