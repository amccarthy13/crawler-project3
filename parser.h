
#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

using namespace std;

string getHostnameFromURL(string url);
string getHostPathFromURL(string url);

vector<string> extractImages(string httpText);

vector< pair<string, string> > extractUrls(string httpText);
bool verifyUrl(string url);
bool verifyDomain(string url);
bool verifyType(string url);
bool hasSuffix(string str, string suffix);
string reformatHttpResponse(string text);

#endif
