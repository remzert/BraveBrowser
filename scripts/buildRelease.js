sh scripts/updateSubModules.js
sh scripts/copyAdBlockTPFiles.js

ninja -C out/DefaultR chrome_public_apk
ninja -C out/DefaultRx86 chrome_public_apk
ninja -C out/DefaultRmipsel chrome_public_apk

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore out/DefaultR/apks/linkbubble_play_keystore out/DefaultR/apks/Brave.apk linkbubble
third_party/android_tools/sdk/build-tools/23.0.1/zipalign -v -p 4 out/DefaultR/apks/Brave.apk out/DefaultR/apks/Bravearm.apk

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore out/DefaultRx86/apks/linkbubble_play_keystore out/DefaultRx86/apks/Brave.apk linkbubble
third_party/android_tools/sdk/build-tools/23.0.1/zipalign -v -p 4 out/DefaultRx86/apks/Brave.apk out/DefaultRx86/apks/Bravearm.apk

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore out/DefaultRmipsel/apks/linkbubble_play_keystore out/DefaultRmipsel/apks/Brave.apk linkbubble
third_party/android_tools/sdk/build-tools/23.0.1/zipalign -v -p 4 out/DefaultRmipsel/apks/Brave.apk out/DefaultRmipsel/apks/Bravearm.apk
