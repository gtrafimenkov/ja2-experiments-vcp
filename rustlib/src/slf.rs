// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use std;
use std::io;

use crate::binreader::LittleEndian;

#[derive(Debug)]
pub struct Header {
    pub lib_name: String,
    pub lib_path: String,
    pub num_entries: u32,
    pub used_entries: u32,
}

#[repr(C)]
struct RawHeader {
    lib_name: [u8; 256],
    lib_path: [u8; 256],
    num_entries: u32,
    used_entries: u32,
    unused1: u16,
    unused2: u16,
    unused3: u32,
    unused4: u32,
}

const RAW_HEADER_SIZE: usize = std::mem::size_of::<RawHeader>();

fn decode_str(reader: &mut dyn io::Read, size: usize) -> io::Result<String> {
    let mut buffer = vec![0; size];
    reader.read_exact(&mut buffer)?;
    Ok(String::from_utf8_lossy(&buffer)
        .trim_end_matches('\0')
        .to_string())
}

pub fn read_header(reader: &mut dyn io::Read) -> io::Result<Header> {
    // read the whole header
    let mut buffer = [0; RAW_HEADER_SIZE];
    reader.read_exact(&mut buffer).unwrap();

    // parse individual fields
    let mut reader = io::Cursor::new(buffer);
    let reader = &mut reader;
    Ok(Header {
        lib_name: decode_str(reader, 256)?,
        lib_path: decode_str(reader, 256)?,
        num_entries: LittleEndian::read_u32(reader)?,
        used_entries: LittleEndian::read_u32(reader)?,
    })
}

#[derive(Debug)]
pub struct Entry {
    pub file_name: String,
    pub offset: u32,
    pub size: u32,
    pub state: u8,
}

#[repr(C)]
struct RawEntry {
    file_name: [u8; 256],
    offset: u32,
    size: u32,
    state: u8,
    reserved: u8,
    unused2: u32,
    unused3: u32,
    unused4: u32,
}

const RAW_ENTRY_SIZE: usize = std::mem::size_of::<RawEntry>();

pub fn read_entry(reader: &mut dyn io::Read) -> io::Result<Entry> {
    // read the whole header
    let mut buffer = [0; RAW_ENTRY_SIZE];
    reader.read_exact(&mut buffer).unwrap();

    // parse individual fields
    let mut reader = io::Cursor::new(buffer);
    let reader = &mut reader;
    Ok(Entry {
        file_name: decode_str(reader, 256)?,
        offset: LittleEndian::read_u32(reader)?,
        size: LittleEndian::read_u32(reader)?,
        state: LittleEndian::read_u8(reader)?,
    })
}

#[derive(Debug)]
pub struct Slf {
    pub header: Header,
    pub entries: Vec<Entry>,
}

pub fn read_slf<T: io::Read + io::Seek>(reader: &mut T) -> io::Result<Slf> {
    let header = read_header(reader)?;
    let mut entries = Vec::with_capacity(header.num_entries as usize);
    if header.num_entries > 0 {
        let entries_start: i64 = header.num_entries as i64 * RAW_ENTRY_SIZE as i64;
        reader.seek(io::SeekFrom::End(-entries_start))?;
        for _i in 0..header.num_entries {
            let entry = read_entry(reader)?;
            if entry.state == 0 {
                entries.push(entry);
            }
        }
    }
    Ok(Slf { header, entries })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn datastructures_sizes() {
        assert_eq!(280, RAW_ENTRY_SIZE);
    }
}
