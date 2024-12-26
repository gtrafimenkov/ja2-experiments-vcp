// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::exp_debug::{debug_log_write, get_current_time};
use crate::colors;

use ggstd::image::{self, color::Color, png, Image};
use ggstd::os;

fn get_exe_folder() -> std::io::Result<std::path::PathBuf> {
    let exe_path = std::env::current_exe()?;
    match exe_path.parent() {
        Some(path) => Ok(path.to_path_buf()),
        None => Err(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!(
                "parent folder of {} is not found",
                exe_path.to_string_lossy()
            ),
        )),
    }
}

/// Store screenshot as PNG.
/// data is a pointer to screen data - 640x480x2 bytes, 2 bytes per pixel.
/// The pixel format is RGB565.
#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "C" fn StoreScreenshot(data: *const u16) -> bool {
    let width = 640;
    let height = 480;
    let mut img = image::Img::new_rgba(&image::rect(0, 0, width, height));

    let mut i = 0_usize;
    for y in 0..480 {
        for x in 0..640 {
            #[allow(clippy::ptr_offset_with_cast)]
            let color = unsafe { *data.offset(i as isize) };
            let color = colors::RGB565(color).to_rgb();
            let color = Color::new_nrgba(color.r, color.g, color.b, 0xff);
            img.set(x, y, &color);
            i += 1;
        }
    }

    let ts = get_current_time(true);
    let filename = format!("screenshot-{}.png", ts);
    let mut filename_path = match get_exe_folder() {
        Ok(path) => path,
        Err(err) => {
            debug_log_write(&err.to_string());
            return false;
        }
    };
    filename_path.push(&filename);
    let mut f = match os::create(filename_path.to_str().unwrap()) {
        Ok(f) => f,
        Err(err) => {
            debug_log_write(&format!("failed to open {} for writing: {}", filename, err));
            return false;
        }
    };
    if let Err(err) = png::encode(&mut f, &img) {
        debug_log_write(&format!("failed to encode png image: {}", err));
        return false;
    }
    if let Err(err) = f.sync_all() {
        debug_log_write(&format!("failed to sync png write: {}", err));
        return false;
    }
    true
}
