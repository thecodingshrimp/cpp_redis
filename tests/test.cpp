#include <catch2/catch_test_macros.hpp>

#include "storage.hpp"

TEST_CASE("storage 2load functions correctly", "[hehe]") {
  Storage storage = Storage();

  storage.del("hehe");
}
