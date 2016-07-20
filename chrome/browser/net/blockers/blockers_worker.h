
#ifndef BLOCKERS_WORKER_H_
#define BLOCKERS_WORKER_H_

#include <string>
#include <vector>

class CTPParser;
class ABPFilterParser;

namespace net {
namespace blockers {

class BlockersWorker {
public:
    BlockersWorker();
    ~BlockersWorker();

    bool shouldTPBlockUrl(const std::string& base_host, const std::string& host);
    bool shouldAdBlockUrl(const std::string& base_host, const std::string& host, unsigned int resource_type);

private:
    bool InitTP();
    bool InitAdBlock();

    bool GetData(const char* fileName, std::vector<unsigned char>& buffer);

    std::vector<unsigned char> tp_buffer_;
    std::vector<unsigned char> adblock_buffer_;
    CTPParser* tp_parser_;
    ABPFilterParser* adblock_parser_;
};

}  // namespace blockers
}  // namespace net

#endif
