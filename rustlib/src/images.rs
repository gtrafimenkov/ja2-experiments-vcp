// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use std::io;

use crate::binreader::LittleEndian;

#[repr(C)]
#[derive(Default, Copy, Clone)]
/// Palette structure, mimics that of Win32
pub struct SGPPaletteEntry {
    pub red: u8,
    pub green: u8,
    pub blue: u8,
    pub _unused: u8,
}

#[repr(C)]
#[derive(Debug)]
/// Structure that describes one image from an indexed STCI file
pub struct Subimage {
    pub data_offset: u32,
    pub data_length: u32,
    pub x_offset: i16,
    pub y_offset: i16,
    pub height: u16,
    pub width: u16,
}

/// Sir-Tech's Crazy Image
pub struct STImage {
    pub height: u16,
    pub width: u16,
    pub pixel_depth: u8,
    pub image_data: Vec<u8>,
    pub indexed: bool,                           // indexed (true) or rgb (false)
    pub palette: Option<[SGPPaletteEntry; 256]>, // only for indexed images
    pub subimages: Option<Vec<Subimage>>,
    pub app_data: Option<Vec<u8>>,
    pub zlib_compressed: bool,
}

pub const STCI_ID_LEN: usize = 4;
const STCI_ID_STRING: [u8; STCI_ID_LEN] = [b'S', b'T', b'C', b'I'];

const STCI_RGB: u32 = 0x0004;
const STCI_INDEXED: u32 = 0x0008;
pub const STCI_ZLIB_COMPRESSED: u32 = 0x0010;
pub const STCI_ETRLE_COMPRESSED: u32 = 0x0020;

/// Read STCI image from an opened file.
pub fn read_stci(reader: &mut dyn io::Read, load_app_data: bool) -> io::Result<STImage> {
    let mut id: [u8; STCI_ID_LEN] = [0; STCI_ID_LEN];
    reader.read_exact(&mut id)?;
    if id != STCI_ID_STRING {
        return Err(io::Error::new(
            io::ErrorKind::Other,
            format!("not STCI file ({id:?})"),
        ));
    }
    let _original_size = LittleEndian::read_u32(reader)?;
    let stored_size = LittleEndian::read_u32(reader)? as usize;
    let _transparent_value = LittleEndian::read_u32(reader)?;
    let flags = LittleEndian::read_u32(reader)?;
    let height = LittleEndian::read_u16(reader)?;
    let width = LittleEndian::read_u16(reader)?;
    if flags & STCI_INDEXED == 0 && flags & STCI_RGB == 0 {
        return Err(io::Error::new(io::ErrorKind::Other, "unknown image format"));
    }
    let indexed = flags & STCI_INDEXED != 0;
    let mut num_subimages = 0;
    if indexed {
        // index
        let number_of_colours = LittleEndian::read_u32(reader)?;
        if number_of_colours != 256 {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                "indexed STCI image must have 256 colors",
            ));
        }
        num_subimages = LittleEndian::read_u16(reader)? as usize;
        let _red_depth = LittleEndian::read_u8(reader)?;
        let _green_depth = LittleEndian::read_u8(reader)?;
        let _blue_depth = LittleEndian::read_u8(reader)?;
        let mut unused: [u8; 11] = [0; 11];
        reader.read_exact(&mut unused)?;
    } else {
        // RGB
        let _red_mask = LittleEndian::read_u32(reader)?;
        let _green_mask = LittleEndian::read_u32(reader)?;
        let _blue_mask = LittleEndian::read_u32(reader)?;
        let _alpha_mask = LittleEndian::read_u32(reader)?;
        let _red_depth = LittleEndian::read_u8(reader)?;
        let _green_depth = LittleEndian::read_u8(reader)?;
        let _blue_depth = LittleEndian::read_u8(reader)?;
        let _alpha_depth = LittleEndian::read_u8(reader)?;
    }

    let pixel_depth = LittleEndian::read_u8(reader)?;
    let _unused1 = LittleEndian::read_u8(reader)?;
    let _unused2 = LittleEndian::read_u8(reader)?;
    let _unused3 = LittleEndian::read_u8(reader)?;
    let app_data_size = LittleEndian::read_u32(reader)? as usize;
    let mut unused: [u8; 12] = [0; 12];
    reader.read_exact(&mut unused)?;

    let zlib_compressed = flags & STCI_ZLIB_COMPRESSED != 0;
    let mut image_data = vec![0; stored_size];
    let mut palette: Option<[SGPPaletteEntry; 256]> = None;
    let mut subimages: Option<Vec<Subimage>> = None;
    let mut app_data: Option<Vec<u8>> = None;

    if indexed {
        // palette
        palette = Some([Default::default(); 256]);
        {
            // palette is 256 rgb u8 values
            let mut data: [u8; 256 * 3] = [0; 256 * 3];
            reader.read_exact(&mut data)?;
            for (i, item) in palette.as_mut().unwrap().iter_mut().enumerate() {
                let start = i * 3;
                item.red = data[start];
                item.green = data[start + 1];
                item.blue = data[start + 2];
                item._unused = 0;
            }
        }

        // subimages
        if num_subimages > 0 {
            let mut collector = Vec::with_capacity(num_subimages);
            let size = 16 * num_subimages;
            let mut buffer = vec![0; size];
            reader.read_exact(&mut buffer)?;
            let mut reader = io::Cursor::new(buffer);
            let reader = &mut reader;
            for _i in 0..num_subimages {
                let subimage = Subimage {
                    data_offset: LittleEndian::read_u32(reader)?,
                    data_length: LittleEndian::read_u32(reader)?,
                    x_offset: LittleEndian::read_i16(reader)?,
                    y_offset: LittleEndian::read_i16(reader)?,
                    height: LittleEndian::read_u16(reader)?,
                    width: LittleEndian::read_u16(reader)?,
                };
                collector.push(subimage);
            }
            subimages = Some(collector);
        }

        reader.read_exact(&mut image_data)?;

        if app_data_size > 0 {
            app_data = Some(vec![0; app_data_size]);
            if app_data_size > 0 && load_app_data {
                reader.read_exact(app_data.as_mut().unwrap())?;
            }
        }
    } else {
        reader.read_exact(&mut image_data)?;
    }
    Ok(STImage {
        height,
        width,
        pixel_depth,
        image_data,
        indexed,
        palette,
        subimages,
        app_data,
        zlib_compressed,
    })
}
