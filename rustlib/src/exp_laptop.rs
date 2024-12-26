// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use super::exp_debug;
use super::state;

/// Get money balance (what is shown on the laptop screen).
#[no_mangle]
pub extern "C" fn LaptopMoneyGetBalance() -> i32 {
    state::get().laptop.current_balance
}

/// Set money balance (what is shown on the laptop screen).
#[no_mangle]
pub extern "C" fn LaptopMoneySetBalance(value: i32) {
    state::get().laptop.current_balance = value;
}

/// Add given amount to the money balance
#[no_mangle]
pub extern "C" fn LaptopMoneyAddToBalance(amount: i32) {
    state::get().laptop.current_balance += amount;
}

/// Load the current balance from the disk
#[no_mangle]
pub extern "C" fn LaptopLoadBalanceFromDisk() {
    state::get().laptop.get_balance_from_disk();
    exp_debug::debug_log_write(&format!(
        "balance from disk: {}",
        state::get().laptop.current_balance
    ));
}
