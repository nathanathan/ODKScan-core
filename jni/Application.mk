APP_STL := gnustl_static
APP_CPPFLAGS := -frtti -fexceptions
APP_ABI := armeabi-v7a
#Some devices (ViewPad7) don't support armabi-v7a and you have to use this param instead:
#to find out if your device is affected run this command: adb shell cat /proc/cpuinf
#APP_ABI := armeabi
