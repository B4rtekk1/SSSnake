import argparse
from pathlib import Path

from agent import DQNAgent
from export_model import export_to_header
from snake_env import SnakeEnv


def train(args):
    env = SnakeEnv(seed=args.seed)
    agent = DQNAgent(
        hidden_size=args.hidden_size,
        gamma=args.gamma,
        lr=args.lr,
        batch_size=args.batch_size,
        epsilon_start=args.epsilon_start,
        epsilon_end=args.epsilon_end,
        epsilon_decay=args.epsilon_decay,
        target_update=args.target_update,
        memory_capacity=args.memory_capacity,
    )

    model_path = Path(args.model)
    if args.resume and model_path.exists():
        agent.load(model_path)

    best_score = -1
    recent_scores: list[int] = []
    for episode in range(1, args.episodes + 1):
        state = env.reset()
        total_reward = 0.0
        last_loss = None

        for _ in range(args.max_steps):
            action = agent.choose_action(state, training=True)
            next_state, reward, done, info = env.step(action)
            agent.remember(state, action, reward, next_state, done)
            loss = agent.train_step()
            if loss is not None:
                last_loss = loss
            state = next_state
            total_reward += reward
            if done:
                break

        agent.end_game_update()
        score = int(info["score"])
        recent_scores.append(score)
        recent_scores = recent_scores[-100:]

        if score > best_score:
            best_score = score
            model_path.parent.mkdir(parents=True, exist_ok=True)
            agent.save(model_path)

        if episode % args.log_every == 0 or episode == 1:
            avg = sum(recent_scores) / len(recent_scores)
            loss_text = "n/a" if last_loss is None else f"{last_loss:.4f}"
            print(
                f"episode={episode} score={score} best={best_score} "
                f"avg100={avg:.2f} epsilon={agent.epsilon:.3f} "
                f"reward={total_reward:.2f} loss={loss_text}"
            )

    model_path.parent.mkdir(parents=True, exist_ok=True)
    agent.save(model_path)
    if args.export:
        export_to_header(model_path, args.header)
        print(f"Exported weights to {args.header}")


def main():
    parser = argparse.ArgumentParser(description="Train the SSSnake DQN agent.")
    parser.add_argument("--episodes", type=int, default=1000)
    parser.add_argument("--max-steps", type=int, default=1500)
    parser.add_argument("--hidden-size", type=int, default=128)
    parser.add_argument("--batch-size", type=int, default=1000)
    parser.add_argument("--memory-capacity", type=int, default=100_000)
    parser.add_argument("--gamma", type=float, default=0.9)
    parser.add_argument("--lr", type=float, default=0.001)
    parser.add_argument("--epsilon-start", type=float, default=1.0)
    parser.add_argument("--epsilon-end", type=float, default=0.01)
    parser.add_argument("--epsilon-decay", type=float, default=0.995)
    parser.add_argument("--target-update", type=int, default=10)
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--log-every", type=int, default=25)
    parser.add_argument("--model", default="ai/dqn_snake.pth")
    parser.add_argument("--header", default="dqn_weights.h")
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--no-export", dest="export", action="store_false")
    parser.set_defaults(export=True)
    args = parser.parse_args()

    train(args)


if __name__ == "__main__":
    main()
