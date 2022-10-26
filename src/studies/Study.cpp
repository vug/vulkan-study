#include "Study.hpp"
#include "StudyRunner.hpp"

namespace vku {
  Study::Study(const StudyRunner& studyRunner) : vc(studyRunner.vc), appSettings(studyRunner.appSettings) {}
}