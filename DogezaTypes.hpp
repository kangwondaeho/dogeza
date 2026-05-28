#pragma once
#include "Framework.hpp"

enum class GameState
{
    TITLE,
    READY,
    APPROACH,
    PRONE_SLIDE,
    JUDGE,
    RESULT
};

enum class JudgeResult
{
    NONE,
    PERFECT,
    GREAT,
    GOOD,
    FAIL
};

enum class MapType
{
    STOP,
    SHORT_INERTIA,
    LONG_INERTIA
};

struct MapRule
{
    MapType type;
    const char* label;
    const char* name;
    float inertiaSeconds;
};

inline const char* ToString(GameState state)
{
    switch (state)
    {
    case GameState::TITLE:       return "TITLE";
    case GameState::READY:       return "READY";
    case GameState::APPROACH:    return "APPROACH";
    case GameState::PRONE_SLIDE: return "PRONE_SLIDE";
    case GameState::JUDGE:       return "JUDGE";
    case GameState::RESULT:      return "RESULT";
    default:                     return "UNKNOWN";
    }
}

inline const char* ToString(JudgeResult result)
{
    switch (result)
    {
    case JudgeResult::PERFECT: return "PERFECT";
    case JudgeResult::GREAT:   return "GREAT";
    case JudgeResult::GOOD:    return "GOOD";
    case JudgeResult::FAIL:    return "FAIL";
    case JudgeResult::NONE:
    default:                   return "NONE";
    }
}

inline int ScoreOf(JudgeResult result)
{
    switch (result)
    {
    case JudgeResult::PERFECT: return 1000;
    case JudgeResult::GREAT:   return 700;
    case JudgeResult::GOOD:    return 300;
    case JudgeResult::FAIL:    return 0;
    case JudgeResult::NONE:
    default:                   return 0;
    }
}
