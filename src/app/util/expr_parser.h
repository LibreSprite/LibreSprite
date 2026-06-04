// LibreSprite
// Copyright (C) 2026 LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UTIL_EXPR_PARSER_H_INCLUDED
#define APP_UTIL_EXPR_PARSER_H_INCLUDED

#include <string>

class ExprParser {
  std::string src;
  std::size_t pos;
  double baseValue;
  bool hasError;
  std::string errorMsg;

  void skipWhitespace();
  bool matchStr(const std::string& s);
  double parseExpression();
  double parseTerm();
  double parseFactor();

public:
  ExprParser(const std::string& input, double baseVal);
  bool getResult(double& outResult);
  std::string getError() const { return errorMsg; }
};

int evalExpr(const std::string& rawText, int base, bool reportError = false);
bool isExpr(const std::string& rawText);

#endif
