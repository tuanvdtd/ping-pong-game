#ifndef PA0_GESTURE_H
#define PA0_GESTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PA0_GESTURE_EVENT_NONE = 0U,
    PA0_GESTURE_EVENT_SINGLE_PRESS = 1U,
    PA0_GESTURE_EVENT_DOUBLE_PRESS = 2U
} Pa0GestureEvent;

typedef struct
{
    uint32_t firstPressTick;
    uint32_t doublePressWindowTicks;
    uint8_t debounceSamples;
    uint8_t candidateSamples;
    uint8_t candidatePressed;
    uint8_t stablePressed;
    uint8_t waitingForSecondPress;
    uint8_t releasedAfterFirstPress;
} Pa0GestureDetector;

static inline void Pa0GestureDetector_Init(Pa0GestureDetector* detector,
                                           uint8_t initialPressed,
                                           uint8_t debounceSamples,
                                           uint32_t doublePressWindowTicks)
{
    const uint8_t pressed = (initialPressed != 0U) ? 1U : 0U;
    const uint8_t samples = (debounceSamples != 0U) ? debounceSamples : 1U;

    detector->firstPressTick = 0U;
    detector->doublePressWindowTicks = doublePressWindowTicks;
    detector->debounceSamples = samples;
    detector->candidateSamples = samples;
    detector->candidatePressed = pressed;
    detector->stablePressed = pressed;
    detector->waitingForSecondPress = 0U;
    detector->releasedAfterFirstPress = 0U;
}

static inline void Pa0GestureDetector_Reset(Pa0GestureDetector* detector,
                                            uint8_t currentPressed)
{
    Pa0GestureDetector_Init(detector,
                            currentPressed,
                            detector->debounceSamples,
                            detector->doublePressWindowTicks);
}

static inline Pa0GestureEvent Pa0GestureDetector_Update(
    Pa0GestureDetector* detector,
    uint8_t sampledPressed,
    uint32_t now)
{
    const uint8_t pressed = (sampledPressed != 0U) ? 1U : 0U;
    uint8_t stableStateChanged = 0U;

    if (pressed != detector->candidatePressed)
    {
        detector->candidatePressed = pressed;
        detector->candidateSamples = 1U;
    }
    else if (detector->candidateSamples < detector->debounceSamples)
    {
        ++detector->candidateSamples;
    }

    if ((detector->candidateSamples >= detector->debounceSamples) &&
        (detector->stablePressed != detector->candidatePressed))
    {
        detector->stablePressed = detector->candidatePressed;
        stableStateChanged = 1U;
    }

    if (detector->waitingForSecondPress != 0U)
    {
        const uint32_t elapsed = now - detector->firstPressTick;

        if ((stableStateChanged != 0U) &&
            (detector->stablePressed == 0U))
        {
            detector->releasedAfterFirstPress = 1U;
        }

        if ((stableStateChanged != 0U) &&
            (detector->stablePressed != 0U) &&
            (detector->releasedAfterFirstPress != 0U))
        {
            if (elapsed <= detector->doublePressWindowTicks)
            {
                detector->waitingForSecondPress = 0U;
                detector->releasedAfterFirstPress = 0U;
                return PA0_GESTURE_EVENT_DOUBLE_PRESS;
            }

            /* The first press is single; this edge starts a new sequence. */
            detector->firstPressTick = now;
            detector->releasedAfterFirstPress = 0U;
            return PA0_GESTURE_EVENT_SINGLE_PRESS;
        }

        /* Keep the configured upper boundary inclusive. */
        if (elapsed > detector->doublePressWindowTicks)
        {
            detector->waitingForSecondPress = 0U;
            detector->releasedAfterFirstPress = 0U;
            return PA0_GESTURE_EVENT_SINGLE_PRESS;
        }

        return PA0_GESTURE_EVENT_NONE;
    }

    if ((stableStateChanged != 0U) && (detector->stablePressed != 0U))
    {
        detector->firstPressTick = now;
        detector->waitingForSecondPress = 1U;
        detector->releasedAfterFirstPress = 0U;
    }

    return PA0_GESTURE_EVENT_NONE;
}

#ifdef __cplusplus
}
#endif

#endif /* PA0_GESTURE_H */
