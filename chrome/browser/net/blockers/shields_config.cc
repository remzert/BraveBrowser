/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
 #include "shields_config.h"
 #include "base/android/jni_android.h"
 #include "base/android/jni_string.h"
 #include "jni/ShieldsConfig_jni.h"

namespace net {
namespace blockers {

ShieldsConfig* gShieldsConfig = nullptr;

ShieldsConfig::ShieldsConfig(JNIEnv* env, jobject obj):
  weak_java_shields_config_(env, obj) {
}

ShieldsConfig::~ShieldsConfig() {
}

std::string ShieldsConfig::getHostSettings(const std::string& host) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> jhost(base::android::ConvertUTF8ToJavaString(env, host));
  return base::android::ConvertJavaStringToUTF8(
    Java_ShieldsConfig_getHostSettings(env, weak_java_shields_config_.get(env).obj(),
    jhost.obj()));
}

void ShieldsConfig::setBlockedCountInfo(const std::string& url, int adsAndTrackers, int httpsUpgrades) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> jurl(base::android::ConvertUTF8ToJavaString(env, url));
  Java_ShieldsConfig_setBlockedCountInfo(env, weak_java_shields_config_.get(env).obj(),
    jurl.obj(), adsAndTrackers, httpsUpgrades);
}

ShieldsConfig* ShieldsConfig::getShieldsConfig() {
    return gShieldsConfig;
}

static void Clear(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj) {
    delete gShieldsConfig;
    gShieldsConfig = nullptr;
}

static void Init(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj) {
  // This will automatically bind to the Java object and pass ownership there.
  gShieldsConfig = new ShieldsConfig(env, obj);
}

// static
bool ShieldsConfig::RegisterShieldsConfig(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}
}
