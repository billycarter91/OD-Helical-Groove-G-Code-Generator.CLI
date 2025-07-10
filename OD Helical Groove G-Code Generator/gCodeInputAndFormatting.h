// gCodeInputAndFormatting.h V1.0.0.0
// Copyright (C) 2025 Billy Carter <billycarter.business@gmail.com>
// This file is licensed under the GNU Affero General Public License v3.0 (AGPLv3).
// See the LICENSE file or <https://www.gnu.org/licenses/agpl-3.0.html> for details.
#pragma once

#include <iostream>    // std::cout, std::endl
#include <string>      // std::string, std::stod
#include <regex>       // std::regex, std::regex_match, std::smatch
#include <cmath>       // std::round
#include <sstream>     // std::ostringstream
#include <iomanip>     // std::fixed, std::setprecision
#include <conio.h>     // _kbhit(), _getch()    (Windows-only)
#include <stdexcept>   // std::runtime_error
#include <cctype>      // std::isdigit

// Define keyboard input key codes
const char ESC = 27;
const char ENTER = '\r';
const char BACKSPACE = 8;
const char SPECIAL_KEY_PREFIX_1 = 0;
const char SPECIAL_KEY_PREFIX_2 = 224;

// Helper: Detects and strips 'mm' or 'MM' (in any case/order) from the end of input.
// Returns true if mm found; sets numberPart to input minus the last two chars if match.
// Otherwise, numberPart is unchanged input.
bool endsWithMM(const std::string& input, std::string& numberPart) {
    if (input.size() >= 2) {
        char c1 = std::tolower(input[input.size() - 2]);
        char c2 = std::tolower(input[input.size() - 1]);
        if ((c1 == 'm' || c1 == 'M') && (c2 == 'm' || c2 == 'M')) {
            numberPart = input.substr(0, input.size() - 2);
            return true;
        }
    }
    numberPart = input;
    return false;
}

// Unified parse helper for all numeric userInput...() functions
bool parseInputWithOptionalMM(const std::string& input, bool allowMM, bool requirePositive, bool requireNegative, bool allowZero, double& result, std::string& errorMsg)
{
    // Regex for fraction of two integers, e.g. "1/2"
    //   - ^(\d+)  : one or more digits (the numerator)
    //   - -?      : optional negative. Always allow option b/c we want that cout << "fraction must be positive" to hit in later if() filtering
    //   - (\d+)$  : one or more digits (the denominator)
    std::regex fractionPattern(R"(^(-?\d+)/(-?\d+)$)");
    // Regex for decimal, e.g. "1", "1.0", "0.5", ".5", etc.
    //   - ^       : start of string    
    //   - -?      : optional negative. Always allow option b/c we want that cout << "value must be positive" to hit in later if() filtering
    //   - (?:     : non-capturing group open parenthesis (this groups alternative matches without capturing them)
    //   - [0-9]*  : zero or more digits (allows leading or trailing digits to be optional)
    //   - \.?     : optional decimal point
    //   - [0-9]+  : one or more digits  (ensures that atleast one side of the decimal point has digits)
    //   - |       : non-capturing group separator
    //   - )       : non-capturing group close parenthesis
    //   - $       : end of string
    std::regex decimalPattern(R"(^-?(?:[0-9]*\.?[0-9]+|[0-9]+\.?[0-9]*)$)");

    std::string coreInput = input;
    bool inputInMM = allowMM && endsWithMM(input, coreInput);

    std::smatch match;
    if (std::regex_match(coreInput, match, fractionPattern)) {
        int num = std::stoi(match[1]);
        int den = std::stoi(match[2]);
        if (den == 0) {
            errorMsg = "Denominator cannot be zero.";
            return false;
        }
        double value = static_cast<double>(num) / den;
        if (requirePositive && value < 0) { errorMsg = "Fraction must be positive."; return false; }
        if (requireNegative && value > 0) { errorMsg = "Fraction must be negative."; return false; }
        if (!allowZero && value == 0) { errorMsg = "Zero not allowed."; return false; }
        if (inputInMM) value /= 25.4;
        result = value;
        return true;
    }
    if (std::regex_match(coreInput, match, decimalPattern)) {
        double val;
        try { val = std::stod(coreInput); }
        catch (...) { errorMsg = "Invalid decimal input."; return false; }
        if (requirePositive && val < 0.0) { errorMsg = "Value must be positive."; return false; }
        if (requireNegative && val > 0.0) { errorMsg = "Value must be negative."; return false; }
        if (!allowZero && val == 0.0) { errorMsg = "Zero not allowed."; return false; }
        if (inputInMM) val /= 25.4;
        result = val;
        return true;
    }
    errorMsg = "Invalid input format.";
    return false;
}

// Input handling/restriction for all numeric userInput...() functions
std::string getUserInputString(bool allowMM) {
    std::string input;
    while (true) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == SPECIAL_KEY_PREFIX_1 || ch == SPECIAL_KEY_PREFIX_2) {
                _getch();
                continue; // special keys give themselves away by having a prefix - ignore them
            }
            if (ch == ESC) {
                throw std::runtime_error("EscapePressed"); //catch in main inputPrompt() while/switch to decrement switch state
            }
            if (ch == ENTER) {
                std::cout << std::endl; // new line in end user command line interface
                break;
            }
            if (ch == BACKSPACE) {
                if (!input.empty()) {
                    input.pop_back();
                    std::cout << "\b \b"; // backspace, overwrite with space, backspace again
                }
                continue;
            }
            // Acceptable characters: digits, dot, slash, minus, m/M if mm allowed
            if (isdigit(ch) || ch == '.' || ch == '/' || ch == '-') {
                input += ch;
                std::cout << ch;
            }
            else if (allowMM && (ch == 'm' || ch == 'M')) {
                input += ch;
                std::cout << ch;
            }
        }
    }
    // Return after Enter (or as needed)
    return input;
}

//input handling functions
//restricts input as implied by the function name
//throws runtime_error("EscapePressed") so you can catch it in an input menu to go back a level
//besides the implied allowed inputs, also allows Escape, Backspace, and Enter. Allows optional mm suffix and will convert to inch. Ignores any other keypresses.

//Allows the user to input only positive numbers, decimal or fraction, with optional mm suffix
double userInputPos(bool allowMM = false)
{
    while (true) {
        std::string input = getUserInputString(allowMM);
        double value;
        std::string errorMsg;
        bool validInput = parseInputWithOptionalMM(input, allowMM, /*requirePositive=*/true, /*requireNegative=*/false, /*allowZero=*/false, value, errorMsg);
        if (validInput) return value;
        std::cout << errorMsg << std::endl;
    }
}

//Allows the user to input only positive numbers, decimal or fraction, and Zero, with optional mm suffix
double userInputPosZero(bool allowMM = false)
{
    while (true) {
        std::string input = getUserInputString(allowMM);
        double value;
        std::string errorMsg;
        bool validInput = parseInputWithOptionalMM(input, allowMM, /*requirePositive=*/true, /*requireNegative=*/false, /*allowZero=*/true, value, errorMsg);
        if (validInput) return value;
        std::cout << errorMsg << std::endl;
    }
}

//Allows the user to input only negative numbers, decimal or fraction, with optional mm suffix
double userInputNeg(bool allowMM = false)
{
    while (true) {
        std::string input = getUserInputString(allowMM);
        double value;
        std::string errorMsg;
        bool validInput = parseInputWithOptionalMM(input, allowMM, /*requirePositive=*/false, /*requireNegative=*/true, /*allowZero=*/false, value, errorMsg);
        if (validInput) return value;
        std::cout << errorMsg << std::endl;
    }
}

//Allows the user to input positive or negative numbers, decimal or fraction, and Zero, with optional mm suffix
double userInputPosNeg(bool allowMM = false)
{
    while (true) {
        std::string input = getUserInputString(allowMM);
        double value;
        std::string errorMsg;
        bool validInput = parseInputWithOptionalMM(input, allowMM, /*requirePositive=*/false, /*requireNegative=*/false, /*allowZero=*/true, value, errorMsg);
        if (validInput) return value;
        std::cout << errorMsg << std::endl;
    }
}

//Additional userInput...() function - Allow the user to input l L r or R for left and right
char userInputLR()
{
    std::string input;

    while (true) {
        if (_kbhit()) {
            char ch = _getch();

            // Handle special keys (arrow keys, function keys)
            if (ch == SPECIAL_KEY_PREFIX_1 || ch == SPECIAL_KEY_PREFIX_2) {
                _getch(); // Discard next character
                continue;
            }

            if (ch == ESC) { // ESC key
                throw std::runtime_error("EscapePressed");
            }
            if (ch == ENTER) { // Enter key
                std::cout << std::endl;
                // Only accept a single char: l L r R
                if (input.size() == 1 &&
                    (input[0] == 'l' || input[0] == 'L' ||
                        input[0] == 'r' || input[0] == 'R')) {
                    return input[0];
                }
                else {
                    std::cout << "Enter 'l', 'L', 'r', or 'R': ";
                    input.clear();
                }
                continue;
            }
            if (ch == BACKSPACE) {
                if (!input.empty()) {
                    input.pop_back();
                    std::cout << "\b \b";
                }
                continue;
            }

            // Only allow 'l', 'L', 'r', 'R'
            if (ch == 'l' || ch == 'L' || ch == 'r' || ch == 'R') {
                if (input.empty()) { // only allow a single character
                    input += ch;
                    std::cout << ch;
                }
            }
            // Ignore all other characters
        }
    }
}

//math helpers for gcode calculation

//rounds results to 6 places
double roundSixDecimal(double d)
{
    return round(d * 1000000) / 1000000;
}
//rounds results to 5 places
double roundFiveDecimal(double d)
{
    return round(d * 100000) / 100000;
}
//rounds results to 4 places
double roundFourDecimal(double d)
{
    return round(d * 10000) / 10000;
}

//rounds results to 3 places
double roundThreeDecimal(double d)
{
    return round(d * 1000) / 1000;
}

//rounds results to 2 places
double roundTwoDecimal(double d)
{
    return round(d * 100) / 100;
}

//rounds results to 1 place
double roundOneDecimal(double d)
{
    return round(d * 10) / 10;
}

//GCode formatting helper

//decimal precision for string output in gcode file. Call from main like "formatGCodeDecimals<5>(xCoordinate)" or "formatGCodeDecimals<2>(IPM)"
//forces decimal appearance. Accepts 1-6 only, limiting up to 6 significant digits but will remove any and all trailing zeros
//also removes leading zeros (e.g. .2 instead of 0.2) and trailing zeros (e.g. 1. instead of 1.0)
//for no decimal e.g. for RPM, you would want to restrict user input or convert to an int before printing the value so this function isn't used in that case
template <int numDecimalPlaces>
std::string formatGCodeDecimals(double val) { // was fmtNum4
    static_assert(numDecimalPlaces >= 1 && numDecimalPlaces <= 6, "numDecimalPlaces must be 1-6");
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(numDecimalPlaces) << val;
    std::string s = oss.str();

    // Check if the number is negative
    size_t startIdx = 0;
    if (!s.empty() && s[0] == '-') {
        startIdx = 1; // Start processing after the minus sign
    }

    // Find the decimal point
    size_t decimalPos = s.find('.', startIdx);
    if (decimalPos != std::string::npos) {
        // Find the position of the last non-zero character after the decimal
        size_t lastNonZero = s.find_last_not_of('0', s.size() - 1);
        if (lastNonZero != std::string::npos) {
            if (lastNonZero < decimalPos) {
                // All decimal digits are zero
                s.erase(decimalPos + 1);
            }
            else {
                // Erase trailing zeros
                s.erase(lastNonZero + 1);
            }
        }
    }

    // Handle leading zero for numbers between -1 and 1 (excluding zero)
    // and ensure that a lone zero is represented as "0."
    if (s.size() > startIdx + 1 && s[startIdx] == '0' && s[startIdx + 1] == '.') {
        // Remove the leading zero
        s.erase(startIdx, 1);
    }

    // Handle the case where you're left with a lone period
    if (s == "." || s == "-.") {
        s = "0.";
    }

    return s;
}