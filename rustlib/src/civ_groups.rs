// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#[derive(Clone, Copy, PartialEq)]
pub enum Hostility {
    Neutral,
    WillEventuallyBecomeHostile,
    WillBecomeHostile,
    Hostile,
}

#[derive(Clone, Copy)]
pub enum Group {
    // NON_CIV_GROUP = 0,
    Rebel = 1,
    Kingpin,
    SanmonaArmsGroup,
    AngelsGroup,
    Beggars,
    Tourists,
    AlmaMilitary,
    Doctors,
    Couple1,
    Hicks,
    Warden,
    Junkyard,
    FactoryKidsGroup,
    Queens,
    Unnamed15,
    Unnamed16,
    Unnamed17,
    Unnamed18,
    Unnamed19,
}

pub struct State {
    hostility: [Hostility; 20],
    // pub force: [[Force; 18]; 18],
    // pub training_paid: [[bool; 18]; 18],
}

impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}

impl State {
    pub fn new() -> Self {
        State {
            hostility: [Hostility::Neutral; 20],
        }
    }
    pub fn get_hostility(&self, group: Group) -> Hostility {
        self.hostility[group as usize]
    }
    pub fn set_hostility(&mut self, group: Group, hostility: Hostility) {
        self.hostility[group as usize] = hostility
    }
}
