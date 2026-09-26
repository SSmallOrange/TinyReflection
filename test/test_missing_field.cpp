#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tinyrefl/reflection_from_json.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Test types
// ---------------------------------------------------------------------------

struct Inner {
  int id;
  std::string label;
};

struct Config {
  bool flag;
  std::optional<double> ratio;  // optional, exempt from strict mode
  Inner inner;                  // required struct; its own fields are
                                 // required too
};

struct AllOptional {
  std::optional<int> a;
  std::optional<std::string> b;
};

struct AllRequired {
  int a;
  std::string b;
};

struct VecOfInner {
  std::vector<Inner> items;  // container element struct is required
};

struct MapOfInner {
  std::map<std::string, Inner> items;  // map value struct is required
};

struct OptInner {
  std::optional<Inner> inner;  // optional struct: entire field may be
                                // absent, but its own fields are required
                                // when the object *is* present
};

// ---------------------------------------------------------------------------
// Loose mode (default): existing behavior, unaffected by this feature
// ---------------------------------------------------------------------------

TEST_CASE("loose mode: missing required field keeps default value") {
  Config obj{};
  auto st = tinyrefl::reflection_from_json(
      obj, R"({"inner":{"id":1,"label":"A"}})");
  REQUIRE(st.ok);
  CHECK(obj.flag == false);
  CHECK_FALSE(obj.ratio.has_value());
  CHECK(obj.inner.id == 1);
}

TEST_CASE("loose mode is the default when mode argument is omitted") {
  Config obj{};
  auto st = tinyrefl::reflection_from_json(obj, R"({})");
  CHECK(st.ok);
}

// ---------------------------------------------------------------------------
// Strict mode: missing required field is reported
// ---------------------------------------------------------------------------

TEST_CASE("strict mode: missing required field fails with field name") {
  Config obj;
  const char* json = R"({"inner":{"id":1,"label":"A"}})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st.ok);
  CHECK(st.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  CHECK(st.error.field_name == "flag");
}

TEST_CASE("strict mode: all required fields present succeeds") {
  Config obj;
  const char* json = R"({"flag":true,"inner":{"id":1,"label":"A"}})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE(st.ok);
  CHECK(obj.flag == true);
  CHECK_FALSE(obj.ratio.has_value());
  CHECK(obj.inner.id == 1);
  CHECK(obj.inner.label == "A");
}

TEST_CASE("strict mode: optional field missing does not fail") {
  Config obj;
  const char* json = R"({"flag":true,"inner":{"id":1,"label":"A"}})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE(st.ok);
  CHECK_FALSE(obj.ratio.has_value());
}

TEST_CASE("strict mode: all-optional struct always succeeds even when empty") {
  AllOptional obj;
  auto st =
      tinyrefl::reflection_from_json(obj, R"({})", tinyrefl::ParseMode::Strict);
  REQUIRE(st.ok);
  CHECK_FALSE(obj.a.has_value());
  CHECK_FALSE(obj.b.has_value());
}

TEST_CASE("strict mode: all-required struct fails on first missing field") {
  AllRequired obj;
  auto st =
      tinyrefl::reflection_from_json(obj, R"({})", tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st.ok);
  CHECK(st.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  // Only "a" or "b" is guaranteed missing; either is an acceptable first hit.
  CHECK((st.error.field_name == "a" || st.error.field_name == "b"));
}

// ---------------------------------------------------------------------------
// Nested struct: required checks apply at every depth
// ---------------------------------------------------------------------------

TEST_CASE("strict mode: nested required struct missing a field fails") {
  Config obj;
  const char* json = R"({"flag":true,"inner":{"id":1}})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st.ok);
  CHECK(st.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  CHECK(st.error.field_name == "label");
}

TEST_CASE("strict mode: vector<struct> element missing a field fails") {
  VecOfInner obj;
  const char* json = R"({"items":[{"id":1,"label":"A"},{"id":2}]})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st.ok);
  CHECK(st.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  CHECK(st.error.field_name == "label");
}

TEST_CASE("strict mode: map<string, struct> value missing a field fails") {
  MapOfInner obj;
  const char* json = R"({"items":{"a":{"id":1,"label":"A"},"b":{"id":2}}})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st.ok);
  CHECK(st.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  CHECK(st.error.field_name == "label");
}

TEST_CASE(
    "strict mode: optional<struct> absent entirely is fine, "
    "but present-and-incomplete still fails") {
  OptInner absent;
  auto st = tinyrefl::reflection_from_json(absent, R"({})",
                                            tinyrefl::ParseMode::Strict);
  REQUIRE(st.ok);
  CHECK_FALSE(absent.inner.has_value());

  OptInner incomplete;
  auto st2 = tinyrefl::reflection_from_json(
      incomplete, R"({"inner":{"id":1}})", tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st2.ok);
  CHECK(st2.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  CHECK(st2.error.field_name == "label");
}

// ---------------------------------------------------------------------------
// Fail-fast: only the first (deepest) missing field is reported
// ---------------------------------------------------------------------------

TEST_CASE("strict mode: multiple missing fields report only the deepest one") {
  Config obj;
  // "flag" is missing at the root, and "label" is missing in the nested
  // "inner" object. EndObject fires depth-first (innermost object first),
  // so the nested "label" is discovered and reported before the root
  // ever gets to check "flag".
  const char* json = R"({"inner":{"id":1}})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE_FALSE(st.ok);
  CHECK(st.error.kind == tinyrefl::ErrorKind::MissingRequiredField);
  CHECK(st.error.field_name == "label");
}

// ---------------------------------------------------------------------------
// null semantics: a present key with value null still counts as "seen"
// ---------------------------------------------------------------------------

TEST_CASE("strict mode: required field explicitly set to null satisfies the "
          "required check") {
  AllRequired obj{42, "unset"};
  const char* json = R"({"a":null,"b":"x"})";
  auto st = tinyrefl::reflection_from_json(obj, json,
                                            tinyrefl::ParseMode::Strict);
  REQUIRE(st.ok);
  // Null on a non-optional scalar is a no-op assignment (pre-existing
  // behavior, unrelated to this feature): the key being present is enough
  // to satisfy the required check, even though the value is left untouched.
  CHECK(obj.a == 42);
  CHECK(obj.b == "x");
}

// ---------------------------------------------------------------------------
// Second overload (returns pair<bool, T>) also accepts and honors mode
// ---------------------------------------------------------------------------

TEST_CASE("pair-returning overload: strict mode missing field yields false") {
  auto [ok, obj] = tinyrefl::reflection_from_json<Config>(
      R"({"inner":{"id":1,"label":"A"}})", tinyrefl::ParseMode::Strict);
  CHECK_FALSE(ok);
}

TEST_CASE("pair-returning overload: defaults to loose mode") {
  auto [ok, obj] =
      tinyrefl::reflection_from_json<Config>(R"({"inner":{"id":1,"label":"A"}})");
  CHECK(ok);
  CHECK(obj.flag == false);
}

TEST_CASE("pair-returning overload: strict mode success case") {
  auto [ok, obj] = tinyrefl::reflection_from_json<Config>(
      R"({"flag":true,"inner":{"id":1,"label":"A"}})",
      tinyrefl::ParseMode::Strict);
  CHECK(ok);
  CHECK(obj.flag == true);
}
