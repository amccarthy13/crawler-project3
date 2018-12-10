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
    int maxThreads = 1;
    int port = 80;
    string directory = "";
    string startUrl = "";
} Config;

struct CrawlerState {
    int threadsCount;
    queue<string> pendingCookies;
    queue<pair<string, string>> pendingSites;
    map<string, bool> discoveredSites;
};

Config config;
CrawlerState crawlerState;
mutex m_mutex;
condition_variable m_condVar;
bool threadFinished;

int pageCount = 0;

void initialize();

void schedule();

void startCrawler(pair<string, string> baseUrl, CrawlerState &crawlerState, string cookie, int count);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Invalid Parameters" << endl;
        return 0;
    }
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
    if (config.startUrl.empty()) {
        cerr << "No start url provided" << endl;
        return 0;
    }

    if (!config.directory.empty() && endChar != '/') {
        config.directory = config.directory + "/";
    }
    initialize();
    schedule();
    cout << "Crawler Finished" << endl;
    return 0;
}

void initialize() {
    crawlerState.threadsCount = 0;
    ClientSocket clientSocket = ClientSocket(
            make_pair(getHostnameFromURL(config.startUrl), getHostPathFromURL(config.startUrl)), config.port);
    for (int i = 1; i <= config.maxThreads; i++) {
        string cookie = clientSocket.getCookie();
        if (cookie.empty()) {
            cerr << "Unable to connect to start Url" << endl;
            exit(0);
        }
        crawlerState.pendingCookies.push(cookie);
    }
    crawlerState.pendingSites.push(make_pair(getHostnameFromURL(config.startUrl), getHostPathFromURL(config.startUrl)));
    crawlerState.discoveredSites[getHostnameFromURL(config.startUrl) + getHostPathFromURL(config.startUrl)] = true;
}

void schedule() {
    while (crawlerState.threadsCount != 0 || !crawlerState.pendingSites.empty()) {
        m_mutex.lock();
        threadFinished = false;
        while (!crawlerState.pendingSites.empty() && crawlerState.threadsCount < config.maxThreads) {
            string cookie = crawlerState.pendingCookies.front();
            crawlerState.pendingCookies.pop();
            crawlerState.pendingCookies.push(cookie);

            pair<string, string> nextSite = crawlerState.pendingSites.front();
            crawlerState.pendingSites.pop();
            crawlerState.threadsCount++;

            pageCount++;
            thread t = thread(startCrawler, nextSite, ref(crawlerState), cookie, pageCount);
            if (t.joinable()) t.detach();
        }
        m_mutex.unlock();
        unique_lock<mutex> m_lock(m_mutex);
        while (!threadFinished) m_condVar.wait(m_lock);
    }
}

void startCrawler(pair<string, string> baseUrl, CrawlerState &crawlerState, string cookie, int count) {
    ClientSocket clientSocket = ClientSocket(baseUrl, config.port);
    SiteStats stats = clientSocket.startDiscovering(config.directory, cookie, count);

    for (int i = 0; i < stats.discoveredPages.size(); i++) {
        pair<string, string> site = stats.discoveredPages[i];
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