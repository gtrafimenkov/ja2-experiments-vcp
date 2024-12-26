// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::sam_sites;
use super::state;

pub const MAP_WORLD_X: i16 = 18;
pub const MAP_WORLD_Y: i16 = 18;

/// Sector ID 0-255 (16x16)
pub type SectorID8 = u8;

/// Sector ID 0-324 (18x18)
pub type SectorID16 = i16;

#[repr(C)]
pub struct SectorPoint {
    pub x: u8,
    pub y: u8,
}

#[no_mangle]
pub extern "C" fn GetSectorPoint(x: u8, y: u8) -> SectorPoint {
    SectorPoint { x, y }
}

/// Convert coordinates ([1-16], [1-16]) to 0-255 index.
/// This function should be prefered over GetSectorID8_STATIC macro.
#[no_mangle]
pub extern "C" fn GetSectorID8(x: u8, y: u8) -> SectorID8 {
    debug_assert!((1..=16).contains(&x));
    debug_assert!((1..=16).contains(&y));
    (y - 1) * 16 + x - 1
}

/// Get X [1-16] from SectorID8
#[no_mangle]
pub extern "C" fn SectorID8_X(sector_id: SectorID8) -> u8 {
    sector_id % 16 + 1
}

/// Get Y [1-16] from SectorID8
#[no_mangle]
pub extern "C" fn SectorID8_Y(sector_id: SectorID8) -> u8 {
    sector_id / 16 + 1
}

/// Convert coordinates (1-16, 1-16) to 0-324 index.
#[no_mangle]
pub extern "C" fn GetSectorID16(x: u8, y: u8) -> SectorID16 {
    x as i16 + y as i16 * MAP_WORLD_X
}

/// Get X [1-16] from SectorID16
#[no_mangle]
pub extern "C" fn SectorID16_X(sector_id: SectorID16) -> u8 {
    debug_assert!((0..18 * 18).contains(&sector_id));
    (sector_id % MAP_WORLD_X) as u8
}

/// Get Y [1-16] from SectorID16
#[no_mangle]
pub extern "C" fn SectorID16_Y(sector_id: SectorID16) -> u8 {
    debug_assert!((0..18 * 18).contains(&sector_id));
    (sector_id / MAP_WORLD_X) as u8
}

#[no_mangle]
pub extern "C" fn IsThisSectorASAMSector(x: u8, y: u8, z: i8) -> bool {
    sam_sites::get_sam_at_sector(x, y, z).is_some()
}

#[no_mangle]
pub extern "C" fn IsSectorEnemyControlled(x: u8, y: u8) -> bool {
    state::get().get_sector(x, y).enemy_controlled
}

#[no_mangle]
pub extern "C" fn SetSectorEnemyControlled(x: u8, y: u8, value: bool) {
    state::get().get_mut_sector(x, y).enemy_controlled = value;
}

#[no_mangle]
pub extern "C" fn IsSectorEnemyAirControlled(x: u8, y: u8) -> bool {
    state::get().get_sector(x, y).enemy_air_controlled
}

#[no_mangle]
pub extern "C" fn SetSectorEnemyAirControlled(x: u8, y: u8, value: bool) {
    state::get().get_mut_sector(x, y).enemy_air_controlled = value;
}

// #[no_mangle]
// pub extern "C" fn MilitiaTrainingAllowedInSector(x: u8, y: u8, z: i8) -> bool {
//     if z != 0 {
//         return false;
//     }

//     if sam_sites::get_sam_at_sector(x, y, z).is_some() {
//         // all SAM sites may have militia trained at them
//         return true;
//     }

//     //     bTownId = GetTownIdForSector(mapX, mapY);
//     //     return (MilitiaTrainingAllowedInTown(bTownId));
// }

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sector_id8_xy() {
        assert_eq!(0, GetSectorID8(1, 1));

        let sec0: SectorID8 = 0;
        assert_eq!(1, SectorID8_X(sec0));
        assert_eq!(1, SectorID8_Y(sec0));
    }

    #[test]
    fn sector_id16_xy() {
        assert_eq!(19, GetSectorID16(1, 1));

        let sec0: SectorID16 = 19;
        assert_eq!(1, SectorID16_X(sec0));
        assert_eq!(1, SectorID16_Y(sec0));
    }

    #[test]
    fn is_sam_sector() {
        assert_eq!(false, IsThisSectorASAMSector(1, 1, 0));

        // non-ground sector is always false
        assert_eq!(false, IsThisSectorASAMSector(2, 4, 1));
        assert_eq!(false, IsThisSectorASAMSector(2, 4, -1));

        assert_eq!(true, IsThisSectorASAMSector(2, 4, 0));
    }
}
