#include "FirstStudy.hpp"
#include "StudyRunner.hpp"

#include <string>

int main() {
  vku::StudyRunner sr;
  sr.studies.emplace_back(std::make_unique<FirstStudy>());
  int ret = sr.run();
  return ret;
}

