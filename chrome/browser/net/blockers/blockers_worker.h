/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLOCKERS_WORKER_H_
#define BLOCKERS_WORKER_H_

#include <string>
#include <vector>
#include <map>
#include <mutex>

class CTPParser;
class ABPFilterParser;
class GURL;
struct sqlite3;

namespace net {
namespace blockers {

struct HTTPSE_REDIRECTS_COUNT_ST {
public:
    HTTPSE_REDIRECTS_COUNT_ST(std::string url, unsigned int redirects):
      url_(url),
      redirects_(redirects) {
    }

    std::string url_;
    unsigned int redirects_;
};

class BlockersWorker {
public:
    BlockersWorker();
    ~BlockersWorker();

    bool shouldTPBlockUrl(const std::string& base_host, const std::string& host);
    bool shouldAdBlockUrl(const std::string& base_host, const std::string& url, unsigned int resource_type);
    std::string getHTTPSURL(const GURL* url);

private:
    bool InitTP();
    bool InitAdBlock();
    bool InitHTTPSE();
    std::string getHTTPSNewHostFromIds(const std::string& ruleIds, const std::string& originalUrl);
    std::string applyHTTPSRule(const std::string& originalUrl, const std::string& rule);
    std::vector<std::string> getTPThirdPartyHosts(const std::string& base_host);

    bool GetData(const char* fileName, std::vector<unsigned char>& buffer, bool only_file_name = false);

    std::string correcttoRuleToRE2Engine(const std::string& to);

    void addHTTPSEUrlToRedirectList(const std::string originalUrl);
    bool shouldHTTPSERedirect(const std::string originalUrl);

    std::vector<unsigned char> tp_buffer_;
    std::vector<unsigned char> adblock_buffer_;
    sqlite3* httpse_db_;
    CTPParser* tp_parser_;
    ABPFilterParser* adblock_parser_;

    std::vector<HTTPSE_REDIRECTS_COUNT_ST> httpse_urls_redirects_count_;
    std::map<std::string, std::vector<std::string>> tp_third_party_hosts_;
    std::vector<std::string> tp_third_party_base_hosts_;
    // That is just temporarily, we will have to figure that out
    // inside the tracking protection lib
    std::vector<std::string> tp_white_list_;

    std::mutex httpse_init_mutex_;
    std::mutex adblock_init_mutex_;
    std::mutex tp_init_mutex_;
    std::mutex tp_get_third_party_hosts_mutex_;
    std::mutex httpse_get_urls_redirects_count_mutex_;
};

}  // namespace blockers
}  // namespace net

#endif
