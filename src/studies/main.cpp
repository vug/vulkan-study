#include "FirstStudy.hpp"
#include "StudyRunner.hpp"

#include <string>

int main() {
  vku::StudyRunner sr;
  auto& study1 = sr.pushStudy(std::make_unique<FirstStudy>());
  int ret = sr.run();
  sr.popStudy(study1); // Example
  return ret;
}

