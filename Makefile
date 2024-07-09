CXXFLAGS = -std=c++17
CLSPVFLAGS = --cl-std=CL2.0 --spv-version=1.3 --inline-entry-points
SHADERS = $(wildcard *.cl)
SPVS = $(patsubst %.cl,%.spv,$(SHADERS))
.PHONY: all build clean easyvk runner 

all: build easyvk runner

build:
	mkdir -p build

easyvk: easyvk/src/easyvk.cpp easyvk/src/easyvk.h
	$(CXX) $(CXXFLAGS) -Ieasyvk/src -c easyvk/src/easyvk.cpp -o build/easyvk.o

runner: easyvk runner.cpp $(SPVS)
	$(CXX) $(CXXFLAGS) -Ieasyvk/src build/easyvk.o  runner.cpp -lvulkan -o build/runner.run

android: build $(SPVS)
	ndk-build APP_BUILD_SCRIPT=./Android.mk NDK_PROJECT_PATH=. NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=./build NDK_OUT=./build/android/obj
	cp corr.spv build/
	cp corr-results.spv build/
	mv build/armeabi-v7a/runner build/

%.spv: %.cl
	clspv $(CLSPVFLAGS) $< -o $@

clean:
	rm -rf build
