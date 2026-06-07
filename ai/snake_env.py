import random
from enum import IntEnum


class Direction(IntEnum):
    UP = 0
    LEFT = 1
    DOWN = 2
    RIGHT = 3


TURN_RIGHT = {
    Direction.UP: Direction.RIGHT,
    Direction.RIGHT: Direction.DOWN,
    Direction.DOWN: Direction.LEFT,
    Direction.LEFT: Direction.UP,
}

TURN_LEFT = {
    Direction.UP: Direction.LEFT,
    Direction.LEFT: Direction.DOWN,
    Direction.DOWN: Direction.RIGHT,
    Direction.RIGHT: Direction.UP,
}


class SnakeEnv:
    """Headless Snake environment matching the Pico firmware state layout."""

    cols = 15
    rows = 15

    def __init__(self, seed: int | None = None):
        self.random = random.Random(seed)
        self.snake: list[tuple[int, int]] = []
        self.direction = Direction.RIGHT
        self.apple = (0, 0)
        self.score = 0
        self.steps_since_apple = 0
        self.reset()

    def reset(self):
        self.snake = [(2, 0), (1, 0), (0, 0)]
        self.direction = Direction.RIGHT
        self.score = 0
        self.steps_since_apple = 0
        self._place_apple()
        return self.get_state()

    def step(self, action: int):
        self.direction = self._action_to_direction(action)
        new_head = self._move_point(self.snake[0], self.direction)
        self.steps_since_apple += 1

        if self._is_wall(new_head) or self._hits_body(new_head, grow=(new_head == self.apple)):
            return self.get_state(), -10.0, True, {"score": self.score}

        reward = -0.01
        if new_head == self.apple:
            self.snake.insert(0, new_head)
            self.score += 1
            self.steps_since_apple = 0
            reward = 10.0
            if len(self.snake) == self.cols * self.rows:
                return self.get_state(), 25.0, True, {"score": self.score}
            self._place_apple()
        else:
            self.snake.insert(0, new_head)
            self.snake.pop()

        if self.steps_since_apple > self.cols * self.rows * 2:
            return self.get_state(), -5.0, True, {"score": self.score}

        return self.get_state(), reward, False, {"score": self.score}

    def get_state(self):
        head_x, head_y = self.snake[0]
        straight = self.direction
        right = TURN_RIGHT[self.direction]
        left = TURN_LEFT[self.direction]
        apple_x, apple_y = self.apple

        return [
            float(self._would_collide(straight)),
            float(self._would_collide(right)),
            float(self._would_collide(left)),
            float(self.direction == Direction.LEFT),
            float(self.direction == Direction.RIGHT),
            float(self.direction == Direction.UP),
            float(self.direction == Direction.DOWN),
            float(apple_x < head_x),
            float(apple_x > head_x),
            float(apple_y < head_y),
            float(apple_y > head_y),
        ]

    def _action_to_direction(self, action: int):
        if action == 1:
            return TURN_RIGHT[self.direction]
        if action == 2:
            return TURN_LEFT[self.direction]
        return self.direction

    def _would_collide(self, direction: Direction):
        new_head = self._move_point(self.snake[0], direction)
        return self._is_wall(new_head) or self._hits_body(new_head, grow=False)

    def _hits_body(self, point: tuple[int, int], grow: bool):
        body = self.snake if grow else self.snake[:-1]
        return point in body

    def _is_wall(self, point: tuple[int, int]):
        x, y = point
        return x < 0 or x >= self.cols or y < 0 or y >= self.rows

    def _move_point(self, point: tuple[int, int], direction: Direction):
        x, y = point
        if direction == Direction.UP:
            return x, y - 1
        if direction == Direction.LEFT:
            return x - 1, y
        if direction == Direction.DOWN:
            return x, y + 1
        return x + 1, y

    def _place_apple(self):
        occupied = set(self.snake)
        free = [
            (x, y)
            for y in range(self.rows)
            for x in range(self.cols)
            if (x, y) not in occupied
        ]
        self.apple = self.random.choice(free)
