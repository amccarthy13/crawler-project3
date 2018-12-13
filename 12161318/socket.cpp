#include <utility>
#include "socket.h"
#include "parser.h"
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <algorithm>
#include <string.h>

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
    request += "GET " + path + " HTTP/1.0\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n\r\n";
    return request;
}

string ClientSocket::createHttpRequestWithCookie(string host, string path, string cookie) {
    string request;
    request += "GET " + path + " HTTP/1.0\r\n";
    request += "Host: " + host + "\r\n";
    request += "Cookie: " + cookie + ";" + "\r\n";
    request += "Connection: close\r\n\r\n";
    return request;
}

string ClientSocket::getCookie() {
    if (!this->startConnection().empty()) {
        return "";
    }

    string send_data = createHttpRequest(hostname.first, hostname.second);
    if (send(sock, send_data.c_str(), strlen(send_data.c_str()), 0) < 0) {
        return "";
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


    string cookie = extractCookie(httpResponse);

    return cookie;

}

char *removeHTTPHeader(char *buffer, int &bodySize) {
    char *t = strstr(buffer, "\r\n\r\n");
    t = t + 4;

    for (auto it = buffer; it != t; ++it) {
        ++bodySize;
    }

    return t;
}

int getPicture(const int &socketfd, const int &bSize, string pictureName) {
    char buffer[bSize];
    ssize_t bReceived;

    bReceived = recv(socketfd, buffer, bSize, 0);
    int bodySize = 0;

    int returnCode = getCode(buffer);

    if (returnCode == 2) {
        return 0;
    }
    if (returnCode == 1) {
        return 1;
    }

    std::ofstream file(pictureName, std::ofstream::binary | std::ofstream::out);

    char *t = removeHTTPHeader(buffer, bodySize);
    bodySize = bReceived - bodySize;

    file.write(t, bodySize);
    memset(buffer, 0, bSize);

    while ((bReceived = recv(socketfd, buffer, bSize, 0)) > 0) {
        file.write(buffer, bReceived);
        memset(buffer, 0, bSize);
    }

    file.close();
    return 0;
}

string getFileName(string path) {
    int pos = path.find_last_of("/");
    string output = path.substr(pos + 1, path.length() - pos);
    return output;
}

SiteStats ClientSocket::startDiscovering(string directory, int count) {
    SiteStats stats;
    stats.hostname = hostname.first;

    string newPath = "";

    int pos = hostname.second.find_last_of("/");
    if (pos != 0 && pos != string::npos) {
        newPath = hostname.second.substr(0, pos);
    }

    string path = hostname.second;
    if (!this->startConnection().empty()) {
        cerr << "Connection for " + hostname.first + hostname.second + " failed\n" << endl;
        return stats;
    }

    string send_data = createHttpRequest(stats.hostname, path);
    if (send(sock, send_data.c_str(), strlen(send_data.c_str()), 0) < 0) {
        cerr << "Connection for " + hostname.first + hostname.second + " failed\n" << endl;
        return stats;
    }

    char recv_data[1024];
    ssize_t bReceived;
    string httpResponse = "";
    string fileName = getFileName(path);
    if (fileName.empty() || fileName == "/" || fileName == "index.html") {
        fileName = "index-" + to_string(count) + ".html";
    }
    std::ofstream file(directory + fileName, std::ofstream::binary | std::ofstream::out);

    bReceived = recv(sock, recv_data, sizeof(recv_data), 0);
    int bodySize = 0;

    char *t = removeHTTPHeader(recv_data, bodySize);
    bodySize = bReceived - bodySize;

    file.write(t, bodySize);
    string ss(recv_data);
    httpResponse += ss;
    memset(recv_data, 0, sizeof(recv_data));

    while ((bReceived = recv(sock, recv_data, sizeof(recv_data), 0)) > 0) {
        file.write(recv_data, bReceived);
        string ss(recv_data);
        httpResponse += ss;
        memset(recv_data, 0, sizeof(recv_data));
    }


    file.close();

    vector<string> downloadLinks = extractDownloads(httpResponse);

    vector<pair<string, string>> downloadUrls;
    map<string, bool> discoveredDownloads;

    for (auto url : downloadLinks) {
        string host = getHost(url);
        if (host == ".") {
            if (!discoveredDownloads[url]) {
                downloadUrls.emplace_back(stats.hostname, newPath + url.substr(1));
                discoveredDownloads[url] = true;
            }
        } else if (!verifyDomain(host)) {
            if (!discoveredDownloads[url]) {
                downloadUrls.emplace_back(stats.hostname, newPath + "/" + url);
                discoveredDownloads[url] = true;
            }
        }
    }

    this->closeConnection();


    vector<pair<string, string>> extractedUrls = extractUrls(httpResponse);


    for (auto url : extractedUrls) {
        if (url.first.empty() || url.first == stats.hostname) {
            if (!discoveredPages[stats.hostname + newPath + url.second]) {
                discoveredPages[stats.hostname + newPath + url.second] = true;
                stats.discoveredPages.emplace_back(stats.hostname, newPath + url.second);
            }
        } else {
            if (!discoveredLinkedSites[url.first + url.second]) {
                discoveredLinkedSites[url.first + url.second] = true;
                stats.linkedSites.push_back(url);
            }
        }
    }

    stats.discoveredDownloads = downloadUrls;

    return stats;
}

bool ClientSocket::startDownload(string directory, string cookie) {
    this->startConnection();

    string download_send_data = createHttpRequestWithCookie(hostname.first, hostname.second, cookie);
    if (send(sock, download_send_data.c_str(), strlen(download_send_data.c_str()), 0) >= 0) {
        int code = getPicture(sock, 1024, directory + getFileName(hostname.second));
        if (code == 1) {
            return true;
        }
    } else {
        while (true) {
            std::this_thread::sleep_for(chrono::milliseconds(500));
            this->closeConnection();
            this->startConnection();
            if (send(sock, download_send_data.c_str(), strlen(download_send_data.c_str()), 0) >= 0) {
                int code = getPicture(sock, 1024, directory + getFileName(hostname.second));
                if (code == 1) {
                    return true;
                }
                break;
            }
        }
    }
    this->closeConnection();
    return false;
}