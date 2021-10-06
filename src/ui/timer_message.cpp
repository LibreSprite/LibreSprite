#include "ui/message.h"
#include "ui/timer.h"

namespace ui {

bool TimerMessage::send() {
  auto timer = this->timer();
  if (!timer)
    return true;
  timer->tick();
  return Message::send();
}

}
