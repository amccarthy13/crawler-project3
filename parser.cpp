#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

string getHostnameFromURL(string url) {
    int offset = 0;
    offset = offset == 0 && url.compare(0, 8, "https://") == 0 ? 8 : offset;
    offset = offset == 0 && url.compare(0, 7, "http://") == 0 ? 7 : offset;

    size_t pos = url.find("/", offset);
    string domain = url.substr(offset, (pos == string::npos ? url.length() : pos) - offset);
    return domain;
}

string getHostPathFromURL(string url) {
    int offset = 0;
    offset = offset == 0 && url.compare(0, 8, "https://") == 0 ? 8 : offset;
    offset = offset == 0 && url.compare(0, 7, "http://") == 0 ? 7 : offset;

    size_t pos = url.find("/", offset);
    string path = pos == string::npos ? "/" : url.substr(pos);

    pos = path.find_first_not_of('/');
    if (pos == string::npos) path = "/";
    else path.erase(0, pos - 1);
    return path;
}

vector<string> extractImages(string httpText) {
    string httpRaw = reformatHttpResponse(httpText);
    const string imgStart = "img src=\"";

    const string urlEndChars = "\"#?, ";

    vector<string> extractedUrls;


    while (true) {

        int startPos = httpRaw.find(imgStart);

        if (startPos == string::npos) break;

        startPos += imgStart.length();

        int endPos = httpRaw.find_first_of(urlEndChars, startPos);

        string url = httpRaw.substr(startPos, endPos - startPos);

        extractedUrls.push_back(url);

        httpRaw.erase(0, endPos);
    }
    return extractedUrls;
}

string extractCookie(string httpText) {

}

vector<pair<string, string> > extractUrls(string httpText) {
    string httpRaw = reformatHttpResponse(httpText);

    const string urlStart[] = {"href=\"", "href = \"", "http://", "https://"};

    const string urlEndChars = "\"#?, ";

    vector<pair<string, string> > extractedUrls;

    for (auto startText : urlStart) {
        while (true) {
            int startPos = httpRaw.find(startText);
            if (startPos == string::npos) break;
            startPos += startText.length();

            int endPos = httpRaw.find_first_of(urlEndChars, startPos);

            string url = httpRaw.substr(startPos, endPos - startPos);

            string host = getHostnameFromURL(url);
            if (host == "." || host == url) {

            }
            if (verifyUrl(url)) {
                string urlDomain = getHostnameFromURL(url);
                extractedUrls.push_back(make_pair(urlDomain, getHostPathFromURL(url)));
            }

            httpRaw.erase(0, endPos);
        }
    }
    return extractedUrls;
}

bool verifyUrl(string url) {
    string urlDomain = getHostnameFromURL(url);

    if (urlDomain != "" && !verifyDomain(urlDomain)) return false;

    if (!verifyType(url)) return false;

    if (url.find("mailto:") != string::npos) return false;

    return true;
}

bool verifyType(string url) {
    string forbiddenTypes[] = {".css", ".js", ".pdf", ".png", ".jpeg", ".jpg", ".ico"};
    for (auto type : forbiddenTypes)
        if (url.find(type) != string::npos) return false;
    return true;
}

bool verifyDomain(string url) {
    string allowedDomains[] = {".com", ".sg", ".net", ".co", ".org", ".me", ".edu"};
    bool flag = true;
    for (auto type : allowedDomains)
        if (hasSuffix(url, type)) {
            flag = false;
            break;
        }
    return !flag;

}

bool hasSuffix(string str, string suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

string reformatHttpResponse(string text) {
    string allowedChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01233456789.,/\":#?+-_= ";
    map<char, char> mm;
    for (char ch: allowedChars) mm[ch] = ch;
    mm['\n'] = ' ';

    string res = "";
    for (char ch : text) {
        if (mm[ch]) res += tolower(mm[ch]);
    }
    return res;
}



