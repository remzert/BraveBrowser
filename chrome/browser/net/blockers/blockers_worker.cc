#include "blockers_worker.h"
#include <fstream>
#include "../../../../base/android/apk_assets.h"
#include "../../../../content/public/common/resource_type.h"
#include "TPParser.h"
#include "ABPFilterParser.h"

#define TP_DATA_FILE       "assets/tp_data.dat"
#define ADBLOCK_DATA_FILE  "assets/ABPFilterParserData.dat"

namespace net {
namespace blockers {

    BlockersWorker::BlockersWorker() :
        tp_mmap_(nullptr),
        adblock_mmap_(nullptr),
        tp_parser_(nullptr),
        adblock_parser_(nullptr) {
        base::ThreadRestrictions::SetIOAllowed(true);
        InitTP();
        InitAdBlock();
        LOG(ERROR) << "!!!!Init";
    }

    BlockersWorker::~BlockersWorker() {
        if (nullptr != tp_mmap_) {
            delete tp_mmap_;
        }
        if (nullptr != tp_parser_) {
            delete tp_parser_;
        }
        if (nullptr != adblock_mmap_) {
            delete adblock_mmap_;
        }
        if (nullptr != adblock_parser_) {
            delete adblock_parser_;
        }
    }

    void BlockersWorker::InitAdBlock() {
        base::MemoryMappedFile::Region region_out;
        int fd_out = base::android::OpenApkAsset(ADBLOCK_DATA_FILE, &region_out);
        if (fd_out < 0) {
            LOG(ERROR) << "InitAdBlock: Cannot open assets/ABPFilterParserData.dat";
            return;
        }

        base::File file(fd_out);
        adblock_mmap_ = new base::MemoryMappedFile();
        if (!adblock_mmap_->Initialize(std::move(file), region_out)) {
            LOG(ERROR) << "InitAdBlock: Cannot init memory mapped file";
            return;
        }

        adblock_parser_ = new ABPFilterParser();
        adblock_parser_->deserialize((char*)adblock_mmap_->data());
    }

    void BlockersWorker::InitTP() {
        base::MemoryMappedFile::Region region_out;
        int fd_out = base::android::OpenApkAsset(TP_DATA_FILE, &region_out);
        if (fd_out < 0) {
            LOG(ERROR) << "InitTP: Cannot open assets/tp_data.dat";
            return;
        }

        base::File file(fd_out);
        tp_mmap_ = new base::MemoryMappedFile();
        if (!tp_mmap_->Initialize(std::move(file), region_out)) {
            LOG(ERROR) << "InitTP: Cannot init memory mapped file";
            return;
        }

        tp_parser_ = new CTPParser();
        tp_parser_->deserialize((char*)tp_mmap_->data());
    }

    bool BlockersWorker::shouldAdBlockUrl(const std::string& base_host, const std::string& host, unsigned int resource_type) {
        if (nullptr == adblock_parser_) {
            return false;
        }

        FilterOption currentOption = FONoFilterOption;
        content::ResourceType internalResource = (content::ResourceType)resource_type;
        if (content::RESOURCE_TYPE_STYLESHEET == internalResource) {
            currentOption = FOStylesheet;
        } else if (content::RESOURCE_TYPE_IMAGE == internalResource) {
            currentOption = FOImage;
        } else if (content::RESOURCE_TYPE_SCRIPT == internalResource) {
            currentOption = FOScript;
        }

        if (adblock_parser_->matches(host.c_str(), currentOption, base_host.c_str())) {
            LOG(ERROR) << "!AdBlock blocked host == " << base_host.c_str() << ", thirdPartyHost == " << host.c_str();

            return true;
        }

        return false;
    }

    bool BlockersWorker::shouldTPBlockUrl(const std::string& base_host, const std::string& host) {
        if (nullptr == tp_parser_) {
            return false;
        }

        if (!tp_parser_->matchesTracker(base_host.c_str(), host.c_str())) {
            return false;
        }

        char* thirdPartyHosts = tp_parser_->findFirstPartyHosts(base_host.c_str());
        std::vector<std::string> hosts;
        if (nullptr != thirdPartyHosts) {
             std::string strThirdPartyHosts = thirdPartyHosts;
             size_t iPos = strThirdPartyHosts.find(",");
             while (iPos != std::string::npos) {
                 std::string thirdParty = strThirdPartyHosts.substr(0, iPos);
                 strThirdPartyHosts = strThirdPartyHosts.substr(iPos + 1);
                 iPos = strThirdPartyHosts.find(",");
                 hosts.push_back(thirdParty);
            }
            delete []thirdPartyHosts;
        }

        for (size_t i = 0; i < hosts.size(); i++) {
            if (host == hosts[i] || host.find((std::string)"." + hosts[i]) != std::string::npos) {
               return false;
            }
            size_t iPos = host.find((std::string)"." + hosts[i]);
            if (iPos == std::string::npos) {
                continue;
            }
            if (hosts[i].length() + ((std::string)".").length() + iPos == host.length()) {
                return false;
            }
        }

        // That is just temporarily, we will have to figure that out
        // inside the tracking protection lib
        std::vector<std::string> whiteList;
        whiteList.push_back("connect.facebook.net");
        whiteList.push_back("connect.facebook.com");
        whiteList.push_back("staticxx.facebook.com");
        whiteList.push_back("www.facebook.com");
        whiteList.push_back("scontent.xx.fbcdn.net");
        whiteList.push_back("pbs.twimg.com");
        whiteList.push_back("scontent-sjc2-1.xx.fbcdn.net");
        whiteList.push_back("platform.twitter.com");
        whiteList.push_back("syndication.twitter.com");
        for (size_t i = 0; i < whiteList.size(); i++) {
            if (whiteList[i] == host) {
                return false;
            }
        }

        LOG(ERROR) << "!TP blocked host == " << base_host.c_str() << ", thirdPartyHost == " << host.c_str();

        return true;
    }

}  // namespace blockers
}  // namespace net
