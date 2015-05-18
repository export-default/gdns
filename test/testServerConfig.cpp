#include <gtest/gtest.h>
#include <easylogging/easylogging++.h>
#include "ServerConfig.h"
#include <arpa/inet.h>

INITIALIZE_EASYLOGGINGPP

namespace test {
    using namespace gdns;

    TEST(ServerConfigTest, ReadConfig) {
        ServerConfig cfg;

        ASSERT_TRUE(cfg.readFromFile("./gdns.conf"));

        EXPECT_EQ(string("127.0.0.1"), cfg.getBindIp());
        EXPECT_EQ(5555, cfg.getBindPort());
        EXPECT_EQ(2000, cfg.getQueryTimeout());
        EXPECT_EQ(10000, cfg.getInitTimeout());
        EXPECT_EQ(7, cfg.getProxies().size());
        EXPECT_EQ(string("233.5.5.5"), cfg.getProxies()[0].getIp());
        EXPECT_EQ(6, cfg.getBlockedDomains().size());
        EXPECT_EQ(18, cfg.getNonBlockedDomains().size());

    }

    TEST(ServerConfigTest, InternalTest) {

        ServerConfig cfg;

        ASSERT_TRUE(cfg.readFromFile("./gdns.conf"));

        LOG(INFO) << cfg;

        vector<string> in{
                "180.149.132.47",
                "220.181.57.218",
                "114.212.80.233",
        };

        for (auto &s : in) {
            struct in_addr addr;
            inet_aton(s.c_str(), &addr);
            EXPECT_TRUE(cfg.isInternalIp(&addr));
        }

        vector<string> out{
                "1.1.127.15",
                "1.1.67.51",
                "95.211.229.156"
        };

        for (auto &s : out) {
            struct in_addr addr;
            inet_aton(s.c_str(), &addr);
            EXPECT_FALSE(cfg.isInternalIp(&addr));
        }
    }

    int main(int argc, char **argv) {
        START_EASYLOGGINGPP(argc, argv);
        testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
}