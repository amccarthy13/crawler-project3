
#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <queue>
#include <vector>
#include <map>

using namespace std;

typedef struct {
    string hostname;
    vector<pair <string, string> > linkedSites;
    vector<pair <string, string> > discoveredPages;
    vector<pair <string, string> > discoveredDownloads;
} SiteStats;

class ClientSocket {
    pair<string,string> hostname;
    int sock, port;
    map<string, bool> discoveredPages;
    map<string, bool> discoveredLinkedSites;

    string startConnection();

    string closeConnection();

    string createHttpRequest(string host, string path);

    string createHttpRequestWithCookie(string host, string path, string cookie);

public:
    explicit ClientSocket(pair<string, string> hostname, int port = 80);

    SiteStats startDiscovering(string directory, int count);
    bool startDownload(string directory, string cookie);

    string getCookie();
};

#endif
