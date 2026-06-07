#pragma once

#include <cstdint>

bool dqnWeightsReady();
void dqnPredict(const float state[11], float q_values[3]);
uint8_t dqnPredictAction(const float state[11]);
