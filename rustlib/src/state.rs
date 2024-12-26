// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use crate::civ_groups;
use crate::clock;
use crate::laptop;
use crate::militia;
use crate::sam_sites;
use crate::sector;
use crate::towns;
use crate::ui;
use std::sync::OnceLock;

// pub static mut STATE: Lazy<State> = Lazy::new(State::new);
pub fn get() -> &'static mut State {
    static mut ALLOC_DB: OnceLock<State> = OnceLock::new();
    unsafe {
        ALLOC_DB.get_or_init(State::new);
        return ALLOC_DB.get_mut().unwrap();
    }
}

pub struct State {
    pub sam_sites: sam_sites::State,
    pub sectors: [[sector::State; 18]; 18],
    pub town_map: towns::TownMap,
    pub towns: towns::State,
    pub militia: militia::State,
    pub ui: ui::State,
    pub civ_groups: civ_groups::State,
    pub laptop: laptop::State,
    pub clock: clock::State,
}

impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}

impl State {
    pub fn new() -> Self {
        let mut state = State {
            sam_sites: sam_sites::State::new(),
            sectors: [[sector::State::new(); 18]; 18],
            town_map: Default::default(),
            towns: Default::default(),
            militia: Default::default(),
            ui: Default::default(),
            civ_groups: Default::default(),
            laptop: Default::default(),
            clock: Default::default(),
        };
        for y in 1..17 {
            for x in 1..17 {
                state.sectors[y][x].enemy_controlled = true;
            }
        }
        state
    }

    pub fn get_sector(&self, x: u8, y: u8) -> &sector::State {
        &self.sectors[y as usize][x as usize]
    }

    pub fn get_mut_sector(&mut self, x: u8, y: u8) -> &mut sector::State {
        &mut self.sectors[y as usize][x as usize]
    }

    pub fn get_militia_force(&self, x: u8, y: u8) -> &militia::Force {
        &self.militia.force[y as usize][x as usize]
    }

    pub fn get_mut_militia_force(&mut self, x: u8, y: u8) -> &mut militia::Force {
        &mut self.militia.force[y as usize][x as usize]
    }

    pub fn get_number_of_sam_under_player_control(&self) -> u8 {
        let mut counter = 0;
        for loc in sam_sites::LOCATIONS.iter() {
            if !self.get_sector(loc.x, loc.y).enemy_controlled {
                counter += 1;
            }
        }
        counter
    }

    /// Update airspace control map based on which SAM sectors are controlled by the enemy.
    pub fn update_airspace_control_map(&mut self) {
        for y in 0..17 {
            for x in 0..17 {
                let sam = sam_sites::get_sam_controlling_sector(x, y);
                self.get_mut_sector(x, y).enemy_air_controlled = match sam {
                    None => false,
                    Some(sam) => {
                        self.get_sector(
                            sam_sites::get_sam_location(sam).x,
                            sam_sites::get_sam_location(sam).y,
                        )
                        .enemy_controlled
                    }
                }
            }
        }
    }

    pub fn inc_town_loyalty(&mut self, town: towns::Town, increase: u32) {
        let loyalty = &mut self.towns.loyalty[town as usize];
        let old_rating = loyalty.rating;
        let rebels_hostility = self.civ_groups.get_hostility(civ_groups::Group::Rebel);
        loyalty.inc_loyalty(town, increase, rebels_hostility);
        if old_rating != loyalty.rating {
            self.ui.map_panel_dirty = true;
        }
    }

    pub fn dec_town_loyalty(&mut self, town: towns::Town, decrease: u32) {
        let loyalty = &mut self.towns.loyalty[town as usize];
        let old_rating = loyalty.rating;
        let rebels_hostility = self.civ_groups.get_hostility(civ_groups::Group::Rebel);
        loyalty.dec_loyalty(town, decrease, rebels_hostility);
        if old_rating != loyalty.rating {
            self.ui.map_panel_dirty = true;
        }
    }

    pub fn start_town_loyalty_first_time(
        &mut self,
        town: towns::Town,
        fact_miguel_read_letter: bool,
        fact_rebels_hate_player: bool,
    ) {
        self.towns.loyalty[town as usize].start_loyalty_first_time(
            town,
            fact_miguel_read_letter,
            fact_rebels_hate_player,
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_state() {
        let state = State::new();
        assert_eq!(0, state.get_militia_force(1, 1).green);
        assert_eq!(0, state.get_militia_force(1, 1).regular);
        assert_eq!(0, state.get_militia_force(1, 1).elite);
        assert_eq!(0, state.laptop.current_balance);
    }

    #[test]
    fn sector_enemy_controlled() {
        let mut state = State::new();

        assert_eq!(true, state.get_sector(1, 1).enemy_controlled);
        assert_eq!(false, state.get_sector(1, 1).enemy_air_controlled);

        state.get_mut_sector(1, 1).enemy_controlled = true;

        assert_eq!(true, state.get_sector(1, 1).enemy_controlled);
        assert_eq!(false, state.get_sector(1, 1).enemy_air_controlled);

        state.get_mut_sector(1, 1).enemy_air_controlled = true;

        assert_eq!(true, state.get_sector(1, 1).enemy_controlled);
        assert_eq!(true, state.get_sector(1, 1).enemy_air_controlled);
    }

    #[test]
    fn sam_controlled_by_player() {
        let mut st = State::new();

        assert_eq!(0, st.get_number_of_sam_under_player_control());

        let loc = sam_sites::get_sam_location(sam_sites::SamSite::Cambria);
        st.get_mut_sector(loc.x, loc.y).enemy_controlled = false;
        assert_eq!(1, st.get_number_of_sam_under_player_control());
    }
}
