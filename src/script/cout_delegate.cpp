#include "engine_delegate.h"

class StdoutEngineDelegate : public script::EngineDelegate {
public:
  void onConsolePrint(const char* text) override {
    printf("%s\n", text);
  }
};

static script::EngineDelegate::Singleton<StdoutEngineDelegate> reg("stdout");
