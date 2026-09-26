/**
 * @file        cvar_cli.h
 * @brief       CLI11 binding for the cvar registry
 *
 * @copyright   Copyright (c) 2026 Tom Clay
 * @license     BSD 3-Clause License
 *
 * @remarks     Header-only and opt-in so that CLI11 stays out of rex/cvar.h and
 *              out of the rexruntime ABI. Include it from tools that own their
 *              own CLI::App and cannot use the standalone cvar::Init pre-parse.
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <CLI/CLI.hpp>

#include <rex/cvar.h>

namespace rex::cvar {

/**
 * Adds a "--<name>" option for every registered flag to `app`. Boolean flags
 * also gain a "--no-<name>" form. Values go through the same validation as
 * SetFlagByName and are recorded as Source::kCommandLine.
 *
 * Call after all cvars have registered and before parsing. Set
 * `app.fallthrough()` beforehand if the tool uses subcommands and should accept
 * overrides spelled after the subcommand name.
 */
inline void RegisterCliOptions(CLI::App& app, std::string_view group = "Configuration Variables") {
  const std::string group_name(group);

  for (const auto& entry : GetRegistry()) {
    if (entry.type == FlagType::Command) {
      continue;
    }

    auto apply = [name = entry.name](const std::string& value) {
      if (!SetFlagFromCommandLine(name, value)) {
        throw CLI::ValidationError("--" + name, "invalid value '" + value + "'");
      }
    };

    CLI::Option* option = nullptr;
    if (entry.type == FlagType::Boolean) {
      option = app.add_flag_function(
          "--" + entry.name + ",!--no-" + entry.name,
          [apply](int64_t count) { apply(count > 0 ? "true" : "false"); }, entry.description);
    } else {
      option = app.add_option_function<std::string>("--" + entry.name, apply, entry.description);
    }
    option->group(group_name);
  }
}

}  // namespace rex::cvar
