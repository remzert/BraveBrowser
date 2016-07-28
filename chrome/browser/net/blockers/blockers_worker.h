
#ifndef BLOCKERS_WORKER_H_
#define BLOCKERS_WORKER_H_

#include <string>
#include <vector>
#include <map>

class CTPParser;
class ABPFilterParser;
class GURL;
struct sqlite3;

namespace net {
namespace blockers {

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

    bool GetData(const char* fileName, std::vector<unsigned char>& buffer, bool only_file_name = false);

    std::vector<unsigned char> tp_buffer_;
    std::vector<unsigned char> adblock_buffer_;
    sqlite3* httpse_db_;
    CTPParser* tp_parser_;
    ABPFilterParser* adblock_parser_;
};

}  // namespace blockers
}  // namespace net

#endif
