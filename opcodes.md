# Virtual machine operation

EvaVM is a stack based virtual machine. Operands are pushed on the stack and operators pop the needed number of
operands and push the result back on the stack.

For example, the sequence:

```
push(3)
push(4)
ADD
```

will result in two values popped and one pushed on the stack (in this case 7).

Addresses are two bytes big endian. For example, `OP_JMP` specifies `<high_byte>` and `<low_byte>`.

# Opcode description

## (0x00) OP_HALT

**Stack requirements**: None

**Stack effect**: None

**Remarks**: End the virtual machine operation.

## (0x01) OP_CONST `<index>`

**Stack requirements**: None

**Stack effect**: push one

**Remarks**: Pushes the constant at `index` on the stack

## (0x02) OP_ADD

**Stack requirements**: pop two

**Stack effect**: push one

**Remarks**: Add two values from the stack and push the result

## (0x03) OP_SUB

**Stack requirements**: pop two

**Stack effect**: push one

**Remarks**: Subtract two values from the stack and push the result

## (0x04) OP_MUL

**Stack requirements**: pop two

**Stack effect**: push one

**Remarks**: Subtract two values from the stack and push the result

## (0x05) OP_DIV

**Stack requirements**: pop two

**Stack effect**: push one

**Remarks**: Subtract two values from the stack and push the result

## (0x06) OP_COMP `<comparison>`

**Stack requirements**: pop two

**Stack effect**: push one

**Remarks**: Compare two values from the stack using `comparison` and push the result. The comparison operation may be:

  * 0x0: greater than
  * 0x5: equal
  * 0x6: not equal

## (0x06) OP_JMP_IF_FALSE `<address>`

**Stack requirements**: pop one

**Stack effect**: none

**Remarks**: If the value from the stack is false, jumps to the address specified.
Addresses are 16 bit values

## (0x06) OP_JMP `<address>`

**Stack requirements**: none

**Stack effect**: none

**Remarks**: Jumps to the address specified.
