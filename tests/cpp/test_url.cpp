#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "urlparser.h"
#include "common.h"


struct UrlData : public BaseData{
    std::string url;
    bool ignore_www;
    std::string protocol;
    std::string userinfo;
    std::string fulldomain;
    std::string subdomain;
    std::string domain;
    std::string domain_name;
    std::string suffix;
    int port;
    std::string query;
    std::string fragment;

    std::string toString() const{
        std::stringstream ss;
        ss << std::boolalpha;
        ss << "{" << "url: " << url << ", ignore_www: " << ignore_www
           << ", url: " << url << ", subdomain: " << subdomain
           << ", domain: " << domain << ", domain_name: " << domain_name
           << ", suffix: " << suffix << "}";
        return ss.str();
    }
};


std::vector<UrlData> load_url_data(const std::string& filename) {
    std::vector<UrlData> url_data_list;
    std::ifstream f(filename);
    std::string line;
    std::getline(f, line); // Skip the header line
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        UrlData url_data;
        std::string ignore_www, port;
        // url,ignore_www,protocol,userinfo,fulldomain,subdomain,domain,domain_name,suffix,port,query,fragment

        std::getline(ss, url_data.url, ',');
        std::getline(ss, ignore_www, ',');
        url_data.ignore_www = ignore_www == "True";
        std::getline(ss, url_data.protocol, ',');
        std::getline(ss, url_data.userinfo, ',');
        std::getline(ss, url_data.fulldomain, ',');
        std::getline(ss, url_data.subdomain, ',');
        std::getline(ss, url_data.domain, ',');
        std::getline(ss, url_data.domain_name, ',');
        std::getline(ss, url_data.suffix, ',');
        std::getline(ss, port, ',');
        url_data.port = std::stoi(port);
        std::getline(ss, url_data.query, ',');
        std::getline(ss, url_data.fragment, ',');

        url_data_list.push_back(url_data);
    }
    return url_data_list;
}

class CSVUrlTest : public ::testing::TestWithParam<UrlData> {};

INSTANTIATE_TEST_SUITE_P(UrlDataTestCases, CSVUrlTest,
                         ::testing::ValuesIn(load_url_data(
                             makeAbsolutePath("data/url_data.csv"))));

TEST_P(CSVUrlTest, UrlDataInput) {
    const UrlData& url_data = GetParam();
    TLD::Url url = TLD::Url(url_data.url, url_data.ignore_www);
    ASSERT_EQ(url.protocol(), url_data.protocol);
    ASSERT_EQ(url.userinfo(), url_data.userinfo);
    ASSERT_EQ(url.fulldomain(), url_data.fulldomain);
    ASSERT_EQ(url.host().str(), url_data.fulldomain);
    ASSERT_EQ(url.suffix(), url_data.suffix);
    ASSERT_EQ(url.port(), url_data.port);
    ASSERT_EQ(url.query(), url_data.query);
    ASSERT_EQ(url.fragment(), url_data.fragment);
}

