# chromium

##Get the code

- check out and install the [depot_tools package](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up).
- create browser-android-tabs dir

  `mkdir browser-android-tabs`

- clone the repository to src folder:

  `git clone https://github.com/brave/browser-android-tabs.git src`
  
##Configure your build (You can only use a Linux build machine for Android builds)

- run script and it will download all third-parties. You will be asked for some interaction during that process. Follow that link when you asked to create a gn file [gn file for debug](https://github.com/brave/browser-android-tabs/wiki/Sample-gn-file-for-debug)

  `sh scripts/getThirdParties.js`
  
##Build the full browser

- build browser:

  `ninja -C out/Default chrome_public_apk`
  
  If you have an error that it could not find the build.ninja file follow those steps:
    - run `gn args out/Default` manually and fill it useing that link [gn file for debug](https://github.com/brave/browser-android-tabs/wiki/Sample-gn-file-for-debug)
    - start `ninja -C out/Default chrome_public_apk` again.
  
- deploy it to your Android device:

  `build/android/adb_install_apk.py out/Default/apks/Brave.apk`

##Debugging

- follow that [link](https://www.chromium.org/developers/how-tos/debugging-on-android) for the general debug process;

- follow that [link](https://www.chromium.org/developers/android-eclipse-dev) to configure Eclipse IDE.

##Android version

You should have at least Android 4.1(Jelly Bean) to run Brave. Min SDK version is 16.
