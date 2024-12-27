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

#[repr(C)]
#[allow(non_snake_case)]
pub struct SGPRect {
    iLeft: i32,
    iTop: i32,
    iRight: i32,
    iBottom: i32,
}

#[no_mangle]
pub extern "C" fn SGPRect_new(left: i32, top: i32, right: i32, bottom: i32) -> SGPRect {
    SGPRect {
        iLeft: left,
        iTop: top,
        iRight: right,
        iBottom: bottom,
    }
}

#[no_mangle]
pub extern "C" fn SGPRect_set(r: *mut SGPRect, left: i32, top: i32, right: i32, bottom: i32) {
    unsafe {
        (*r).iLeft = left;
        (*r).iTop = top;
        (*r).iRight = right;
        (*r).iBottom = bottom;
    }
}

#[repr(C)]
#[allow(non_snake_case)]
pub struct SGPPoint {
    iX: i32,
    iY: i32,
}

#[no_mangle]
pub extern "C" fn SGPPoint_new(x: i32, y: i32) -> SGPPoint {
    SGPPoint { iX: x, iY: y }
}

#[no_mangle]
pub extern "C" fn SGPPoint_set(r: *mut SGPPoint, x: i32, y: i32) {
    unsafe {
        (*r).iX = x;
        (*r).iY = y;
    }
}

#[repr(C)]
#[allow(non_snake_case)]
pub struct SGPBox {
    x: i32,
    y: i32,
    w: i32,
    h: i32,
}

#[no_mangle]
pub extern "C" fn SGPBox_set(r: *mut SGPBox, x: i32, y: i32, w: i32, h: i32) {
    unsafe {
        (*r).x = x;
        (*r).y = y;
        (*r).w = w;
        (*r).h = h;
    }
}

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
