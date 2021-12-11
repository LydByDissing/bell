#ifndef BELL_HTTP_SERVER_H
#define BELL_HTTP_SERVER_H

#include <functional>
#include <map>
#include <optional>
#include <memory>
#include <regex>
#include <optional>
#include <set>
#include <iostream>
#include <queue>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <BellLogger.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <sys/socket.h>
#include <string>
#include <netdb.h>
#include <mutex>
#include <fcntl.h>
#include "BaseHTTPServer.h"

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK O_NONBLOCK
#endif

namespace bell
{
    class HTTPServer : public bell::BaseHTTPServer
    {
    private:
        std::regex routerPattern = std::regex(":([^\\/]+)?");
        fd_set master;
        fd_set readFds;
        fd_set activeFdSet, readFdSet;

        bool isClosed = true;
        bool writingResponse = false;

        std::map<std::string, std::vector<HTTPRoute>> routes;
        std::map<int, HTTPConnection> connections;
        void writeResponse(const HTTPResponse &);
        void writeResponseEvents(int connFd);
        void findAndHandleRoute(std::string &, std::string &, int connectionFd);

        std::vector<std::string> splitUrl(const std::string &url, char delimiter);
        std::mutex responseMutex;
        std::vector<char> responseBuffer = std::vector<char>(128);

        void readFromClient(int clientFd);
        std::map<std::string, std::string> parseQueryString(const std::string &queryString);
        unsigned char h2int(char c);
        std::string urlDecode(std::string str);

    public:
        HTTPServer(int serverPort);

        int serverPort;
        void registerHandler(RequestType requestType, const std::string &, httpHandler);
        void respond(const HTTPResponse &);
        void publishEvent(std::string eventName, std::string eventData);
        void closeConnection(int connection);
        void listen();
    };
}
#endif
