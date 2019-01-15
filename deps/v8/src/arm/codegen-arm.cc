// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if V8_TARGET_ARCH_ARM

#include <memory>

#include "src/arm/assembler-arm-inl.h"
#include "src/arm/simulator-arm.h"
#include "src/macro-assembler.h"

namespace v8 {
namespace internal {

#define __ masm.

#if defined(V8_HOST_ARCH_ARM)

MemCopyUint8Function CreateMemCopyUint8Function(MemCopyUint8Function stub) {
#if defined(USE_SIMULATOR)
  return stub;
#else
  v8::PageAllocator* page_allocator = GetPlatformPageAllocator();
  size_t allocated = 0;
  byte* buffer = AllocatePage(page_allocator,
                              page_allocator->GetRandomMmapAddr(), &allocated);
  if (buffer == nullptr) return stub;

  MacroAssembler masm(AssemblerOptions{}, buffer, static_cast<int>(allocated));

  Register dest = r0;
  Register src = r1;
  Register chars = r2;
  Register temp1 = r3;
  Label less_4;

  {
    UseScratchRegisterScope temps(&masm);
    Register temp2 = temps.Acquire();
    Label loop;

    __ bic(temp2, chars, Operand(0x3), SetCC);
    __ b(&less_4, eq);
    __ add(temp2, dest, temp2);

    __ bind(&loop);
    __ ldr(temp1, MemOperand(src, 4, PostIndex));
    __ str(temp1, MemOperand(dest, 4, PostIndex));
    __ cmp(dest, temp2);
    __ b(&loop, ne);
  }

  __ bind(&less_4);
  __ mov(chars, Operand(chars, LSL, 31), SetCC);
  // bit0 => Z (ne), bit1 => C (cs)
  __ ldrh(temp1, MemOperand(src, 2, PostIndex), cs);
  __ strh(temp1, MemOperand(dest, 2, PostIndex), cs);
  __ ldrb(temp1, MemOperand(src), ne);
  __ strb(temp1, MemOperand(dest), ne);
  __ Ret();

  CodeDesc desc;
  masm.GetCode(nullptr, &desc);
  DCHECK(!RelocInfo::RequiresRelocationAfterCodegen(desc));

  Assembler::FlushICache(buffer, allocated);
  CHECK(SetPermissions(page_allocator, buffer, allocated,
                       PageAllocator::kReadExecute));
  return FUNCTION_CAST<MemCopyUint8Function>(buffer);
#endif
}


// Convert 8 to 16. The number of character to copy must be at least 8.
MemCopyUint16Uint8Function CreateMemCopyUint16Uint8Function(
    MemCopyUint16Uint8Function stub) {
#if defined(USE_SIMULATOR)
  return stub;
#else
  v8::PageAllocator* page_allocator = GetPlatformPageAllocator();
  size_t allocated = 0;
  byte* buffer = AllocatePage(page_allocator,
                              page_allocator->GetRandomMmapAddr(), &allocated);
  if (buffer == nullptr) return stub;

  MacroAssembler masm(AssemblerOptions{}, buffer, static_cast<int>(allocated));

  Register dest = r0;
  Register src = r1;
  Register chars = r2;

  {
    UseScratchRegisterScope temps(&masm);

    Register temp1 = r3;
    Register temp2 = temps.Acquire();
    Register temp3 = lr;
    Register temp4 = r4;
    Label loop;
    Label not_two;

    __ Push(lr, r4);
    __ bic(temp2, chars, Operand(0x3));
    __ add(temp2, dest, Operand(temp2, LSL, 1));

    __ bind(&loop);
    __ ldr(temp1, MemOperand(src, 4, PostIndex));
    __ uxtb16(temp3, temp1);
    __ uxtb16(temp4, temp1, 8);
    __ pkhbt(temp1, temp3, Operand(temp4, LSL, 16));
    __ str(temp1, MemOperand(dest));
    __ pkhtb(temp1, temp4, Operand(temp3, ASR, 16));
    __ str(temp1, MemOperand(dest, 4));
    __ add(dest, dest, Operand(8));
    __ cmp(dest, temp2);
    __ b(&loop, ne);

    __ mov(chars, Operand(chars, LSL, 31), SetCC);  // bit0 => ne, bit1 => cs
    __ b(&not_two, cc);
    __ ldrh(temp1, MemOperand(src, 2, PostIndex));
    __ uxtb(temp3, temp1, 8);
    __ mov(temp3, Operand(temp3, LSL, 16));
    __ uxtab(temp3, temp3, temp1);
    __ str(temp3, MemOperand(dest, 4, PostIndex));
    __ bind(&not_two);
    __ ldrb(temp1, MemOperand(src), ne);
    __ strh(temp1, MemOperand(dest), ne);
    __ Pop(pc, r4);
  }

  CodeDesc desc;
  masm.GetCode(nullptr, &desc);

  Assembler::FlushICache(buffer, allocated);
  CHECK(SetPermissions(page_allocator, buffer, allocated,
                       PageAllocator::kReadExecute));
  return FUNCTION_CAST<MemCopyUint16Uint8Function>(buffer);
#endif
}
#endif

#undef __

}  // namespace internal
}  // namespace v8

#endif  // V8_TARGET_ARCH_ARM
