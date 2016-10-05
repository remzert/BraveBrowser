# chromium

##Get the code

- check out and install the [depot_tools package](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up).
- create chromium dir and clone the repository to src folder:

  `git clone https://github.com/brave/chromium.git src`
  
##Configure your build (You can only use a Linux build machine for Android builds)

- create a a file called `chromium.gyp_env` with the following contents:

  `/chromium$ echo "{ 'GYP_DEFINES': 'OS=android target_arch=arm', }" > chromium.gyp_env`
  
- once `chromium.gyp_env` is ready, you need to run the following command to update projects from gyp files. You may need to run this again when you have added new files, updated gyp files, or sync'ed your repository:
  
  `/chromium$ gclient runhooks`
  
- create a build directory and set the build flags with:
  
  `/chromium/src$ gn args out/Default`
  
  This command will bring up your editor with the GN build args (re-run gn args on that directory to edit the flags in the future. ). In this file add:

      target_os = "android"
      target_cpu = "arm"  # (default)
      is_debug = true  # (default)

      is_component_build = true
      is_clang = true
      symbol_level = 1  # Faster build with fewer symbols. -g1 rather than -g2
      enable_incremental_javac = true  # Much faster; experimental
  
- prepare the environment:

  `/chromium/src$ . build/android/envsetup.sh`
  
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
