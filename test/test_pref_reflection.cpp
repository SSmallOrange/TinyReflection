// perf_reflection.cpp
#include "tinyrefl/reflection_to_json.hpp"
#include "tinyrefl/reflection_from_json.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <random>

// --------------------- 测试用结构体 ---------------------

struct Inner {
    int id;
    std::string label;
};

struct Config {
    bool flag;
    double ratio;
    std::vector<int> values;
    Inner inner;
    std::vector<Inner> inner_list;
};

struct Complex {
    std::string name;
    Config config;
    std::vector<std::vector<int>> matrix;
    std::vector<std::vector<Inner>> inner_matrix;
};

// --------------------- 简单造数据函数 ---------------------

Complex MakeComplex(std::size_t idx) {
    Complex obj;
    obj.name = "Complex_" + std::to_string(idx);

    obj.config.flag = (idx % 2 == 0);
    obj.config.ratio = 3.14 + static_cast<double>(idx) * 0.001;

    // values
    obj.config.values.clear();
    for (int i = 0; i < 10; ++i) {
        obj.config.values.push_back(static_cast<int>(idx * 10 + i));
    }

    // inner
    obj.config.inner.id = static_cast<int>(idx);
    obj.config.inner.label = "Inner_" + std::to_string(idx);

    // inner_list
    obj.config.inner_list.clear();
    for (int i = 0; i < 5; ++i) {
        Inner in;
        in.id = static_cast<int>(idx * 100 + i);
        in.label = "List_" + std::to_string(idx) + "_" + std::to_string(i);
        obj.config.inner_list.push_back(std::move(in));
    }

    // matrix: 3x3
    obj.matrix.assign(3, std::vector<int>(3, 0));
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            obj.matrix[r][c] = static_cast<int>(idx * 1000 + r * 10 + c);
        }
    }

    // inner_matrix: 2x2
    obj.inner_matrix.assign(2, std::vector<Inner>(2));
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            auto& in = obj.inner_matrix[r][c];
            in.id = static_cast<int>(idx * 10000 + r * 10 + c);
            in.label = "M_" + std::to_string(idx) + "_" + std::to_string(r) + "_" + std::to_string(c);
        }
    }

    return obj;
}

// --------------------- 计时工具 ---------------------

using Clock = std::chrono::high_resolution_clock;

template <typename F>
double MeasureMs(F&& fn) {
    auto start = Clock::now();
    fn();
    auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// --------------------- 性能测试 ---------------------

int main() {
    // 可按需要调大 / 调小
    const std::size_t N_OBJECTS = 1000000;  // 对象个数
    const int N_ROUNDS = 1;                 // 正式测试轮数

    std::cout << "TinyReflection JSON serialize/deserialize benchmark\n";
    std::cout << "Objects: " << N_OBJECTS << "\n";
    std::cout << "Benchmark rounds: " << N_ROUNDS << "\n\n";

    // 1. 预先构造一批 Complex 对象
    std::vector<Complex> objects;
    objects.reserve(N_OBJECTS);
    for (std::size_t i = 0; i < N_OBJECTS; ++i) {
        objects.push_back(MakeComplex(i));
    }

    // 保存序列化结果，用于反序列化测试
    std::vector<std::string> jsonStrings;
    jsonStrings.reserve(N_OBJECTS);

    // ---------------- 序列化性能测试 ----------------
    double totalSerializeMs = 0.0;

    for (int r = 0; r < N_ROUNDS; ++r) {
        jsonStrings.clear();
        jsonStrings.reserve(N_OBJECTS);

        double elapsed = MeasureMs([&] {
            for (const auto& obj : objects) {
                std::string out;
                tinyrefl::reflection_to_json(obj, out);
                jsonStrings.emplace_back(std::move(out));
            }
        });

        totalSerializeMs += elapsed;
        std::cout << "[Round " << r << "] serialize: "
                  << elapsed << " ms\n";
    }

    double avgSerializeMs = totalSerializeMs / N_ROUNDS;
    double serializeOpsPerSec = (N_OBJECTS * 1000.0) / avgSerializeMs;

    std::cout << "\nSerialize avg: " << avgSerializeMs << " ms, "
              << serializeOpsPerSec << " objs/s\n\n";

    // ---------------- 反序列化性能测试 ----------------
    double totalDeserializeMs = 0.0;

    for (int r = 0; r < N_ROUNDS; ++r) {
        // 确保每轮都真正解析 N_OBJECTS 次
        double elapsed = MeasureMs([&] {
            for (const auto& js : jsonStrings) {
                auto [ok, res] = tinyrefl::reflection_from_json<Complex>(js.c_str());
            }
        });

        totalDeserializeMs += elapsed;
        std::cout << "[Round " << r << "] deserialize: "
                  << elapsed << " ms\n";
    }

    double avgDeserializeMs = totalDeserializeMs / N_ROUNDS;
    double deserializeOpsPerSec = (N_OBJECTS * 1000.0) / avgDeserializeMs;

    std::cout << "\nDeserialize avg: " << avgDeserializeMs << " ms, "
              << deserializeOpsPerSec << " objs/s\n";

    return 0;
}
