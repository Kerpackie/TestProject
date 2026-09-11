#pragma once

#include <string>

namespace database::observability {

class QueryFingerprint final {
 public:
  static std::string sanitize_and_fingerprint(const std::string& sql) {
    std::string fingerprint;
    fingerprint.reserve(sql.size());

    bool in_single_quote = false;
    bool in_digit = false;

    for (std::size_t i = 0; i < sql.size(); ++i) {
      char c = sql[i];
      if (c == '\'') {
        if (!in_single_quote) {
          in_single_quote = true;
          fingerprint += "?";
        } else {
          in_single_quote = false;
        }
        continue;
      }

      if (in_single_quote) {
        continue; // Mask string literals inside quotes
      }

      if (std::isdigit(static_cast<unsigned char>(c))) {
        if (!in_digit) {
          in_digit = true;
          fingerprint += "?";
        }
        continue; // Mask numeric literals
      } else {
        in_digit = false;
      }

      fingerprint += c;
    }

    return fingerprint;
  }
};

}  // namespace database::observability
