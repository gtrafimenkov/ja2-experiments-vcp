// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::exp_debug;

use std::sync::OnceLock;
use std::{
    alloc::{alloc, Layout},
    collections::HashMap,
};

fn get_db() -> &'static mut DB {
    static mut ALLOC_DB: OnceLock<DB> = OnceLock::new();
    unsafe {
        ALLOC_DB.get_or_init(DB::new);
        return ALLOC_DB.get_mut().unwrap();
    }
}

pub struct DB {
    // mapping of allocated addresses to layout objects used for allocating these addresses
    alloc_map: HashMap<*mut u8, Layout>,
}

impl DB {
    fn new() -> Self {
        Self {
            alloc_map: HashMap::new(),
        }
    }

    fn store(&mut self, address: *mut u8, layout: Layout) {
        if self.alloc_map.contains_key(&address) {
            let message = format!("address {address:?} is already in the allocation map");
            exp_debug::debug_log_write(&message);
            panic!("{message}")
        }
        self.alloc_map.insert(address, layout);
    }

    fn pull(&mut self, address: *mut u8) -> Layout {
        match self.alloc_map.remove(&address) {
            Some(layout) => layout,
            None => {
                let message = format!("address {address:?} is not found in the allocation map");
                exp_debug::debug_log_write(&message);
                panic!("{message}")
            }
        }
    }
}

#[no_mangle]
/// Allocate memory in Rust.
/// The program will panic if memory cannot be allocated.
///
/// # Safety
///
/// This function is not thread-safe.
pub extern "C" fn RustAlloc(size: usize) -> *mut u8 {
    let layout = Layout::from_size_align(size, std::mem::align_of::<u8>()).unwrap();
    let buffer = unsafe { alloc(layout) };
    if buffer.is_null() {
        let message = format!("failed to allocate {size} bytes");
        exp_debug::debug_log_write(&message);
        panic!("{message}")
    }
    // exp_debug::debug_log_write(&format!("rust_alloc: allocated {size} bytes at {buffer:?}"));
    get_db().store(buffer, layout);
    buffer
}

#[no_mangle]
/// Deallocate memory allocated earlier in Rust.
///
/// # Safety
///
/// Pass only the pointer returned earlier by RustAlloc.
/// Don't deallocate memory more that once.
///
/// This function is not thread-safe.
pub unsafe extern "C" fn RustDealloc(pointer: *mut u8) {
    if !pointer.is_null() {
        // exp_debug::debug_log_write(&format!("rust_alloc: deallocating {pointer:?}"));
        unsafe {
            let layout = get_db().pull(pointer);
            std::alloc::dealloc(pointer, layout);
        }
    }
}

#[no_mangle]
/// Reallocate memory allocated earlier in Rust.
///
/// # Safety
///
/// Pass only the pointer returned earlier by RustAlloc.
///
/// This function is not thread-safe.
pub unsafe extern "C" fn RustRealloc(pointer: *mut u8, new_size: usize) -> *mut u8 {
    if !pointer.is_null() {
        // exp_debug::debug_log_write(&format!(
        //     "rust_realloc: reallocating {pointer:?} to {new_size} bytes"
        // ));
        unsafe {
            let layout = get_db().pull(pointer);
            let new_pointer = std::alloc::realloc(pointer, layout, new_size);
            if new_pointer.is_null() {
                let old_size = layout.size();
                let message = format!(
                    "rust_realloc: failed to reallocate from {old_size} bytes to {new_size} bytes"
                );
                exp_debug::debug_log_write(&message);
                panic!("{message}");
            }
            get_db().store(new_pointer, layout);
            new_pointer
        }
    } else {
        RustAlloc(new_size)
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;

//     #[test]
//     fn simple_alloc_dealloc() {
//         let buf = RustAlloc(10);
//         assert!(!buf.is_null());
//         unsafe {
//             RustDealloc(buf);
//         }
//     }

//     #[test]
//     fn realloc() {
//         let p1 = RustAlloc(10);
//         assert!(!p1.is_null());
//         unsafe {
//             let p2 = RustRealloc(p1, 2048);
//             RustDealloc(p2);
//         }
//     }

//     #[test]
//     #[should_panic]
//     fn double_dealloc() {
//         let buf = RustAlloc(100);
//         assert!(!buf.is_null());
//         unsafe {
//             RustDealloc(buf);
//             RustDealloc(buf);
//         }
//     }
// }
