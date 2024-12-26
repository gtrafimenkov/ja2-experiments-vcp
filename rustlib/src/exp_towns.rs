// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use crate::{sector, state, towns};

#[repr(C)]
#[allow(non_camel_case_types)]
pub enum TownID {
    BLANK_SECTOR = 0,
    OMERTA = 1,
    DRASSEN = 2,
    ALMA = 3,
    GRUMM = 4,
    TIXA = 5,
    CAMBRIA = 6,
    SAN_MONA = 7,
    ESTONI = 8,
    ORTA = 9,
    BALIME = 10,
    MEDUNA = 11,
    CHITZENA = 12,
}

impl TownID {
    #[allow(dead_code)]
    fn from_internal(value: towns::Town) -> Self {
        match value {
            towns::Town::Omerta => TownID::OMERTA,
            towns::Town::Drassen => TownID::DRASSEN,
            towns::Town::Alma => TownID::ALMA,
            towns::Town::Grumm => TownID::GRUMM,
            towns::Town::Tixa => TownID::TIXA,
            towns::Town::Cambria => TownID::CAMBRIA,
            towns::Town::SanMona => TownID::SAN_MONA,
            towns::Town::Estoni => TownID::ESTONI,
            towns::Town::Orta => TownID::ORTA,
            towns::Town::Balime => TownID::BALIME,
            towns::Town::Meduna => TownID::MEDUNA,
            towns::Town::Chitzena => TownID::CHITZENA,
        }
    }
    const fn to_internal(&self) -> towns::Town {
        match self {
            TownID::OMERTA => towns::Town::Omerta,
            TownID::DRASSEN => towns::Town::Drassen,
            TownID::ALMA => towns::Town::Alma,
            TownID::GRUMM => towns::Town::Grumm,
            TownID::TIXA => towns::Town::Tixa,
            TownID::CAMBRIA => towns::Town::Cambria,
            TownID::SAN_MONA => towns::Town::SanMona,
            TownID::ESTONI => towns::Town::Estoni,
            TownID::ORTA => towns::Town::Orta,
            TownID::BALIME => towns::Town::Balime,
            TownID::MEDUNA => towns::Town::Meduna,
            TownID::CHITZENA => towns::Town::Chitzena,
            TownID::BLANK_SECTOR => {
                // this inconsistency should not leak to the proper code
                panic!("an attempt to convert BLANK_SECTOR to a proper town");
            }
        }
    }
}

#[repr(C)]
#[allow(non_snake_case)]
pub struct TownSector {
    townID: TownID,
    x: u8,
    y: u8,
}

#[repr(C)]
pub struct TownSectors {
    count: u8,
    sectors: [TownSector; 40],
}

#[no_mangle]
/// Returs the list of all sectors belonging to towns
pub extern "C" fn GetAllTownSectors(data: &mut TownSectors) {
    let mut cnt = 0;
    for town in towns::ALL_TOWNS {
        for sec in town.get_sectors() {
            data.sectors[cnt].x = sec.x;
            data.sectors[cnt].y = sec.y;
            data.sectors[cnt].townID = TownID::from_internal(town);
            cnt += 1;
        }
    }
    data.count = cnt as u8;
}

/// Is militia training allowed in this town?
#[no_mangle]
pub extern "C" fn MilitiaTrainingAllowedInTown(town: TownID) -> bool {
    match town {
        TownID::BLANK_SECTOR => false,
        _ => town.to_internal().is_militia_training_allowed(),
    }
}

/// Does town uses loyalty mechanic
#[no_mangle]
pub extern "C" fn DoesTownUseLoyalty(town: TownID) -> bool {
    match town {
        TownID::BLANK_SECTOR => false,
        _ => town.to_internal().does_use_loyalty(),
    }
}

/// Return TownID the sector belongs to.
#[no_mangle]
pub extern "C" fn GetTownIdForSector(x: u8, y: u8) -> TownID {
    if !sector::valid_coords(x, y) {
        return TownID::BLANK_SECTOR;
    }
    let town = state::get().town_map.get(x, y);
    match town {
        None => TownID::BLANK_SECTOR,
        Some(town) => TownID::from_internal(town),
    }
}

#[repr(C)]
#[allow(non_camel_case_types)]
// Data structure for saving and loading the town loyalty
pub struct SAVE_LOAD_TOWN_LOYALTY {
    rating: u8,
    change: i16,
    started: u8,
    unused1: u8,
    liberated: u8,
    unused2: [u8; 19],
}

#[no_mangle]
pub extern "C" fn GetRawTownLoyalty(town: TownID) -> SAVE_LOAD_TOWN_LOYALTY {
    match town {
        TownID::BLANK_SECTOR => SAVE_LOAD_TOWN_LOYALTY {
            rating: 0,
            change: 0,
            started: 0,
            unused1: 0,
            liberated: 0,
            unused2: [0; 19],
        },
        _ => {
            let internal = state::get().towns.get_loyalty(town.to_internal());
            SAVE_LOAD_TOWN_LOYALTY {
                rating: internal.rating,
                change: internal.change,
                started: internal.started as u8,
                unused1: 0,
                liberated: internal.liberated as u8,
                unused2: [0; 19],
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn SetRawTownLoyalty(town: TownID, data: &SAVE_LOAD_TOWN_LOYALTY) {
    match town {
        TownID::BLANK_SECTOR => {}
        _ => {
            let internal = state::get().towns.get_mut_loyalty(town.to_internal());
            internal.change = data.change;
            internal.rating = data.rating;
            internal.started = data.started != 0;
            internal.liberated = data.liberated != 0;
        }
    }
}

#[no_mangle]
pub extern "C" fn GetTownLoyaltyRating(town: TownID) -> u8 {
    state::get().towns.get_loyalty(town.to_internal()).rating
}

#[no_mangle]
pub extern "C" fn IsTownLoyaltyStarted(town: TownID) -> bool {
    state::get().towns.get_loyalty(town.to_internal()).started
}

#[no_mangle]
pub extern "C" fn IsTownLiberated(town: TownID) -> bool {
    state::get().towns.get_loyalty(town.to_internal()).liberated
}

#[no_mangle]
pub extern "C" fn SetTownAsLiberated(town: TownID) {
    state::get()
        .towns
        .get_mut_loyalty(town.to_internal())
        .liberated = true
}

#[no_mangle]
pub extern "C" fn InitTownLoyalty() {
    state::get().towns.init_loyalty()
}

#[no_mangle]
pub extern "C" fn SetTownLoyalty(town: TownID, rating: u8) {
    state::get()
        .towns
        .set_town_loyalty(town.to_internal(), rating)
}

#[no_mangle]
pub extern "C" fn IncrementTownLoyalty(town: TownID, increase: u32) {
    state::get().inc_town_loyalty(town.to_internal(), increase)
}

#[no_mangle]
pub extern "C" fn DecrementTownLoyalty(town: TownID, decrease: u32) {
    state::get().dec_town_loyalty(town.to_internal(), decrease)
}

#[no_mangle]
pub extern "C" fn StartTownLoyaltyFirstTime(
    town: TownID,
    fact_miguel_read_letter: bool,
    fact_rebels_hate_player: bool,
) {
    state::get().start_town_loyalty_first_time(
        town.to_internal(),
        fact_miguel_read_letter,
        fact_rebels_hate_player,
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn militia_training_allowed() {
        assert_eq!(false, MilitiaTrainingAllowedInTown(TownID::BLANK_SECTOR));
        assert_eq!(false, MilitiaTrainingAllowedInTown(TownID::TIXA));

        assert_eq!(true, MilitiaTrainingAllowedInTown(TownID::MEDUNA));
    }
}
