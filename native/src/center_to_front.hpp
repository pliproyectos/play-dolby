#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace bridge {
// 7.1 Windows: FL FR FC LFE BL BR SL SR. Apply ONLY to the console branch,
// after routing and before mixing Chrome. This helper is not yet in the shipped EXE.
// Caller must ramp the coefficient if changing it while audio is running.
constexpr float CenterToFrontMinus12Db = 0.2511886432f;
inline void centerToFront(std::array<float, 8>& frame, float amount) {
    if (!std::isfinite(amount) || amount <= 0.f) return;
    amount = std::min(amount, 0.5f);
    const float center = frame[2];
    frame[0] += center * amount;
    frame[1] += center * amount;
    // Linked peak protection: reduce the whole frame only if the addition exceeds
    // full scale, preserving spatial ratios instead of clipping either front.
    float peak = 1.f;
    for (float sample : frame) peak = std::max(peak, std::abs(sample));
    if (peak > 1.f) for (float& sample : frame) sample /= peak;
}
} // namespace bridge
