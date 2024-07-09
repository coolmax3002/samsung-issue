# Samsung-Issue

This repository reproduces a potenial Samsung mobile GPU coherence bug.

## Building 
To reproduce this bug, first clone the repository and its submodules using the following command:
```git clone --recurse-submodules https://github.com/coolmax3002/samsung-issue.git```

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







