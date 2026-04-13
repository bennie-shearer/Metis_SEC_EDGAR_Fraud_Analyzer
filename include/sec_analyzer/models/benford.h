/**
 * SEC EDGAR Fraud Analyzer - Benford's Law Model
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 * 
 * Implementation of Benford's Law analysis for detecting anomalies in
 * financial data digit distribution.
 * Reference: Benford, F. (1938) "The Law of Anomalous Numbers"
 */

#ifndef SEC_ANALYZER_MODELS_BENFORD_H
#define SEC_ANALYZER_MODELS_BENFORD_H

#include "../types.h"
#include <vector>
#include <array>

namespace sec_analyzer {

/**
 * Benford's Law Model
 * 
 * Benford's Law states that in many naturally occurring datasets,
 * the leading digit distribution follows a specific pattern:
 * 
 * Digit 1: 30.1%
 * Digit 2: 17.6%
 * Digit 3: 12.5%
 * Digit 4: 9.7%
 * Digit 5: 7.9%
 * Digit 6: 6.7%
 * Digit 7: 5.8%
 * Digit 8: 5.1%
 * Digit 9: 4.6%
 * 
 * Financial data that significantly deviates from this distribution
 * may indicate manipulation.
 * 
 * Tests used:
 * - Chi-Square test
 * - Mean Absolute Deviation (MAD)
 * - Z-test for individual digits
 */
class BenfordModel {
public:
    BenfordModel() = default;
    ~BenfordModel() = default;
    
    // Calculate Benford's Law analysis
    BenfordResult calculate(const std::vector<double>& values);
    
    // Get expected distribution
    static std::array<double, 9> get_expected_distribution();
    
    // Calculate actual distribution from values
    std::array<double, 9> calculate_actual_distribution(const std::vector<double>& values);
    
    // Statistical tests
    double calculate_chi_square(const std::array<double, 9>& expected,
                                const std::array<double, 9>& actual,
                                size_t n);
    double calculate_mad(const std::array<double, 9>& expected,
                        const std::array<double, 9>& actual);
    std::vector<int> identify_suspicious_digits(const std::array<double, 9>& expected,
                                                 const std::array<double, 9>& actual,
                                                 size_t n);
    
    // Interpret results
    static bool is_suspicious(double mad);
    static std::string get_conformity_level(double mad);
    static double mad_to_risk(double mad);
    
    // MAD thresholds (Nigrini guidelines)
    static constexpr double MAD_CLOSE_CONFORMITY = 0.006;
    static constexpr double MAD_ACCEPTABLE = 0.012;
    static constexpr double MAD_MARGINALLY_ACCEPTABLE = 0.015;
    static constexpr double MAD_NONCONFORMITY = 0.015;
    
    // Expected first digit probabilities.
    // Declared inline static const (not constexpr) to avoid Apple Clang /
    // libc++ 16+ __add_alignment_assumption error on constexpr std::array
    // class members when vector.h is transitively included.
    inline static const std::array<double, 9> EXPECTED = {
        0.301, 0.176, 0.125, 0.097, 0.079, 0.067, 0.058, 0.051, 0.046
    };

private:
    int extract_first_digit(double value);
    bool is_valid_value(double value);
};

/**
 * Second digit Benford analysis
 */
class BenfordSecondDigitModel {
public:
    BenfordResult calculate(const std::vector<double>& values);
    static std::array<double, 10> get_expected_distribution();
};

} // namespace sec_analyzer

#endif // SEC_ANALYZER_MODELS_BENFORD_H
