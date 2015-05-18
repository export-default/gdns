#include "ServerConfig.h"
#include <fstream>
#include "easylogging/easylogging++.h"
#include "libconfig.h++"
#include <arpa/inet.h>
#include <string>

namespace gdns {
    using namespace libconfig;

    ServerConfig::ServerConfig() : bindIp{}, bindPort{}, queryTimeout{}, initTimeout{}, subnetsFilePath{},
                                   proxies{}, blockedDomains{}, nonBlockedDomains{} {
    }

    const string &ServerConfig::getBindIp() const {
        return bindIp;
    }

    int ServerConfig::getBindPort() const {
        return bindPort;
    }

    int ServerConfig::getQueryTimeout() const {
        return queryTimeout;
    }

    int ServerConfig::getInitTimeout() const {
        return initTimeout;
    }

    const vector<Proxy> &ServerConfig::getProxies() const {
        return proxies;
    }

    const vector<string> &ServerConfig::getBlockedDomains() const {
        return blockedDomains;
    }

    const vector<string> &ServerConfig::getNonBlockedDomains() const {
        return nonBlockedDomains;
    }

    const string &ServerConfig::getSubnetsFilePath() const {
        return subnetsFilePath;
    }

    bool ServerConfig::readFromFile(string filename) {
        Config cfg;

        try {
            cfg.readFile(filename.c_str());

            bindIp = cfg.lookup("server.ip").c_str();
            bindPort = cfg.lookup("server.port");
            queryTimeout = cfg.lookup("server.query_timeout");
            initTimeout = cfg.lookup("server.init_timeout");
            subnetsFilePath = cfg.lookup("server.subnets_path").c_str();

            const Setting &pgroup = cfg.lookup("server.proxies");

            for (int i = 0; i < pgroup.getLength(); ++i) {
                const Setting &p = pgroup[i];
                this->proxies.push_back({p["ip"], p["port"], p["tcp"], p["internal"]});
            }

            const Setting &bgroup = cfg.lookup("domains.blocked");
            for (int i = 0; i < bgroup.getLength(); ++i) {
                this->blockedDomains.push_back(bgroup[i]);
            }

            const Setting &ngroup = cfg.lookup("domains.non_blocked");
            for (int i = 0; i < ngroup.getLength(); ++i) {
                this->nonBlockedDomains.push_back(ngroup[i]);
            }
        } catch (const FileIOException &ce) {
            LOG(FATAL) << "Can't read file: " << filename;
            return false;
        } catch (const SettingNotFoundException &nf) {
            LOG(FATAL) << "Can't find setting: " << nf.getPath();
            return false;
        }
        return parseSubnetsFile();
    }

    static bool subnetPairLessThan(const pair<in_addr_t, in_addr_t> &p1, const pair<in_addr_t, in_addr_t> &p2) {
        return p1.first < p2.first;
    }

    bool ServerConfig::parseSubnetsFile() {
        ifstream fin{subnetsFilePath};
        if (!fin) {
            LOG(FATAL) << "Can't read file " << subnetsFilePath << endl;
            return false;
        }

        string line;
        while (fin >> line) {
            auto delimPos = line.find('/');
            auto ip = line.substr(0, delimPos);
            auto mask = stoi(line.substr(delimPos + 1), nullptr, 10);
            mask = ~(~(in_addr_t) 0 >> mask);
            subnets.push_back({ntohl(inet_addr(ip.c_str())) & mask, mask});
        }

        sort(begin(subnets), end(subnets), subnetPairLessThan);
        return true;
    }

    bool ServerConfig::isInternalIp(struct in_addr *addr) {
        in_addr_t target = ntohl(addr->s_addr);

        auto upper = upper_bound(begin(subnets), end(subnets), pair<in_addr_t, in_addr_t>{target, 0},
                                 subnetPairLessThan);

        if (upper == begin(subnets) || upper == end(subnets)) {
            return false;
        } else {
            --upper;
            return upper->first == (target & upper->second);
        }

    }

    ostream &operator<<(ostream &out, const ServerConfig &cfg) {
        out << "Address = " << cfg.getBindIp() << ":" << cfg.getBindPort();
        out << "\tQuery timout = " << cfg.getQueryTimeout();
        out << "\tInit timeout = " << cfg.getInitTimeout();
        out << "\t Subnets path = " << cfg.getSubnetsFilePath();
        out << "Proxies:" << endl;
        for (auto &p : cfg.getProxies()) {
            out << "\t" << p << endl;
        }
        out << "Blocked domains:";
        for (auto &domain : cfg.getBlockedDomains()) {
            out << "\t" << domain;
        }
        out << endl;
        out << "Non blocked domains:";
        for (auto &domain : cfg.getNonBlockedDomains()) {
            out << "\t" << domain;
        }
        out << endl;
    }
}