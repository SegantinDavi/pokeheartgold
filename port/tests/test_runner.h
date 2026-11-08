/**
 * @file test_runner.h
 * @brief Lightweight, zero-dependency C++ test framework for HAL and port testing.
 */

#ifndef PORT_TEST_RUNNER_H
#define PORT_TEST_RUNNER_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace PortTest {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& Instance() {
        static TestRegistry instance;
        return instance;
    }

    void Register(const std::string& name, std::function<void()> func) {
        tests_.push_back({name, func});
    }

    int RunAll() {
        int passed = 0;
        int failed = 0;
        std::cout << "\n========================================\n";
        std::cout << " Running " << tests_.size() << " native test suite(s)...\n";
        std::cout << "========================================\n\n";

        for (const auto& test : tests_) {
            currentTestName_ = test.name;
            currentTestFailed_ = false;
            try {
                test.func();
            } catch (const std::exception& e) {
                Fail(std::string("Unhandled exception: ") + e.what(), __FILE__, __LINE__);
            } catch (...) {
                Fail("Unknown exception caught", __FILE__, __LINE__);
            }

            if (!currentTestFailed_) {
                std::cout << " [ PASS ] " << test.name << "\n";
                passed++;
            } else {
                std::cout << " [ FAIL ] " << test.name << "\n";
                failed++;
            }
        }

        std::cout << "\n========================================\n";
        std::cout << " Test Summary: " << passed << " passed, " << failed << " failed, "
                  << tests_.size() << " total.\n";
        std::cout << "========================================\n";

        return (failed == 0) ? 0 : 1;
    }

    void Fail(const std::string& message, const char* file, int line) {
        currentTestFailed_ = true;
        std::cerr << "   FAILED at " << file << ":" << line << ": " << message << "\n";
    }

private:
    std::vector<TestCase> tests_;
    std::string currentTestName_;
    bool currentTestFailed_ = false;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::Instance().Register(name, func);
    }
};

} // namespace PortTest

#define TEST_CASE(name) \
    static void test_func_##name(); \
    static PortTest::TestRegistrar registrar_##name(#name, test_func_##name); \
    static void test_func_##name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            PortTest::TestRegistry::Instance().Fail(#condition " is FALSE", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            PortTest::TestRegistry::Instance().Fail(#condition " is TRUE", __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if (!((expected) == (actual))) { \
            PortTest::TestRegistry::Instance().Fail( \
                std::string("Expected: ") + std::to_string(expected) + " == Actual: " + std::to_string(actual), \
                __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_NE(val1, val2) \
    do { \
        if ((val1) == (val2)) { \
            PortTest::TestRegistry::Instance().Fail( \
                std::string("Values should not be equal: ") + std::to_string(val1), \
                __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_GE(val1, val2) \
    do { \
        if (!((val1) >= (val2))) { \
            PortTest::TestRegistry::Instance().Fail( \
                std::string("Expected: ") + std::to_string(val1) + " >= " + std::to_string(val2), \
                __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_LE(val1, val2) \
    do { \
        if (!((val1) <= (val2))) { \
            PortTest::TestRegistry::Instance().Fail( \
                std::string("Expected: ") + std::to_string(val1) + " <= " + std::to_string(val2), \
                __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_NEAR(expected, actual, epsilon) \
    do { \
        if (std::fabs((expected) - (actual)) > (epsilon)) { \
            PortTest::TestRegistry::Instance().Fail( \
                std::string("Expected: ") + std::to_string(expected) + " near Actual: " + std::to_string(actual), \
                __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#endif // PORT_TEST_RUNNER_H
