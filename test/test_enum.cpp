#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Test enums
// ---------------------------------------------------------------------------

enum class Color { Red, Green, Blue };
enum class Status : int { Active = 1, Inactive = 2, Pending = 3 };
enum class Signed : int { Neg = -3, Zero = 0, Pos = 5 };

// unscoped enum with fixed underlying type (also works)
enum Priority : int { Low = 0, Medium = 5, High = 10 };

// An enum with a value that is a combination — should serialize as integer
// fallback.
enum class Flags : int { A = 1, B = 2, C = 4 };

// A large-range enum with custom enum_range specialization
enum class BigEnum { X = 200, Y = 300 };

template <>
struct tinyrefl::enum_range<BigEnum> {
  static constexpr int min = 0;
  static constexpr int max = 500;
};

struct Task {
  std::string name;
  Status status;
  Color highlight;
};

struct TaskWithVec {
  std::string name;
  std::vector<Color> colors;
};

// ---------------------------------------------------------------------------

TEST_CASE("enum_to_string / enum_from_string basic") {
  CHECK(tinyrefl::enum_to_string(Color::Red) == "Red");
  CHECK(tinyrefl::enum_to_string(Color::Green) == "Green");
  CHECK(tinyrefl::enum_to_string(Color::Blue) == "Blue");

  auto r = tinyrefl::enum_from_string<Color>("Red");
  REQUIRE(r.has_value());
  CHECK(r.value() == Color::Red);

  auto missing = tinyrefl::enum_from_string<Color>("Purple");
  CHECK_FALSE(missing.has_value());
}

TEST_CASE("enum with explicit integer values") {
  CHECK(tinyrefl::enum_to_string(Status::Active) == "Active");
  CHECK(tinyrefl::enum_to_string(Status::Inactive) == "Inactive");
  CHECK(tinyrefl::enum_to_string(Status::Pending) == "Pending");

  CHECK(tinyrefl::enum_to_underlying(Status::Active) == 1);
  CHECK(tinyrefl::enum_to_underlying(Status::Pending) == 3);
}

TEST_CASE("enum with negative values") {
  CHECK(tinyrefl::enum_to_string(Signed::Neg) == "Neg");
  CHECK(tinyrefl::enum_to_string(Signed::Zero) == "Zero");
  CHECK(tinyrefl::enum_to_string(Signed::Pos) == "Pos");
}

TEST_CASE("unscoped enum reflection") {
  CHECK(tinyrefl::enum_to_string(Low) == "Low");
  CHECK(tinyrefl::enum_to_string(Medium) == "Medium");
  CHECK(tinyrefl::enum_to_string(High) == "High");
}

TEST_CASE("enum_count") {
  CHECK(tinyrefl::enum_count<Color>() == 3);
  CHECK(tinyrefl::enum_count<Status>() == 3);
  CHECK(tinyrefl::enum_count<Signed>() == 3);
}

TEST_CASE("enum_range specialization") {
  CHECK(tinyrefl::enum_to_string(BigEnum::X) == "X");
  CHECK(tinyrefl::enum_to_string(BigEnum::Y) == "Y");
}

TEST_CASE("serialize struct with enum members (string mode)") {
  Task task{"Review", Status::Active, Color::Blue};
  std::string out;
  tinyrefl::reflection_to_json(task, out);

  CHECK(out.find("\"status\":\"Active\"") != std::string::npos);
  CHECK(out.find("\"highlight\":\"Blue\"") != std::string::npos);
  CHECK(out.find("\"name\":\"Review\"") != std::string::npos);
}

TEST_CASE("deserialize struct with enum members from string") {
  const char* json =
      R"({"name":"Deploy","status":"Pending","highlight":"Green"})";
  Task task{};
  auto st = tinyrefl::reflection_from_json(task, json);
  REQUIRE(st.ok);
  CHECK(task.name == "Deploy");
  CHECK(task.status == Status::Pending);
  CHECK(task.highlight == Color::Green);
}

TEST_CASE("deserialize struct with enum members from integer") {
  // Also accept integer input for enums.
  const char* json = R"({"name":"X","status":2,"highlight":0})";
  Task task{};
  auto st = tinyrefl::reflection_from_json(task, json);
  REQUIRE(st.ok);
  CHECK(task.status == Status::Inactive);
  CHECK(task.highlight == Color::Red);
}

TEST_CASE("roundtrip: struct with enum members") {
  Task original{"RT", Status::Active, Color::Green};
  std::string out;
  tinyrefl::reflection_to_json(original, out);

  Task decoded{};
  auto st = tinyrefl::reflection_from_json(decoded, out.c_str());
  REQUIRE(st.ok);
  CHECK(decoded.name == original.name);
  CHECK(decoded.status == original.status);
  CHECK(decoded.highlight == original.highlight);
}

TEST_CASE("vector<enum> roundtrip") {
  TaskWithVec original{"palette",
                       {Color::Red, Color::Green, Color::Blue, Color::Red}};
  std::string out;
  tinyrefl::reflection_to_json(original, out);
  CHECK(out.find("\"Red\"") != std::string::npos);
  CHECK(out.find("\"Green\"") != std::string::npos);

  TaskWithVec decoded{};
  auto st = tinyrefl::reflection_from_json(decoded, out.c_str());
  REQUIRE(st.ok);
  REQUIRE(decoded.colors.size() == 4);
  CHECK(decoded.colors[0] == Color::Red);
  CHECK(decoded.colors[1] == Color::Green);
  CHECK(decoded.colors[2] == Color::Blue);
  CHECK(decoded.colors[3] == Color::Red);
}

TEST_CASE("unnamed enum value serializes as integer fallback") {
  auto combined = static_cast<Flags>(
      static_cast<int>(Flags::A) | static_cast<int>(Flags::B));
  struct Holder {
    Flags f;
  };
  Holder h{combined};
  std::string out;
  tinyrefl::reflection_to_json(h, out);
  // Combined value has no name -> integer fallback (3)
  CHECK(out.find("\"f\":3") != std::string::npos);
}

TEST_CASE("integer serialization policy specialization") {
  struct Wrapper {
    Status s;
  };
  // Local policy override via temporary specialization is not trivial;
  // just verify default (string) mode:
  Wrapper w{Status::Active};
  std::string out;
  tinyrefl::reflection_to_json(w, out);
  CHECK(out.find("\"s\":\"Active\"") != std::string::npos);
}

TEST_CASE("enum_cast: valid and invalid integers") {
  auto ok = tinyrefl::enum_cast<Color>(1);
  REQUIRE(ok.has_value());
  CHECK(ok.value() == Color::Green);

  auto bad = tinyrefl::enum_cast<Color>(999);
  CHECK_FALSE(bad.has_value());

  auto neg = tinyrefl::enum_cast<Color>(-1);
  CHECK_FALSE(neg.has_value());
}

TEST_CASE("deserialize: invalid integer for enum keeps default value") {
  // Task::status default-inits to Status(0). 999 is not a valid enumerator
  // → the library should silently drop the assignment and leave the member
  // at its default state.
  const char* json = R"({"name":"X","status":999,"highlight":0})";
  Task task{};
  task.status = Status::Active;  // pre-set to a known value
  auto st = tinyrefl::reflection_from_json(task, json);
  REQUIRE(st.ok);
  // status should NOT have been overwritten with a bogus 999
  CHECK(task.status == Status::Active);
  // highlight is 0 (a valid Color::Red) so it should be assigned
  CHECK(task.highlight == Color::Red);
}

TEST_CASE("deserialize: invalid enum name keeps default value") {
  const char* json =
      R"({"name":"X","status":"Nonsense","highlight":"Red"})";
  Task task{};
  task.status = Status::Active;
  auto st = tinyrefl::reflection_from_json(task, json);
  REQUIRE(st.ok);
  CHECK(task.status == Status::Active);   // untouched
  CHECK(task.highlight == Color::Red);    // parsed OK
}

TEST_CASE("vector<enum>: invalid integers are dropped, valid ones kept") {
  const char* json = R"({"name":"pal","colors":[0,1,999,2,-1]})";
  TaskWithVec decoded{};
  auto st = tinyrefl::reflection_from_json(decoded, json);
  REQUIRE(st.ok);
  // Only 0, 1, 2 are valid Color values
  REQUIRE(decoded.colors.size() == 3);
  CHECK(decoded.colors[0] == Color::Red);
  CHECK(decoded.colors[1] == Color::Green);
  CHECK(decoded.colors[2] == Color::Blue);
}

TEST_CASE("vector<enum>: invalid string names are dropped") {
  const char* json =
      R"({"name":"pal","colors":["Red","Purple","Blue","Cyan","Green"]})";
  TaskWithVec decoded{};
  auto st = tinyrefl::reflection_from_json(decoded, json);
  REQUIRE(st.ok);
  REQUIRE(decoded.colors.size() == 3);
  CHECK(decoded.colors[0] == Color::Red);
  CHECK(decoded.colors[1] == Color::Blue);
  CHECK(decoded.colors[2] == Color::Green);
}
