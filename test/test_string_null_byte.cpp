#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <string>
#include <vector>

using namespace std;

struct StrField {
  string s;
};
struct TwoStrFields {
  string a;
  string b;
};
struct WithVecStr {
  vector<string> items;
};

// ============================================================
// BUG: std::string field assigned via "member = str" (const char*)
// truncates at the first embedded null byte (\u0000).
// RapidJSON provides the correct 'length' in the String() callback,
// but the library ignores it and uses operator=(const char*).
// Fix: use member.assign(str, length) instead of member = str.
// ============================================================

TEST_CASE("BUG: string field with embedded \\u0000 is truncated") {
  // JSON encodes a null byte as \u0000
  // The resulting string should have 11 chars: "hello\0world"
  const char* json = R"({"s": "hello\u0000world"})";

  StrField restored{};
  auto st = tinyrefl::reflection_from_json(restored, json);
  CHECK(st.ok == true);

  // With the bug: s.size() == 5 ("hello" only, truncated at \0)
  // After fix:    s.size() == 11 ("hello\0world")
  CHECK(restored.s.size() == 11);
  CHECK(restored.s[0] == 'h');
  CHECK(restored.s[4] == 'o');
  CHECK(restored.s[5] == '\0');
  CHECK(restored.s[6] == 'w');
  CHECK(restored.s[10] == 'd');
}

TEST_CASE(
    "BUG: second string field after null-embedded first is also correct") {
  // Verifies that even if the first field has an embedded null,
  // the second string field is correctly parsed afterward.
  const char* json = R"({"a": "prefix\u0000suffix", "b": "normal"})";

  TwoStrFields restored{};
  auto st = tinyrefl::reflection_from_json(restored, json);
  CHECK(st.ok == true);
  CHECK(restored.a.size() == 13);  // "prefix" + '\0' + "suffix" = 6+1+6 = 13
  CHECK(restored.a[6] == '\0');
  CHECK(restored.b == "normal");
}

TEST_CASE("CONTROL: normal string without null bytes works correctly") {
  const char* json = R"({"s": "hello world"})";
  StrField restored{};
  auto st = tinyrefl::reflection_from_json(restored, json);
  CHECK(st.ok == true);
  CHECK(restored.s == "hello world");
  CHECK(restored.s.size() == 11);
}

TEST_CASE("BUG: vector<string> element with embedded null byte") {
  // An element in a string vector that has an embedded null
  const char* json = R"({"items": ["abc\u0000def", "normal"]})";

  WithVecStr restored{};
  auto st = tinyrefl::reflection_from_json(restored, json);
  CHECK(st.ok == true);
  REQUIRE(restored.items.size() == 2);

  // First element: "abc\0def" = 7 chars
  CHECK(restored.items[0].size() == 7);
  CHECK(restored.items[0][3] == '\0');
  CHECK(restored.items[0][4] == 'd');

  // Second element: normal
  CHECK(restored.items[1] == "normal");
}
