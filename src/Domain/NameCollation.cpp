#include "Domain/NameCollation.h"

#include <algorithm>

namespace et::domain {

namespace {

wchar_t FoldAscii(wchar_t character) {
    return (character >= L'A' && character <= L'Z') ? static_cast<wchar_t>(character + (L'a' - L'A'))
                                                    : character;
}

bool IsDigit(wchar_t character) {
    return character >= L'0' && character <= L'9';
}

// Consumes one digit run starting at `position` and returns it without leading zeros.
std::wstring_view TakeDigitRun(std::wstring_view text, size_t& position) {
    const size_t begin = position;
    while (position < text.size() && IsDigit(text[position])) {
        ++position;
    }
    std::wstring_view run = text.substr(begin, position - begin);
    const size_t firstSignificant = run.find_first_not_of(L'0');
    return firstSignificant == std::wstring_view::npos ? run.substr(run.size() - 1)
                                                       : run.substr(firstSignificant);
}

// Negative, zero or positive like strcmp.
int CompareDigitRuns(std::wstring_view left, std::wstring_view right) {
    if (left.size() != right.size()) {
        return left.size() < right.size() ? -1 : 1;
    }
    return left.compare(right);
}

}  // namespace

bool SimpleNameCollation::Equals(std::wstring_view left, std::wstring_view right) const {
    return std::equal(left.begin(), left.end(), right.begin(), right.end(),
                      [](wchar_t a, wchar_t b) { return FoldAscii(a) == FoldAscii(b); });
}

bool SimpleNameCollation::NaturalLess(std::wstring_view left, std::wstring_view right) const {
    size_t leftPosition = 0;
    size_t rightPosition = 0;
    while (leftPosition < left.size() && rightPosition < right.size()) {
        if (IsDigit(left[leftPosition]) && IsDigit(right[rightPosition])) {
            const int order = CompareDigitRuns(TakeDigitRun(left, leftPosition),
                                               TakeDigitRun(right, rightPosition));
            if (order != 0) {
                return order < 0;
            }
            continue;
        }
        const wchar_t leftChar = FoldAscii(left[leftPosition]);
        const wchar_t rightChar = FoldAscii(right[rightPosition]);
        if (leftChar != rightChar) {
            return leftChar < rightChar;
        }
        ++leftPosition;
        ++rightPosition;
    }
    const bool leftExhausted = leftPosition == left.size();
    const bool rightExhausted = rightPosition == right.size();
    if (leftExhausted != rightExhausted) {
        return leftExhausted;
    }
    // Equal under folding: fall back to ordinal order so the ordering stays strict.
    return left < right;
}

}  // namespace et::domain
