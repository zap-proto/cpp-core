#include "test-util.h"
#include <kj/main.h>
#include "serialize.h"
#include <zap/test.zap.h>
#include <unistd.h>

/* This is the entry point of a fuzz target to be used with libFuzzer
 * or another fuzz driver.
 * Such a fuzz driver is used by the autotools target zap-llvm-fuzzer-testcase
 * when the environment variable LIB_FUZZING_ENGINE is defined
 * for instance LIB_FUZZING_ENGINE=-fsanitize=fuzzer for libFuzzer
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  kj::ArrayPtr<const uint8_t> array(Data, Size);
  kj::ArrayInputStream ais(array);

  if (kj::none != kj::runCatchingExceptions([&]() {
    zap::InputStreamMessageReader reader(ais);
    zap::_::checkTestMessage(reader.getRoot<zap::_::TestAllTypes>());
    zap::_::checkDynamicTestMessage(reader.getRoot<zap::DynamicStruct>(zap::Schema::from<zap::_::TestAllTypes>()));
    kj::str(reader.getRoot<zap::_::TestAllTypes>());
  })) {
    KJ_LOG(ERROR, "threw");
  }
  return 0;
}
