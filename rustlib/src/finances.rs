// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use std::fs::File;
use std::io::{self, Read};

#[allow(dead_code)]
pub static FINANCES_DATA_FILE: &str = "Temp/finances.dat";

pub fn read_balance_from_disk(path: &str) -> io::Result<i32> {
    let mut file = File::open(path)?;
    let mut buffer = [0u8; 4];
    file.read_exact(&mut buffer)?;
    let value = i32::from_le_bytes(buffer);
    Ok(value)
}

#[cfg(test)]
mod tests {

    use super::*;

    #[test]
    fn read_finances_20230619() {
        let value = read_balance_from_disk("test-data/finances-2023-06-19/finances.dat").unwrap();
        assert_eq!(2460, value);
    }
}
