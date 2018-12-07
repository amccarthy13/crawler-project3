#include <utility>

#include "socket.h"
#include "parser.h"
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <fstream>

using namespace std;
using namespace std::chrono;

const string CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

ClientSocket::ClientSocket(pair<string, string> hostname, int port) {
    this->hostname = hostname;
    this->port = port;
    this->discoveredPages.clear();
    this->discoveredLinkedSites.clear();
}

string ClientSocket::startConnection() {
    struct hostent *host;
    struct sockaddr_in serv_addr;

    host = gethostbyname(hostname.first.c_str());
    if (host == NULL || host->h_addr == NULL) {
        return "Cannot get DNS info";
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return "Cannot create socket";
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *) host->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) == -1) {
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
    string request;
    request += "GET " + path + " HTTP/1.1\r\n";
    request += "HOST:" + host + "\r\n";
    request += "Connection: close\r\n\r\n";
    return request;
}

SiteStats ClientSocket::startDiscovering(string directory) {
    SiteStats stats;
    stats.hostname = hostname.first;

    string path = hostname.second;
    if (!this->startConnection().empty()) {
        stats.numberOfPagesFailed++;
        return stats;
    }

    string send_data = createHttpRequest(stats.hostname, path);
    if (send(sock, send_data.c_str(), strlen(send_data.c_str()), 0) < 0) {
        stats.numberOfPagesFailed++;
        return stats;
    }

    char recv_data[1024];
    int totalBytesRead = 0;
    string httpResponse = "";
    while (true) {
        bzero(recv_data, sizeof(recv_data));
        int bytesRead = recv(sock, recv_data, sizeof(recv_data), 0);

        if (bytesRead > 0) {
            string ss(recv_data);
            httpResponse += ss;
            totalBytesRead += bytesRead;
        } else {
            break;
        }
    }

    vector<string> downloadLinks = extractDownloads(httpResponse);

    vector<pair<string, string>> downloadUrls;
    for (auto url : downloadLinks) {
        string host = getHostnameFromURL(url);
        if (host == ".") {
            downloadUrls.push_back(make_pair(stats.hostname , url.substr(1)));
        } else if (host == url) {
            downloadUrls.push_back(make_pair(stats.hostname , "/" + url));
        } else {
            downloadUrls.push_back(make_pair(getHostnameFromURL(url), getHostPathFromURL(url)));
        }
    }

    vector<pair<string, string>> extractedUrls = extractUrls(httpResponse);
    for (auto url : extractedUrls) {
        if (url.first.empty() || url.first == stats.hostname) {
            if (!discoveredPages[stats.hostname + url.second]) {
                discoveredPages[stats.hostname + url.second] = true;
                stats.discoveredPages.emplace_back(stats.hostname, url.second);
            }
        } else {
            if (!discoveredLinkedSites[url.first + url.second]) {
                discoveredLinkedSites[url.first + url.second] = true;
                stats.linkedSites.push_back(url);
            }
        }
    }

    for (auto link : downloadUrls) {
        string send_data = createHttpRequest(link.first, link.second);
        send(sock, send_data.c_str(), strlen(send_data.c_str()), 0);
        ofstream file(directory + link.first + link.second);
        char recv_data[1024];
        int totalBytesRead = 0;
        while (true) {
            bzero(recv_data, sizeof(recv_data));
            int bytesRead = recv(sock, recv_data, sizeof(recv_data), 0);

            if (bytesRead > 0) {
                file << recv_data;
                totalBytesRead += bytesRead;
            } else {
                break;
            }
        }
    }


    this->closeConnection();

    return stats;
}