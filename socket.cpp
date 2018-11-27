#include "socket.h"
#include "parser.h"
#include <netdb.h>
#include <unistd.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

ClientSocket::ClientSocket(string hostname, int port, int pagesLimit, int crawlDelay) {
    this->pagesLimit = pagesLimit;
    this->hostname = hostname;
    this->port = port;
    this->pendingPages.push("/");
    this->discoveredPages["/"] = true;
    this->crawlDelay = crawlDelay;
    this->discoveredLinkedSites.clear();
}

string ClientSocket::startConnection() {
    struct hostent *host;
    struct sockaddr_in serv_addr;

    host = gethostbyname(hostname.c_str());
    if (host == NULL || host->h_addr == NULL) {
        return "Cannot get DNS info";
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return "Cannot create socket";
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
        return "Cannot connect to server";
    }

    return "";
}

string ClientSocket::closeConnection() {
    try {
        close(sock);
    } catch (exception &error) {
        return error.what();
    }
    return "";
}

string ClientSocket::createHttpRequest(string host, string path) {
    string request = "";
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "HOST:" + host + "\r\n";
    request += "Connection: close\r\n\r\n";
    return request;
}

SiteStats ClientSocket::startDiscovering() {
    SiteStats stats;
    stats.hostname = hostname;

    while(!pendingPages.empty() && (pagesLimit == -1 || int(stats.discoveredPages.size()) < pagesLimit)) {
        string path = pendingPages.front();
        pendingPages.pop();

        if (path != "/") usleep(crawlDelay);

        high_resolution_clock::time_point startTime = high_resolution_clock::now();

        if (this->startConnection() != "") {
            stats.numberOfPagesFailed++;
            continue;
        }

        string send_data = createHttpRequest(hostname, path);
        if (send(sock, send_data.c_str(), strlen(send_data.c_str()), 0) < 0) {
            stats.numberOfPagesFailed++;
            continue;
        }

        char recv_data[1024];
        int totalBytesRead = 0;
        string httpResponse = "";
        double responseTime = -1;
        while (true) {
            bzero(recv_data, sizeof(recv_data));
            int bytesRead = recv(sock, recv_data, sizeof(recv_data), 0);

            if (responseTime < -0.5) {
                high_resolution_clock::time_point endTime = high_resolution_clock::now();
                responseTime = duration<double, milli>(endTime - startTime).count();
            }
            if (bytesRead > 0) {
                string ss(recv_data);
                httpResponse += ss;
                totalBytesRead += bytesRead;
            } else {
                break;
            }
        }

        this->closeConnection();

        stats.discoveredPages.push_back(make_pair(hostname+path, responseTime));

        vector< pair<string, string> > extractedUrls = extractUrls(httpResponse);
        for (auto url : extractedUrls) {
            if (url.first == "" || url.first == hostname) {
                if (!discoveredPages[url.second]) {
                    pendingPages.push(url.second);
                    discoveredPages[url.second] = true;
                }
            } else {
                if (!discoveredLinkedSites[url.first]) {
                    discoveredLinkedSites[url.first] = true;
                    stats.linkedSites.push_back(url.first);
                }
            }
        }
    }

    double totalResponseTime = 0;
    for (auto page : stats.discoveredPages) {
        totalResponseTime += page.second;
        stats.minResponseTime = stats.minResponseTime < 0 ? page.second : min(stats.minResponseTime, page.second);
        stats.maxResponseTime = stats.maxResponseTime < 0 ? page.second : max(stats.maxResponseTime, page.second);
    }
    if (!stats.discoveredPages.empty())
        stats.averageResponseTime = totalResponseTime / stats.discoveredPages.size();

    return stats;
}
