#include <optional>
#include <sstream>

#include "parser.hpp"

std::optional<Command> Parser::parse(const std::string& input) {
  std::stringstream ss(input);
  std::istream_iterator<std::string> begin(ss);
  std::istream_iterator<std::string> end;
  std::vector<std::string> tokens(begin, end);

  if (tokens.empty()) {
    return std::nullopt;
  }

  Command cmd;
  cmd.name = tokens[0];
  for (char& c : cmd.name) {
    c = toupper(c);
  }

  cmd.args.assign(tokens.begin() + 1, tokens.end());

  return cmd;
}
