#include <utility>

#include "socket.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <iomanip>
#include <condition_variable>

using namespace std;

typedef struct {
    int maxThreads = 10;
    int port = 80;
    string directory = "";
    string startUrl = "http://eychtipi.cs.uchicago.edu/";
} Config;

struct CrawlerState {
    int threadsCount;
    queue<pair<string, string>> pendingSites;
    map<string, bool> discoveredSites;
};

Config config;
CrawlerState crawlerState;
mutex m_mutex;
condition_variable m_condVar;
bool threadFinished;

void initialize();

void schedule();

void startCrawler(pair<string, string> baseUrl, CrawlerState &crawlerState);

int main(int argc, char *argv[]) {
    /*if (argc < 2) {
        cout << "Invalid Parameters" << endl;
        return 0;
    } */
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            config.maxThreads = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-h") == 0) {
            config.startUrl = argv[i + 1];
        }
        if (strcmp(argv[i], "-p") == 0) {
            config.port = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-f") == 0) {
            config.directory = argv[i + 1];
        }
    }
    char endChar = config.directory.back();
    /*
    if (config.startUrl.empty()) {
        cerr << "No start url provided" << endl;
        return 0;
    } */

    if (!config.directory.empty() && endChar != '/') {
        config.directory = config.directory + "/";
    }
    initialize();
    schedule();
    return 0;
}

void initialize() {
    crawlerState.threadsCount = 0;
    crawlerState.pendingSites.push(make_pair(getHostnameFromURL(config.startUrl), getHostPathFromURL(config.startUrl)));
    crawlerState.discoveredSites[getHostnameFromURL(config.startUrl) + getHostPathFromURL(config.startUrl)] = true;
}

void schedule() {
    while (crawlerState.threadsCount != 0 || !crawlerState.pendingSites.empty()) {
        m_mutex.lock();
        threadFinished = false;
        while (!crawlerState.pendingSites.empty() && crawlerState.threadsCount < config.maxThreads) {
            pair<string, string> nextSite = crawlerState.pendingSites.front();
            crawlerState.pendingSites.pop();
            crawlerState.threadsCount++;
            thread t = thread(startCrawler, nextSite, ref(crawlerState));
            if (t.joinable()) t.detach();
        }
        m_mutex.unlock();
        unique_lock<mutex> m_lock(m_mutex);
        while (!threadFinished) m_condVar.wait(m_lock);
    }
}

void startCrawler(pair<string, string> baseUrl, CrawlerState &crawlerState) {
    ClientSocket clientSocket = ClientSocket(baseUrl, config.port);
    SiteStats stats = clientSocket.startDiscovering(config.directory);

    for (int i = 0; i < stats.discoveredPages.size(); i++) {
        pair<string,string> site = stats.discoveredPages[i];
        if (!crawlerState.discoveredSites[site.first + site.second]) {
            crawlerState.pendingSites.push(site);
            crawlerState.discoveredSites[site.first + site.second] = true;
        }
    }
    crawlerState.threadsCount--;
    threadFinished = true;
    m_mutex.unlock();

    m_condVar.notify_one();
}