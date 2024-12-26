// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#[derive(Clone, Copy)]
pub enum Rank {
    Green = 0,
    Regular,
    Elite,
}

impl Rank {
    pub const fn next_rank(&self) -> Rank {
        match self {
            Rank::Green => Rank::Regular,
            Rank::Regular => Rank::Elite,
            Rank::Elite => panic!("there is no next rank after Elite"),
        }
    }
}

pub struct State {
    pub force: [[Force; 18]; 18],
    pub training_paid: [[bool; 18]; 18],
}

impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}

impl State {
    pub fn new() -> Self {
        State {
            force: Default::default(),
            training_paid: Default::default(),
        }
    }

    pub fn get_force(&self, x: u8, y: u8) -> &Force {
        &self.force[y as usize][x as usize]
    }

    pub fn get_mut_force(&mut self, x: u8, y: u8) -> &mut Force {
        &mut self.force[y as usize][x as usize]
    }

    pub fn get_force_of_rank(&self, x: u8, y: u8, rank: Rank) -> u8 {
        let mil = self.get_force(x, y);
        match rank {
            Rank::Green => mil.green,
            Rank::Regular => mil.regular,
            Rank::Elite => mil.elite,
        }
    }

    pub fn set_force_of_rank(&mut self, x: u8, y: u8, rank: Rank, value: u8) {
        let mil = self.get_mut_force(x, y);
        match rank {
            Rank::Green => mil.green = value,
            Rank::Regular => mil.regular = value,
            Rank::Elite => mil.elite = value,
        }
    }

    pub fn inc_force_of_rank(&mut self, x: u8, y: u8, rank: Rank, increase: u8) {
        let mil = self.get_mut_force(x, y);
        match rank {
            Rank::Green => mil.green += increase,
            Rank::Regular => mil.regular += increase,
            Rank::Elite => mil.elite += increase,
        }
    }

    pub fn remove_from_sector(&mut self, x: u8, y: u8, rank: Rank, how_many: u8) {
        let current_count = self.get_force_of_rank(x, y, rank);
        if current_count < how_many {
            return;
        }
        self.set_force_of_rank(x, y, rank, current_count - how_many);
    }

    pub fn promote(&mut self, x: u8, y: u8, rank: Rank, how_many: u8) {
        let current_count = self.get_force_of_rank(x, y, rank);
        if current_count < how_many {
            return;
        }
        self.set_force_of_rank(x, y, rank, current_count - how_many);
        self.inc_force_of_rank(x, y, rank.next_rank(), how_many);
    }
}

/// Militia force in a sector.
pub struct Force {
    pub green: u8,
    pub regular: u8,
    pub elite: u8,
}

impl Default for Force {
    fn default() -> Self {
        Self::new()
    }
}

impl Force {
    pub const fn new() -> Self {
        Force {
            green: 0,
            regular: 0,
            elite: 0,
        }
    }

    pub const fn count(&self) -> u8 {
        self.green + self.regular + self.elite
    }
}
