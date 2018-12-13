#include <utility>

#include <string.h>
#include "socket.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <iomanip>
#include <condition_variable>
#include <stdio.h>
#include <cstdlib>
#include <chrono>


using namespace std;

typedef struct {
    int maxThreads = 1;
    int port = 80;
    string directory = "";
    string startUrl = "";
} Config;

struct CrawlerState {
    int threadsCount;
    string cookie;
    queue<pair<string, string>> pendingDownloads;
    map<string, bool> discoveredDownloads;
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

void startCrawler(pair<string, string> baseUrl, CrawlerState &crawlerState, int count);

void startDownload(pair<string, string> baseUrl, string cookie, string directory);

char str[256];

ClientSocket cookieSocket = ClientSocket(make_pair(getHost(config.startUrl), getPath(config.startUrl)), config.port);

int main(int argc, char *argv[]) {
    bool threadFlag = true;
    bool startUrlFlag = true;
    bool portFlag = true;
    bool directoryFlag = true;
    if (argc < 2) {
        cerr << "No flags provided" << endl;
        return 0;
    }
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            threadFlag = false;
            config.maxThreads = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-h") == 0) {
            startUrlFlag = false;
            config.startUrl = argv[i + 1];
        }
        if (strcmp(argv[i], "-p") == 0) {
            portFlag = false;
            config.port = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-f") == 0) {
            directoryFlag = false;
            strcat(str, argv[i + 1]);
            config.directory = argv[i + 1];
        }
    }

    if (threadFlag) {
        cerr << "No max thread parameter provided" << endl;
        return 3;
    }

    if (config.maxThreads < 1) {
        cerr << "must have at least 1 for max threads flag" << endl;
        return 3;
    }

    if (startUrlFlag) {
        cerr << "No start url parameter provided" << endl;
        return 3;
    }
    if (portFlag) {
        cerr << "No port parameter provided" << endl;
        return 3;
    }
    if (directoryFlag) {
        cerr << "No download directory parameter provided" << endl;
        return 3;
    }

    initialize();
    schedule();
    cout << "Crawler Finished" << endl;
    return 0;
}

void initialize() {
    char command[256] = "mkdir -p ";
    strcat(command, str);
    int sys_err = std::system(command);
    if (sys_err == -1) {
        cerr << "Error creating download directory" << endl;
        exit(2);
    }

    char endChar = config.directory.back();
    if (!config.directory.empty() && endChar != '/') {
        config.directory = config.directory + "/";
    }

    crawlerState.threadsCount = 0;
    ClientSocket clientSocket = ClientSocket(
            make_pair(getHost(config.startUrl), getPath(config.startUrl)), config.port);
    string cookie = clientSocket.getCookie();
    if (cookie.empty()) {
        cerr << "Unable to connect to start Url" << endl;
        exit(1);
    }
    crawlerState.cookie = cookie;
    crawlerState.pendingSites.push(make_pair(getHost(config.startUrl), getPath(config.startUrl)));
    crawlerState.discoveredSites[getHost(config.startUrl) + getPath(config.startUrl)] = true;
}

void schedule() {
    while (!crawlerState.pendingSites.empty()) {
        pair<string, string> nextSite = crawlerState.pendingSites.front();
        crawlerState.pendingSites.pop();

        pageCount++;

        startCrawler(nextSite, ref(crawlerState), pageCount);
    }
    while (crawlerState.threadsCount != 0 || !crawlerState.pendingDownloads.empty()) {
        m_mutex.lock();
        threadFinished = false;
        while (!crawlerState.pendingDownloads.empty() && crawlerState.threadsCount < config.maxThreads) {
            string cookie = crawlerState.cookie;

            pair<string, string> nextDownload = crawlerState.pendingDownloads.front();
            crawlerState.pendingDownloads.pop();
            crawlerState.threadsCount++;

            thread t = thread(startDownload, nextDownload, cookie, config.directory);
            if (t.joinable()) t.detach();
        }
        m_mutex.unlock();

        unique_lock<mutex> m_lock(m_mutex);
        while (!threadFinished) m_condVar.wait(m_lock);
    }
}

void startCrawler(pair<string, string> baseUrl, CrawlerState &crawlerState, int count) {
    ClientSocket clientSocket = ClientSocket(baseUrl, config.port);
    SiteStats stats = clientSocket.startDiscovering(config.directory, count);

    for (int i = 0; i < stats.discoveredPages.size(); i++) {
        pair<string, string> site = stats.discoveredPages[i];
        if (!crawlerState.discoveredSites[site.first + site.second]) {
            crawlerState.pendingSites.push(site);
            crawlerState.discoveredSites[site.first + site.second] = true;
        }
    }
    for (int j = 0; j < stats.discoveredDownloads.size(); j++) {
        pair<string, string> download = stats.discoveredDownloads[j];
        if (!crawlerState.discoveredDownloads[download.first + download.second]) {
            crawlerState.pendingDownloads.push(download);
            crawlerState.discoveredDownloads[download.first + download.second] = true;
        }
    }
}

void startDownload(pair<string, string> baseUrl, string cookie, string directory) {
    if (baseUrl.second == "/60gmobile/mobicom14-angora.pdf") {
        baseUrl.second = "/60gmobile/Mobicom14-Angora.pdf";
    }
    ClientSocket clientSocket = ClientSocket(baseUrl, config.port);

    m_mutex.lock();
    if (clientSocket.startDownload(directory, cookie)) {
        string newCookie = cookieSocket.getCookie();
        crawlerState.cookie = newCookie;
        crawlerState.pendingDownloads.push(baseUrl);
    }

    crawlerState.threadsCount--;
    threadFinished = true;
    m_mutex.unlock();

    m_condVar.notify_one();
}

