#pragma once

// The game's ad hoc session as the menu sees it: which server the game uses,
// hosting one with the built-in server, and joining one.
#include "adhoc/server.hpp"

#include <string>

namespace mhp3rd {

// Applies changed network settings (hle_adhoc.cpp). Turning ad hoc play off
// takes the game off line at once, as a normal disconnect; a new server or
// nickname is used the next time the game goes on line, or at once with
// `switch_now`, which ends a session in progress as a disconnect.
void adhoc_apply_settings(bool switch_now = false);

// True while the game is in an ad hoc group or joining one, or hosting:
// pausing it then would stop it answering the other players.
[[nodiscard]] bool adhoc_session_active();

// The name other players see: the nickname setting, or the hunter name.
[[nodiscard]] std::string adhoc_player_name();

// The server the game goes on line with: this instance's own while it hosts,
// otherwise the Server setting. Empty for none.
[[nodiscard]] std::string adhoc_server_address();

// Hosting (adhoc/host.cpp) ------------------------------------------------------

// Starts the built-in server on the host port from the settings (27312 and
// the relay on 27313), announces it on the local network, turns ad hoc play
// on and points this instance's game at it. False when the server cannot
// start; adhoc_host_error() says why.
bool adhoc_host_start();
// Stops announcing and stops the server, which disconnects everyone; this
// instance's game goes back to the Server setting and, if it was on line,
// sees a normal disconnect.
void adhoc_host_stop();
[[nodiscard]] bool adhoc_hosting();
[[nodiscard]] std::string adhoc_host_error();
[[nodiscard]] adhoc::ServerStatus adhoc_host_status();

// Stops hosting, discovery and the client, and joins their threads. Called
// once when the game ends, however it ends, before static destructors run.
void adhoc_shutdown() noexcept;

// Joining: makes `address` ("host" or "host:port") the server, remembers it
// among the recent ones, turns ad hoc play on and uses it at once. Stops
// hosting first if this instance hosts.
void adhoc_join(const std::string &address);

} // namespace mhp3rd
