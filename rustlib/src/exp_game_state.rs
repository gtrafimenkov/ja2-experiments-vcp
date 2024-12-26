// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::state;

/// Handle game restart.
#[no_mangle]
pub extern "C" fn PrepareGameRestart() {
    state::get().clock.game_paused = true
}
