#include "Global.h"

LoggerInit([]{Logger::getUserHeader() << "[" << FILENAME << "]";});

std::string toLower(std::string s){
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
  return s;
}

int toTPCticks(int tdcTicks) {
  return tdcTicks / get_conversion_tdc_to_tpc();
}

int toTDCticks(int tpcTicks) {
  return tpcTicks * get_conversion_tdc_to_tpc();
}
