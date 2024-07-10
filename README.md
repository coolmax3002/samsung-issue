# Coherence Issue on Samsung Xclipse 920 GPU
## Overview
We produced a coherency issue on the Samsung S22 related to reordering subsequent reads of the same value. The core of this test consists of two threads running in parallel: 

`Initial State: *x = 0`

Thread 1:
```
atomicStore(x, 1)
```

Thread 2: 
```
r0 = atomicLoad(x)
r1 = atomicLoad(x)
```

In this test, it should be impossible for `r0 == 1` and `r1 == 0`. However, we found that the Samsung Xclipse 920 GPU on the Samsung S22 exhibited this behavior sometimes. Due to the rarity of this bug, we run many instances of this test in parallel.

The shaders for this test were originally written in OpenCL and then compiled to SPIR-V using [clspv](https://github.com/google/clspv). Both the original OpenCL shader and compiled versions are included in the repository. 

## Prerequistes
To reproduce this issue, you will need these dependencies installed on your machine:
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
- [Android NDK](https://developer.android.com/ndk/downloads)
- [Android SDK for ADB](https://developer.android.com/tools/releases/platform-tools#downloads.html)
- Optional if compiling SPIR-V [clspv](https://github.com/google/clspv)
  
## Building 
To reproduce this issue, first clone the repository and its submodules using the following command:
```
git clone --recurse-submodules https://github.com/coolmax3002/samsung-issue.git
```

Then, run `make android` to build a `armeabi-v7a` ABI within the `build/` directory. This will also copy the spv files to the `build/` directory. 

Next, push the `build/` directory to your device by using adb and the following command: 
```
adb push build/ /data/local/tmp
```

The test can be run on the device using this command:
```
adb shell "cd /data/local/tmp/build && ./runner"
```

Once the test has finished running, the number of weak behavior that occured will be shown. For example :
```
Starting Test...
Test Complete, total weak behaviors: 18
```







