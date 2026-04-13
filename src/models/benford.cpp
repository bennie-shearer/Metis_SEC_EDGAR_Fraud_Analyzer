/**
 * SEC EDGAR Fraud Analyzer - Benford's Law Model Implementation
 * Version: 3.1.3
 * Author: Bennie Shearer (Retired)
 */

#include <sec_analyzer/models/benford.h>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace sec_analyzer {

std::array<double, 9> BenfordModel::get_expected_distribution() {
    return EXPECTED;
}

bool BenfordModel::is_valid_value(double value) {
    // Exclude zeros, very small values, and non-finite values
    return std::isfinite(value) && std::abs(value) >= 1.0;
}

int BenfordModel::extract_first_digit(double value) {
    if (!is_valid_value(value)) return 0;
    
    value = std::abs(value);
    
    // Normalize to get first digit
    while (value >= 10.0) value /= 10.0;
    while (value < 1.0) value *= 10.0;
    
    int digit = static_cast<int>(value);
    return (digit >= 1 && digit <= 9) ? digit : 0;
}

std::array<double, 9> BenfordModel::calculate_actual_distribution(const std::vector<double>& values) {
    std::array<int, 9> counts = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    int total = 0;
    
    for (double value : values) {
        int digit = extract_first_digit(value);
        if (digit >= 1 && digit <= 9) {
            counts[digit - 1]++;
            total++;
        }
    }
    
    std::array<double, 9> distribution;
    for (int i = 0; i < 9; ++i) {
        distribution[i] = (total > 0) ? static_cast<double>(counts[i]) / total : 0.0;
    }
    
    return distribution;
}

double BenfordModel::calculate_chi_square(const std::array<double, 9>& expected,
                                          const std::array<double, 9>& actual,
                                          size_t n) {
    double chi_sq = 0.0;
    
    for (int i = 0; i < 9; ++i) {
        double exp_count = expected[i] * n;
        double act_count = actual[i] * n;
        if (exp_count > 0) {
            chi_sq += std::pow(act_count - exp_count, 2) / exp_count;
        }
    }
    
    return chi_sq;
}

double BenfordModel::calculate_mad(const std::array<double, 9>& expected,
                                   const std::array<double, 9>& actual) {
    double sum = 0.0;
    
    for (int i = 0; i < 9; ++i) {
        sum += std::abs(actual[i] - expected[i]);
    }
    
    return sum / 9.0;
}

std::vector<int> BenfordModel::identify_suspicious_digits(const std::array<double, 9>& expected,
                                                          const std::array<double, 9>& actual,
                                                          size_t n) {
    std::vector<int> suspicious;
    
    for (int i = 0; i < 9; ++i) {
        // Z-test for each digit
        double p = expected[i];
        double p_hat = actual[i];
        double se = std::sqrt(p * (1 - p) / n);
        
        if (se > 0) {
            double z = std::abs(p_hat - p) / se;
            // Z > 2.576 corresponds to p < 0.01 (two-tailed)
            if (z > 2.576) {
                suspicious.push_back(i + 1);
            }
        }
    }
    
    return suspicious;
}

bool BenfordModel::is_suspicious(double mad) {
    return mad > MAD_MARGINALLY_ACCEPTABLE;
}

std::string BenfordModel::get_conformity_level(double mad) {
    if (mad <= MAD_CLOSE_CONFORMITY) return "Close Conformity";
    if (mad <= MAD_ACCEPTABLE) return "Acceptable Conformity";
    if (mad <= MAD_MARGINALLY_ACCEPTABLE) return "Marginally Acceptable";
    return "Nonconformity";
}

double BenfordModel::mad_to_risk(double mad) {
    // Convert MAD to risk score (0-1)
    // MAD of 0 = 0 risk, MAD > 0.02 = high risk
    return std::clamp(mad / 0.02, 0.0, 1.0);
}

BenfordResult BenfordModel::calculate(const std::vector<double>& values) {
    BenfordResult result;
    
    // Get expected distribution
    auto expected = get_expected_distribution();
    result.expected_distribution = std::vector<double>(expected.begin(), expected.end());
    
    // Calculate actual distribution
    auto actual = calculate_actual_distribution(values);
    result.actual_distribution = std::vector<double>(actual.begin(), actual.end());
    
    // Count valid values
    size_t n = 0;
    for (double v : values) {
        if (is_valid_value(v)) n++;
    }
    
    // Calculate statistics
    if (n > 0) {
        result.chi_square = calculate_chi_square(expected, actual, n);
        result.mad = calculate_mad(expected, actual);
        result.deviation_percent = result.mad * 100.0;
        
        // Identify suspicious digits
        auto suspicious_digits = identify_suspicious_digits(expected, actual, n);
        for (int digit : suspicious_digits) {
            result.anomalies.push_back("Digit " + std::to_string(digit) + 
                                       " significantly deviates from expected");
        }
    }
    
    // Interpret
    result.is_suspicious = is_suspicious(result.mad);
    
    return result;
}

// Second digit model
std::array<double, 10> BenfordSecondDigitModel::get_expected_distribution() {
    // Second digit expected probabilities (Benford's Law)
    return {
        0.1197, 0.1139, 0.1088, 0.1043, 0.1003,
        0.0967, 0.0934, 0.0904, 0.0876, 0.0850
    };
}

BenfordResult BenfordSecondDigitModel::calculate(const std::vector<double>& values) {
    BenfordResult result;
    
    auto expected = get_expected_distribution();
    result.expected_distribution = std::vector<double>(expected.begin(), expected.end());
    
    // Count second digits
    std::array<int, 10> counts = {0};
    int total = 0;
    
    for (double value : values) {
        if (std::isfinite(value) && std::abs(value) >= 10.0) {
            double v = std::abs(value);
            while (v >= 100.0) v /= 10.0;
            while (v < 10.0) v *= 10.0;
            
            int second_digit = static_cast<int>(v) % 10;
            if (second_digit >= 0 && second_digit <= 9) {
                counts[second_digit]++;
                total++;
            }
        }
    }
    
    // Calculate actual distribution
    std::array<double, 10> actual;
    for (int i = 0; i < 10; ++i) {
        actual[i] = (total > 0) ? static_cast<double>(counts[i]) / total : 0.0;
    }
    result.actual_distribution = std::vector<double>(actual.begin(), actual.end());
    
    // Calculate MAD
    double mad = 0.0;
    for (int i = 0; i < 10; ++i) {
        mad += std::abs(actual[i] - expected[i]);
    }
    result.mad = mad / 10.0;
    result.deviation_percent = result.mad * 100.0;
    result.is_suspicious = result.mad > 0.012;
    
    return result;
}

} // namespace sec_analyzer
