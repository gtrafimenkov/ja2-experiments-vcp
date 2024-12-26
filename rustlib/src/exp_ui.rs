// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::state;

#[no_mangle]
pub extern "C" fn GetMapPanelDirty() -> bool {
    state::get().ui.map_panel_dirty
}

#[no_mangle]
pub extern "C" fn SetMapPanelDirty(value: bool) {
    state::get().ui.map_panel_dirty = value
}

#[no_mangle]
pub extern "C" fn GetMapScreenBottomDirty() -> bool {
    state::get().ui.map_screen_bottom_dirty
}

#[no_mangle]
pub extern "C" fn SetMapScreenBottomDirty(value: bool) {
    state::get().ui.map_screen_bottom_dirty = value
}
