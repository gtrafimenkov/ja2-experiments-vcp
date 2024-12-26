// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

//! Exported functions to implement platform-dependent code.

use super::platform;
use std::env;
use std::ffi::{c_char, CStr};
use std::fs;
use std::path::PathBuf;

/// Converts utf-8 encoded null-terminated C string into PathBuf.
fn cstr_utf8_to_pathbuf(cstr: *const c_char) -> Option<PathBuf> {
    unsafe {
        if cstr.is_null() {
            return None;
        }
        let str = CStr::from_ptr(cstr);
        let str_decoded = String::from_utf8_lossy(str.to_bytes()).to_string();
        Some(PathBuf::from(&str_decoded))
    }
}

#[no_mangle]
/// Change the current working directory.
/// The path string must be utf8 encoded.
pub extern "C" fn Plat_SetCurrentDirectory(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => env::set_current_dir(path).is_ok(),
    }
}

#[no_mangle]
/// Check if directory exists.
pub extern "C" fn Plat_DirectoryExists(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => path.is_dir(),
    }
}

#[no_mangle]
/// Check if file or directory exists.
pub extern "C" fn Plat_FileEntityExists(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => path.is_dir() || path.is_file(),
    }
}

#[no_mangle]
/// Create directory.
pub extern "C" fn Plat_CreateDirectory(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => fs::create_dir(path).is_ok(),
    }
}

#[no_mangle]
pub extern "C" fn Plat_DeleteFile(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => fs::remove_file(path).is_ok(),
    }
}

#[no_mangle]
/// Remove all files in a directory, but not the directory itself or any subdirectories.
pub extern "C" fn Plat_RemoveFilesInDirectory(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => platform::remove_files_in_directory(&path).is_ok(),
    }
}

#[no_mangle]
/// Remove the directory and all its content including subdirectories.
pub extern "C" fn Plat_RemoveDirectory(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => fs::remove_dir_all(path).is_ok(),
    }
}

#[no_mangle]
/// Remove read-only attribute from a file.
pub extern "C" fn Plat_RemoveReadOnlyAttribute(path_utf8: *const c_char) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => platform::remove_readonly_attribute(&path).is_ok(),
    }
}

#[repr(C)]
pub struct Str512 {
    pub buf: [c_char; 512],
}

fn strcpy(dest: &mut Str512, str: &str) -> bool {
    copy_string_to_c_buffer(str, dest.buf.as_mut_ptr(), 512)
}

#[no_mangle]
/// Return the current working directory.
pub extern "C" fn Plat_GetCurrentDirectory(dest: &mut Str512) -> bool {
    match env::current_dir() {
        Err(_) => false,
        Ok(path) => strcpy(dest, &path.to_string_lossy()),
    }
}

#[no_mangle]
/// Return the directory where executable file is located.
pub extern "C" fn Plat_GetExecutableDirectory(dest: &mut Str512) -> bool {
    match std::env::current_exe() {
        Err(_) => false,
        Ok(path) => match path.parent() {
            None => false,
            Some(parent) => strcpy(dest, &parent.to_string_lossy()),
        },
    }
}

#[no_mangle]
/// Given a path, fill dest with the file name.
pub extern "C" fn Plat_FileBaseName(path_utf8: *const c_char, dest: &mut Str512) -> bool {
    match cstr_utf8_to_pathbuf(path_utf8) {
        None => false,
        Some(path) => match path.file_name() {
            None => false,
            Some(path) => strcpy(dest, &path.to_string_lossy()),
        },
    }
}

/// Copy a rust string into a buffer from C.
/// The result string will be in utf-8 encoding.
/// If the buffer is too small, the buffer will be filled with zeroes, but the string will not be copied.
/// Returns true if the string was successfully copied.
fn copy_string_to_c_buffer(str: &str, buf: *mut c_char, buf_size: usize) -> bool {
    if buf_size == 0 {
        return false;
    }

    let str_bytes = str.as_bytes();
    let str_bytes_len = str_bytes.len();

    unsafe {
        // first fill the buffer with zeroes; this is a safey measure
        let buffer = std::slice::from_raw_parts_mut(buf as *mut u8, buf_size);
        buffer.fill(0);

        if str_bytes_len + 1 > buf_size {
            // not enough space
            return false;
        }

        buffer[..str_bytes_len].copy_from_slice(str_bytes);
        true
    }
}

#[no_mangle]
/// Copy string "Foo" into the buffer.
/// If not enough space, return false and fill the buffer with zeroes.
pub extern "C" fn GetStrTest_Foo(buf: *mut c_char, buf_size: usize) -> bool {
    copy_string_to_c_buffer("Foo", buf, buf_size)
}

#[no_mangle]
/// Copy string "Привет" into the buffer.
/// The string will be utf-8 encoded.
/// If not enough space, return false and fill the buffer with zeroes.
pub extern "C" fn GetStrTest_HelloRus(buf: *mut c_char, buf_size: usize) -> bool {
    copy_string_to_c_buffer("Привет", buf, buf_size)
}

#[no_mangle]
/// Copy string "Hello String512" to str.
pub extern "C" fn GetStrTest_Hello512(str: &mut Str512) -> bool {
    // str.copy("Hello String512")
    strcpy(str, "Hello String512")
}
