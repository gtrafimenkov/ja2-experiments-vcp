// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use crate::civ_groups;

use super::sector::Point;

/// Town IDs.
/// It is almost the same as exp_towns::TownID, but without BLANK_SECTOR.
#[derive(Copy, Clone, Debug, PartialEq)]
pub enum Town {
    Omerta,
    Drassen,
    Alma,
    Grumm,
    Tixa,
    Cambria,
    SanMona,
    Estoni,
    Orta,
    Balime,
    Meduna,
    Chitzena,
}

const NUM_TOWNS: usize = 12;

pub const ALL_TOWNS: [Town; NUM_TOWNS] = [
    Town::Omerta,
    Town::Drassen,
    Town::Alma,
    Town::Grumm,
    Town::Tixa,
    Town::Cambria,
    Town::SanMona,
    Town::Estoni,
    Town::Orta,
    Town::Balime,
    Town::Meduna,
    Town::Chitzena,
];

/// TownMap implements mapping from sector to Town.
pub struct TownMap {
    towns: [[Option<Town>; 17]; 17],
}

impl Default for TownMap {
    fn default() -> Self {
        TownMap::new()
    }
}
impl TownMap {
    pub fn new() -> Self {
        let mut map = TownMap {
            towns: [[None; 17]; 17],
        };
        for town in ALL_TOWNS {
            for sector in town.get_sectors() {
                map.towns[sector.y as usize][sector.x as usize] = Some(town);
            }
        }
        map
    }

    /// Returns a town associated with the give sector
    pub const fn get(&self, x: u8, y: u8) -> Option<Town> {
        debug_assert!(x >= 1 && x <= 16);
        debug_assert!(y >= 1 && y <= 16);
        self.towns[y as usize][x as usize]
    }
}

const OMERTA_SECTORS: [Point; 2] = [Point::new(9, 1), Point::new(10, 1)];
const CHITZENA_SECTORS: [Point; 2] = [Point::new(2, 2), Point::new(2, 1)];
const SAN_MONA_SECTORS: [Point; 4] = [
    Point::new(5, 3),
    Point::new(6, 3),
    Point::new(5, 4),
    Point::new(4, 4),
];
const DRASSEN_SECTORS: [Point; 3] = [Point::new(13, 2), Point::new(13, 3), Point::new(13, 4)];
const GRUMM_SECTORS: [Point; 5] = [
    Point::new(1, 7),
    Point::new(1, 8),
    Point::new(2, 7),
    Point::new(2, 8),
    Point::new(3, 8),
];
const ESTONI_SECTORS: [Point; 1] = [Point::new(6, 9)];
const TIXA_SECTORS: [Point; 1] = [Point::new(9, 10)];
const CAMBRIA_SECTORS: [Point; 5] = [
    Point::new(8, 6),
    Point::new(9, 6),
    Point::new(8, 7),
    Point::new(9, 7),
    Point::new(8, 8),
];
const ALMA_SECTORS: [Point; 4] = [
    Point::new(13, 9),
    Point::new(14, 9),
    Point::new(13, 8),
    Point::new(14, 8),
];
const ORTA_SECTORS: [Point; 1] = [Point::new(4, 11)];
const BALIME_SECTORS: [Point; 2] = [Point::new(11, 12), Point::new(12, 12)];
const MEDUNA_SECTORS: [Point; 6] = [
    Point::new(3, 14),
    Point::new(4, 14),
    Point::new(5, 14),
    Point::new(3, 15),
    Point::new(4, 15),
    Point::new(3, 16),
];

impl Town {
    pub const fn is_militia_training_allowed(&self) -> bool {
        matches!(
            self,
            Town::Drassen
                | Town::Alma
                | Town::Grumm
                | Town::Cambria
                | Town::Balime
                | Town::Meduna
                | Town::Chitzena
        )
    }

    pub const fn does_use_loyalty(&self) -> bool {
        matches!(
            self,
            Town::Omerta
                | Town::Drassen
                | Town::Alma
                | Town::Grumm
                | Town::Cambria
                | Town::Balime
                | Town::Meduna
                | Town::Chitzena
        )
    }

    pub fn get_sectors(&self) -> core::slice::Iter<Point> {
        match self {
            Town::Omerta => OMERTA_SECTORS.iter(),
            Town::Chitzena => CHITZENA_SECTORS.iter(),
            Town::SanMona => SAN_MONA_SECTORS.iter(),
            Town::Drassen => DRASSEN_SECTORS.iter(),
            Town::Grumm => GRUMM_SECTORS.iter(),
            Town::Estoni => ESTONI_SECTORS.iter(),
            Town::Tixa => TIXA_SECTORS.iter(),
            Town::Cambria => CAMBRIA_SECTORS.iter(),
            Town::Alma => ALMA_SECTORS.iter(),
            Town::Orta => ORTA_SECTORS.iter(),
            Town::Balime => BALIME_SECTORS.iter(),
            Town::Meduna => MEDUNA_SECTORS.iter(),
        }
    }
}

#[derive(Clone, Copy)]
pub struct Loyalty {
    pub rating: u8,
    pub change: i16,
    pub started: bool,
    pub liberated: bool,
}

impl Default for Loyalty {
    fn default() -> Self {
        Self::new()
    }
}

// gain pts per real loyalty pt
const GAIN_PTS_PER_LOYALTY_PT: i16 = 500;

const MAX_LOYALTY_VALUE: u8 = 100;
// loyalty Omerta drops to and maxes out at if the player betrays the rebels
const HOSTILE_OMERTA_LOYALTY_RATING: u8 = 10;

impl Loyalty {
    pub fn new() -> Self {
        Loyalty {
            rating: 0,
            change: 0,
            started: false,
            liberated: false,
        }
    }

    pub fn set_loyalty(&mut self, rating: u8) {
        self.rating = rating;
        self.change = 0;
        self.started = true;
    }

    fn update_rating(&mut self, town: Town, rebels_hostility: civ_groups::Hostility) {
        let rating_change = self.change / GAIN_PTS_PER_LOYALTY_PT;

        // if loyalty is ready to increase
        if rating_change > 0 {
            let mut max_loyalty = MAX_LOYALTY_VALUE;
            // if the town is Omerta, and the rebels are/will become hostile
            if town == Town::Omerta && rebels_hostility != civ_groups::Hostility::Neutral {
                // maximum loyalty is much less than normal
                max_loyalty = HOSTILE_OMERTA_LOYALTY_RATING;
            }

            // check if we'd be going over the max
            if self.rating as i16 + rating_change >= max_loyalty as i16 {
                // set to max and null out gain pts
                self.rating = max_loyalty;
                self.change = 0;
            } else {
                // increment loyalty rating, reduce change
                self.rating = (self.rating as i16 + rating_change) as u8;
                self.change %= GAIN_PTS_PER_LOYALTY_PT;
            }
        } else {
            // if loyalty is ready to decrease
            if rating_change < 0 {
                // check if we'd be going below zero
                if self.rating as i16 + rating_change < 0 {
                    // set to zero and null out gain pts
                    self.rating = 0;
                    self.change = 0;
                } else {
                    // decrement loyalty rating, reduce change
                    self.rating = (self.rating as i16 + rating_change) as u8;
                    self.change %= GAIN_PTS_PER_LOYALTY_PT;
                }
            }
        }
    }

    pub fn inc_loyalty(
        &mut self,
        town: Town,
        increase: u32,
        rebels_hostility: civ_groups::Hostility,
    ) {
        if !self.started {
            return;
        }

        // modify loyalty change by town's individual attitude toward rebelling (20 is typical)
        let mut increase = increase;
        increase *= 5 * REBEL_SENTIMENT[town as usize] as u32;
        increase /= 100;

        // this whole thing is a hack to avoid rolling over the -32 to 32k range on the sChange value
        // only do a maximum of 10000 pts at a time...
        let mut remaining = increase;
        while remaining > 0 {
            let this_increment = std::cmp::min(remaining, 10000) as i16;
            // up the gain value
            self.change += this_increment;
            // update town value now
            self.update_rating(town, rebels_hostility);
            remaining -= this_increment as u32;
        }
    }

    pub fn dec_loyalty(
        &mut self,
        town: Town,
        decrease: u32,
        rebels_hostility: civ_groups::Hostility,
    ) {
        if !self.started {
            return;
        }

        // modify loyalty change by town's individual attitude toward rebelling (20 is typical)
        let mut decrease = decrease;
        decrease *= 5 * REBEL_SENTIMENT[town as usize] as u32;
        decrease /= 100;

        // this whole thing is a hack to avoid rolling over the -32 to 32k range on the sChange value
        // only do a maximum of 10000 pts at a time...
        let mut remaining = decrease;
        while remaining > 0 {
            let this_decrement = std::cmp::min(remaining, 10000) as i16;
            // up the gain value
            self.change -= this_decrement;
            // update town value now
            self.update_rating(town, rebels_hostility);
            remaining -= this_decrement as u32;
        }
    }

    pub fn start_loyalty_first_time(
        &mut self,
        town: Town,
        fact_miguel_read_letter: bool,
        fact_rebels_hate_player: bool,
    ) {
        if self.started || !town.does_use_loyalty() {
            return;
        }

        self.rating = REBEL_SENTIMENT[town as usize];

        // if player hasn't made contact with Miguel yet, or the rebels hate the player
        if !fact_miguel_read_letter || fact_rebels_hate_player {
            // if town is Omerta
            if town == Town::Omerta {
                // start loyalty there at 0, since rebels distrust the player until Miguel receives the
                // letter
                self.rating = 0;
            } else {
                // starting loyalty is halved - locals not sure what to make of the player's presence
                self.rating /= 2;
            }
        }

        self.change = 0;

        // remember we've started
        self.started = true;
    }
}

pub struct State {
    pub loyalty: [Loyalty; NUM_TOWNS],
}

impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}

impl State {
    pub fn new() -> Self {
        State {
            loyalty: [Loyalty::new(); NUM_TOWNS],
        }
    }

    pub fn get_loyalty(&self, town: Town) -> &Loyalty {
        &self.loyalty[town as usize]
    }

    pub fn get_mut_loyalty(&mut self, town: Town) -> &mut Loyalty {
        &mut self.loyalty[town as usize]
    }

    pub fn init_loyalty(&mut self) {
        for mut item in self.loyalty {
            item.rating = 0;
            item.change = 0;
            item.started = false;
            item.liberated = false;
        }
    }

    pub fn set_town_loyalty(&mut self, town: Town, rating: u8) {
        if town.does_use_loyalty() {
            self.loyalty[town as usize].set_loyalty(rating)
        }
    }
}

// on a scale of 1-100, this is a measure of how much each town hates the Queen's opression & is
// willing to stand against it it primarily controls the RATE of loyalty change in each town: the
// loyalty effect of the same events depends on it
const REBEL_SENTIMENT: [u8; NUM_TOWNS] = [
    90, // OMERTA	- They ARE the rebels!!!
    30, // DRASSEN	- Rebel friendly, makes it pretty easy to get first mine's income going at the start
    12, // ALMA	- Military town, high loyalty to Queen, need quests to get 100%
    15, // GRUMM - Close to Meduna, strong influence
    20, // TIXA - Not a real town
    15, // CAMBRIA, - Artificially much lower 'cause it's big and central and too easy to get loyalty up there
    20, // SAN_MONA - Neutral ground, loyalty doesn't vary
    20, // ESTONI,	- Not a real town
    20, // ORTA - Not a real town
    12, // BALIME,	- Rich town, high loyalty to Queen
    10, // MEDUNA,	- Enemy HQ, for God's sake!
    35, // CHITZENA, - Artificially high 'cause there's not enough fights near it to get the loyalty up otherwise
];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn militia_training_allowed() {
        assert_eq!(false, Town::Tixa.is_militia_training_allowed());
        assert_eq!(true, Town::Meduna.is_militia_training_allowed());
    }

    #[test]
    fn omerta_sectors_count() {
        assert_eq!(2, Town::Omerta.get_sectors().len());
    }

    #[test]
    fn town_sectors() {
        let map = TownMap::new();

        assert_eq!(None, map.get(8, 1));
        assert_eq!(Some(Town::Omerta), map.get(9, 1));
        assert_eq!(Some(Town::Omerta), map.get(10, 1));
        assert_eq!(None, map.get(11, 1));
        assert_eq!(None, map.get(8, 2));
        assert_eq!(None, map.get(9, 2));
        assert_eq!(None, map.get(10, 2));
        assert_eq!(None, map.get(11, 2));

        assert_eq!(None, map.get(13, 1));
        assert_eq!(Some(Town::Drassen), map.get(13, 2));
        assert_eq!(Some(Town::Drassen), map.get(13, 3));
        assert_eq!(Some(Town::Drassen), map.get(13, 4));
        assert_eq!(None, map.get(13, 5));
    }

    #[test]
    fn does_use_loyalty() {
        assert_eq!(false, Town::Tixa.does_use_loyalty());
        assert_eq!(true, Town::Omerta.does_use_loyalty());
        assert_eq!(true, Town::Alma.does_use_loyalty());
    }

    #[test]
    fn loyalty_start() {
        // half the value (30% / 2) for Drassen when Miguel hasn't read the letter
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Drassen, false, false);
            assert_eq!(15, loyalty.rating);
            assert_eq!(0, loyalty.change);
        }

        // normal value for Drassen (30%) when Miguel has read the letter
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Drassen, true, false);
            assert_eq!(30, loyalty.rating); // half the value becuase Miguel hasn't read the letter
            assert_eq!(0, loyalty.change);
        }

        // half the value for Alma (12% / 2) when rebels hate the player
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Alma, false, true);
            assert_eq!(6, loyalty.rating); // half the value becuase Miguel hasn't read the letter
            assert_eq!(0, loyalty.change);
        }
    }

    #[test]
    fn loyalty_increase_and_decrease() {
        let mut loyalty = Loyalty::new();
        loyalty.start_loyalty_first_time(Town::Drassen, false, false);
        assert_eq!(15, loyalty.rating);
        assert_eq!(0, loyalty.change);

        // increase by 10% but because the rebel santiment for Drassen is 1.5x the increase
        // will be 15%
        loyalty.inc_loyalty(
            Town::Drassen,
            10 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::Neutral,
        );
        assert_eq!(30, loyalty.rating);
        assert_eq!(0, loyalty.change);
    }

    #[test]
    fn loyalty_increase_in_omerta() {
        // Initial rating is 0% if miguel hasn't read the letter.
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Omerta, false, false);
            assert_eq!(0, loyalty.rating);
            assert_eq!(0, loyalty.change);
        }

        // Initial rating is 90% if Miguel read the letter, but in the game this
        // condition never happens because the town is visited before Miguel can read the letter.
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Omerta, true, false);
            assert_eq!(90, loyalty.rating);
            assert_eq!(0, loyalty.change);
        }

        // under normal circumstances the sentiment multiplicator is 4.5 for Omerta
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Omerta, false, false);
            assert_eq!(0, loyalty.rating);
            assert_eq!(0, loyalty.change);

            loyalty.inc_loyalty(
                Town::Omerta,
                10 * GAIN_PTS_PER_LOYALTY_PT as u32,
                civ_groups::Hostility::Neutral,
            );
            assert_eq!(45, loyalty.rating);
            assert_eq!(0, loyalty.change);
        }

        // when rebels are not neutral to the player, the max rating is 10%
        {
            let mut loyalty = Loyalty::new();
            loyalty.start_loyalty_first_time(Town::Omerta, false, false);
            assert_eq!(0, loyalty.rating);
            assert_eq!(0, loyalty.change);

            loyalty.inc_loyalty(
                Town::Omerta,
                5 * GAIN_PTS_PER_LOYALTY_PT as u32,
                civ_groups::Hostility::WillEventuallyBecomeHostile,
            );
            assert_eq!(10, loyalty.rating);
            assert_eq!(0, loyalty.change);
        }
    }

    #[test]
    fn loyalty_small_increases() {
        // One rating point corresponds 500 change points.
        // When adding rating in rating points because of the sentiment multiplicator,
        // there can be some change.
        // For example, when adding 1% rating to Drassen, 4.5 multiplicator gives 4.5% increase,
        // which will be 4 points increase to rating and 250 points increase to change (500/2).
        let mut loyalty = Loyalty::new();
        loyalty.start_loyalty_first_time(Town::Omerta, false, false);
        assert_eq!(0, loyalty.rating);
        assert_eq!(0, loyalty.change);

        loyalty.inc_loyalty(
            Town::Omerta,
            1 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::WillEventuallyBecomeHostile,
        );
        assert_eq!(4, loyalty.rating);
        assert_eq!(250, loyalty.change);

        loyalty.inc_loyalty(
            Town::Omerta,
            1 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::WillEventuallyBecomeHostile,
        );
        assert_eq!(9, loyalty.rating);
        assert_eq!(0, loyalty.change);
    }

    #[test]
    fn loyalty_decrease() {
        let mut loyalty = Loyalty::new();
        loyalty.start_loyalty_first_time(Town::Omerta, false, false);
        assert_eq!(0, loyalty.rating);
        assert_eq!(0, loyalty.change);

        loyalty.inc_loyalty(
            Town::Omerta,
            10 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::Neutral,
        );
        assert_eq!(45, loyalty.rating);
        assert_eq!(0, loyalty.change);

        loyalty.dec_loyalty(
            Town::Omerta,
            2 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::Neutral,
        );
        assert_eq!(36, loyalty.rating);
        assert_eq!(0, loyalty.change);

        loyalty.dec_loyalty(
            Town::Omerta,
            1 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::Neutral,
        );
        assert_eq!(32, loyalty.rating);
        assert_eq!(-250, loyalty.change);

        loyalty.dec_loyalty(
            Town::Omerta,
            1 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::Neutral,
        );
        assert_eq!(27, loyalty.rating);
        assert_eq!(0, loyalty.change);

        // must not go negative
        loyalty.dec_loyalty(
            Town::Omerta,
            10 * GAIN_PTS_PER_LOYALTY_PT as u32,
            civ_groups::Hostility::Neutral,
        );
        assert_eq!(0, loyalty.rating);
        assert_eq!(0, loyalty.change);
    }
}
