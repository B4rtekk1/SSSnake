#include "dqn_inference.h"

#include "dqn_weights.h"

static_assert(DQN_INPUT_SIZE == 11, "SSSnake DQN expects 11 input features");
static_assert(DQN_OUTPUT_SIZE == 3, "SSSnake DQN expects 3 relative actions");

static float relu(float value) {
    return value > 0.0f ? value : 0.0f;
}

bool dqnWeightsReady() {
    return DQN_WEIGHTS_READY;
}

void dqnPredict(const float state[11], float q_values[3]) {
    float hidden1[DQN_HIDDEN_SIZE];
    float hidden2[DQN_HIDDEN_SIZE];

    for (uint16_t i = 0; i < DQN_HIDDEN_SIZE; ++i) {
        float sum = DQN_FC1_BIAS[i];
        for (uint16_t j = 0; j < DQN_INPUT_SIZE; ++j) {
            sum += DQN_FC1_WEIGHTS[i][j] * state[j];
        }
        hidden1[i] = relu(sum);
    }

    for (uint16_t i = 0; i < DQN_HIDDEN_SIZE; ++i) {
        float sum = DQN_FC2_BIAS[i];
        for (uint16_t j = 0; j < DQN_HIDDEN_SIZE; ++j) {
            sum += DQN_FC2_WEIGHTS[i][j] * hidden1[j];
        }
        hidden2[i] = relu(sum);
    }

    for (uint16_t i = 0; i < DQN_OUTPUT_SIZE; ++i) {
        float sum = DQN_FC3_BIAS[i];
        for (uint16_t j = 0; j < DQN_HIDDEN_SIZE; ++j) {
            sum += DQN_FC3_WEIGHTS[i][j] * hidden2[j];
        }
        q_values[i] = sum;
    }
}

uint8_t dqnPredictAction(const float state[11]) {
    float q_values[3];
    dqnPredict(state, q_values);

    uint8_t best = 0;
    if (q_values[1] > q_values[best]) {
        best = 1;
    }
    if (q_values[2] > q_values[best]) {
        best = 2;
    }
    return best;
}
