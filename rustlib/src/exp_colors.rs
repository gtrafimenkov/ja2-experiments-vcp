// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::colors;

/// Convert RGB565 value to RGB32.
#[no_mangle]
pub extern "C" fn rgb565_to_rgb32(rgb565: u16) -> u32 {
    colors::rgb565_to_rgb32(rgb565)
}

/// Convert RGB32 value to RGB32.
#[no_mangle]
pub extern "C" fn rgb32_to_rgb565(rgb565: u32) -> u16 {
    colors::rgb32_to_rgb565(rgb565)
}
