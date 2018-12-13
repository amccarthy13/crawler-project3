#include <string.h>
#include "parser.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

string getHost(string url) {
    int offset = 0;
    offset = offset == 0 && url.compare(0, 8, "https://") == 0 ? 8 : offset;
    offset = offset == 0 && url.compare(0, 7, "http://") == 0 ? 7 : offset;

    size_t pos = url.find("/", offset);
    string domain = url.substr(offset, (pos == string::npos ? url.length() : pos) - offset);
    return domain;
}

string getPath(string url) {
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

vector<string> extractDownloads(string httpText) {
    string httpRaw = reformatHttp(httpText);
    string temp = httpRaw;

    string urlStart[] = {"href=\"", "href = \"", "href=\'", "href = \'"};

    const string urlEndChars = "\"\'#, ";

    vector<string> extractedUrls;

    for (auto startText : urlStart) {
        httpRaw = temp;
        while (true) {
            int startPos = httpRaw.find(startText);
            if (startPos == string::npos) break;
            startPos += startText.length();

            int endPos = httpRaw.find_first_of(urlEndChars, startPos);

            string url = httpRaw.substr(startPos, endPos - startPos);
            if (!verifyDomain(url) && findDomain(url)) {
                extractedUrls.push_back(url);
            }

            httpRaw.erase(0, endPos);
        }
    }

    string httpRawImage = reformatHttp(httpText);

    const string imgStart[] = {"src=\"", "src = \"", "src=\'", "src = \'"};

    for (auto startText : imgStart) {
        httpRaw = temp;
        while (true) {
            int startPos = httpRawImage.find(startText);
            if (startPos == string::npos) break;
            startPos += startText.length();

            int endPos = httpRawImage.find_first_of(urlEndChars, startPos);

            string url = httpRawImage.substr(startPos, endPos - startPos);

            if (!verifyDomain(url)) {
                extractedUrls.push_back(url);
            }

            httpRawImage.erase(0, endPos);
        }
    }

    return extractedUrls;
}

string extractCookie(string httpText) {
    string httpRaw = reformatHttp(httpText);

    const string urlStart = "set-cookie: ";

    const string endChar = " ";

    int startPos = httpRaw.find(urlStart);
    if (startPos == string::npos) return "";
    startPos += urlStart.length();

    int endPos = httpRaw.find_first_of(endChar, startPos);

    string cookie = httpRaw.substr(startPos, endPos - startPos);

    int count = 0;
    for (char i : cookie) {
        if (i == '=') {
            break;
        }
        count++;
    }
    string front = cookie.substr(0, count);
    string back = cookie.substr(count, cookie.length());
    for (auto &c: front) c = toupper(c);
    string output = front + back;
    return output;
}

int getCode(string httpText) {
    string httpRaw = reformatHttp(httpText);
    string coinCode = "402 insert coin to continue";
    int startPos = httpRaw.find(coinCode);
    if (startPos != string::npos) {
        return 1;
    }

    string notFoundCode = "404 not found";
    int twoPos = httpRaw.find(notFoundCode);
    if (twoPos != string::npos) {
        return 2;
    }

    return 0;
}


vector<pair<string, string> > extractUrls(string httpText) {
    string httpRaw = reformatHttp(httpText);
    string temp = httpRaw;

    const string urlStart[] = {"href=\"", "href = \"", "href=\'", "href = \'", "http://", "https://"};

    const string urlEndChars = "\"\'#, ";

    vector<pair<string, string> > extractedUrls;

    for (auto startText : urlStart) {
        httpRaw = temp;
        while (true) {
            int startPos = httpRaw.find(startText);
            if (startPos == string::npos) break;
            startPos += startText.length();

            int endPos = httpRaw.find_first_of(urlEndChars, startPos);

            string url = httpRaw.substr(startPos, endPos - startPos);

            string host = getHost(url);
            if (host == ".") {
                if (verifyType(url) && findType(url)) {
                    extractedUrls.push_back(make_pair("", url.substr(1)));
                }
            } else if (host == url) {
                if (verifyType(url) && findType(url)) {
                    extractedUrls.push_back(make_pair("", "/" + url));
                }
            } else if (verifyUrl(url)) {
                string urlDomain = getHost(url);
                extractedUrls.push_back(make_pair(urlDomain, getPath(url)));
            }

            httpRaw.erase(0, endPos);
        }
    }
    return extractedUrls;
}

bool verifyUrl(string url) {
    string urlDomain = getHost(url);

    if (urlDomain != "" && !verifyDomain(urlDomain)) return false;

    if (!verifyType(url)) return false;

    if (url.find("mailto:") != string::npos) return false;

    return true;
}

bool verifyType(string url) {
    string forbiddenTypes[] = {".css", ".js", ".pdf", ".png", ".jpeg", ".jpg", ".ico", ".zip", ".gif"};
    if (url.find(".json") != string::npos) return true;
    for (auto type : forbiddenTypes)
        if (url.find(type) != string::npos) return false;
    return true;
}

bool verifyDomain(string url) {
    string allowedDomains[] = {".com", ".sg", ".net", ".co", ".org", ".me", ".edu", ".gov"};
    bool flag = true;
    for (auto type : allowedDomains) {
        if (hasSuffix(url, type)) {
            flag = false;
            break;
        }
    }
    return !flag;

}

bool findDomain(string url) {
    string allowedDomains[] = {".com", ".sg", ".net", ".co", ".org", ".me", ".edu", ".gov"};
    for (auto type : allowedDomains) {
        if (url.find(type) != string::npos) {
            return false;
        }
    }
    return true;
}

bool findType(string url) {
    string allowed = ".com";
    if (url.find(allowed) != string::npos) {
        return false;
    }
    return true;
}

bool hasSuffix(string str, string suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

string reformatHttp(string text) {
    string allowedChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01233456789.,/\":#?+-_=\' ";
    map<char, char> mm;
    for (char ch: allowedChars) mm[ch] = ch;
    mm['\n'] = ' ';

    string res = "";
    for (char ch : text) {
        if (mm[ch]) res += tolower(mm[ch]);
    }
    return res;
}



