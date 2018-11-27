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
    int linkedSitesLimit = 10;
    vector<string> startUrls;
}Config;

struct CrawlerState {
    int threadsCount;
    queue< pair<string, int> > pendingSites;
    map<string, bool> discoveredSites;
};

Config config;
CrawlerState crawlerState;
mutex m_mutex;
condition_variable m_condVar;
bool threadFinished;

void initialize();
void scheduleCrawlers();
void startCrawler(string baseUrl, int currentDepth, CrawlerState &crawlerState);

int main(int argc, const char * argv[]) {
    initialize();
    scheduleCrawlers();
    return 0;
}

void initialize() {
    crawlerState.threadsCount = 0;
    for (auto url: config.startUrls) {
        crawlerState.pendingSites.push(make_pair(getHostnameFromURL(url), 0));
        crawlerState.discoveredSites[getHostnameFromURL(url)] = true;
    }
}

void scheduleCrawlers() {
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
        while(!threadFinished) m_condVar.wait(m_lock);
    }
}

void startCrawler(string hostname, int currentDepth, CrawlerState &crawlerState) {

}