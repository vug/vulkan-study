#include "studies/FirstStudy.hpp"
#include "studies/SecondStudy.hpp"
#include "StudyApp/StudyRunner.hpp"

#include <string>

int main() {
  vku::StudyRunner sr;
  auto& study1 = sr.pushStudy(std::make_unique<FirstStudy>());
  auto& study2 = sr.pushStudy(std::make_unique<SecondStudy>());
  int ret = sr.run();
  //sr.popStudy(study1); // Example
  return ret;
}

