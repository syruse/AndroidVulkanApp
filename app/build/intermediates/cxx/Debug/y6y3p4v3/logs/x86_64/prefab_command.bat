@echo off
"C:\\Program Files\\Android\\Android Studio\\jbr\\bin\\java" ^
  --class-path ^
  "C:\\Users\\s_y_r\\.gradle\\caches\\modules-2\\files-2.1\\com.google.prefab\\cli\\2.0.0\\f2702b5ca13df54e3ca92f29d6b403fb6285d8df\\cli-2.0.0-all.jar" ^
  com.google.prefab.cli.AppKt ^
  --build-system ^
  cmake ^
  --platform ^
  android ^
  --abi ^
  x86_64 ^
  --os-version ^
  30 ^
  --stl ^
  c++_shared ^
  --ndk-version ^
  25 ^
  --output ^
  "C:\\Users\\s_y_r\\AppData\\Local\\Temp\\agp-prefab-staging8217423279915219829\\staged-cli-output" ^
  "C:\\Users\\s_y_r\\.gradle\\caches\\transforms-3\\bd8bf4dccf63e39dd20023c2bbb3b716\\transformed\\jetified-games-activity-1.2.2-alpha01\\prefab"
