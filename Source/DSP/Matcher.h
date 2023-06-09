#ifndef ZLLMATCH_MATCHER_H
#define ZLLMATCH_MATCHER_H

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
#include <limits>

#include "Monitor.h"
#include "dsp_defines.h"

template<typename FloatType>
struct Translator {
    FloatType mainLoudness[ZLDsp::loudness::loudnessNUM];
    FloatType targetLoudness[ZLDsp::loudness::loudnessNUM];
    FloatType peak;
    int64_t position;
};

template<typename FloatType>
class LoudnessPos {
public:
    LoudnessPos(std::vector<FloatType> mainLoudness,
                std::vector<FloatType> targetLoudness, FloatType peak,
                int64_t position) {
        m_mainLoudness = mainLoudness;
        m_targetLoudness = targetLoudness;
        m_peak = peak;
        m_position = position;
    }

    LoudnessPos(Translator<FloatType> translator) {
        m_mainLoudness.assign(translator.mainLoudness, translator.mainLoudness + ZLDsp::loudness::loudnessNUM);
        m_targetLoudness.assign(translator.targetLoudness, translator.targetLoudness + ZLDsp::loudness::loudnessNUM);
        m_peak = translator.peak;
        m_position = translator.position;
    }

    FloatType getGain(size_t loudnessID, bool ceil, bool useTargetV,
                      FloatType targetV) {
        FloatType gain;
        if (useTargetV) {
            gain = targetV - m_mainLoudness[loudnessID];
        } else {
            gain = m_targetLoudness[loudnessID] - m_mainLoudness[loudnessID];
        }
        if (ceil) {
            gain = juce::jmin(-m_peak, gain);
        }
        return gain;
    }

    int64_t getPosition() { return m_position; }

    size_t getSize() {
        return sizeof(FloatType) * (ZLDsp::loudness::loudnessNUM * 2 + 1) + sizeof(int64_t);
    }

    Translator<FloatType> toStruct() {
        return Translator<FloatType>{{m_mainLoudness[0], m_mainLoudness[1], m_mainLoudness[2]},
                                     {m_targetLoudness[0], m_targetLoudness[1], m_targetLoudness[2]},
                                     m_peak, m_position};
    }

private:
    std::vector<FloatType> m_mainLoudness;
    std::vector<FloatType> m_targetLoudness;
    FloatType m_peak;
    int64_t m_position;


};

template<typename FloatType>
class Matcher {
public:
    Matcher() = default;

    void reset() {
        diffs.clear();
        diffs.emplace_back(ZLDsp::loudness::getEmptyLoudness<FloatType>(),
                           ZLDsp::loudness::getEmptyLoudness<FloatType>(), 0, m_position);
        prev_id = 0;
    }

    void setPosition(int64_t pos) { m_position = pos; }

    void setCeil(bool f) { ceil = f; }

    void setSideID(int ID) { sideID = ID; }

    void setLoudnessID(int ID) { loudnessID = ID; }

    void setBound(FloatType v) { bound = v; }

    void setTarget(FloatType v) { target = v; }

    void learnDiff(std::vector<FloatType> mainL, std::vector<FloatType> targetL,
                   FloatType mainPeak) {
        while (!diffs.empty() && diffs.back().getPosition() >= m_position) {
            diffs.pop_back();
        }
        diffs.emplace_back(mainL, targetL, mainPeak, m_position);
    }

    FloatType getDiff() {
        if (m_position < diffs.front().getPosition() || m_position >= diffs.back().getPosition()) {
            return static_cast<FloatType>(0);
        }
        for (size_t shift_id = 0; shift_id < diffs.size(); ++shift_id) {
            auto index_id = (prev_id + shift_id) % diffs.size();
            auto left_id = (prev_id + shift_id - 1) % diffs.size();
            if (m_position < diffs[index_id].getPosition() &&
                m_position >= diffs[left_id].getPosition()) {
                prev_id = index_id;
                auto gain = diffs[index_id].getGain(
                        static_cast<size_t>(loudnessID.load()), ceil.load(),
                        sideID.load() == ZLDsp::side::value, target.load());
                gain = juce::jlimit(-bound.load(), bound.load(), gain);
                return gain;
            }
        }
        return static_cast<FloatType>(0);
    }

    juce::String toString() {
        std::vector<Translator<FloatType>> translators;
        for (auto &d: diffs) {
            translators.push_back(d.toStruct());
        }
        return ZLDsp::VectorToBase64String(translators);
    }

    void fromString(const juce::String &str) {
        diffs.clear();
        auto translators = ZLDsp::Base64StringToVector<Translator<FloatType>>(str);
        for (auto &t: translators) {
            diffs.emplace_back(t);
        }
    }

private:
    std::atomic<int64_t> m_position = 0;
    size_t prev_id = 0;
    std::vector<LoudnessPos<FloatType>> diffs;

    std::atomic<bool> ceil = ZLDsp::ceil::defaultV;
    std::atomic<int> sideID = ZLDsp::side::defaultI;
    std::atomic<int> loudnessID = ZLDsp::loudness::defaultI;
    std::atomic<FloatType> bound = ZLDsp::bound::defaultV;
    std::atomic<FloatType> target = ZLDsp::target::defaultV;
};

#endif // ZLLMATCH_MATCHER_H
