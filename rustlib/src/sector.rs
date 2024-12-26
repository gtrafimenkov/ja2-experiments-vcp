// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

pub const MAP_WORLD_X: i16 = 18;
pub const MAP_WORLD_Y: i16 = 18;

pub struct Point {
    pub x: u8,
    pub y: u8,
}

impl Point {
    pub const fn new(x: u8, y: u8) -> Self {
        debug_assert!(valid_coords(x, y));
        Point { x, y }
    }
}

pub const fn valid_coords(x: u8, y: u8) -> bool {
    (x >= 1) && (x <= 16) && (y >= 1) && (y <= 16)
}

#[derive(Copy, Clone)]
pub struct State {
    pub enemy_controlled: bool,
    pub enemy_air_controlled: bool,
}

impl State {
    pub const fn new() -> Self {
        Self {
            enemy_controlled: false,
            enemy_air_controlled: false,
        }
    }
}
