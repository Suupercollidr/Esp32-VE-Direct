#include <unity.h>
#include <string>
using String = std::string;
#include <map>
#include <vector>
#include "../src/VEDirectDecoder.h"

// ------------------------
// Dummy Arduino/ESP32 stubs
// ------------------------
#ifdef UNIT_TEST

using String = std::string;

class EventLogger {
public:
    enum class LogLevel { INFO, WARNING, ERROR };
    void log(const String&, LogLevel = LogLevel::ERROR) {}
};

// Global eventLog stub
EventLogger eventLog;

#endif

// ------------------------
// Testobjekt
// ------------------------
VEDirectDecoder decoder;

// Körs före varje test
void setUp(void) {
    // Lägg in exempel-mapping
    decoder.setMapping("VE_MPPT_ERR", {
        {1, "Trasig lök"},
        {2, "Slut på smör"},
        {4, "För varm laddare"}
    });
}

// ------------------------
// Test: exact match
// ------------------------
void test_exact_match(void) {
    SensorValue val = decoder.VEDirectCodeToHumanReadable("VE_MPPT_ERR", 2);
    TEST_ASSERT_EQUAL(2, val.intValue);
    TEST_ASSERT_TRUE(val.isNumeric);
    TEST_ASSERT_EQUAL_STRING("Slut på smör", val.strValue.c_str());
}

// ------------------------
// Test: combination 2+4=6
// ------------------------
void test_combination(void) {
    SensorValue val = decoder.VEDirectCodeToHumanReadable("VE_MPPT_ERR", 6);
    TEST_ASSERT_EQUAL(6, val.intValue);
    TEST_ASSERT_TRUE(val.isNumeric);

    // std::string::find returnerar size_t
    TEST_ASSERT(val.strValue.find("Slut på smör") != std::string::npos);
    TEST_ASSERT(val.strValue.find("För varm laddare") != std::string::npos);
}

// ------------------------
// Test: unknown code
// ------------------------
void test_unknown_code(void) {
    SensorValue val = decoder.VEDirectCodeToHumanReadable("VE_MPPT_ERR", 99);
    TEST_ASSERT_EQUAL(99, val.intValue);
    TEST_ASSERT_TRUE(val.isNumeric);
    TEST_ASSERT_EQUAL_STRING("99", val.strValue.c_str()); // fallback
}

// ------------------------
// Test: unknown label
// ------------------------
void test_unknown_label(void) {
    SensorValue val = decoder.VEDirectCodeToHumanReadable("UNKNOWN_LABEL", 1);
    TEST_ASSERT_EQUAL(1, val.intValue);
    TEST_ASSERT_TRUE(val.isNumeric);
    TEST_ASSERT(val.strValue.find("Okänd label") != std::string::npos);
}

// ------------------------
// Unity main
// ------------------------
int main(int argc, char **argv) {
    UNITY_BEGIN();

    setUp(); // Körs en gång före testerna
    RUN_TEST(test_exact_match);
    RUN_TEST(test_combination);
    RUN_TEST(test_unknown_code);
    RUN_TEST(test_unknown_label);

    return UNITY_END();
}
