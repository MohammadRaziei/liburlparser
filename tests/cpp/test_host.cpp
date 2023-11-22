#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "urlparser.h"
#include "common.h"



struct HostData : public BaseData{
    std::string url;
    bool ignore_www;
    std::string host;
    std::string subdomain;
    std::string domain;
    std::string domain_name;
    std::string suffix;

    std::string toString() const{
        std::stringstream ss;
        ss << std::boolalpha;
        ss << "{" << "url: " << url << ", ignore_www: " << ignore_www
            << ", host: " << host << ", subdomain: " << subdomain
            << ", domain: " << domain << ", domain_name: " << domain_name
            << ", suffix: " << suffix << "}";
        return ss.str();
    }
};


std::vector<HostData> load_host_data(const std::string& filename) {
    std::vector<HostData> host_data_list;
    std::ifstream f(filename);
    std::string line;
    std::getline(f, line); // Skip the header line
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        HostData host_data;
        std::string ignore_www;
        std::getline(ss, host_data.url, ',');
        std::getline(ss, ignore_www, ',');
        std::getline(ss, host_data.host, ',');
        std::getline(ss, host_data.subdomain, ',');
        std::getline(ss, host_data.domain, ',');
        std::getline(ss, host_data.domain_name, ',');
        std::getline(ss, host_data.suffix);
        host_data.ignore_www = ignore_www == "True";
        host_data_list.push_back(host_data);
    }
    return host_data_list;
}

class CSVHostTest : public ::testing::TestWithParam<HostData> {};

INSTANTIATE_TEST_SUITE_P(HostDataTestCases, CSVHostTest,
                         ::testing::ValuesIn(load_host_data(
                             makeAbsolutePath("data/host_data.csv"))));

TEST(CSVHostTest, CheckPSLisLoaded){
    ASSERT_TRUE(TLD::Host::isPslLoaded()) << "PSL is not loaded";
}

TEST_P(CSVHostTest, HostDataInput) {
    const HostData& host_data = GetParam();
    TLD::Host host = TLD::Host::fromUrl(host_data.url, host_data.ignore_www);
    EXPECT_EQ(host.str(), host_data.host);
    EXPECT_EQ(host.domain(), host_data.domain);
    EXPECT_EQ(host.domainName(), host_data.domain_name);
    EXPECT_EQ(host.suffix(), host_data.suffix);
}

