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
    int crawlDelay = 1000;
    int maxThreads = 10;
    int depthLimit = 10;
    int pagesLimit = 10;
    int linkedSitesLimit = 20;
    int port = 80;
    string directory = "";
    string startUrl = "sandlab.cs.uchicago.edu";
} Config;

struct CrawlerState {
    int threadsCount;
    queue<pair<string, int> > pendingSites;
    map<string, bool> discoveredSites;
};

Config config;
CrawlerState crawlerState;
mutex m_mutex;
condition_variable m_condVar;
bool threadFinished;

void initialize();

void schedule();

void startCrawler(string baseUrl, int currentDepth, CrawlerState &crawlerState);

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
    }/*
    if (config.startUrl == "") {
        cerr << "No start url provided" << endl;
        return 0;
    }

    if (config.directory == "") {
        cerr << "No download directory provided" << endl;
        return 0;
    } */
    initialize();
    schedule();
    return 0;
}

void initialize() {
    crawlerState.threadsCount = 0;
    crawlerState.pendingSites.push(make_pair(getHostnameFromURL(config.startUrl), 0));
    crawlerState.discoveredSites[getHostnameFromURL(config.startUrl)] = true;
}

void schedule() {
    while (crawlerState.threadsCount != 0 || !crawlerState.pendingSites.empty()) {
        m_mutex.lock();
        threadFinished = false;
        while (!crawlerState.pendingSites.empty() && crawlerState.threadsCount < config.maxThreads) {
            auto nextSite = crawlerState.pendingSites.front();
            crawlerState.pendingSites.pop();
            crawlerState.threadsCount++;

            thread t = thread(startCrawler, nextSite.first, nextSite.second, ref(crawlerState));
            if (t.joinable()) t.detach();
        }
        m_mutex.unlock();

        unique_lock<mutex> m_lock(m_mutex);
        while (!threadFinished) m_condVar.wait(m_lock);
    }
}

void startCrawler(string hostname, int currentDepth, CrawlerState &crawlerState) {
    ClientSocket clientSocket = ClientSocket(hostname, config.port, config.pagesLimit, config.crawlDelay);
    SiteStats stats = clientSocket.startDiscovering();

    m_mutex.lock();
    cout << "Website: " << stats.hostname << endl;
    cout << "Depth: " << currentDepth << endl;
    cout << "Pages Discovered: " << stats.discoveredPages.size() << endl;
    cout << "Pages Failed to Discover: " << stats.numberOfPagesFailed << endl;
    cout << "Number of Linked Sites: " << stats.linkedSites.size() << endl;
    if (stats.minResponseTime < 0) cout << "Min Response Time: N.A." << endl;
    else cout << "Min. Response Time: " << stats.minResponseTime << "ms" << endl;
    if (stats.maxResponseTime < 0) cout << "Max. Response Time: N.A" << endl;
    else cout << "Max. Response Time: " << stats.maxResponseTime << "ms" << endl;
    if (stats.averageResponseTime < 0) cout << "Average Response Time: N.A" << endl;
    else cout << "Average Response Time: " << stats.averageResponseTime << "ms" << endl;
    if (!stats.discoveredPages.empty()) {
        cout << "List of visited pages:" << endl;
        cout << "    " << setw(15) << "Response Time" << "    " << "URL" << endl;
        for (auto page : stats.discoveredPages) {
            cout << "    " << setw(13) << page.second << "ms" << "    " << page.first << endl;
        }
    }

    if (currentDepth < config.depthLimit) {
        for (int i = 0; i < min(int(stats.linkedSites.size()), config.linkedSitesLimit); i++) {
            string site = stats.linkedSites[i];
            if (!crawlerState.discoveredSites[site]) {
                crawlerState.pendingSites.push(make_pair(site, currentDepth + 1));
                crawlerState.discoveredSites[site] = true;
            }
        }
    }
    crawlerState.threadsCount--;
    threadFinished = true;
    m_mutex.unlock();

    m_condVar.notify_one();
}