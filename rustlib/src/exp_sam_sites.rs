// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::sam_sites;
use super::state;

/// Min condition for sam site to be functional
pub const MIN_CONDITION_FOR_SAM_SITE_TO_WORK: u8 = 80;

#[repr(C)]
#[derive(Copy, Clone)]
pub enum SamSite {
    SamSiteChitzena = 0, /* sam_sites::SamSite::Chitzena as isize */
    SamSiteDrassen = 1,  /* sam_sites::SamSite::Drassen as isize*/
    SamSiteCambria = 2,  /* sam_sites::SamSite::Cambria as isize*/
    SamSiteMeduna = 3,   /* sam_sites::SamSite::Meduna as isize*/
}

impl SamSite {
    fn from_internal(value: sam_sites::SamSite) -> Self {
        match value {
            sam_sites::SamSite::Chitzena => SamSite::SamSiteChitzena,
            sam_sites::SamSite::Drassen => SamSite::SamSiteDrassen,
            sam_sites::SamSite::Cambria => SamSite::SamSiteCambria,
            sam_sites::SamSite::Meduna => SamSite::SamSiteMeduna,
        }
    }
    fn to_internal(self) -> sam_sites::SamSite {
        match self {
            SamSite::SamSiteChitzena => sam_sites::SamSite::Chitzena,
            SamSite::SamSiteDrassen => sam_sites::SamSite::Drassen,
            SamSite::SamSiteCambria => sam_sites::SamSite::Cambria,
            SamSite::SamSiteMeduna => sam_sites::SamSite::Meduna,
        }
    }
}

const SITES: [SamSite; 4] = [
    SamSite::SamSiteChitzena,
    SamSite::SamSiteDrassen,
    SamSite::SamSiteCambria,
    SamSite::SamSiteMeduna,
];

#[no_mangle]
/// Return total number of SAM sites
pub extern "C" fn GetSamSiteCount() -> u8 {
    sam_sites::LOCATIONS.len() as u8
}

#[no_mangle]
/// Return X location of the SAM site
pub extern "C" fn GetSamSiteX(site: SamSite) -> u8 {
    sam_sites::get_sam_location(site.to_internal()).x
}

#[no_mangle]
/// Return Y location of the SAM site
pub extern "C" fn GetSamSiteY(site: SamSite) -> u8 {
    sam_sites::get_sam_location(site.to_internal()).y
}

#[no_mangle]
/// Check if the SAM site was found.
pub extern "C" fn IsSamSiteFound(site: SamSite) -> bool {
    state::get().sam_sites.is_found(site.to_internal())
}

#[no_mangle]
/// Set if the SAM site was found.
pub extern "C" fn SetSamSiteFound(site: SamSite, value: bool) {
    state::get().sam_sites.set_found(site.to_internal(), value)
}

#[no_mangle]
pub extern "C" fn GetSamGraphicsID(site: SamSite) -> u8 {
    // ATE: Update this w/ graphic used
    // Use 3 if / orientation, 4 if \ orientation
    const GRAPHICS_LIST: [u8; 4] = [4, 3, 3, 3];
    GRAPHICS_LIST[site as usize]
}

#[no_mangle]
pub extern "C" fn GetSamGridNoA(site: SamSite) -> i16 {
    const GRID_NO_A: [i16; 4] = [10196, 11295, 16080, 11913];
    GRID_NO_A[site as usize]
}

#[no_mangle]
pub extern "C" fn GetSamGridNoB(site: SamSite) -> i16 {
    const GRID_NO_B: [i16; 4] = [10195, 11135, 15920, 11912];
    GRID_NO_B[site as usize]
}

#[no_mangle]
pub extern "C" fn DoesSAMExistHere(sector_x: u8, sector_y: u8, sector_z: i8, grid_no: i16) -> bool {
    if sector_z != 0 {
        return false;
    }

    for site in SITES.iter() {
        if GetSamSiteX(*site) == sector_x
            && GetSamSiteY(*site) == sector_y
            && (GetSamGridNoA(*site) == grid_no || GetSamGridNoB(*site) == grid_no)
        {
            return true;
        }
    }

    false
}

#[no_mangle]
pub extern "C" fn GetSamCondition(site: SamSite) -> u8 {
    state::get().sam_sites.sites[site as usize].get_condition()
}

#[no_mangle]
pub extern "C" fn SetSamCondition(site: SamSite, value: u8) {
    state::get().sam_sites.sites[site as usize].set_condition(value);
}

#[repr(C)]
pub enum OptionalSamSite {
    Some(SamSite),
    None,
}

#[no_mangle]
/// Return SAM if it is in the given sector.
pub extern "C" fn GetSamAtSector(x: u8, y: u8, z: i8) -> OptionalSamSite {
    match sam_sites::get_sam_at_sector(x, y, z) {
        None => OptionalSamSite::None,
        Some(site) => OptionalSamSite::Some(SamSite::from_internal(site)),
    }
}

#[no_mangle]
/// Returns SAM controlling the given sector.
pub extern "C" fn GetSamControllingSector(x: u8, y: u8) -> OptionalSamSite {
    match sam_sites::get_sam_controlling_sector(x, y) {
        None => OptionalSamSite::None,
        Some(site) => OptionalSamSite::Some(SamSite::from_internal(site)),
    }
}

#[no_mangle]
pub extern "C" fn IsThereAFunctionalSamInSector(x: u8, y: u8, z: i8) -> bool {
    match sam_sites::get_sam_at_sector(x, y, z) {
        None => false,
        Some(site) => {
            state::get().sam_sites.sites[site as usize].get_condition()
                >= MIN_CONDITION_FOR_SAM_SITE_TO_WORK
        }
    }
}

#[no_mangle]
pub extern "C" fn GetNumberOfSAMSitesUnderPlayerControl() -> u8 {
    state::get().get_number_of_sam_under_player_control()
}

#[no_mangle]
pub extern "C" fn IsSamUnderPlayerControl(site: SamSite) -> bool {
    !state::get()
        .get_sector(GetSamSiteX(site), GetSamSiteY(site))
        .enemy_controlled
}

#[no_mangle]
/// Update airspace control map based on which SAM sectors are controlled by the enemy.
pub extern "C" fn UpdateAirspaceControlMap() {
    state::get().update_airspace_control_map()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn count_and_coords() {
        assert_eq!(4, GetSamSiteCount());
        assert_eq!(15, GetSamSiteX(SamSite::SamSiteDrassen));
        assert_eq!(4, GetSamSiteY(SamSite::SamSiteDrassen));
    }

    #[test]
    fn sam_existance() {
        assert_eq!(false, DoesSAMExistHere(1, 1, 0, 1));

        assert_eq!(true, DoesSAMExistHere(15, 4, 0, 11295));
        assert_eq!(true, DoesSAMExistHere(15, 4, 0, 11135));
        assert_eq!(false, DoesSAMExistHere(15, 4, 1, 11135));
    }
}
