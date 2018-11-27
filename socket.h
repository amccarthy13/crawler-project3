
#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <queue>
#include <vector>
#include <map>

using namespace std;

typedef struct {
    string hostname;
    double averageResponseTime = -1;
    double minResponseTime = -1;
    double maxResponseTime = -1;
    int numberOfPagesFailed = 0;
    vector<string> linkedSites;
    vector< pair<string, double> > discoveredPages;
}SiteStats;

class ClientSocket {
    public:
        ClientSocket(string hostname, int port=80, int pagesLimit=-1, int crawlDelay=1000);
        SiteStats startDiscovering();
    private:
        string hostname;
        int sock, pagesLimit, port, crawlDelay;
        queue<string> pendingPages;
        map<string, bool> discoveredPages;
        map<string, bool> discoveredLinkedSites;
        string startConnection();
        string closeConnection();
        string createHttpRequest(string host, string path);
};

#endif
