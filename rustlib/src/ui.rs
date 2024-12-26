// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

pub struct State {
    pub map_panel_dirty: bool,
    pub map_screen_bottom_dirty: bool, // the dirty state of the mapscreen interface bottom
}

impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}

impl State {
    pub fn new() -> Self {
        State {
            map_panel_dirty: true,
            map_screen_bottom_dirty: true,
        }
    }
}
