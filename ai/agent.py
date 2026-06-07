import random
import torch
import torch.nn as nn
import torch.optim as optim

from model import DQN
from replay_memory import ReplayMemory

class DQNAgent:
    def __init__(
        self,
        input_size: int = 11,
        hidden_size: int = 128,
        output_size: int = 3,
        gamma: float = 0.9,
        lr: float = 0.001,
        batch_size: int = 1000,
        epsilon_start: float = 1.0,
        epsilon_end: float = 0.01,
        epsilon_decay: float = 0.995,
        target_update: int = 10,
        memory_capacity: int = 100_000,
    ):

        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.gamma = gamma
        self.batch_size = batch_size
        self.epsilon = epsilon_start
        self.epsilon_end = epsilon_end
        self.epsilon_decay = epsilon_decay
        self.target_update = target_update

        self.policy_net = DQN(input_size, hidden_size, output_size).to(self.device)
        self.target_net = DQN(input_size, hidden_size, output_size).to(self.device)
        self.target_net.load_state_dict(self.policy_net.state_dict())
        self.target_net.eval()

        self.optimizer = optim.Adam(self.policy_net.parameters(), lr=lr)
        self.loss_fn = nn.SmoothL1Loss()
        self.memory = ReplayMemory(memory_capacity)
        self.games_played = 0
    
    def choose_action(self, state, training: bool = True):
        if training and random.random() < self.epsilon:
            return random.randint(0, 2)
        state = torch.tensor(state, dtype=torch.float32, device=self.device).unsqueeze(0)

        with torch.no_grad():
            q_values = self.policy_net(state)
        
        return q_values.argmax().item()
    
    def remember(self, state, action, reward, next_state, done):
        self.memory.push(state, action, reward, next_state, done)
    
    def train_step(self):
        if len(self.memory) < self.batch_size:
            return None
        
        batch = self.memory.sample(self.batch_size)
        states, actions, rewards, next_states, dones = zip(*batch)

        states = torch.tensor(states, dtype=torch.float32, device=self.device)
        actions = torch.tensor(actions, dtype=torch.int64, device=self.device).unsqueeze(1)
        rewards = torch.tensor(rewards, dtype=torch.float32, device=self.device).unsqueeze(1)
        next_states = torch.tensor(next_states, dtype=torch.float32, device=self.device)
        dones = torch.tensor(dones, dtype=torch.float32, device=self.device).unsqueeze(1)

        current_q_values = self.policy_net(states).gather(1, actions)
        with torch.no_grad():
            max_next_q_values = self.target_net(next_states).max(1)[0].unsqueeze(1)
            target_q_values = rewards + (self.gamma * max_next_q_values * (1 - dones))
        
        loss = self.loss_fn(current_q_values, target_q_values)

        self.optimizer.zero_grad()
        loss.backward()
        torch.nn.utils.clip_grad_norm_(self.policy_net.parameters(), max_norm=10.0)
        self.optimizer.step()
        return float(loss.item())
        
    def end_game_update(self):
        self.games_played += 1

        self.epsilon = max(self.epsilon_end, self.epsilon * self.epsilon_decay)

        if self.games_played % self.target_update == 0:
            self.target_net.load_state_dict(self.policy_net.state_dict())

    def save(self, path="double_dqn_snake.pth"):
        torch.save(self.policy_net.state_dict(), path)

    def load(self, path="double_dqn_snake.pth"):
        try:
            state_dict = torch.load(path, map_location=self.device, weights_only=True)
        except TypeError:
            state_dict = torch.load(path, map_location=self.device)
        self.policy_net.load_state_dict(state_dict)
        self.target_net.load_state_dict(self.policy_net.state_dict())
