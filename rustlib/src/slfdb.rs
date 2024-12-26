// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

//! Database of files from slf archives and functions to read these files.

use crate::slf;
use std::collections::HashMap;
use std::fs::{self, File};
use std::io::{self, Read, Seek};

#[cfg(windows)]
use std::os::windows::prelude::AsRawHandle;

/// Name of an slf library.  The name is taken from slf file header.
type LibName = String;

/// Information about a file from an slf library.
#[derive(Debug, Clone)]
struct LibFileInfo {
    lib: LibName,
    /// offset from the beginning of the slf file
    offset: u32,
    /// size of the file
    size: u32,
}

impl LibFileInfo {
    fn copy(&self) -> Self {
        LibFileInfo {
            lib: self.lib.clone(),
            offset: self.offset,
            size: self.size,
        }
    }
}

/// Opened library file
#[derive(Debug)]
pub struct OpenedLibFile {
    // path: String,
    info: LibFileInfo,
    /// read position for the file relative to offset
    read_pos: u32,
}

impl OpenedLibFile {
    pub fn get_size(&self) -> u32 {
        self.info.size
    }
    pub fn get_lib_name(&self) -> &str {
        &self.info.lib
    }
}

#[derive(Debug)]
#[allow(dead_code)]
struct Lib {
    slf_info: slf::Slf,
    slf_file_path: std::path::PathBuf,
    opened_file: fs::File,
}

// Contains information about files from slf archives.
#[derive(Debug)]
pub struct DB {
    /// Loaded slf libraries.  This is a map of library names to library structures.
    libs: HashMap<LibName, Lib>,

    /// Mapping of file names (paths) contained in libraries to library names.
    lib_files: HashMap<String, LibFileInfo>,
}

impl DB {
    pub fn new() -> Self {
        DB {
            libs: HashMap::new(),
            lib_files: HashMap::new(),
        }
    }

    /// Get Windows handle that can be used to read the library file.
    /// On Linux return 0.
    #[cfg(windows)]
    pub fn get_win_handle_to_read_libfile(&mut self, f: &OpenedLibFile) -> u64 {
        match self.libs.get_mut(f.get_lib_name()) {
            Some(lib) => {
                let handle = lib.opened_file.as_raw_handle();
                // Also set file position at the beginning of the library file.
                // So that reading using this handle will read the library file.
                if lib
                    .opened_file
                    .seek(io::SeekFrom::Start(f.info.offset as u64))
                    .is_ok()
                {
                    return handle as u64;
                }
                0
            }
            None => 0,
        }
    }

    #[cfg(not(windows))]
    pub fn get_win_handle_to_read_libfile(&mut self, _f: &OpenedLibFile) -> u64 {
        0
    }

    /// Load slf files from directory `dir`.
    /// After the load content of slf files will be availabe for access.
    pub fn load_slf_from_dir(&mut self, dir: &str) -> io::Result<()> {
        let iter = std::fs::read_dir(dir)?;
        for entry in iter {
            let entry = entry?;
            if entry.path().is_file() {
                let name = entry.file_name().to_string_lossy().to_lowercase();
                if name.ends_with(".slf") {
                    let mut f = File::open(entry.path())?;
                    let slf = slf::read_slf(&mut f)?;

                    for entry in slf.entries.iter() {
                        let libfile_path =
                            build_canonical_libfile_path(&slf.header.lib_path, &entry.file_name);
                        let location = LibFileInfo {
                            lib: slf.header.lib_name.clone(),
                            offset: entry.offset,
                            size: entry.size,
                        };
                        self.lib_files.insert(libfile_path, location);
                    }

                    self.libs.insert(
                        slf.header.lib_name.clone(),
                        Lib {
                            slf_info: slf,
                            slf_file_path: entry.path(),
                            opened_file: f,
                        },
                    );
                }
            }
        }
        Ok(())
    }

    /// Get number of libraries in the DB.
    pub fn count(&self) -> usize {
        self.libs.len()
    }

    /// Check if the database has library (slf file) with name `name` loaded.
    pub fn has_library(&self, name: &str) -> bool {
        self.libs.get(name).is_some()
    }

    /// Check if the database has the given file in one of the loaded libraries.
    pub fn has_libfile(&self, path: &str) -> bool {
        self.lib_files.get(&canonize_libfile_path(path)).is_some()
    }

    /// Return the size of a file from a library.
    /// If file doens't exist, io::ErrorKind::NotFound will be returned.
    pub fn get_libfile_size(&self, path: &str) -> io::Result<u32> {
        match self.lib_files.get(&canonize_libfile_path(path)) {
            None => Err(io::Error::from(io::ErrorKind::NotFound)),
            Some(location) => Ok(location.size),
        }
    }

    /// Open library file for reading.
    pub fn open_for_reading(&self, path: &str) -> io::Result<OpenedLibFile> {
        let path = canonize_libfile_path(path);
        match self.lib_files.get(&path) {
            None => Err(io::Error::from(io::ErrorKind::NotFound)),
            Some(info) => Ok(OpenedLibFile {
                // path,
                info: info.copy(),
                read_pos: 0,
            }),
        }
    }

    /// Read a library file into buffer.
    /// The buffer must not be bigger that the size of the file.
    pub fn read_libfile_exact(
        &mut self,
        file: &mut OpenedLibFile,
        buffer: &mut [u8],
    ) -> io::Result<()> {
        if buffer.len() + file.read_pos as usize > file.info.size as usize {
            return Err(io::Error::from(io::ErrorKind::InvalidInput));
        }
        let lib = self.libs.get_mut(&file.info.lib).unwrap();
        let read_pos = file.info.offset + file.read_pos;
        lib.opened_file.seek(io::SeekFrom::Start(read_pos as u64))?;
        lib.opened_file.read_exact(buffer)?;
        file.read_pos += buffer.len() as u32;
        Ok(())
    }

    /// Read up to buf.len() bytes from a file.
    /// Return number of bytes read (n).  If n == 0, the file end was reached.
    pub fn read_libfile(&mut self, file: &mut OpenedLibFile, buf: &mut [u8]) -> io::Result<usize> {
        let max_bytes_to_read = std::cmp::min((file.info.size - file.read_pos) as usize, buf.len());
        let buffer = &mut buf[0..max_bytes_to_read];
        let lib = self.libs.get_mut(&file.info.lib).unwrap();
        let read_pos = file.info.offset + file.read_pos;
        lib.opened_file.seek(io::SeekFrom::Start(read_pos as u64))?;
        let n = lib.opened_file.read(buffer)?;
        file.read_pos += n as u32;
        Ok(n)
    }

    /// Change read position for the library file.
    pub fn seek_libfile(&mut self, file: &mut OpenedLibFile, pos: io::SeekFrom) -> io::Result<u64> {
        // checking that we are not going out of the available range
        let cur_abs_pos = (file.info.offset + file.read_pos) as i64;
        let end_abs_pos = (file.info.offset + file.info.size) as i64;
        let new_abs_pos = match pos {
            io::SeekFrom::Start(val) => file.info.offset as i64 + val as i64,
            io::SeekFrom::Current(val) => cur_abs_pos + val,
            io::SeekFrom::End(val) => end_abs_pos + val,
        };
        if new_abs_pos < file.info.offset as i64 || new_abs_pos >= end_abs_pos {
            return Err(io::Error::from(io::ErrorKind::InvalidInput));
        }

        // actual seek will happen just before the read
        // for now just storing the new position
        file.read_pos = (new_abs_pos - file.info.offset as i64) as u32;
        Ok(file.read_pos as u64)
    }
}

impl Default for DB {
    fn default() -> Self {
        Self::new()
    }
}

/// Build canonical path for a library file given the library path (from slf header)
/// and the file name from slf entry.
fn build_canonical_libfile_path(lib_path: &str, file_name: &str) -> String {
    if lib_path.is_empty() {
        canonize_libfile_path(file_name)
    } else {
        canonize_libfile_path(&format!(
            "{}/{}",
            lib_path.trim_end_matches('\\').trim_end_matches('/'),
            file_name
        ))
    }
}

// Modify library file path to the form which is used internally.
fn canonize_libfile_path(path: &str) -> String {
    path.to_lowercase().replace('\\', "/")
}

#[cfg(test)]
mod tests {
    use super::*;
    use ggstd::crypto::sha256;
    use ggstd::encoding::hex;

    #[test]
    fn loading_slfs() {
        let mut fdb = DB::new();
        fdb.load_slf_from_dir("../tools/editor").unwrap();
        assert_eq!(true, fdb.has_library("EDITOR.SLF"));

        assert_eq!(true, fdb.has_libfile("editor/exitgridbut.sti"));
        assert_eq!(true, fdb.has_libfile("editor\\exitgridbut.sti"));
        assert_eq!(true, fdb.has_libfile("Editor\\EXITGRIDBUT.STI"));

        assert_eq!(
            5712,
            fdb.get_libfile_size("Editor\\EXITGRIDBUT.STI").unwrap()
        );
    }

    #[test]
    fn reading_file_from_slf() {
        let mut fdb = DB::new();
        fdb.load_slf_from_dir("../tools/editor").unwrap();

        let test_path = "Editor\\EXITGRIDBUT.STI";
        let test_hash = "b7099f0fefcc7cff52081c12d304b84d712fb36c4212407afe921cb4208b7adc";

        assert_eq!(5712, fdb.get_libfile_size(test_path).unwrap());

        let f = &mut fdb.open_for_reading(test_path).unwrap();

        // reading the file and testing the content
        {
            let mut buffer = [0; 5712];
            fdb.read_libfile_exact(f, &mut buffer).unwrap();

            let hash = sha256::sum256(&buffer);
            let hash_str = hex::encode_to_string(&hash);
            assert_eq!(test_hash, hash_str);
        }

        // reading again
        {
            fdb.seek_libfile(f, io::SeekFrom::Start(0)).unwrap();

            let mut buffer = [0; 5712];
            fdb.read_libfile_exact(f, &mut buffer).unwrap();

            let hash = sha256::sum256(&buffer);
            let hash_str = hex::encode_to_string(&hash);
            assert_eq!(test_hash, hash_str);
        }

        // reading again with read_libfile function
        {
            fdb.seek_libfile(f, io::SeekFrom::Start(0)).unwrap();

            let mut buffer = [0; 6000];
            let bytes_read = fdb.read_libfile(f, &mut buffer).unwrap();
            assert_eq!(5712, bytes_read);

            let hash = sha256::sum256(&buffer[0..bytes_read]);
            let hash_str = hex::encode_to_string(&hash);
            assert_eq!(test_hash, hash_str);
        }

        // reading again with many read_libfile reads
        {
            fdb.seek_libfile(f, io::SeekFrom::Start(0)).unwrap();

            let mut buffer = [0; 6000];
            assert_eq!(2000, fdb.read_libfile(f, &mut buffer[0..2000]).unwrap());
            assert_eq!(2000, fdb.read_libfile(f, &mut buffer[2000..4000]).unwrap());
            assert_eq!(1712, fdb.read_libfile(f, &mut buffer[4000..5712]).unwrap());

            let hash = sha256::sum256(&buffer[0..5712]);
            let hash_str = hex::encode_to_string(&hash);
            assert_eq!(test_hash, hash_str);
        }

        // reading again with many read_libfile reads and seek operations
        {
            let mut buffer = [0; 6000];

            fdb.seek_libfile(f, io::SeekFrom::Start(0)).unwrap();
            assert_eq!(2000, fdb.read_libfile(f, &mut buffer[0..2000]).unwrap());

            fdb.seek_libfile(f, io::SeekFrom::End(-1712)).unwrap();
            assert_eq!(1712, fdb.read_libfile(f, &mut buffer[4000..5712]).unwrap());

            fdb.seek_libfile(f, io::SeekFrom::Start(0)).unwrap();
            fdb.seek_libfile(f, io::SeekFrom::Current(2000)).unwrap();
            assert_eq!(2000, fdb.read_libfile(f, &mut buffer[2000..4000]).unwrap());

            let hash = sha256::sum256(&buffer[0..5712]);
            let hash_str = hex::encode_to_string(&hash);
            assert_eq!(test_hash, hash_str);

            assert_eq!('S', buffer[0] as char);
            assert_eq!('T', buffer[1] as char);
            assert_eq!('C', buffer[2] as char);
        }
    }

    #[test]
    fn canonical_libfile_path() {
        assert_eq!(
            "editor/exitgridbut.sti",
            build_canonical_libfile_path("Editor\\", "EXITGRIDBUT.STI")
        );
        assert_eq!(
            "genbutn.sti",
            build_canonical_libfile_path("", "GENBUTN.STI")
        );
    }
}
