#ifndef GDNS_SERVERCONFIG_H
#define GDNS_SERVERCONFIG_H

#include <cstdint>
#include <string>
#include <vector>
#include <netinet/in.h>
#include "Proxy.h"

namespace gdns {
    using namespace std;

    class ServerConfig {
    public:
        ServerConfig();

        bool readFromFile(string filename);


        const string &getBindIp() const;

        int getBindPort() const;

        int getQueryTimeout() const;

        int getInitTimeout() const;

        const vector<Proxy> &getProxies() const;

        const vector<string> &getBlockedDomains() const;

        const vector<string> &getNonBlockedDomains() const;

        const string &getSubnetsFilePath() const;

        bool isInternalIp(struct in_addr *addr);

    private:
        bool parseSubnetsFile();

    private:
        string bindIp;
        int bindPort;
        int queryTimeout;
        int initTimeout;
        string subnetsFilePath;
        vector<Proxy> proxies;
        vector<string> blockedDomains;
        vector<string> nonBlockedDomains;
        vector<pair<in_addr_t, in_addr_t >> subnets;
    };

    ostream &operator<<(ostream &out, const ServerConfig &cfg);
}

#endif //GDNS_SERVERCONFIG_H
