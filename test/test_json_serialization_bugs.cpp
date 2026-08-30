#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <cmath>
#include <map>
#include <string>
#include <vector>

using namespace std;

// === Test Structures ===

struct SimpleStr {
  string text;
};

struct TwoStrings {
  string a;
  string b;
};

struct WithInt {
  int x;
  int y;
};

struct Nested {
  int id;
  SimpleStr inner;
};

struct WithVec {
  vector<string> items;
};

struct WithMap {
  map<string, int> data;
};

struct WithDouble {
  double val;
};

struct WithUnsigned {
  unsigned int count;
};

struct MixedTypes {
  int num;
  string text;
  bool flag;
  double ratio;
  vector<int> list;
};

// ============================================================
// BUG CANDIDATE 1: String with special characters in to_json
// std::string serialization doesn't escape special chars
// ============================================================

TEST_CASE("to_json - string with double quotes") {
  SimpleStr s{"say \"hello\""};
  string json;
  tinyrefl::reflection_to_json(s, json);

  // The JSON should have escaped quotes: "say \"hello\""
  // If not escaped, the JSON will be invalid: {"text":"say "hello""}
  // Try to parse it back
  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "say \"hello\"");
}

TEST_CASE("to_json - string with backslash") {
  SimpleStr s{"path\\to\\file"};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "path\\to\\file");
}

TEST_CASE("to_json - string with newline") {
  SimpleStr s{"line1\nline2"};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "line1\nline2");
}

TEST_CASE("to_json - string with tab") {
  SimpleStr s{"col1\tcol2"};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "col1\tcol2");
}

TEST_CASE("to_json - string with carriage return") {
  SimpleStr s{"line1\r\nline2"};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "line1\r\nline2");
}

TEST_CASE("to_json - string with control characters") {
  // Note: strings with embedded \0 cannot fully roundtrip via const char*
  // assignment Test only non-null control characters
  SimpleStr s{string("ctrl\x01\x02\x03 chars")};
  string json;
  tinyrefl::reflection_to_json(s, json);

  // Should produce valid JSON with \u0001 etc.
  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == s.text);
}

TEST_CASE("to_json - string with all JSON special chars") {
  SimpleStr s{
      "quote:\" backslash:\\ slash:/ backspace:\b formfeed:\f newline:\n "
      "return:\r tab:\t"};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == s.text);
}

// ============================================================
// BUG CANDIDATE 2: Nested struct string escaping roundtrip
// ============================================================

TEST_CASE("roundtrip - nested struct with special string") {
  Nested n{42, {"hello \"world\""}};
  string json;
  tinyrefl::reflection_to_json(n, json);

  Nested restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.id == 42);
  CHECK(restored.inner.text == "hello \"world\"");
}

// ============================================================
// BUG CANDIDATE 3: Vector of strings with special chars
// ============================================================

TEST_CASE("roundtrip - vector of strings with special chars") {
  WithVec wv{{"normal", "with \"quotes\"", "with\nnewline", "back\\slash"}};
  string json;
  tinyrefl::reflection_to_json(wv, json);

  WithVec restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  REQUIRE(restored.items.size() == 4);
  CHECK(restored.items[0] == "normal");
  CHECK(restored.items[1] == "with \"quotes\"");
  CHECK(restored.items[2] == "with\nnewline");
  CHECK(restored.items[3] == "back\\slash");
}

// ============================================================
// BUG CANDIDATE 4: Map serialization/deserialization
// ============================================================

TEST_CASE("roundtrip - map string to int") {
  WithMap wm{};
  wm.data["alpha"] = 1;
  wm.data["beta"] = 2;
  wm.data["gamma"] = 3;

  string json;
  tinyrefl::reflection_to_json(wm, json);

  // Check JSON is valid
  CHECK(json.find("\"alpha\"") != string::npos);
  CHECK(json.find("\"beta\"") != string::npos);
}

// ============================================================
// BUG CANDIDATE 5: Unsigned integer handling
// ============================================================

TEST_CASE("roundtrip - unsigned int") {
  WithUnsigned wu{4294967295u};  // UINT_MAX
  string json;
  tinyrefl::reflection_to_json(wu, json);

  WithUnsigned restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.count == 4294967295u);
}

TEST_CASE("from_json - unsigned int from JSON") {
  const char* json = R"({"count": 12345})";
  WithUnsigned wu{};
  auto status = tinyrefl::reflection_from_json(wu, json);
  CHECK(status.ok == true);
  CHECK(wu.count == 12345);
}

// ============================================================
// BUG CANDIDATE 6: Double precision roundtrip
// ============================================================

TEST_CASE("roundtrip - double precision") {
  WithDouble wd{3.141592653589793};
  string json;
  tinyrefl::reflection_to_json(wd, json);

  WithDouble restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(std::abs(restored.val - 3.141592653589793) < 1e-10);
}

TEST_CASE("roundtrip - double negative") {
  WithDouble wd{-123.456};
  string json;
  tinyrefl::reflection_to_json(wd, json);

  WithDouble restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(std::abs(restored.val - (-123.456)) < 1e-10);
}

TEST_CASE("roundtrip - double zero") {
  WithDouble wd{0.0};
  string json;
  tinyrefl::reflection_to_json(wd, json);

  WithDouble restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.val == 0.0);
}

// ============================================================
// BUG CANDIDATE 7: Mixed types roundtrip
// ============================================================

TEST_CASE("roundtrip - mixed types") {
  MixedTypes mt{42, "hello", true, 2.718, {1, 2, 3}};
  string json;
  tinyrefl::reflection_to_json(mt, json);

  MixedTypes restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.num == 42);
  CHECK(restored.text == "hello");
  CHECK(restored.flag == true);
  CHECK(std::abs(restored.ratio - 2.718) < 1e-10);
  REQUIRE(restored.list.size() == 3);
  CHECK(restored.list[0] == 1);
  CHECK(restored.list[1] == 2);
  CHECK(restored.list[2] == 3);
}

// ============================================================
// BUG CANDIDATE 8: Empty string
// ============================================================

TEST_CASE("roundtrip - empty string") {
  SimpleStr s{""};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  restored.text = "not_empty";
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "");
}

// ============================================================
// BUG CANDIDATE 9: String with only special chars
// ============================================================

TEST_CASE("roundtrip - string is just a quote") {
  SimpleStr s{"\""};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == "\"");
}

// ============================================================
// BUG CANDIDATE 10: Multiple strings - verify correct assignment
// ============================================================

TEST_CASE("roundtrip - two strings") {
  TwoStrings ts{"first", "second"};
  string json;
  tinyrefl::reflection_to_json(ts, json);

  TwoStrings restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.a == "first");
  CHECK(restored.b == "second");
}

// ============================================================
// BUG CANDIDATE 11: String containing JSON-like content
// ============================================================

TEST_CASE("roundtrip - string containing JSON content") {
  SimpleStr s{R"({"key": "value", "num": 123})"};
  string json;
  tinyrefl::reflection_to_json(s, json);

  SimpleStr restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.text == R"({"key": "value", "num": 123})");
}

// ============================================================
// BUG CANDIDATE 12: Large vectors
// ============================================================

TEST_CASE("roundtrip - large vector") {
  WithVec wv{};
  for (int i = 0; i < 100; i++) {
    wv.items.push_back("item_" + to_string(i));
  }

  string json;
  tinyrefl::reflection_to_json(wv, json);

  WithVec restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  REQUIRE(restored.items.size() == 100);
  CHECK(restored.items[0] == "item_0");
  CHECK(restored.items[99] == "item_99");
}

// ============================================================
// BUG CANDIDATE 13: Negative int roundtrip
// ============================================================

TEST_CASE("roundtrip - negative int") {
  WithInt wi{-2147483648, 2147483647};  // INT_MIN, INT_MAX
  string json;
  tinyrefl::reflection_to_json(wi, json);

  WithInt restored{};
  auto status = tinyrefl::reflection_from_json(restored, json.c_str());
  CHECK(status.ok == true);
  CHECK(restored.x == -2147483648);
  CHECK(restored.y == 2147483647);
}
