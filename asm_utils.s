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
