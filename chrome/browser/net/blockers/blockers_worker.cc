#include "blockers_worker.h"
#include <fstream>
#include "../../../../base/android/apk_assets.h"
#include "../../../../content/public/common/resource_type.h"
#include "../../../../base/files/file_util.h"
#include "../../../../base/path_service.h"
#include "TPParser.h"
#include "ABPFilterParser.h"

#define TP_DATA_FILE       "TrackingProtectionDownloaded.dat"
#define ADBLOCK_DATA_FILE  "ABPFilterParserDataDownloaded.dat"

namespace net {
namespace blockers {

    BlockersWorker::BlockersWorker() :
        tp_parser_(nullptr),
        adblock_parser_(nullptr) {
        base::ThreadRestrictions::SetIOAllowed(true);
        InitTP();
        InitAdBlock();
    }

    BlockersWorker::~BlockersWorker() {
        if (nullptr != tp_parser_) {
            delete tp_parser_;
        }
        if (nullptr != adblock_parser_) {
            delete adblock_parser_;
        }
    }

    bool BlockersWorker::InitAdBlock() {
        if (!GetData(ADBLOCK_DATA_FILE, adblock_buffer_)) {
            return false;
        }

        adblock_parser_ = new ABPFilterParser();
        adblock_parser_->deserialize((char*)&adblock_buffer_.front());

        return false;
    }

    bool BlockersWorker::InitTP() {
        if (!GetData(TP_DATA_FILE, tp_buffer_)) {
            return false;
        }

        tp_parser_ = new CTPParser();
        tp_parser_->deserialize((char*)&tp_buffer_.front());

        return true;
    }

    bool BlockersWorker::GetData(const char* fileName, std::vector<unsigned char>& buffer) {
        base::FilePath app_data_path;
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);

        base::FilePath dataFilePathDownloaded = app_data_path.Append(fileName);
        int64_t size = 0;
        if (!base::PathExists(dataFilePathDownloaded)
            || !base::GetFileSize(dataFilePathDownloaded, &size)
            || 0 == size) {
            LOG(ERROR) << "GetData: the dat info file is corrupted.";

            return false;
        }
        std::vector<char> data(size + 1);
        if (size != base::ReadFile(dataFilePathDownloaded, (char*)&data.front(), size)) {
            LOG(ERROR) << "BlockersWorker::InitTP: cannot read dat info file " << fileName;

            return false;
        }
        data[size] = '\0';

        base::FilePath dataFilePath = app_data_path.Append(&data.front());
        if (!base::PathExists(dataFilePath)
            || !base::GetFileSize(dataFilePath, &size)
            || 0 == size) {
            LOG(ERROR) << "BlockersWorker::InitTP: the dat file is corrupted " << &data.front();

            return false;
        }
        buffer.resize(size);
        if (size != base::ReadFile(dataFilePath, (char*)&buffer.front(), size)) {
            LOG(ERROR) << "BlockersWorker::InitTP: cannot read dat file " << &data.front();

            return false;
        }

        return true;
    }

    bool BlockersWorker::shouldAdBlockUrl(const std::string& base_host, const std::string& host, unsigned int resource_type) {
        if (nullptr == adblock_parser_ && !InitAdBlock()) {
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
            return true;
        }

        return false;
    }

    bool BlockersWorker::shouldTPBlockUrl(const std::string& base_host, const std::string& host) {
        if (nullptr == tp_parser_ && !InitTP()) {
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
            if (0 != strThirdPartyHosts.length()) {
              hosts.push_back(strThirdPartyHosts);
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

        return true;
    }

}  // namespace blockers
}  // namespace net
