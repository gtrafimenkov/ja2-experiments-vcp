// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::state;
use crate::civ_groups;

#[repr(C)]
#[allow(non_camel_case_types)]
#[derive(Clone, Copy)]
pub enum CivGroupHostility {
    CIV_GROUP_NEUTRAL = 0,
    CIV_GROUP_WILL_EVENTUALLY_BECOME_HOSTILE = 1,
    CIV_GROUP_WILL_BECOME_HOSTILE = 2,
    CIV_GROUP_HOSTILE = 3,
}

impl CivGroupHostility {
    pub const fn to_internal(&self) -> civ_groups::Hostility {
        match self {
            CivGroupHostility::CIV_GROUP_NEUTRAL => civ_groups::Hostility::Neutral,
            CivGroupHostility::CIV_GROUP_WILL_EVENTUALLY_BECOME_HOSTILE => {
                civ_groups::Hostility::WillEventuallyBecomeHostile
            }
            CivGroupHostility::CIV_GROUP_WILL_BECOME_HOSTILE => {
                civ_groups::Hostility::WillBecomeHostile
            }
            CivGroupHostility::CIV_GROUP_HOSTILE => civ_groups::Hostility::Hostile,
        }
    }

    pub const fn from_internal(hostility: civ_groups::Hostility) -> CivGroupHostility {
        match hostility {
            civ_groups::Hostility::Neutral => CivGroupHostility::CIV_GROUP_NEUTRAL,
            civ_groups::Hostility::WillEventuallyBecomeHostile => {
                CivGroupHostility::CIV_GROUP_WILL_EVENTUALLY_BECOME_HOSTILE
            }
            civ_groups::Hostility::WillBecomeHostile => {
                CivGroupHostility::CIV_GROUP_WILL_BECOME_HOSTILE
            }
            civ_groups::Hostility::Hostile => CivGroupHostility::CIV_GROUP_HOSTILE,
        }
    }
}

#[repr(C)]
#[allow(non_camel_case_types)]
#[derive(Clone, Copy)]
pub enum CivGroup {
    NON_CIV_GROUP = 0,
    REBEL_CIV_GROUP,
    KINGPIN_CIV_GROUP,
    SANMONA_ARMS_GROUP,
    ANGELS_GROUP,
    BEGGARS_CIV_GROUP,
    TOURISTS_CIV_GROUP,
    ALMA_MILITARY_CIV_GROUP,
    DOCTORS_CIV_GROUP,
    COUPLE1_CIV_GROUP,
    HICKS_CIV_GROUP,
    WARDEN_CIV_GROUP,
    JUNKYARD_CIV_GROUP,
    FACTORY_KIDS_GROUP,
    QUEENS_CIV_GROUP,
    UNNAMED_CIV_GROUP_15,
    UNNAMED_CIV_GROUP_16,
    UNNAMED_CIV_GROUP_17,
    UNNAMED_CIV_GROUP_18,
    UNNAMED_CIV_GROUP_19,
}
pub const FIRST_CIV_GROUP: u8 = 1;
pub const NUM_CIV_GROUPS: u8 = 20;

impl CivGroup {
    pub const fn to_internal(&self) -> civ_groups::Group {
        match self {
            CivGroup::NON_CIV_GROUP => panic!("an attemp to convert NON_CIV_GROUP to internal"),
            CivGroup::REBEL_CIV_GROUP => civ_groups::Group::Rebel,
            CivGroup::KINGPIN_CIV_GROUP => civ_groups::Group::Kingpin,
            CivGroup::SANMONA_ARMS_GROUP => civ_groups::Group::SanmonaArmsGroup,
            CivGroup::ANGELS_GROUP => civ_groups::Group::AngelsGroup,
            CivGroup::BEGGARS_CIV_GROUP => civ_groups::Group::Beggars,
            CivGroup::TOURISTS_CIV_GROUP => civ_groups::Group::Tourists,
            CivGroup::ALMA_MILITARY_CIV_GROUP => civ_groups::Group::AlmaMilitary,
            CivGroup::DOCTORS_CIV_GROUP => civ_groups::Group::Doctors,
            CivGroup::COUPLE1_CIV_GROUP => civ_groups::Group::Couple1,
            CivGroup::HICKS_CIV_GROUP => civ_groups::Group::Hicks,
            CivGroup::WARDEN_CIV_GROUP => civ_groups::Group::Warden,
            CivGroup::JUNKYARD_CIV_GROUP => civ_groups::Group::Junkyard,
            CivGroup::FACTORY_KIDS_GROUP => civ_groups::Group::FactoryKidsGroup,
            CivGroup::QUEENS_CIV_GROUP => civ_groups::Group::Queens,
            CivGroup::UNNAMED_CIV_GROUP_15 => civ_groups::Group::Unnamed15,
            CivGroup::UNNAMED_CIV_GROUP_16 => civ_groups::Group::Unnamed16,
            CivGroup::UNNAMED_CIV_GROUP_17 => civ_groups::Group::Unnamed17,
            CivGroup::UNNAMED_CIV_GROUP_18 => civ_groups::Group::Unnamed18,
            CivGroup::UNNAMED_CIV_GROUP_19 => civ_groups::Group::Unnamed19,
        }
    }
}

#[no_mangle]
#[allow(unused_variables)]
pub extern "C" fn GetCivGroupHostility(group: CivGroup) -> CivGroupHostility {
    CivGroupHostility::from_internal(state::get().civ_groups.get_hostility(group.to_internal()))
}

#[no_mangle]
#[allow(unused_variables)]
pub extern "C" fn SetCivGroupHostility(group: CivGroup, hostility: CivGroupHostility) {
    state::get()
        .civ_groups
        .set_hostility(group.to_internal(), hostility.to_internal())
}
