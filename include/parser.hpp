#ifndef PARSER_HPP
#define PARSER_HPP

#include <optional>
#include <string>
#include <vector>

struct Command {
  std::string name;
  std::vector<std::string> args;
};

class Parser {
public:
  std::optional<Command> parse(const std::string &input);
};

#endif
