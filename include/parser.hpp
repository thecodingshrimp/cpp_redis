#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <optional>

struct Command {
  std::string name;
  std::vector<std::string> args;
};

class Parser {
public:
  std::optional<Command> parse(const std::string& input);
};

#endif