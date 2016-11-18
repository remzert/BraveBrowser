cp -f scripts/.gclient ../.gclient
cp -f scripts/.gclient_entries ../.gclient_entries
gclient sync --with_branch_heads
cd ..
echo "{ 'GYP_DEFINES': 'OS=android target_arch=arm buildtype=Official', }" > chromium.gyp_env
gclient runhooks
cd src
gn args out/Default
build/install-build-deps-android.sh
gclient sync
sh . build/android/envsetup.sh
