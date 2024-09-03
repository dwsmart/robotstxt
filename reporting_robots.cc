#include "reporting_robots.h"

#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>


#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"



namespace googlebot {
// The kUnsupportedTags tags are popular tags in robots.txt files, but Google
// doesn't use them for anything. Other search engines may, however, so we
// parse them out so users of the library can highlight them for their own
// users if they so wish.
// These are different from the "unknown" tags, since we know that these may
// have some use cases; to the best of our knowledge other tags we find, don't.
// (for example, "unicorn" from "unicorn: /value")
static const std::vector<std::string> kUnsupportedTags = {
    "clean-param", "crawl-delay", "host", "noarchive", "noindex", "nofollow"};

// sanitize the string so that it can be used in json
std::string escape_json(const std::string &s) {
     std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}



void RobotsParsingReporter::Digest(int line_num,
                                   RobotsParsedLine::RobotsTagName parsed_tag) {
  if (line_num > last_line_seen_) {
    last_line_seen_ = line_num;
  }
  if (parsed_tag != RobotsParsedLine::kUnknown &&
      parsed_tag != RobotsParsedLine::kUnused) {
    ++valid_directives_;
  }

  RobotsParsedLine& line = robots_parse_results_[line_num];
  line.line_num = line_num;
  line.tag_name = parsed_tag;
}

void RobotsParsingReporter::ReportLineMetadata(int line_num,
                                               const LineMetadata& metadata) {
  if (line_num > last_line_seen_) {
    last_line_seen_ = line_num;
  }
  RobotsParsedLine& line = robots_parse_results_[line_num];
  line.line_num = line_num;
  line.is_typo = metadata.is_acceptable_typo;
  line.metadata = metadata;
}

void RobotsParsingReporter::HandleRobotsStart() {
  last_line_seen_ = 0;
  valid_directives_ = 0;
  unused_directives_ = 0;
  unused_directives_string_ = "";
}
void RobotsParsingReporter::HandleRobotsEnd() {}
void RobotsParsingReporter::HandleUserAgent(int line_num,
                                            absl::string_view line_value) {
  Digest(line_num, RobotsParsedLine::kUserAgent);
}
void RobotsParsingReporter::HandleAllow(int line_num,
                                        absl::string_view line_value) {
  Digest(line_num, RobotsParsedLine::kAllow);
}
void RobotsParsingReporter::HandleDisallow(int line_num,
                                           absl::string_view line_value) {
  Digest(line_num, RobotsParsedLine::kDisallow);
}
void RobotsParsingReporter::HandleSitemap(int line_num,
                                          absl::string_view line_value) {
  Digest(line_num, RobotsParsedLine::kSitemap);
}
void RobotsParsingReporter::HandleUnknownAction(int line_num,
                                                absl::string_view action,
                                                absl::string_view line_value) {
  RobotsParsedLine::RobotsTagName rtn =
      std::count(kUnsupportedTags.begin(), kUnsupportedTags.end(),
                 absl::AsciiStrToLower(action)) > 0
          ? RobotsParsedLine::kUnused
          : RobotsParsedLine::kUnknown;
  unused_directives_++;
  std::string action_str = std::string(action);
  std::string action_str_escaped = escape_json(action_str);
  std::string line_value_str = std::string(line_value);
  std::string line_value_str_escaped = escape_json(line_value_str);
  unused_directives_string_  +=  "{\"line_number\":" + std::to_string(line_num) + ",\"action\": \"" + action_str_escaped + "\",\"value\": \"" + line_value_str_escaped + "\"}" + ",";
  Digest(line_num, rtn);
}

}  // namespace googlebot
