// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#[derive(Copy, Clone, PartialEq, Debug)]
pub enum SamSite {
    Chitzena = 0,
    Drassen = 1,
    Cambria = 2,
    Meduna = 3,
}

impl SamSite {
    fn from_int(value: u8) -> Self {
        match value {
            0 => SamSite::Chitzena,
            1 => SamSite::Drassen,
            2 => SamSite::Cambria,
            3 => SamSite::Meduna,
            _ => panic!("invalid value {value} for SamSite enum"),
        }
    }
}

#[derive(Clone, Copy)]
pub struct SamSiteLocation {
    pub x: u8,
    pub y: u8,
}

pub const LOCATIONS: [SamSiteLocation; 4] = [
    SamSiteLocation { x: 2, y: 4 },
    SamSiteLocation { x: 15, y: 4 },
    SamSiteLocation { x: 8, y: 9 },
    SamSiteLocation { x: 4, y: 14 },
];

pub fn get_sam_location(site: SamSite) -> SamSiteLocation {
    LOCATIONS[site as usize]
}

pub struct SamState {
    found: bool,
    condition: u8, // 0 - 100
}

impl SamState {
    pub const fn new() -> Self {
        Self {
            found: false,
            condition: 100,
        }
    }
    pub fn get_condition(&self) -> u8 {
        self.condition
    }
    pub fn set_condition(&mut self, condition: u8) {
        debug_assert!(condition <= 100);
        self.condition = condition;
    }
}

pub struct State {
    pub sites: [SamState; 4],
    // found: [bool; 4],
    // condition: [u8; 4], // 0 - 100
}

impl State {
    pub const fn new() -> Self {
        State {
            sites: [
                SamState::new(),
                SamState::new(),
                SamState::new(),
                SamState::new(),
            ],
        }
    }
    pub fn is_found(&self, site: SamSite) -> bool {
        self.sites[site as usize].found
    }
    pub fn set_found(&mut self, site: SamSite, value: bool) {
        self.sites[site as usize].found = value;
    }
    pub fn get_condition(&self, site: SamSite) -> u8 {
        self.sites[site as usize].get_condition()
    }
    pub fn set_condition(&mut self, site: SamSite, condition: u8) {
        debug_assert!(condition <= 100);
        self.sites[site as usize].set_condition(condition);
    }
}

const SAM_CONTROLLED_SECTORS: [[u8; 18]; 18] = [
    //  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 2, 0], // A
    [0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0], // B
    [0, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 0], // C
    [0, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2, 2, 2, 2, 2, 2, 0], // D
    [0, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 0], // E
    [0, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 0], // F
    [0, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0], // G
    [0, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0], // H
    [0, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0], // I
    [0, 1, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0], // J
    [0, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0], // K
    [0, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0], // L
    [0, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0], // M
    [0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 0], // N
    [0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 0], // O
    [0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 0], // P
    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
];

pub fn get_sam_controlling_sector(x: u8, y: u8) -> Option<SamSite> {
    let index = SAM_CONTROLLED_SECTORS[y as usize][x as usize];
    match index {
        0 => None,
        _ => Some(SamSite::from_int(index - 1)),
    }
}

pub fn get_sam_at_sector(x: u8, y: u8, z: i8) -> Option<SamSite> {
    if z == 0 {
        for (i, loc) in LOCATIONS.iter().enumerate() {
            if loc.x == x && loc.y == y {
                return Some(SamSite::from_int(i as u8));
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_state() {
        let state = State::new();
        assert_eq!(4, state.sites.len());
        assert_eq!(false, state.sites[3].found);
        assert_eq!(100, state.sites[3].condition);
    }

    #[test]
    fn test_get_sam_controlling_sector() {
        assert_eq!(SamSite::Drassen, get_sam_controlling_sector(15, 4).unwrap());
        assert!(get_sam_controlling_sector(0, 0).is_none()); // invaid coordinates
        assert!(get_sam_controlling_sector(9, 1).is_none()); // Omerta
    }

    #[test]
    fn get_sam_id_for_sector() {
        assert!(get_sam_at_sector(1, 1, 0).is_none());

        assert!(get_sam_at_sector(2, 4, 1).is_none());
        assert!(get_sam_at_sector(2, 4, -1).is_none());

        assert_eq!(SamSite::Chitzena, get_sam_at_sector(2, 4, 0).unwrap());
        assert_eq!(SamSite::Meduna, get_sam_at_sector(4, 14, 0).unwrap());
    }
}
