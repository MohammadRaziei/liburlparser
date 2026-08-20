#include "utest.h"
#include <cstring>
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


// Runs all field checks for a single CSV row. Non-fatal (EXPECT_*) so every
// mismatched field is reported, not just the first one.
static void check_url_row(int *utest_result, const UrlData& url_data) {
    TLD::Url url(url_data.url, url_data.ignore_www);
    EXPECT_STREQ(url.protocol().c_str(), url_data.protocol.c_str());
    EXPECT_STREQ(url.userinfo().c_str(), url_data.userinfo.c_str());
    EXPECT_STREQ(url.fulldomain().c_str(), url_data.fulldomain.c_str());
    EXPECT_STREQ(url.host().str().c_str(), url_data.fulldomain.c_str());
    EXPECT_STREQ(url.suffix().c_str(), url_data.suffix.c_str());
    EXPECT_EQ(url.port(), url_data.port);
    EXPECT_STREQ(url.query().c_str(), url_data.query.c_str());
    EXPECT_STREQ(url.fragment().c_str(), url_data.fragment.c_str());
}

UTEST(CSVUrlTest, CheckPSLisLoaded){
    ASSERT_TRUE(TLD::Host::isPslLoaded());
}

UTEST(CSVUrlTest, UrlDataInput) {
    const std::vector<UrlData> rows = load_url_data(makeAbsolutePath("data/url_data.csv"));
    for (size_t i = 0; i < rows.size(); ++i) {
        const UrlData& row = rows[i];
        int row_result = 0;
        check_url_row(&row_result, row);
        if (row_result) {
            UTEST_PRINTF("  ^ failed on row %zu: %s\n", i, row.toString().c_str());
            *utest_result = 1;
        }
    }
}
