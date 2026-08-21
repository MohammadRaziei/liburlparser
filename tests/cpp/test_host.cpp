#include "utest.h"
#include <cstring>
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


// Runs all field checks for a single CSV row. Non-fatal (EXPECT_*) so every
// mismatched field is reported, not just the first one.
static void check_host_row(int *utest_result, const HostData& host_data) {
    urlparser::Host host = urlparser::Host::fromUrl(host_data.url, host_data.ignore_www);
    EXPECT_STREQ(host.str().c_str(), host_data.host.c_str());
    EXPECT_STREQ(host.domain().c_str(), host_data.domain.c_str());
    EXPECT_STREQ(host.domainName().c_str(), host_data.domain_name.c_str());
    EXPECT_STREQ(host.suffix().c_str(), host_data.suffix.c_str());
}

UTEST(CSVHostTest, CheckPSLisLoaded){
    ASSERT_TRUE(urlparser::Host::isPslLoaded());
}

UTEST(CSVHostTest, HostDataInput) {
    const std::vector<HostData> rows = load_host_data(makeAbsolutePath("data/host_data.csv"));
    for (size_t i = 0; i < rows.size(); ++i) {
        const HostData& row = rows[i];
        int row_result = 0;
        check_host_row(&row_result, row);
        if (row_result) {
            UTEST_PRINTF("  ^ failed on row %zu: %s\n", i, row.toString().c_str());
            *utest_result = 1;
        }
    }
}
