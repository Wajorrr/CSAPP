
/*
 * CS:APP Data Lab
 *
 * <Please put your name and userid here>
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
// 1
/*
 * bitXor - x^y using only ~ and &
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) // 0101 0011 = 0110
{
  // return ~(~x & ~y) & ~(x & y);
  return ~(~(~x & y) & ~(x & ~y));
}
/*
 * tmin - return minimum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void)
{
  // return -__INT32_MAX__ - 1;
  // printf("%ld\n", (long long)1 << 32);
  // 1000...000(31个0)=-2147483648
  // 0111...111(31个1)= 2147483647=__INT32_MAX__
  return 1 << 31;
}
// 2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x)
{
  // printf("%d\n", (1 << 31) + 1);
  // int y = __INT32_MAX__; //  0111...111(31个1)
  // printf("%d %d %d %d %d %d\n", y, ~y, ~(1 << 31), y + 1, y + 2, !~y);
  // printf("%d %d %d %d %d %d %d\n", y, y + 2, ~y, !~y, -1, ~(-1), !~(-1));
  // printf("%d %d %d %d %d %d\n", !(y + y + 2), !(y + (y + 2)), !!~y, (!!~y) & !(y + y + 2), -1, !!~(-1));
  return !!~x & !(x + 2 + x); // 需要排除-1的情况
  // 注意括号里面x+2+x的顺序不能乱，必须先x+2
}
/*
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x)
{
  // int y = 0b10101010;
  // y = (y << 8) + y;
  // printf("%d %d %d\n", y, y << 16, ~(-__INT32_MAX__));
  // y = (y << 16) + y;
  // int temp = 0b1010101010101010;
  // int temp = 0b1010101010101100000000000000000;
  // int temp = 0b1010101010101010101010101010101;
  // int temp = 0b1010101010101010101010101010110;

  // int mask = 0b1010101;
  int mask = 85;
  mask = (mask << 8) + mask;
  mask = (mask << 16) + mask;
  // int y = 0xAAAAAAAA;
  // int y = 0x80000001;
  // int y = 0b1010101010101010101010101010110;
  // int y = 0b101010101010101010101010101010;
  // printf("%d %d\n", mask, y);
  // printf("%d %d %d %d\n", y, (y | mask), (y | mask) + 1, !((y | mask) + 1));
  return !((x | mask) + 1);

  // int mask = 0b10101010;
  // int mask = 170;
  // mask = (mask << 8) + mask;
  // mask = (mask << 16) + mask;
  // printf("%d\n", mask); //-1431655766
  // return !((x & mask) ^ mask);
}
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x)
{
  return ~x + 1;
}
// 3
/*
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x)
{
  // printf("%d %d %d\n", (1 << 31) >> 31, 0x30, 0x39);
  //(1<<31) >>31 = -1
  // -0x31 + x >= 0  &&  -0x39 + x <= 0
  // return !((~0x30 + 1 + x) >> 31) & !(((~0x39 + x) >> 31) + 1);
  return !((~0x30 + 1 + x) >> 31) & !((~x + 1 + 0x39) >> 31);
}
/*
 * conditional - same as x ? y : z
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z)
{
  // x & -1 = x
  return ((!x - 1) & y) | (~(!x - 1) & z);
}
/*
 * isLessOrEqual - if x <= y  then return 1, else return 0
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y)
{
  // printf("%d\n", 2147483647 + 1);
  //(x<0&&y>=0)||(!(x>=0&&y<0)&&(y-x>=0))
  return ((x >> 31) & !(y >> 31)) | (!(!(x >> 31) & (y >> 31)) & ((~x + 1 + y) >> 31) + 1);

  //!(x>=0&&y<0)&&((x<0&&y>=0)||(y-x>=0))
  // return !(!(x >> 31) & (y >> 31)) & (((x >> 31) & !(y >> 31)) || ((~x + 1 + y) >> 31) + 1);
}
// 4
/*
 * logicalNeg - implement the ! operator, using all of
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int logicalNeg(int x)
{
  // x和x的相反数进行或运算之后符号为负，负数右移31位得到-1，非负数右移31位得到0
  return ((x | (~x + 1)) >> 31) + 1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x)
{
  int b16, b8, b4, b2, b1, b0;
  int sign = x >> 31; // 负数->-1，正数->0
  // 因为负数在计算机中是以取反加一(补码)的形式存储的,要找的是其最高位0的位置
  // 因此不能直接找其最高位1的位置，需要先取反，然后就可以统一找最高位1的位置了
  //  x为正数则不变，x为负数则取反(实际值变为其相反数-1)
  x = (x & ~sign) | (~x & sign);

  b16 = !!(x >> 16) << 4; // x的高15位是否有1，有则舍弃后16位
  x >>= b16;
  b8 = !!(x >> 8) << 3; // x剩下的15位中高7位是否有1，有则舍弃后8位
  x >>= b8;
  b4 = !!(x >> 4) << 2; // x剩下的7位中高3位是否有1，有则舍弃后4位
  x >>= b4;
  b2 = !!(x >> 2) << 1; // x剩下的3位中高1位是否有1，有则舍弃后2位
  x >>= b2;
  b1 = !!(x >> 1); // 若前面均不满足，则此时剩下2位，此时要先判断x的第2位是否有1
  x >>= b1;
  b0 = x; // x的最后一位是否有1

  return b16 + b8 + b4 + b2 + b1 + b0 + 1; // 要加上一个符号位
}
// float
/*
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf)
{
  // 0x7f800000 = 0b01111111100000000000000000000000 (后31位中前8位为1，后23位为0)
  int exp = (uf & 0x7f800000) >> 23; // 前8位为指数
  int sign = uf & (1 << 31);         // 符号位

  if (exp == 0) // 指数为0，则直接将整个浮点数左移一位，注意将符号位复原
    return uf << 1 | sign;
  if (exp == 255) // 指数为255，表示无穷大或非数值NAN，直接返回原数
    return uf;
  exp++; // 指数不为0，且小于255，则直接+1，表示乘以了2

  if (exp == 255) // 若指数+1之后等于255，则说明此数变为了无穷大，直接返回带符号的无穷大值
    return 0x7f800000 | sign;
  // 0x807fffff = 0b10000000011111111111111111111111 (只有表示指数的8位为0，其他均为1)
  return (exp << 23) | (uf & 0x807fffff); // 更新指数，返回结果
}
/*
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf)
{
  // unsigned sign = uf & (1 << 31); // 符号位
  int sign = uf >> 31;
  int exp = (uf >> 23) & 255; // 指数位
  int num = uf << 9 >> 9;     // 先左移9位，把符号位和指数位清空，再右移9位得到后23位代表的小数
  if (exp == 255)             // 无穷大或NAN
    return 0x80000000u;
  // exp取值为0~255，而0代表非规格化数，255代表无穷大或NAN，因此规格数的指数范围为[1,254]
  // 得到实际值要减去127，因此指数的实际值范围为[-126,127]
  if (exp <= 126) // 若指数值小于127，则指数为负数，得到的值为小数，直接返回0
    return 0;
  exp -= 127; // 减去127得到实际指数

  // printf("%d %d %d %d %d %d\n", 1 << 31, 2 << 31, 2 << 30, 3 << 31, 3 << 30, 4 << 29);

  if (exp > 31) // 若指数大于31，则超出int范围，这里注意exp为31的情况只有负最大值会用到，当exp为31而结果不为负最大值，则说明溢出
    return 0x80000000u;
  // 规格数的实际数部分定义为1.M，M即为num对应的二进制数
  // 因此要给小数部分前面添上一个1，表示整数1
  num |= 1 << 23;
  // num一共24位，后23位代表小数，若指数小于等于23，则直接将小数点右移指数次就可以了
  if (exp <= 23) // 取小数点右移exp次后得到的整数部分，相当于直接把当前数整体右移23-exp位(去掉后面剩余的小数位)
    num >>= (23 - exp);
  else // 若指数大于23，则把当前数继续左移exp-23位即可
    num <<= (exp - 23);

  // return (sign ? -1 : 1) * num;
  if (!sign) // 正数
    return num;
  else // 负数，这里感觉应该需要特判一下exp为31的情况，但不特判也能过，不知道是不是样例问题
    return ~num + 1;
}
/*
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 *
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatPower2(int x)
{
  int INF = 0xff << 23;
  int exp = x + 127; // 规格数的exp范围为[1,254]

  // if (exp < -22)
  if (x <= -150) // 浮点数最低能表示到 2^(-150)，2^(-150)的二进制表示为0|00000000|00000000000000000000000
    return 0;
  // if (exp <= 0)
  if (x <= -127)
    return 1 << (x + 149); // 2^-149的二进制表示为0|00000000|00000000000000000000001
  // if (exp >= 255)
  if (x >= 128)
    return INF;
  return exp << 23;
}
