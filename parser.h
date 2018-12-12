
#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

using namespace std;

string getHost(string url);
string getPath(string url);

vector< pair<string, string> > extractUrls(string httpText);
vector<string> extractDownloads(string httpText);
bool verifyUrl(string url);
bool verifyDomain(string url);
bool verifyType(string url);
bool hasSuffix(string str, string suffix);
string reformatHttp(string text);
string extractCookie(string httpText);
int getCode(string httpText);
bool findDomain(string url);
bool findType(string url);

#endif
