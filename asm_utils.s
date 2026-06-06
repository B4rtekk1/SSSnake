.syntax unified
.cpu cortex-m33
.thumb

.global asm_is_opposite
.type asm_is_opposite, %function

asm_is_opposite:
eors r0, r0, r1
cmp r0, #2
ite eq
moveq r0, #1
movne r0, #0
bx lr

.global asm_snake_collision
.type asm_snake_collision, %function

@r0 = pointer to snake array
@r1 = limit (number of segments to check)
@r2 = x coordinate of new head
@r3 = y coordinate of new head
@returns 1 if collision, 0 otherwise
asm_snake_collision:
push {r4, lr} @ Save registers and return address

cmp r1, #1
bls no_collision @ If limit <= 1, no collision possible

adds r0, r0, #2 @ Move pointer to second segment (skip head - snake[0])
subs r1, r1, #1 @ Decrease limit by 1 (we've skipped the head)

loop:
ldrb r4, [r0] @ Load x coordinate of current segment
cmp r4, r2 @ Compare with new head x
bne next

ldrb r4, [r0, #1] @ Load y coordinate of current segment
cmp r4, r3 @ Compare with new head y
beq collision @ If both match, we have a collision
next:
adds r0, r0, #2 @ Move to next segment (each segment is 2 bytes)
subs r1, r1, #1 @ Decrease limit
bne loop

no_collision:
movs r0, #0 @ No collision
pop {r4, pc} @ Restore registers and return

collision:
movs r0, #1 @ Collision detected
pop {r4, pc} @ Restore registers and return

.global asm_shift_snake
.type asm_shift_snake, %function

@r0 = pointer to snake array
@r1 = length of snake
asm_shift_snake:
push {r4}
cmp r1, #0
beq shift_done @ If length is 0, nothing to shift

@ r2 = destination pointer: snake&[length]
add r2, r0, r1, lsl #1 @ Point to the end of the snake array (after the last segment)
@r3 = source pointer: &snake[length-1]
subs r3, r2, #2

shift_loop:
ldrh r4, [r3] @ Load x/y bytes of current segment
strh r4, [r2] @ Store them at the destination

subs r2, r2, #2 @ Move destination pointer back by 2 bytes
subs r3, r3, #2 @ Move source pointer back by 2 bytes
subs r1, r1, #1 @ Decrease length
bne shift_loop

shift_done:
pop {r4}
bx lr @ Return from function

.global asm_clear_grid
.type asm_clear_grid, %function

@r0 = pointer to grid array
@r1 = size of grid (number of cells)
asm_clear_grid:
movs r2, #0
cmp r1, #0
beq clear_done @ If size is 0, nothing to clear

clear_loop:
strb r2, [r0], #1 @ Store 0 (EMPTY) in current cell and move to next
subs r1, r1, #1 @ Decrease size
bne clear_loop
clear_done:
bx lr @ Return from function

.global asm_place_apple
.type asm_place_apple, %function

@r0 = pointer to grid array
@r1 = total number of cells in grid
@r2 = target empty cell index
@ return 1 if apple placed, 0 otherwise

asm_place_apple:
cmp r1, #0
beq apple_not_found @ If grid size is 0, can't place apple

apple_loop:
ldrb r3, [r0] @ Load current cell state
cmp r3, #0 @ Check if cell is EMPTY
bne apple_next

cmp r2, #0
beq apple_found @ If target index is 0, place apple here
subs r2, r2, #1 @ Decrease target index

apple_next:
adds r0, r0, #1 @ Move to next cell
subs r1, r1, #1 @ Decrease cell count
bne apple_loop

apple_not_found:
movs r0, #0 @ No empty cell found
bx lr

apple_found:
movs r3, #2 @ APPLE
strb r3, [r0]
movs r0, #1 @ Apple placed successfully
bx lr
