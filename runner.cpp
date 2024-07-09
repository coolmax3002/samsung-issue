#include <android/log.h>
#include <easyvk.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;
using namespace easyvk;

const int ITERATIONS = 100;
const int NUM_OUTPUT = 4;
const int WORKGROUP_SIZE = 256;
const int TESTING_WORKGROUP = 512;
const int MAX_WORKGROUP = 1024;
const int MEM_STRIDE = 4;
const int SCRATCH_MEMORY_SIZE = 2048;
const int MEM_STRESS_ITER = 1024;
const int PRE_STRESS_ITER = 128;
const int PERMUTE_FIRST = 419;
const int PERMUTE_SECOND = 1;
const int ALIASED_MEMORY = 1;
const int STRESS_LINE_SIZE = 62;
const int STRESS_TARGET_LINES = 2;
const int deviceID = 0;

const char *shader_file = "corr.spv";
const char *result_shader_file = "corr-results.spv";

void clearMemory(Buffer &gpuMem, int size) {
  for (int i = 0; i < size; i++) {
    gpuMem.store<uint32_t>(i, 0);
  }
}

void setStressParams(Buffer &stressParams) {
  stressParams.store(0, 1);
  stressParams.store(1, 1);
  stressParams.store(2, MEM_STRESS_ITER);
  stressParams.store(3, 2);
  stressParams.store(4, 1);
  stressParams.store(5, PRE_STRESS_ITER);
  stressParams.store(6, 2);
  stressParams.store(7, PERMUTE_FIRST);
  stressParams.store(8, PERMUTE_SECOND);
  stressParams.store(9, TESTING_WORKGROUP);
  stressParams.store(10, MEM_STRIDE);
  stressParams.store(11, 0);
}

void setShuffledWorkgroups(Buffer &shuffledWorkgroups, int numWorkgroups) {
  for (int i = 0; i < numWorkgroups; i++) {
    shuffledWorkgroups.store<uint32_t>(i, i);
  }
  for (int i = numWorkgroups - 1; i > 0; i--) {
    int swap = rand() % (i + 1);
    int temp = shuffledWorkgroups.load<uint32_t>(i);
    shuffledWorkgroups.store<uint32_t>(i, shuffledWorkgroups.load<uint32_t>(swap));
    shuffledWorkgroups.store<uint32_t>(swap, temp);
  }
}

void setScratchLocations(Buffer &locations, int numWorkgroups) {
  set<int> usedRegions;
  int numRegions = SCRATCH_MEMORY_SIZE / STRESS_LINE_SIZE;
  for (int i = 0; i < STRESS_TARGET_LINES; i++) {
    int region = rand() % numRegions;
    while (usedRegions.count(region)) region = rand() % numRegions;
    int locInRegion = rand() % (STRESS_LINE_SIZE);
    for (int j = i; j < numWorkgroups; j += STRESS_TARGET_LINES) {
      locations.store(j, (region * STRESS_LINE_SIZE) + locInRegion);
    }
  }
}

int main(int argc, char *argv[]) {
  cout << "Starting Test..." << endl;
  bool enableValidationLayers = false;
  auto bufferSize = 1024;
  int testingThreads = WORKGROUP_SIZE * TESTING_WORKGROUP;
  int testLocSize = testingThreads * MEM_STRIDE;

  auto instance = Instance(enableValidationLayers);
  auto physicalDevices = instance.physicalDevices();
  auto device = Device(instance, physicalDevices.at(deviceID));

  auto testLocations = Buffer(device, testLocSize, sizeof(uint32_t));
  auto readResults = Buffer(device, NUM_OUTPUT * testingThreads, sizeof(uint32_t));
  auto testResults = Buffer(device, 4, sizeof(uint32_t));
  auto shuffledWorkgroups = Buffer(device, MAX_WORKGROUP, sizeof(uint32_t));
  auto barrier = Buffer(device, 1, sizeof(uint32_t));
  auto scratchpad = Buffer(device, SCRATCH_MEMORY_SIZE, sizeof(uint32_t));
  auto scratchLocations = Buffer(device, MAX_WORKGROUP, sizeof(uint32_t));
  auto stressParams = Buffer(device, 12, sizeof(uint32_t));
  setStressParams(stressParams);

  vector<Buffer> buffers = {testLocations, readResults,      shuffledWorkgroups, barrier,
                            scratchpad,    scratchLocations, stressParams};
  vector<Buffer> resultBuffers = {testLocations, readResults, testResults, stressParams};

  int numSeq = 0;
  int numInter = 0;
  int numWeak = 0;

  for (int i = 0; i < ITERATIONS; i++) {
    auto program = Program(device, shader_file, buffers);
    auto resultProgram = Program(device, result_shader_file, resultBuffers);
    int numWorkgroups = MAX_WORKGROUP;

    setShuffledWorkgroups(shuffledWorkgroups, numWorkgroups);
    setScratchLocations(scratchLocations, numWorkgroups);

    program.setWorkgroups(numWorkgroups);
    program.setWorkgroupSize(WORKGROUP_SIZE);

    resultProgram.setWorkgroups(TESTING_WORKGROUP);
    resultProgram.setWorkgroupSize(WORKGROUP_SIZE);

    program.initialize("litmus_test");
    program.run();

    resultProgram.initialize("litmus_test");
    resultProgram.run();

    numSeq += testResults.load<int>(0) + testResults.load<int>(1);
    numInter += testResults.load<int>(2);
    numWeak += testResults.load<int>(3);

    clearMemory(testResults, 4);

    program.teardown();
    resultProgram.teardown();
  }

  for (Buffer buffer : buffers) {
    buffer.teardown();
  }
  testResults.teardown();
  device.teardown();
  instance.teardown();
  cout << "Test Complete, total weak behaviors: " << numWeak << endl;

  return 0;
}
