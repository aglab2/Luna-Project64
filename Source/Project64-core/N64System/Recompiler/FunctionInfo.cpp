#include "stdafx.h"
#include <Project64-core/N64System/Recompiler/FunctionInfo.h>

CCompiledFunc::CCompiledFunc( const CCodeBlock & CodeBlock ) :
    m_EnterPC(CodeBlock.VAddrEnter()),
    m_MinPC(CodeBlock.VAddrFirst()),
    m_MaxPC(CodeBlock.VAddrLast()),
    m_Hash(CodeBlock.Hash()),
    m_Function((Func)CodeBlock.CompiledLocation()),
    m_FunctionEnd(CodeBlock.CompiledLocationEnd()),
    m_Next(nullptr)
{
    m_MemContents = CodeBlock.MemContents();
    m_MemLocation = CodeBlock.MemLocation();

#if defined(__arm__) || defined(_M_ARM)
    // Make sure function starts at an odd address so that the system knows it is in thumb mode
    if ((((uint32_t)m_Function) % 2) == 0)
    {
        m_Function = (Func)(((uint32_t)m_Function) + 1);
    }
#endif
}