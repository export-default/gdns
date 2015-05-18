#include <ostream>
#include "Proxy.h"

namespace gdns {
    Proxy::Proxy(const string &ip, int port, bool tcp, bool internal) : ip(ip), port(port), tcp(tcp),
                                                                        internal(internal) { }

    const string &Proxy::getIp() const {
        return ip;
    }

    int Proxy::getPort() const {
        return port;
    }

    bool Proxy::isTcp() const {
        return tcp;
    }

    bool Proxy::isInternal() const {
        return internal;
    }

    ostream &operator<<(ostream &out, const Proxy &p) {
        out << "Proxy[" << p.getIp() << ":" << p.getPort() << "]";
        out << boolalpha << "\tInternal: " << p.isInternal() << "\tTCP: " << p.isTcp();
    }
}