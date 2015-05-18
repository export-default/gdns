#ifndef GDNS_PROXY_H
#define GDNS_PROXY_H

#include <string>

namespace gdns {
    using namespace std;

    class Proxy {

    public:
        Proxy(const string &ip, int port, bool tcp, bool internal);

        const string &getIp() const;

        int getPort() const;

        bool isTcp() const;

        bool isInternal() const;

    private:
        string ip;
        int port;
        bool tcp;
        bool internal;
    };

    ostream & operator<<(ostream & out, const Proxy & p);
}

#endif //GDNS_PROXY_H
