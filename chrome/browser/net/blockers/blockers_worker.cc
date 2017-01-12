/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "blockers_worker.h"
#include <fstream>
#include <sstream>
#include "../../../../base/android/apk_assets.h"
#include "../../../../content/public/common/resource_type.h"
#include "../../../../base/files/file_util.h"
#include "../../../../base/path_service.h"
#include "../../../../base/json/json_reader.h"
#include "../../../../base/values.h"
#include "../../../../third_party/leveldatabase/src/include/leveldb/db.h"
#include "../../../../third_party/re2/src/re2/re2.h"
#include "../../../../url/gurl.h"
#include "TPParser.h"
#include "ABPFilterParser.h"

#define TP_DATA_FILE                        "TrackingProtectionDownloaded.dat"
#define ADBLOCK_DATA_FILE                   "ABPFilterParserDataDownloaded.dat"
#define ADBLOCK_REGIONAL_DATA_FILE          "ABPRegionalDataDownloaded.dat"
#define HTTPSE_DATA_FILE_NEW                "httpse.leveldbDownloaded.zip"
#define TP_THIRD_PARTY_HOSTS_QUEUE          20
#define HTTPSE_URLS_REDIRECTS_COUNT_QUEUE   1
#define HTTPSE_URL_MAX_REDIRECTS_COUNT      5

namespace net {
namespace blockers {

    static std::vector<std::string> split(const std::string &s, char delim) {
        std::stringstream ss(s);
        std::string item;
        std::vector<std::string> result;
        while (getline(ss, item, delim)) {
            result.push_back(item);
        }

        return result;
    }

    // returns parts in reverse order, makes list of lookup domains like com.foo.*
    static std::vector<std::string> expandDomainForLookup(const std::string &domain)
    {
        std::vector<std::string> resultDomains;
        std::vector<std::string> domainParts = split(domain, '.');
        if (domainParts.empty()) {
            return resultDomains;
        }

        for (size_t i = 0; i < domainParts.size() - 1; i++) {  // i < size()-1 is correct: don't want 'com.*' added to resultDomains
            std::string slice = "";
            std::string dot = "";
            for (int j = domainParts.size() - 1; j >= (int)i; j--) {
                slice += dot + domainParts[j];
                dot = ".";
            }
            resultDomains.push_back(slice);
            if (0 != i) {
              // We don't want * on the top URL
              resultDomains.push_back(slice + ".*");
            }
        }

        return resultDomains;
    }

    static std::string leveldbGet(leveldb::DB* db, const std::string &key)
    {
        if (!db) {
            return "";
        }

        std::string value;
        leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
        return s.ok() ? value : "";
    }


    BlockersWorker::BlockersWorker() :
        level_db_(nullptr),
        tp_parser_(nullptr),
        adblock_parser_(nullptr) {
        base::ThreadRestrictions::SetIOAllowed(true);
    }

    BlockersWorker::~BlockersWorker() {
        if (nullptr != tp_parser_) {
            delete tp_parser_;
        }
        if (nullptr != adblock_parser_) {
            delete adblock_parser_;
        }
        for (size_t i = 0; i < adblock_regional_parsers_.size(); i++) {
            delete adblock_regional_parsers_[i];
        }
        adblock_regional_parsers_.clear();
        if (nullptr != level_db_) {
            delete level_db_;
        }
    }

    bool BlockersWorker::InitAdBlock() {
        std::lock_guard<std::mutex> guard(adblock_init_mutex_);

        if (adblock_parser_) {
            return true;
        }

        if (!GetData(ADBLOCK_DATA_FILE, adblock_buffer_)) {
            return false;
        }

        adblock_parser_ = new ABPFilterParser();
        if (!adblock_parser_->deserialize((char*)&adblock_buffer_.front())) {
            delete adblock_parser_;
            adblock_parser_ = nullptr;
            LOG(ERROR) << "adblock deserialize failed";

            return false;
        }

        return true;
    }

    bool BlockersWorker::InitAdBlockRegional() {
        std::lock_guard<std::mutex> guard(adblock_regional_init_mutex_);

        if (0 != adblock_regional_parsers_.size()) {
            return true;
        }

        std::vector<unsigned char> db_file_name;
        if (!GetData(ADBLOCK_REGIONAL_DATA_FILE, db_file_name, true)) {
            return false;
        }
        std::vector<std::string> files = split((char*)&db_file_name.front(), ';');
        for (size_t i = 0; i < files.size(); i++) {
            adblock_regional_buffer_.push_back(std::vector<unsigned char>());
            if (!adblock_regional_buffer_.size()) {
                continue;
            }
            if (!GetBufferData(files[i].c_str(), adblock_regional_buffer_[adblock_regional_buffer_.size() - 1])) {
                adblock_regional_buffer_.erase(adblock_regional_buffer_.begin() + adblock_regional_buffer_.size() - 1);
                continue;
            }

            ABPFilterParser* parser = new ABPFilterParser();
            if (!parser) {
                adblock_regional_buffer_.erase(adblock_regional_buffer_.begin() + adblock_regional_buffer_.size() - 1);
                continue;
            }
            if (!parser->deserialize((char*)&adblock_regional_buffer_[adblock_regional_buffer_.size() - 1].front())) {
                delete parser;
                adblock_regional_buffer_.erase(adblock_regional_buffer_.begin() + adblock_regional_buffer_.size() - 1);
                LOG(ERROR) << "adblock_regional deserialize failed";
                continue;
            }
            adblock_regional_parsers_.push_back(parser);
        }

        return true;
    }

    bool BlockersWorker::InitTP() {
        std::lock_guard<std::mutex> guard(tp_init_mutex_);

        if (tp_parser_) {
            return true;
        }

        if (!GetData(TP_DATA_FILE, tp_buffer_)) {
            return false;
        }

        tp_parser_ = new CTPParser();
        if (!tp_parser_->deserialize((char*)&tp_buffer_.front())) {
            delete tp_parser_;
            tp_parser_ = nullptr;
            LOG(ERROR) << "tp deserialize failed";

            return false;
        }

        tp_white_list_.push_back("connect.facebook.net");
        tp_white_list_.push_back("connect.facebook.com");
        tp_white_list_.push_back("staticxx.facebook.com");
        tp_white_list_.push_back("www.facebook.com");
        tp_white_list_.push_back("scontent.xx.fbcdn.net");
        tp_white_list_.push_back("pbs.twimg.com");
        tp_white_list_.push_back("scontent-sjc2-1.xx.fbcdn.net");
        tp_white_list_.push_back("platform.twitter.com");
        tp_white_list_.push_back("syndication.twitter.com");

        return true;
    }

    bool BlockersWorker::InitHTTPSE() {
        std::lock_guard<std::mutex> guard(httpse_init_mutex_);

        if (level_db_) {
            return true;
        }

        // Init level database
        std::vector<unsigned char> db_file_name;
        if (!GetData(HTTPSE_DATA_FILE_NEW, db_file_name, true)) {
            return false;
        }
        base::FilePath app_data_path;
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
        base::FilePath dbFilePath = app_data_path.Append((char*)&db_file_name.front());

        leveldb::Options options;
        leveldb::Status status = leveldb::DB::Open(options, dbFilePath.value().c_str(), &level_db_);
        if (!status.ok() || !level_db_) {
            if (level_db_) {
                delete level_db_;
                level_db_ = nullptr;
            }

            LOG(ERROR) << "level db open error " << dbFilePath.value().c_str();

            return false;
        }

        return true;
    }

    bool BlockersWorker::GetData(const char* fileName, std::vector<unsigned char>& buffer, bool only_file_name) {
        base::FilePath app_data_path;
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);

        base::FilePath dataFilePathDownloaded = app_data_path.Append(fileName);
        int64_t size = 0;
        if (!base::PathExists(dataFilePathDownloaded)
            || !base::GetFileSize(dataFilePathDownloaded, &size)
            || 0 == size) {
            return false;
        }
        std::vector<char> data(size + 1);
        if (size != base::ReadFile(dataFilePathDownloaded, (char*)&data.front(), size)) {
            LOG(ERROR) << "BlockersWorker::GetData: cannot read dat info file " << fileName;

            return false;
        }
        data[size] = '\0';
        if (only_file_name) {
            buffer.resize(size + 1);
            ::memcpy(&buffer.front(), &data.front(), size + 1);

            return true;
        }

        return GetBufferData(&data.front(), buffer);
    }

    bool BlockersWorker::GetBufferData(const char* fileName, std::vector<unsigned char>& buffer) {
        base::FilePath app_data_path;
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);

        base::FilePath dataFilePath = app_data_path.Append(fileName);
        int64_t size = 0;
        if (!base::PathExists(dataFilePath)
            || !base::GetFileSize(dataFilePath, &size)
            || 0 == size) {
            LOG(ERROR) << "BlockersWorker::GetBufferData: the dat file is corrupted " << fileName;

            return false;
        }
        buffer.resize(size);
        if (size != base::ReadFile(dataFilePath, (char*)&buffer.front(), size)) {
            LOG(ERROR) << "BlockersWorker::GetData: cannot read dat file " << fileName;

            return false;
        }

        return true;
    }

    bool BlockersWorker::shouldAdBlockUrl(const std::string& base_host, const std::string& url,
                                          unsigned int resource_type, bool isAdBlockRegionalEnabled) {
        if (!InitAdBlock()) {
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

        if (adblock_parser_->matches(url.c_str(), currentOption, base_host.c_str())) {
            return true;
        }

        // Check regional ad block
        if (!isAdBlockRegionalEnabled || !InitAdBlockRegional()) {
            return false;
        }
        for (size_t i = 0; i < adblock_regional_parsers_.size(); i++) {
            if (adblock_regional_parsers_[i]->matches(url.c_str(), currentOption, base_host.c_str())) {
                return true;
            }
        }
        //

        return false;
    }

    std::vector<std::string> BlockersWorker::getTPThirdPartyHosts(const std::string& base_host) {
        {
            std::lock_guard<std::mutex> guard(tp_get_third_party_hosts_mutex_);
            std::map<std::string, std::vector<std::string>>::const_iterator iter = tp_third_party_hosts_.find(base_host);
            if (tp_third_party_hosts_.end() != iter) {
                if (tp_third_party_base_hosts_.size() != 0
                      && tp_third_party_base_hosts_[tp_third_party_hosts_.size() - 1] != base_host) {
                    for (size_t i = 0; i < tp_third_party_base_hosts_.size(); i++) {
                        if (tp_third_party_base_hosts_[i] == base_host) {
                            tp_third_party_base_hosts_.erase(tp_third_party_base_hosts_.begin() + i);
                            tp_third_party_base_hosts_.push_back(base_host);
                            break;
                        }
                    }
                }
                return iter->second;
            }
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
        {
            std::lock_guard<std::mutex> guard(tp_get_third_party_hosts_mutex_);
            if (tp_third_party_hosts_.size() == TP_THIRD_PARTY_HOSTS_QUEUE
                  && tp_third_party_base_hosts_.size() == TP_THIRD_PARTY_HOSTS_QUEUE) {
                tp_third_party_hosts_.erase(tp_third_party_base_hosts_[0]);
                tp_third_party_base_hosts_.erase(tp_third_party_base_hosts_.begin());
            }
            tp_third_party_base_hosts_.push_back(base_host);
            tp_third_party_hosts_.insert(std::pair<std::string, std::vector<std::string>>(base_host, hosts));
        }

        return hosts;
    }

    bool BlockersWorker::shouldTPBlockUrl(const std::string& base_host, const std::string& host) {
        if (!InitTP()) {
            return false;
        }

        if (!tp_parser_->matchesTracker(base_host.c_str(), host.c_str())) {
            return false;
        }

        std::vector<std::string> hosts(getTPThirdPartyHosts(base_host));
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

        for (size_t i = 0; i < tp_white_list_.size(); i++) {
            if (tp_white_list_[i] == host) {
                return false;
            }
        }

        return true;
    }

    std::string BlockersWorker::getHTTPSURL(const GURL* url) {
        if (nullptr == url
          || url->scheme() == "https"
          || !InitHTTPSE()) {
            return url->spec();
        }
        if (!shouldHTTPSERedirect(url->spec())) {
            return url->spec();
        }

        if (recently_used_cache_.data.count(url->spec()) > 0) {
            addHTTPSEUrlToRedirectList(url->spec());

            return recently_used_cache_.data[url->spec()];
        }

        const std::vector<std::string> domains = expandDomainForLookup(url->host());
        for (auto domain : domains) {
            std::string value = leveldbGet(level_db_, domain);
            if (!value.empty()) {
                std::string newURL = applyHTTPSRule(url->spec(), value);
                if (0 != newURL.length()) {
                    recently_used_cache_.data[url->spec()] = newURL;
                    addHTTPSEUrlToRedirectList(url->spec());

                    return newURL;
                }
            }
        }
        recently_used_cache_.data[url->spec()] = "";

        return url->spec();
    }

    bool BlockersWorker::shouldHTTPSERedirect(const std::string originalUrl) {
        std::lock_guard<std::mutex> guard(httpse_get_urls_redirects_count_mutex_);
        for (size_t i = 0; i < httpse_urls_redirects_count_.size(); i++) {
            if (originalUrl == httpse_urls_redirects_count_[i].url_
              && httpse_urls_redirects_count_[i].redirects_ >= HTTPSE_URL_MAX_REDIRECTS_COUNT - 1) {
                return false;
            }
        }

        return true;
    }

    void BlockersWorker::addHTTPSEUrlToRedirectList(const std::string originalUrl) {
        // Adding redirects count for the current website
        std::lock_guard<std::mutex> guard(httpse_get_urls_redirects_count_mutex_);
        bool hostFound = false;
        for (size_t i = 0; i < httpse_urls_redirects_count_.size(); i++) {
            if (originalUrl == httpse_urls_redirects_count_[i].url_) {
                // Found the host, just increment the redirects_count
                httpse_urls_redirects_count_[i].redirects_++;
                hostFound = true;
                break;
            }
        }
        if (!hostFound) {
            // The host is new, adding it to the redirects list
            if (httpse_urls_redirects_count_.size() >= HTTPSE_URLS_REDIRECTS_COUNT_QUEUE) {
                // The queue is full, erase the first element
                httpse_urls_redirects_count_.erase(httpse_urls_redirects_count_.begin());
            }
            httpse_urls_redirects_count_.push_back(HTTPSE_REDIRECTS_COUNT_ST(originalUrl, 1));
        }
    }

    std::string BlockersWorker::applyHTTPSRule(const std::string& originalUrl, const std::string& rule) {
        std::unique_ptr<base::Value> json_object = base::JSONReader::Read(rule);
        if (nullptr == json_object.get()) {
            LOG(ERROR) << "applyHTTPSRule: incorrect json rule";

            return "";
        }

        const base::ListValue* topValues = nullptr;
        json_object->GetAsList(&topValues);
        if (nullptr == topValues) {
            return "";
        }

        for (size_t i = 0; i < topValues->GetSize(); ++i) {
            const base::Value* childTopValue = nullptr;
            if (!topValues->Get(i, &childTopValue)) {
                continue;
            }
            const base::DictionaryValue* childTopDictionary = nullptr;
            childTopValue->GetAsDictionary(&childTopDictionary);
            if (nullptr == childTopDictionary) {
                continue;
            }

            const base::Value* exclusion = nullptr;
            if (childTopDictionary->Get("e", &exclusion)) {
                const base::ListValue* eValues = nullptr;
                exclusion->GetAsList(&eValues);
                if (nullptr != eValues) {
                    for (size_t j = 0; j < eValues->GetSize(); ++j) {
                        const base::Value* pValue = nullptr;
                        if (!eValues->Get(j, &pValue)) {
                            continue;
                        }
                        const base::DictionaryValue* pDictionary = nullptr;
                        pValue->GetAsDictionary(&pDictionary);
                        if (nullptr == pDictionary) {
                            continue;
                        }
                        const base::Value* patternValue = nullptr;
                        if (!pDictionary->Get("p", &patternValue)) {
                            continue;
                        }
                        std::string pattern;
                        if (!patternValue->GetAsString(&pattern)) {
                            continue;
                        }
                        pattern = correcttoRuleToRE2Engine(pattern);
                        if (RE2::FullMatch(originalUrl, pattern)) {
                            return "";
                        }
                    }
                }
            }

            const base::Value* rules = nullptr;
            if (!childTopDictionary->Get("r", &rules)) {
                return "";
            }
            const base::ListValue* rValues = nullptr;
            rules->GetAsList(&rValues);
            if (nullptr == rValues) {
                return "";
            }

            for (size_t j = 0; j < rValues->GetSize(); ++j) {
                const base::Value* pValue = nullptr;
                if (!rValues->Get(j, &pValue)) {
                    continue;
                }
                const base::DictionaryValue* pDictionary = nullptr;
                pValue->GetAsDictionary(&pDictionary);
                if (nullptr == pDictionary) {
                    continue;
                }
                const base::Value* patternValue = nullptr;
                if (pDictionary->Get("d", &patternValue)) {
                    std::string newUrl(originalUrl);

                    return newUrl.insert(4, "s");
                }

                const base::Value* from_value = nullptr;
                const base::Value* to_value = nullptr;
                if (!pDictionary->Get("f", &from_value)
                      || !pDictionary->Get("t", &to_value)) {
                    continue;
                }
                std::string from, to;
                if (!from_value->GetAsString(&from)
                      || !to_value->GetAsString(&to)) {
                    continue;
                }

                to = correcttoRuleToRE2Engine(to);
                std::string newUrl(originalUrl);
                RE2 regExp(from);

                if (RE2::Replace(&newUrl, regExp, to) && newUrl != originalUrl) {
                    return newUrl;
                }
            }
        }

        return "";
  }

    std::string BlockersWorker::correcttoRuleToRE2Engine(const std::string& to) {
        std::string correctedto(to);
        size_t pos = to.find("$");
        while (std::string::npos != pos) {
          correctedto[pos] = '\\';
          pos = correctedto.find("$");
        }

        return correctedto;
    }

}  // namespace blockers
}  // namespace net
