// This is not free software.
// This file may contain code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

use crate::exp_debug;
use std::env;

#[repr(C)]
#[derive(Default, Debug)]
pub struct ParsedCommandLine {
    editor_auto: bool,
    editor: bool,
    no_sound: bool,
    quick_save: bool,
}

#[no_mangle]
/// Parse command line of the application.
pub extern "C" fn ParseCommandLine() -> ParsedCommandLine {
    let mut res = ParsedCommandLine::default();
    let args: Vec<String> = env::args().collect();
    for arg in args {
        exp_debug::debug_log_write(&arg);
        if arg == "/NOSOUND" {
            res.no_sound = true;
        } else if arg == "-quicksave" {
            res.quick_save = true;
        } else if arg == "-EDITORAUTO" {
            res.editor_auto = true;
        } else if arg == "-EDITOR" {
            res.editor = true;
        }
    }
    res
}
