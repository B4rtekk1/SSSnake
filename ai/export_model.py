import argparse
from pathlib import Path

import torch

from model import DQN


INPUT_SIZE = 11
OUTPUT_SIZE = 3


def _load_state_dict(path: Path):
    try:
        return torch.load(path, map_location="cpu", weights_only=True)
    except TypeError:
        return torch.load(path, map_location="cpu")


def _format_float(value: float) -> str:
    return f"{value:.9g}f"


def _format_vector(values) -> str:
    return "{ " + ", ".join(_format_float(float(v)) for v in values) + " }"


def _format_matrix(tensor) -> str:
    rows = []
    for row in tensor.detach().cpu().tolist():
        rows.append("    " + _format_vector(row))
    return "{\n" + ",\n".join(rows) + "\n}"


def export_to_header(model_path: str | Path, header_path: str | Path) -> None:
    model_path = Path(model_path)
    header_path = Path(header_path)
    state = _load_state_dict(model_path)

    if "fc1.weight" not in state or "fc3.weight" not in state:
        raise ValueError(f"{model_path} does not look like an SSSnake DQN state_dict")

    hidden_size = int(state["fc1.weight"].shape[0])
    input_size = int(state["fc1.weight"].shape[1])
    output_size = int(state["fc3.weight"].shape[0])
    if input_size != INPUT_SIZE or output_size != OUTPUT_SIZE:
        raise ValueError(
            f"Expected DQN shape {INPUT_SIZE}->hidden->{OUTPUT_SIZE}, "
            f"got {input_size}->{hidden_size}->{output_size}"
        )

    model = DQN(input_size, hidden_size, output_size)
    model.load_state_dict(state)
    model.eval()

    header = f"""#pragma once

#include <cstdint>

static constexpr bool DQN_WEIGHTS_READY = true;
static constexpr uint16_t DQN_INPUT_SIZE = {input_size};
static constexpr uint16_t DQN_HIDDEN_SIZE = {hidden_size};
static constexpr uint16_t DQN_OUTPUT_SIZE = {output_size};

static constexpr float DQN_FC1_WEIGHTS[DQN_HIDDEN_SIZE][DQN_INPUT_SIZE] =
{_format_matrix(model.fc1.weight)};

static constexpr float DQN_FC1_BIAS[DQN_HIDDEN_SIZE] =
{_format_vector(model.fc1.bias.detach().cpu().tolist())};

static constexpr float DQN_FC2_WEIGHTS[DQN_HIDDEN_SIZE][DQN_HIDDEN_SIZE] =
{_format_matrix(model.fc2.weight)};

static constexpr float DQN_FC2_BIAS[DQN_HIDDEN_SIZE] =
{_format_vector(model.fc2.bias.detach().cpu().tolist())};

static constexpr float DQN_FC3_WEIGHTS[DQN_OUTPUT_SIZE][DQN_HIDDEN_SIZE] =
{_format_matrix(model.fc3.weight)};

static constexpr float DQN_FC3_BIAS[DQN_OUTPUT_SIZE] =
{_format_vector(model.fc3.bias.detach().cpu().tolist())};
"""
    header_path.write_text(header, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Export trained SSSnake DQN weights to a Pico C++ header.")
    parser.add_argument("--model", default="ai/dqn_snake.pth", help="Path to a trained PyTorch state_dict.")
    parser.add_argument("--out", default="dqn_weights.h", help="Output C++ header path.")
    args = parser.parse_args()

    export_to_header(args.model, args.out)
    print(f"Exported {args.model} -> {args.out}")


if __name__ == "__main__":
    main()
