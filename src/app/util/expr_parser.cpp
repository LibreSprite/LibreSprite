// LibreSprite
// Copyright (C) 2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/util/expr_parser.h"
#include "app/console.h"
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <algorithm>

void ExprParser::skipWhitespace() {
  while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) {
    pos++;
  }
}

bool ExprParser::matchStr(const std::string& s) {
  if (pos + s.size() > src.size()) return false;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(src[pos + i])) != std::tolower(static_cast<unsigned char>(s[i]))) {
      return false;
    }
  }
  pos += s.size();
  return true;
}

ExprParser::ExprParser(const std::string& input, double baseVal)
    : src(input), pos(0), baseValue(baseVal), hasError(false) {
  skipWhitespace();
  if (pos < src.size()) {
    char c = src[pos];
    if (c == '*' || c == '/') {
      src = std::to_string(baseValue) + src.substr(pos);
      pos = 0;
    } else if (c == '+' || c == '-') {
      std::size_t p = pos + 1;
      while (p < src.size() && std::isspace(static_cast<unsigned char>(src[p]))) {
        p++;
      }
      if (p < src.size() && (std::isdigit(static_cast<unsigned char>(src[p])) || src[p] == '.')) {
        std::size_t nextPos = 0;
        try {
          std::stod(src.substr(p), &nextPos);
          p += nextPos;
          if (p < src.size() && (src[p] == 'f' || src[p] == 'F')) {
            p++;
          }
          while (p < src.size() && std::isspace(static_cast<unsigned char>(src[p]))) {
            p++;
          }
          if (p < src.size() && src[p] == '%') {
            p++;
          } else if (p + 1 < src.size() && std::tolower(static_cast<unsigned char>(src[p])) == 'p' && std::tolower(static_cast<unsigned char>(src[p+1])) == 'x') {
            p += 2;
          }
          while (p < src.size() && std::isspace(static_cast<unsigned char>(src[p]))) {
            p++;
          }
          if (p >= src.size()) {
            src = std::to_string(baseValue) + src.substr(pos);
            pos = 0;
          }
        } catch (...) {
          // Do not prepend if parsing fails
        }
      }
    }
  }
}

double ExprParser::parseExpression() {
  double val = parseTerm();
  while (true) {
    skipWhitespace();
    if (pos >= src.size()) break;
    char op = src[pos];
    if (op == '+' || op == '-') {
      pos++;
      double rhs = parseTerm();
      if (op == '+') val += rhs;
      else val -= rhs;
    } else {
      break;
    }
  }
  return val;
}

double ExprParser::parseTerm() {
  double val = parseFactor();
  while (true) {
    skipWhitespace();
    if (pos >= src.size()) break;
    char op = src[pos];
    if (op == '*' || op == '/') {
      pos++;
      double rhs = parseFactor();
      if (op == '*') {
        val *= rhs;
      } else {
        if (rhs == 0.0) {
          hasError = true;
          errorMsg = "Division by zero";
          val = 0.0;
        } else {
          val /= rhs;
        }
      }
    } else {
      break;
    }
  }
  return val;
}

double ExprParser::parseFactor() {
  skipWhitespace();
  if (pos >= src.size()) {
    hasError = true;
    errorMsg = "Unexpected end of expression";
    return 0.0;
  }

  bool negative = false;
  if (src[pos] == '+') {
    pos++;
    skipWhitespace();
  } else if (src[pos] == '-') {
    pos++;
    negative = true;
    skipWhitespace();
  }

  double val = 0.0;
  if (src[pos] == '(') {
    pos++;
    val = parseExpression();
    skipWhitespace();
    if (pos < src.size() && src[pos] == ')') {
      pos++;
    } else {
      hasError = true;
      errorMsg = "Missing closing parenthesis";
    }
  } else if (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.') {
    std::size_t nextPos = 0;
    try {
      val = std::stod(src.substr(pos), &nextPos);
      pos += nextPos;
      if (pos < src.size() && (src[pos] == 'f' || src[pos] == 'F')) {
        pos++;
      }
    } catch (...) {
      hasError = true;
      errorMsg = "Invalid number format";
    }
  } else {
    hasError = true;
    errorMsg = std::string("Unexpected character: '") + src[pos] + "'";
    pos++;
  }

  if (negative) {
    val = -val;
  }

  while (true) {
    skipWhitespace();
    if (pos >= src.size()) break;
    if (src[pos] == '%') {
      val *= 0.01;
      pos++;
    } else if (matchStr("px")) {
      continue;
    } else {
      break;
    }
  }

  return val;
}

bool ExprParser::getResult(double& outResult) {
  try {
    outResult = parseExpression();
    skipWhitespace();
    if (pos < src.size()) {
      if (matchStr("px")) {
        skipWhitespace();
      }
      if (pos < src.size()) {
        hasError = true;
        errorMsg = std::string("Unexpected trailing characters: '") + src.substr(pos) + "'";
      }
    }
  } catch (const std::exception& e) {
    hasError = true;
    errorMsg = e.what();
  }
  return !hasError;
}

int evalExpr(const std::string& rawText, int base, bool reportError) {
  double result = 0.0;
  ExprParser parser(rawText, base);
  if (parser.getResult(result)) {
    return (int)std::round(result);
  } else {
    if (reportError) {
      app::Console console;
      console.printf("Error evaluating expression '%s': %s\n", rawText.c_str(), parser.getError().c_str());
    }
    return base;
  }
}

bool isExpr(const std::string& rawText) {
  for (char c : rawText) {
    if (!std::isdigit(static_cast<unsigned char>(c)) && !std::isspace(static_cast<unsigned char>(c))) {
      return true;
    }
  }
  return false;
}
