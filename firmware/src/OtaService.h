#pragma once

class OtaService {
 public:
  void begin();
 void loop();

 private:
  bool started = false;
};
