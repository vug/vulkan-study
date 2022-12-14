#include "StudyApp/StudyRunner.hpp"
#include "studies/01-ClearStudy.hpp"
#include "studies/02-FirstStudy.hpp"
#include "studies/02b-SecondStudy.hpp"
#include "studies/03-Vertices.hpp"
#include "studies/04-Uniforms.hpp"
#include "studies/05-Instanced.hpp"
#include "studies/06-Transforms.hpp"
#include "studies/07-TransformsCompute.hpp"
#include "studies/08-Outlines.hpp"

#include <string>

int main() {
  vku::StudyRunner sr;
  [[maybe_unused]] auto& study0 = sr.pushStudy(std::make_unique<ClearStudy>());
  // sr.pushStudy(std::make_unique<FirstStudy>());
  // sr.pushStudy(std::make_unique<SecondStudy>());
  // sr.pushStudy(std::make_unique<UniformsStudy>());
  // sr.pushStudy(std::make_unique<InstancingStudy>());
  // sr.pushStudy(std::make_unique<TransformConstructionStudy>());
  //sr.pushStudy(std::make_unique<TransformGPUConstructionStudy>());
  sr.pushStudy(std::make_unique<OutlinesViaDepthBuffer>());
  int ret = sr.run();
  // sr.popStudy(study0); // Example of removal
  return ret;
}
