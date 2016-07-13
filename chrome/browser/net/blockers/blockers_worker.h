
#ifndef BLOCKERS_WORKER_H_
#define BLOCKERS_WORKER_H_

#include <string>

namespace base {
    class MemoryMappedFile;
}

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
    void InitTP();
    void InitAdBlock();

    base::MemoryMappedFile* tp_mmap_;
    base::MemoryMappedFile* adblock_mmap_;
    CTPParser* tp_parser_;
    ABPFilterParser* adblock_parser_;
};

}  // namespace blockers
}  // namespace net

#endif
