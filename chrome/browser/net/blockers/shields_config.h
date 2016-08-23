/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SHIELDS_CONFIG_H_
#define SHIELDS_CONFIG_H_

#include <jni.h>
#include "../../../../base/android/jni_weak_ref.h"

namespace net {
namespace blockers {

class ShieldsConfig {
public:
    ShieldsConfig(JNIEnv* env, jobject obj);
    ~ShieldsConfig();

    std::string getHostSettings(const std::string& host);

    static ShieldsConfig* getShieldsConfig();
    // Register the ShieldsConfig's native methods through JNI.
    static bool RegisterShieldsConfig(JNIEnv* env);

private:
    JavaObjectWeakGlobalRef weak_java_shields_config_;
};
}
}

#endif //SHIELDS_CONFIG_H_
