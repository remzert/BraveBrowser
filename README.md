# chromium

##Get the code

- check out and install the [depot_tools package](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up).
- create chromium dir and clone the repository to src folder:

  `git clone https://github.com/brave/browser-android-tabs.git src`
  
##Configure your build (You can only use a Linux build machine for Android builds)

- run script and it will download all third-parties. You will ask for some interaction during that process. Follow that link when you asked to create a gn file [gn file for debug](https://github.com/brave/browser-android-tabs/wiki/Sample-gn-file-for-debug)

  `sh scripts/getThirdParties.js`
  
##Build the full browser

Note: When adding new resource files or java files in gyp builds, you need to run 'gclient runhooks' again to get them in the build.

- build browser:

  `/chromium/src$ ninja -C out/Default chrome_public_apk`
  
- deploy it to your Android device:

  `/chromium/src$ build/android/adb_install_apk.py out/Default/apks/Brave.apk`

##Debugging

- follow that [link](https://www.chromium.org/developers/how-tos/debugging-on-android) for the general debug process;

- follow that [link](https://www.chromium.org/developers/android-eclipse-dev) to configure Eclipse IDE.

##Android version

You should have at least Android 4.1(Jelly Bean) to run Brave. Min SDK version is 16.
