// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::state;
use crate::exp_ui;
use crate::militia;

#[repr(C)]
#[allow(non_camel_case_types)]
#[derive(Clone, Copy)]
pub enum MilitiaRank {
    GREEN_MILITIA = 0,
    REGULAR_MILITIA,
    ELITE_MILITIA,
}

impl MilitiaRank {
    pub const fn to_internal(&self) -> militia::Rank {
        match self {
            MilitiaRank::GREEN_MILITIA => militia::Rank::Green,
            MilitiaRank::REGULAR_MILITIA => militia::Rank::Regular,
            MilitiaRank::ELITE_MILITIA => militia::Rank::Elite,
        }
    }
}

pub const MAX_MILITIA_LEVELS: u8 = 3;

#[repr(C)]
#[allow(non_snake_case)]
/// Militia force in a sector.
pub struct MilitiaCount {
    pub green: u8,
    pub regular: u8,
    pub elite: u8,
}

impl MilitiaCount {
    // const fn to_internal(&self) -> militia::Force {
    //     militia::Force {
    //         green: self.green,
    //         regular: self.regular,
    //         elite: self.elite,
    //     }
    // }
    const fn from_internal(value: &militia::Force) -> Self {
        MilitiaCount {
            green: value.green,
            regular: value.regular,
            elite: value.elite,
        }
    }
}

#[no_mangle]
/// Return militia force in a sector.
pub extern "C" fn GetMilitiaInSector(x: u8, y: u8) -> MilitiaCount {
    MilitiaCount::from_internal(state::get().get_militia_force(x, y))
}

#[no_mangle]
/// Set militia force in a sector.
pub extern "C" fn SetMilitiaInSector(x: u8, y: u8, value: MilitiaCount) {
    let force = state::get().get_mut_militia_force(x, y);
    force.green = value.green;
    force.regular = value.regular;
    force.elite = value.elite;
}

#[no_mangle]
pub extern "C" fn GetMilitiaOfRankInSector(x: u8, y: u8, rank: MilitiaRank) -> u8 {
    state::get()
        .militia
        .get_force_of_rank(x, y, rank.to_internal())
}

#[no_mangle]
pub extern "C" fn CountMilitiaInSector(x: u8, y: u8) -> u8 {
    state::get().get_militia_force(x, y).count()
}

#[no_mangle]
pub extern "C" fn CountMilitiaInSector3D(x: u8, y: u8, z: i8) -> u8 {
    match z {
        0 => state::get().get_militia_force(x, y).count(),
        _ => 0,
    }
}

#[no_mangle]
pub extern "C" fn SetMilitiaOfRankInSector(x: u8, y: u8, rank: MilitiaRank, value: u8) {
    state::get()
        .militia
        .set_force_of_rank(x, y, rank.to_internal(), value)
}

#[no_mangle]
pub extern "C" fn IncMilitiaOfRankInSector(x: u8, y: u8, rank: MilitiaRank, increase: u8) {
    state::get()
        .militia
        .inc_force_of_rank(x, y, rank.to_internal(), increase)
}

#[no_mangle]
pub extern "C" fn IsMilitiaTrainingPayedForSector(x: u8, y: u8) -> bool {
    state::get().militia.training_paid[y as usize][x as usize]
}

#[no_mangle]
pub extern "C" fn SetMilitiaTrainingPayedForSector(x: u8, y: u8, value: bool) {
    state::get().militia.training_paid[y as usize][x as usize] = value
}

#[no_mangle]
pub extern "C" fn RemoveMilitiaFromSector(x: u8, y: u8, rank: MilitiaRank, how_many: u8) {
    state::get()
        .militia
        .remove_from_sector(x, y, rank.to_internal(), how_many);
    exp_ui::SetMapPanelDirty(true);
}

#[no_mangle]
pub extern "C" fn PromoteMilitia(x: u8, y: u8, rank: MilitiaRank, how_many: u8) {
    state::get()
        .militia
        .promote(x, y, rank.to_internal(), how_many);
    exp_ui::SetMapPanelDirty(true);
}
