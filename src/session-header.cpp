#include "session-header.hpp"

NeonSessionHeader::NeonSessionHeader()
    : logger("SessionHeader", DebugLogger::DebugColor::COLOR_BLUE, false),
      sessionID("Invalid") {}

NeonSessionHeader::NeonSessionHeader(const std::string sessID)
    : logger("SessionHeader", DebugLogger::DebugColor::COLOR_BLUE, false),
      sessionID(sessID) {}

NeonSessionHeader::~NeonSessionHeader() {}

void NeonSessionHeader::parse(const std::string &data) {
  sessionID = data;
  logger.WriteLog(DebugLogger::DebugLevel::DEBUG_INFO, "Parsing [%s]",
                  data.c_str());
}

void NeonSessionHeader::write(std::ostream &stream) const {
  stream << sessionID;
}
