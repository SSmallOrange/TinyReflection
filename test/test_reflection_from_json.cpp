#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <string>
#include <vector>

using namespace std;

struct Inner {
  int id;
  string label;
};

struct Config {
  bool flag;
  double ratio;
  vector<int> values;
  Inner inner;
  vector<Inner> inner_list;
};

struct Complex {
  string name;
  Config config;
  vector<vector<int>> matrix;
  vector<vector<Inner>> inner_matrix;
  tinyrefl::ignore<std::shared_ptr<Inner>> ptr;  // 被忽略的字段
};

TEST_CASE("reflection_from_json - parse succeeds") {
  const char* json = R"(
        {
            "name": "TestComplex",
            "config": {
                "flag": true,
                "ratio": 3.1415,
                "values": [10, 20, 30],
                "inner": {
                    "id": 42,
                    "label": "InnerLabel"
                },
                "inner_list": [
                    { "id": 1, "label": "A" },
                    { "id": 2, "label": "B" },
                    { "id": 3, "label": "C" }
                ]
            },
            "matrix": [
                [1, 2, 3],
                [4, 5, 6]
            ],
            "inner_matrix": [
                [
                    { "id": 101, "label": "X" },
                    { "id": 102, "label": "Y" }
                ],
                [
                    { "id": 201, "label": "Z" },
                    { "id": 202, "label": "W" }
                ]
            ]
        }
    )";

  auto [ok, res] = tinyrefl::reflection_from_json<Complex>(json);

  // 解析应当返回成功
  CHECK(ok == true);

  // ignore 字段应保持默认值 nullptr
  CHECK(res.ptr.get() == nullptr);
}

TEST_CASE("reflection_from_json - roundtrip to_json does not crash") {
  const char* json = R"(
        {
            "name": "TestComplex",
            "config": {
                "flag": true,
                "ratio": 3.1415,
                "values": [10, 20, 30],
                "inner": {
                    "id": 42,
                    "label": "InnerLabel"
                },
                "inner_list": [
                    { "id": 1, "label": "A" }
                ]
            },
            "matrix": [[1, 2, 3]],
            "inner_matrix": [[{ "id": 101, "label": "X" }]]
        }
    )";

  auto [ok, res] = tinyrefl::reflection_from_json<Complex>(json);
  CHECK(ok == true);

  // 序列化回 JSON 应不崩溃
  std::string output;
  tinyrefl::reflection_to_json(res, output);
  CHECK(!output.empty());
}
