// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2018-2020 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

/// @file
/// EVMC instance and entry point of evmone is defined here.
/// The file name matches the evmone.h public header.

#include "baseline.hpp"
#include "execution.hpp"
#include <evmone/evmone.h>

#include "instruction_traits.hpp"
#include <evmc/instructions.h>
#include <cstdio>
#include <cstring>
#include <map>

namespace evmone
{
namespace
{
struct BasicTracer : VMTracer
{
    std::map<evmc_opcode, int> opcode_counter;

    void onBeginExecution() noexcept final { opcode_counter.clear(); }

    void onOpcode(evmc_opcode opcode) noexcept final
    {
        std::puts(instr::traits[opcode].name);
        ++opcode_counter[opcode];
    }

    void onEndExecution() noexcept final
    {
        std::puts("\nTotal:");
        for (const auto [opcode, count] : opcode_counter)
            printf("%s,%d\n", instr::traits[opcode].name, count);
    }
};

void destroy(evmc_vm* vm) noexcept
{
    // TODO: Mark function with [[gnu:nonnull]] or add CHECK().
    delete static_cast<VM*>(vm);
}

constexpr evmc_capabilities_flagset get_capabilities(evmc_vm* /*vm*/) noexcept
{
    return EVMC_CAPABILITY_EVM1;
}

evmc_set_option_result set_option(evmc_vm* vm, char const* name, char const* value) noexcept
{
    if (name[0] == 'O' && name[1] == '\0')
    {
        if (value[0] == '0' && value[1] == '\0')  // O=0
        {
            vm->execute = evmone::baseline_execute;
            return EVMC_SET_OPTION_SUCCESS;
        }
        else if (value[0] == '2' && value[1] == '\0')  // O=2
        {
            vm->execute = evmone::execute;
            return EVMC_SET_OPTION_SUCCESS;
        }
        return EVMC_SET_OPTION_INVALID_VALUE;
    }
    else if (std::strcmp(name, "trace") == 0)
    {
        static_cast<VM*>(vm)->tracer = std::make_unique<BasicTracer>();
        return EVMC_SET_OPTION_SUCCESS;
    }
    return EVMC_SET_OPTION_INVALID_NAME;
}

}  // namespace


inline constexpr VM::VM() noexcept
  : evmc_vm{
        EVMC_ABI_VERSION,
        "evmone",
        PROJECT_VERSION,
        evmone::destroy,
        evmone::execute,
        evmone::get_capabilities,
        evmone::set_option,
    }
{}

}  // namespace evmone

extern "C" {
EVMC_EXPORT evmc_vm* evmc_create_evmone() noexcept
{
    return new evmone::VM{};
}
}
