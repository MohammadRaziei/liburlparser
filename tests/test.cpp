#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "urlparser.h"

struct HostData {
    std::string url;
    bool ignore_www;
    std::string host;
    std::string subdomain;
    std::string domain;
    std::string domain_name;
    std::string suffix;
};

std::vector<HostData> load_host_data() {
    std::vector<HostData> host_data_list;
    std::ifstream f("data/host_data.csv");
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        HostData host_data;
        ss >> host_data.url >> host_data.ignore_www >> host_data.host >>
            host_data.subdomain >> host_data.domain >> host_data.domain_name >>
            host_data.suffix;
        host_data_list.push_back(host_data);
    }
    return host_data_list;
}

TEST(HostTest, FromUrl) {
    std::vector<HostData> host_data_list = load_host_data();
    for (const auto& host_data : host_data_list) {
        TLD::Host host = TLD::Host::fromUrl(host_data.url, host_data.ignore_www);
//        ASSERT_EQ(std::string(host), host_data.host);
        ASSERT_EQ(host.domain(), host_data.domain);
        ASSERT_EQ(host.domainName(), host_data.domain_name);
        ASSERT_EQ(host.suffix(), host_data.suffix);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
