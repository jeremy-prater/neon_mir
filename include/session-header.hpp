#include "debuglogger.hpp"
#include <pistache/http_header.h>

using namespace Pistache;

class NeonSessionHeader : public Http::Header::Header {
public:
  NAME("SessionID")

  NeonSessionHeader();
  NeonSessionHeader(const std::string sessID);
  ~NeonSessionHeader();
  void parse(const std::string &data) override;
  void write(std::ostream &stream) const override;

private:
  std::string sessionID;
  DebugLogger logger;
};