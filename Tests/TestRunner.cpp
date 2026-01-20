/**
 * Test Runner for ProgFlow
 *
 * Runs all registered JUCE UnitTests.
 */

#include <juce_core/juce_core.h>

int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);

    // Run all tests
    runner.runAllTests();

    // Print results
    int totalTests = 0;
    int totalFailures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* result = runner.getResult(i);
        totalTests += result->passes + result->failures;
        totalFailures += result->failures;

        if (result->failures > 0)
        {
            std::cout << "FAILED: " << result->unitTestName << "\n";
            for (const auto& msg : result->messages)
            {
                std::cout << "  " << msg << "\n";
            }
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "Tests: " << totalTests
              << ", Failures: " << totalFailures << "\n";
    std::cout << "========================================\n";

    return totalFailures > 0 ? 1 : 0;
}
