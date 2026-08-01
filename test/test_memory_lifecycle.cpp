// test_memory_lifecycle.cpp
//
// 验证 DispatchHandler 的 handler 内存生命周期。
//
// 方法：
//   利用 IHandler::on_construct / IHandler::on_destruct 钩子，
//   精确统计 IHandler 派生类（ReaderHandler / SequenceReaderHandler）
//   的构造次数与析构次数。
//
//   若修复正确（pop_handler 调用了 delete），则每次反序列化结束后：
//       构造次数 == 析构次数  =>  net == 0
//
//   若存在泄漏（pop_handler 没有 delete），被 pop 出去的 handler 不会
//   在 DispatchHandler 析构时再被 delete，导致：
//       析构次数 < 构造次数  =>  net > 0
//
// ============================================================

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "tinyrefl/reflection_from_json.hpp"
#include "tinyrefl/reflection_to_json.hpp"

#include <atomic>
#include <vector>
#include <string>

using namespace std;

// ============================================================
// 全局计数器 + RAII 追踪 guard
// ============================================================
namespace {
    atomic<int> g_ctor{0};
    atomic<int> g_dtor{0};

    void on_ctor() { ++g_ctor; }
    void on_dtor() { ++g_dtor; }

    struct LifecycleGuard {
        LifecycleGuard() {
            g_ctor.store(0);
            g_dtor.store(0);
            tinyrefl::detail::IHandler::on_construct = on_ctor;
            tinyrefl::detail::IHandler::on_destruct  = on_dtor;
        }
        ~LifecycleGuard() {
            // 关闭钩子，避免干扰其他测试
            tinyrefl::detail::IHandler::on_construct = nullptr;
            tinyrefl::detail::IHandler::on_destruct  = nullptr;
        }
        int ctor() const { return g_ctor.load(); }
        int dtor() const { return g_dtor.load(); }
        int net()  const { return ctor() - dtor(); }
    };
}

// ============================================================
// 测试结构体
// ============================================================
struct Pt2   { int x; int y; };
struct Seg2  { Pt2 a; Pt2 b; };
struct Row3  { string name; vector<int> vals; };
struct Tbl2  { string title; vector<Row3> rows; int ver; };
struct WithVec2 { vector<int> data; };

// ============================================================
// CASE 1：简单 struct（只有根 handler，无 pop）
//
// 期望：
//   ctor = 1（根 ReaderHandler<Pt2>）
//   dtor = 1（DispatchHandler 析构时 delete 根 handler）
//   net  = 0
// ============================================================
TEST_CASE("lifecycle - simple struct: ctor == dtor") {
    Pt2 obj{};
    const char* json = R"({"x":1,"y":2})";

    LifecycleGuard g;
    auto st = tinyrefl::reflection_from_json(obj, json);
    CHECK(st.ok);
    CHECK(obj.x == 1); CHECK(obj.y == 2);

    INFO("ctor=" << g.ctor() << "  dtor=" << g.dtor() << "  net=" << g.net());
    CHECK(g.ctor() >= 1);   // 至少 1 个根 handler 被构造
    CHECK(g.net() == 0);    // 每个构造都有对应析构
}

// ============================================================
// CASE 2：含两个嵌套 struct（Seg2: Pt2 a, Pt2 b）
//
// 期望：
//   ctor = 3（根 + ReaderHandler<Pt2> for a + ReaderHandler<Pt2> for b）
//   dtor = 3
//   net  = 0
//
//   【修复前】pop_handler 不 delete：
//     a 的 handler EndObject 后被 pop 但不 delete  ->  dtor 只有 2
//     b 的 handler EndObject 后被 pop 但不 delete  ->  dtor 只有 1
//     根 handler 由 ~DispatchHandler delete          ->  dtor = 1
//     net = 3 - 1 = 2  (泄漏 2 个)
// ============================================================
TEST_CASE("lifecycle - two nested structs: ctor == dtor") {
    Seg2 obj{};
    const char* json = R"({"a":{"x":1,"y":2},"b":{"x":3,"y":4}})";

    LifecycleGuard g;
    auto st = tinyrefl::reflection_from_json(obj, json);
    CHECK(st.ok);
    CHECK(obj.a.x == 1); CHECK(obj.b.x == 3);

    INFO("ctor=" << g.ctor() << "  dtor=" << g.dtor() << "  net=" << g.net());
    CHECK(g.ctor() == 3);   // 根 + handler for a + handler for b
    CHECK(g.dtor() == 3);
    CHECK(g.net() == 0);
}

// ============================================================
// CASE 3：含 vector<int>（SequenceReaderHandler 被 push/pop）
//
// 期望：
//   ctor = 2（根 + SequenceReaderHandler<vector<int>>）
//   dtor = 2
//   net  = 0
//
//   【修复前】：EndArray 后 pop_handler 不 delete，net = 1
// ============================================================
TEST_CASE("lifecycle - vector field: ctor == dtor") {
    WithVec2 obj{};
    const char* json = R"({"data":[1,2,3,4,5]})";

    LifecycleGuard g;
    auto st = tinyrefl::reflection_from_json(obj, json);
    CHECK(st.ok);
    REQUIRE(obj.data.size() == 5);

    INFO("ctor=" << g.ctor() << "  dtor=" << g.dtor() << "  net=" << g.net());
    CHECK(g.ctor() == 2);
    CHECK(g.dtor() == 2);
    CHECK(g.net() == 0);
}

// ============================================================
// CASE 4：table（嵌套 struct 含 vector，vector 含嵌套 struct）
//
// Tbl2: 根 handler
//   vector<Row3>: SequenceReaderHandler<vector<Row3>>
//     Row3[0]: ReaderHandler<Row3>
//       vector<int>: SequenceReaderHandler<vector<int>>
//     Row3[1]: ReaderHandler<Row3>
//       vector<int>: SequenceReaderHandler<vector<int>>
//     Row3[2]: ReaderHandler<Row3>
//       vector<int>: SequenceReaderHandler<vector<int>>
//
// ctor = 1(根) + 1(vec<Row3>) + 3*( 1(ReaderHandler<Row3>) + 1(vec<int>) ) = 8
// dtor = 8, net = 0
//
// 【修复前】：每个被 pop 的 handler 都泄漏，net 会是较大正数
// ============================================================
TEST_CASE("lifecycle - table with nested rows: ctor == dtor") {
    Tbl2 obj{};
    const char* json = R"({
        "title": "t",
        "rows": [
            {"name":"r0","vals":[10,20,30]},
            {"name":"r1","vals":[40,50]},
            {"name":"r2","vals":[]}
        ],
        "ver": 1
    })";

    LifecycleGuard g;
    auto st = tinyrefl::reflection_from_json(obj, json);
    CHECK(st.ok);
    REQUIRE(obj.rows.size() == 3);
    CHECK(obj.rows[0].name == "r0");
    CHECK(obj.ver == 1);

    INFO("ctor=" << g.ctor() << "  dtor=" << g.dtor() << "  net=" << g.net());
    // 无论具体数值如何，最重要的断言是 net == 0
    CHECK(g.ctor() > 1);     // 必然有多个 handler 被创建
    CHECK(g.net() == 0);     // 每个 new 都有对应 delete
}

// ============================================================
// CASE 5：连续 10 次调用，每次 net 均为 0（无累积泄漏）
// ============================================================
TEST_CASE("lifecycle - repeated calls: no cumulative leak") {
    for (int i = 0; i < 10; ++i) {
        Seg2 obj{};
        string json = R"({"a":{"x":)" + to_string(i) +
                      R"(,"y":0},"b":{"x":0,"y":)" + to_string(i) + R"(}})";

        LifecycleGuard g;
        auto st = tinyrefl::reflection_from_json(obj, json.c_str());
        CHECK(st.ok);

        INFO("iter=" << i << "  ctor=" << g.ctor() << "  dtor=" << g.dtor() << "  net=" << g.net());
        CHECK(g.net() == 0);
    }
}

// ============================================================
// CASE 6：空 vector —— SequenceReaderHandler 仍然被 push/pop
// ============================================================
struct EmptyVecStruct { vector<int> data; string tag; };

TEST_CASE("lifecycle - empty vector field: ctor == dtor") {
    EmptyVecStruct obj{};
    const char* json = R"({"data":[],"tag":"end"})";

    LifecycleGuard g;
    auto st = tinyrefl::reflection_from_json(obj, json);
    CHECK(st.ok);
    CHECK(obj.data.empty());
    CHECK(obj.tag == "end");

    INFO("ctor=" << g.ctor() << "  dtor=" << g.dtor() << "  net=" << g.net());
    CHECK(g.net() == 0);
}
