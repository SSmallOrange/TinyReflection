#include "tinyrefl/reflection.hpp"

#include <iostream>
#include <string>
#include <cassert>
#include <type_traits>

// 测试结构体定义
struct Test1 { int a; };
struct Test2 { double x; double y; };
struct Test3 { std::string name; int age; bool active; };
struct Test4 { char c; short s; int i; long l; };
struct Test5 { float f1; float f2; float f3; float f4; float f5; };

// 测试函数
void run_reflection_tests() {
    std::cout << "===== Starting Reflection Tests =====" << std::endl;

    // 测试1: 单个成员结构体
    {
        Test1 t1{42};
        auto& ref = tinyrefl::struct_member_reference<0>(t1);
        static_assert(std::is_same_v<decltype(ref), int&>);
        
        ref = 100;
        assert(t1.a == 100);
        std::cout << "Test1 passed: " << t1.a << std::endl;
    }

    // 测试2: 两个成员结构体
    {
        Test2 t2{3.14, 2.71};
        auto& ref_x = tinyrefl::struct_member_reference<0>(t2);
        auto& ref_y = tinyrefl::struct_member_reference<1>(t2);
        static_assert(std::is_same_v<decltype(ref_x), double&>);
        static_assert(std::is_same_v<decltype(ref_y), double&>);
        
        ref_x = 1.0;
        ref_y = 2.0;
        assert(t2.x == 1.0 && t2.y == 2.0);
        std::cout << "Test2 passed: " << t2.x << ", " << t2.y << std::endl;
    }

    // 测试3: 三个成员结构体（边界情况）
    {
        Test3 t3{"Alice", 30, true};
        auto& name_ref = tinyrefl::struct_member_reference<0>(t3);
        auto& age_ref = tinyrefl::struct_member_reference<1>(t3);
        auto& active_ref = tinyrefl::struct_member_reference<2>(t3);
        static_assert(std::is_same_v<decltype(name_ref), std::string&>);
        static_assert(std::is_same_v<decltype(age_ref), int&>);
        static_assert(std::is_same_v<decltype(active_ref), bool&>);
        
        name_ref = "Bob";
        age_ref = 25;
        active_ref = false;
        
        assert(t3.name == "Bob");
        assert(t3.age == 25);
        assert(t3.active == false);
        std::cout << "Test3 passed: " << t3.name << ", " << t3.age << ", " << t3.active << std::endl;
    }

    // 测试4: 四个成员结构体
    {
        Test4 t4{'A', 10, 20, 30L};
        auto c_ref = tinyrefl::struct_member_reference<0>(t4);
        auto s_ref = tinyrefl::struct_member_reference<1>(t4);
        auto i_ref = tinyrefl::struct_member_reference<2>(t4);
        auto l_ref = tinyrefl::struct_member_reference<3>(t4);
        
        c_ref = 'B';
        s_ref = 100;
        i_ref = 200;
        l_ref = 300L;
        
        assert(t4.c == 'B');
        assert(t4.s == 100);
        assert(t4.i == 200);
        assert(t4.l == 300L);
        std::cout << "Test4 passed" << std::endl;
    }

    // 测试5: 五个成员结构体（边界情况）
    {
        Test5 t5{1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
        auto f1_ref = tinyrefl::struct_member_reference<0>(t5);
        auto f5_ref = tinyrefl::struct_member_reference<4>(t5);
        
        f1_ref = 10.0f;
        f5_ref = 50.0f;
        
        assert(t5.f1 == 10.0f);
        assert(t5.f5 == 50.0f);
        std::cout << "Test5 passed" << std::endl;
    }

    // 测试6: const 对象
    {
        const Test3 const_t3{"Charlie", 40, true};
        const auto& name_ref = tinyrefl::struct_member_reference<0>(const_t3);
        const auto& age_ref = tinyrefl::struct_member_reference<1>(const_t3);
        static_assert(std::is_same_v<decltype(name_ref), const std::string&>);
        static_assert(std::is_same_v<decltype(age_ref), const int&>);
        
        assert(name_ref == "Charlie");
        assert(age_ref == 40);
        std::cout << "Test6 (const) passed" << std::endl;
    }

    // 测试7: 临时对象（右值）
    {
        auto&& name_ref = tinyrefl::struct_member_reference<0>(Test3{"Dave", 50, false});
        // 注意：临时对象的引用只在表达式生命周期内有效
        assert(name_ref == "Dave");
        std::cout << "Test7 (rvalue) passed" << std::endl;
    }

    // 测试8: 成员类型验证
    {
        Test3 t3;
        static_assert(std::is_same_v<
            decltype(tinyrefl::struct_member_reference<0>(t3)),
            std::string&
        >);
        static_assert(!std::is_same_v<
            decltype(tinyrefl::struct_member_reference<1>(Test3{})),
            int&&
        >);
        static_assert(std::is_same_v<
            decltype(tinyrefl::struct_member_reference<2>(t3)),
            bool&
        >);
        std::cout << "Test8 (type traits) passed" << std::endl;
    }

    // 测试9: 索引边界检查（编译时）
    {
        // 应该触发静态断言
        // Test2 t2;
        // auto& invalid_ref = tinyrefl::struct_member_reference<2>(t2); // 应失败
        std::cout << "Test9 (boundary check) - manual verification required" << std::endl;
    }

    // 测试10: 不同值类别
    {
        Test2 t2{1.0, 2.0};
        
        // 左值引用
        auto& lv_ref = tinyrefl::struct_member_reference<0>(t2);
        lv_ref = 10.0;
        assert(t2.x == 10.0);
        
        // 右值引用
        auto&& rv_ref = tinyrefl::struct_member_reference<1>(std::move(t2));
        rv_ref = 20.0;
        assert(t2.y == 20.0);
        
        std::cout << "Test10 (value categories) passed" << std::endl;
    }

    std::cout << "===== All Reflection Tests Passed =====" << std::endl;
}

int main() {
    // 确保成员计数正确（编译时检查）
    static_assert(tinyrefl::members_count_v<Test1> == 1);
    static_assert(tinyrefl::members_count_v<Test2> == 2);
    static_assert(tinyrefl::members_count_v<Test3> == 3);
    static_assert(tinyrefl::members_count_v<Test4> == 4);
    static_assert(tinyrefl::members_count_v<Test5> == 5);
    
    run_reflection_tests();
    return 0;
}