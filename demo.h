#ifndef DEMO_H
#define DEMO_H

#include <mutex>
#include <deque>
#include <thread>

const int BUFFER_SIZE = 4096;

extern std::deque<std::thread*> runningWorkers;
extern std::mutex runningWorkersMutex;

extern int startTCPGameServer(int listenPort);
extern int startUDPEchoServer(int listenPort);
extern int startHTTPServer(int listenPort);

extern std::string sockaddr2string(const sockaddr_in *client_addr);

#endif
