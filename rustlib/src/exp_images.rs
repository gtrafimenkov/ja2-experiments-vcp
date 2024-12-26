// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::exp_alloc;
use super::exp_debug::debug_log_write;
use super::exp_fileman;
use super::exp_fileman::FileID;
use super::images;
use std::ffi::{c_char, CStr};
use std::io;

#[repr(C)]
pub struct Image {
    width: u16,
    height: u16,
    bit_depth: u8,
    palette: *mut SGPPaletteEntry,
    palette16bpp: *mut u16,
    app_data: *mut u8,
    app_data_size: u32,
    image_data: *mut u8,
    image_data_size: u32,
    subimages: *mut Subimage,
    number_of_subimages: u16,
}

#[no_mangle]
/// Load STI image.
/// If the function was successful, don't forget to free memory allocated for palette, subimages, app_data, image_data.
pub extern "C" fn LoadSTCIFileToImage(path: *const c_char, load_app_data: bool) -> *mut Image {
    match cstr_utf8_to_string(path) {
        None => std::ptr::null_mut(),
        Some(path) => match exp_fileman::get_db().open_for_reading(&path) {
            Err(err) => {
                debug_log_write(&format!("failed to open file {path} for reading: {err}"));
                std::ptr::null_mut()
            }
            Ok(file_id) => {
                let img = read_stci(file_id, load_app_data);

                if let Err(err) = img {
                    debug_log_write(&format!("failed to read stci image: {err}"));
                    return std::ptr::null_mut();
                }

                let img = img.unwrap();

                // allocate memory and copy palette array
                let mut palette_data_copy: *mut SGPPaletteEntry = std::ptr::null_mut();
                if let Some(palette) = img.palette {
                    let palette_data_size = palette.len() * std::mem::size_of::<SGPPaletteEntry>();
                    unsafe {
                        palette_data_copy =
                            std::mem::transmute(exp_alloc::RustAlloc(palette_data_size));
                        let palette_copy =
                            std::slice::from_raw_parts_mut(palette_data_copy, palette.len());
                        for i in 0..256 {
                            palette_copy[i] = SGPPaletteEntry::from_internal(&palette[i]);
                        }
                    }
                }

                // allocate memory and copy image data
                let image_data_size = img.image_data.len();
                let image_data_copy = exp_alloc::RustAlloc(image_data_size);
                unsafe {
                    std::ptr::copy(img.image_data.as_ptr(), image_data_copy, image_data_size);
                }

                // allocate memory for subimages array and copy data
                let mut number_of_subimages = 0;
                let mut subimages_ptr: *mut Subimage = std::ptr::null_mut();
                if let Some(subimages) = img.subimages {
                    number_of_subimages = subimages.len();
                    let data_size = number_of_subimages * std::mem::size_of::<Subimage>();
                    unsafe {
                        subimages_ptr = std::mem::transmute(exp_alloc::RustAlloc(data_size));
                        let subimages_copy =
                            std::slice::from_raw_parts_mut(subimages_ptr, number_of_subimages);
                        for i in 0..number_of_subimages {
                            subimages_copy[i] = Subimage::from_internal(&subimages[i]);
                        }
                    }
                }

                // allocate memory and copy application data
                let mut app_data_size = 0;
                let mut app_data_copy: *mut u8 = std::ptr::null_mut();
                if let Some(app_data) = img.app_data {
                    app_data_size = app_data.len();
                    unsafe {
                        app_data_copy = exp_alloc::RustAlloc(app_data_size);
                        std::ptr::copy(app_data.as_ptr(), app_data_copy, app_data.len());
                    }
                }

                exp_fileman::File_Close(file_id);

                unsafe {
                    let res: *mut Image =
                        std::mem::transmute(exp_alloc::RustAlloc(std::mem::size_of::<Image>()));
                    *res = Image {
                        height: img.height,
                        width: img.width,
                        bit_depth: img.pixel_depth,
                        image_data_size: image_data_size as u32,
                        image_data: image_data_copy,
                        palette: palette_data_copy,
                        palette16bpp: std::ptr::null_mut(),
                        number_of_subimages: number_of_subimages as u16,
                        subimages: subimages_ptr,
                        app_data_size: app_data_size as u32,
                        app_data: app_data_copy,
                    };
                    res
                }
            }
        },
    }
}

#[repr(C)]
#[derive(Default, Copy, Clone)]
/// Palette structure, mimics that of Win32
pub struct SGPPaletteEntry {
    red: u8,
    green: u8,
    blue: u8,
    _unused: u8,
}

impl SGPPaletteEntry {
    pub const fn from_internal(entry: &images::SGPPaletteEntry) -> Self {
        Self {
            red: entry.red,
            green: entry.green,
            blue: entry.blue,
            _unused: 0,
        }
    }
}

#[repr(C)]
#[derive(Debug)]
/// Structure that describes one image from an indexed STCI file
pub struct Subimage {
    data_offset: u32,
    data_length: u32,
    x_offset: i16,
    y_offset: i16,
    height: u16,
    width: u16,
}

impl Subimage {
    pub const fn from_internal(val: &images::Subimage) -> Self {
        Self {
            data_offset: val.data_offset,
            data_length: val.data_length,
            x_offset: val.x_offset,
            y_offset: val.y_offset,
            height: val.height,
            width: val.width,
        }
    }
}

/*
Sir-Tech's Crazy Image (STCI) file format specifications.  Each file is composed of:
- ImageFileHeader, uncompressed
- Palette (STCI_INDEXED, size = uiNumberOfColours * PALETTE_ELEMENT_SIZE), uncompressed
- SubRectInfo's (usNumberOfRects > 0, size = usNumberOfSubRects * sizeof(SubRectInfo)), uncompressed
- Bytes of image data, possibly compressed
*/

/// Converts utf-8 encoded null-terminated C string into String.
/// If cstr is null, return None.
fn cstr_utf8_to_string(cstr: *const c_char) -> Option<String> {
    if cstr.is_null() {
        return None;
    }
    let str = unsafe { CStr::from_ptr(cstr) };
    Some(String::from_utf8_lossy(str.to_bytes()).to_string())
}

/// Read STCI image from an opened file.
fn read_stci(file_id: FileID, load_app_data: bool) -> io::Result<images::STImage> {
    let mut reader = exp_fileman::Reader::new(file_id);
    images::read_stci(&mut reader, load_app_data)
}

#[cfg(test)]
mod tests {
    use super::super::exp_fileman;

    use super::*;

    #[test]
    fn load_stci() {
        exp_fileman::get_db()
            .load_slf_from_dir("../tools/editor")
            .unwrap();
        let file_id = exp_fileman::get_db()
            .open_for_reading("Editor\\CANCEL.STI")
            .unwrap();
        let img = read_stci(file_id, true).unwrap();
        assert_eq!(640, img.width);
        assert_eq!(480, img.height);
        assert_eq!(true, img.indexed);
        assert_eq!(false, img.zlib_compressed);
        assert_eq!(8, img.pixel_depth);
        assert_eq!(4800, img.image_data.len());
        assert_eq!(true, img.app_data.is_none());
        let subimage0 = &img.subimages.as_ref().unwrap()[0];
        let subimage4 = &img.subimages.as_ref().unwrap()[4];
        assert_eq!(5, img.subimages.as_ref().unwrap().len());
        assert_eq!(0, subimage0.x_offset);
        assert_eq!(0, subimage0.y_offset);
        assert_eq!(30, subimage0.height);
        assert_eq!(30, subimage0.width);
        assert_eq!(0, subimage0.data_offset);
        assert_eq!(960, subimage0.data_length);
        assert_eq!(0, subimage4.x_offset);
        assert_eq!(0, subimage4.y_offset);
        assert_eq!(30, subimage4.height);
        assert_eq!(30, subimage4.width);
        assert_eq!(3840, subimage4.data_offset);
        assert_eq!(960, subimage4.data_length);
    }
}
