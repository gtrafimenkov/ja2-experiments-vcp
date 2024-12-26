// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

/// RGB is color with u8 components for red, green and blue.
#[repr(C)]
#[derive(Debug, PartialEq)]
pub struct RGB {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    _unused: u8,
}

impl RGB {
    pub const fn new(r: u8, g: u8, b: u8) -> Self {
        Self {
            r,
            g,
            b,
            _unused: 0,
        }
    }
    pub const fn to_rgb565(self) -> RGB565 {
        RGB565::new(self.r, self.g, self.b)
    }
}

/// RGB32 is RGB color packed into 32 bits in the format 0x00BBGGRR
pub struct RGB32(pub u32);

impl RGB32 {
    pub const fn new(r: u8, g: u8, b: u8) -> Self {
        Self((r as u32) | ((g as u32) << 8) | ((b as u32) << 16))
    }
    pub const fn to_rgb(self) -> RGB {
        let r = self.0 as u8;
        let g = (self.0 >> 8) as u8;
        let b = (self.0 >> 16) as u8;
        RGB::new(r, g, b)
    }
}

/// RGB565 is RGB color packed into 2 bytes.
/// 5 bits for R, 6 for G, 5 for blue.
pub struct RGB565(pub u16);

impl RGB565 {
    pub const fn new(r: u8, g: u8, b: u8) -> Self {
        Self(
            (((r as u16) << 8) & 0b_11111000_00000000)
                | (((g as u16) << 3) & 0b_00000111_11100000)
                | ((b as u16) >> 3),
        )
    }
    pub const fn to_rgb(self) -> RGB {
        let r = ((self.0 & 0b_11111000_00000000) >> 8) as u8;
        let g = ((self.0 & 0b_00000111_11100000) >> 3) as u8;
        let b = ((self.0 & 0b_00000000_00011111) << 3) as u8;
        RGB::new(r, g, b)
    }
    pub const fn to_rgb32(self) -> RGB32 {
        let r = ((self.0 & 0b_11111000_00000000) >> 8) as u8;
        let g = ((self.0 & 0b_00000111_11100000) >> 3) as u8;
        let b = ((self.0 & 0b_00000000_00011111) << 3) as u8;
        RGB32::new(r, g, b)
    }
}

/// Convert RGB565 value to RGB32.
pub fn rgb565_to_rgb32(rgb565: u16) -> u32 {
    RGB565(rgb565).to_rgb32().0
}

/// Convert RGB32 value to RGB565.
pub fn rgb32_to_rgb565(rgb32: u32) -> u16 {
    RGB32(rgb32).to_rgb().to_rgb565().0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_rgb32_new() {
        assert_eq!(0x00000000, RGB32::new(0, 0, 0).0);
        assert_eq!(0x00000001, RGB32::new(1, 0, 0).0);
        assert_eq!(0x00030201, RGB32::new(1, 2, 3).0);
    }

    #[test]
    fn test_rgb565_to_rgb() {
        assert_eq!(RGB565(0x0000).to_rgb(), RGB::new(0, 0, 0));
        assert_eq!(RGB565(0x0001).to_rgb(), RGB::new(0, 0, 8));
        assert_eq!(
            RGB565(0xffff).to_rgb(),
            RGB::new(0b11111000, 0b11111100, 0b11111000)
        );
    }

    #[test]
    fn test_rgb_to_rgb565() {
        assert_eq!(RGB::new(0, 0, 0).to_rgb565().0, 0x0000);
        assert_eq!(RGB::new(0, 0, 8).to_rgb565().0, 0x0001);
        assert_eq!(RGB::new(248, 252, 248).to_rgb565().0, 0xffff);
        assert_eq!(RGB::new(255, 255, 255).to_rgb565().0, 0xffff);
    }

    #[test]
    fn test_rgb32_to_rgb() {
        assert_eq!(RGB32(0x00000000).to_rgb(), RGB::new(0, 0, 0));
        assert_eq!(RGB32(0xff000000).to_rgb(), RGB::new(0, 0, 0));
        assert_eq!(RGB32(0x00000001).to_rgb(), RGB::new(1, 0, 0));
        assert_eq!(RGB32(0x00030201).to_rgb(), RGB::new(1, 2, 3));
        assert_eq!(RGB32(0x55ffffff).to_rgb(), RGB::new(0xff, 0xff, 0xff));
    }

    #[test]
    fn test_rgb565_to_rgb32() {
        assert_eq!(0x00000000, rgb565_to_rgb32(0x0000));
        assert_eq!(0x00080000, rgb565_to_rgb32(0x0001));
    }

    #[test]
    fn test_rgb32_to_rgb565() {
        assert_eq!(rgb32_to_rgb565(0x00000000), 0x0000);
        assert_eq!(rgb32_to_rgb565(0x00080000), 0x0001);
    }
}
