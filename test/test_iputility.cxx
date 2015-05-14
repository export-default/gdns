#include <gtest/gtest.h>
#include <uv.h>
extern "C" {
#include "../src/iputility.h"
}

namespace TestIPUtility {

    const char *IP_IN[] = {
            "180.149.132.47",
            "220.181.57.218",
            "114.212.80.233",
    };
    const char *IP_OUT[] = {
            "1.1.127.15",
            "1.1.67.51",
            "95.211.229.156"
    };

    class IPUtilityTest : public ::testing::Test {
    protected:
        static subnet_list_t list;

        static void SetUpTestCase() {
            subnet_list_init("subnets.txt", &list);
        }

        static void TearDownTestCase() {
            subnet_list_free(&list);
        }

        static boolean is_in_list(const char *ip) {
            struct in_addr addr;
            inet_aton(ip, &addr);
            return ip_in_subnet_list(&list, &addr);
        }
    };

    subnet_list_t IPUtilityTest::list;

    TEST_F(IPUtilityTest, InSubnet) {
        int i = 0;
        for (i = 0; i < sizeof(IP_IN) / sizeof(char *); ++i) {
            EXPECT_TRUE(is_in_list(IP_IN[i])) << IP_IN[i] ;
        }
    }

    TEST_F(IPUtilityTest, NotInSubnet) {
        int i = 0;
        for (i = 0; i < sizeof(IP_OUT) / sizeof(char *); ++i) {
            EXPECT_FALSE(is_in_list(IP_OUT[i])) << IP_OUT[i];
        }
    }

}
