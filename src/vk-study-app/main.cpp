#include "studies/ClearStudy.hpp"
#include "studies/FirstStudy.hpp"
#include "studies/SecondStudy.hpp"
#include "StudyApp/StudyRunner.hpp"

#include <string>

int main() {
  vku::StudyRunner sr;
  auto& study0 = sr.pushStudy(std::make_unique<ClearStudy>());
  sr.pushStudy(std::make_unique<FirstStudy>());
  sr.pushStudy(std::make_unique<SecondStudy>());
  int ret = sr.run();
  //sr.popStudy(study0); // Example of removal
  return ret;
}

