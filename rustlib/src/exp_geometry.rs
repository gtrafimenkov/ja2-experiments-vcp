// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#[repr(C)]
#[allow(non_snake_case)]
// #[derive(Default, Copy, Clone)]
/// Copy of SGPRect from the C codebase.
pub struct GRect {
    iLeft: i32,
    iTop: i32,
    iRight: i32,
    iBottom: i32,
}

#[repr(C)]
// #[derive(Default, Copy, Clone)]
/// Copy of Rect from the C codebase.
pub struct Rect {
    left: i32,
    top: i32,
    right: i32,
    bottom: i32,
}

// struct Box {
//     left: u16,
//     top: u16,
//     width: u16,
//     height: u16,
// }

#[no_mangle]
/// Create new GRect structure
pub extern "C" fn NewGRect(left: i32, top: i32, width: i32, height: i32) -> GRect {
    GRect {
        iLeft: left,
        iTop: top,
        iRight: left + width,
        iBottom: top + height,
    }
}

#[no_mangle]
/// Create new Rect structure
pub extern "C" fn NewRect(left: i32, top: i32, width: i32, height: i32) -> Rect {
    Rect {
        left,
        top,
        right: left + width,
        bottom: top + height,
    }
}
