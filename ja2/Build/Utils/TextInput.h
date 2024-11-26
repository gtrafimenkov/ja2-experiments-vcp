#ifndef __TEXT_INPUT_H
#define __TEXT_INPUT_H

#include "SGP/Input.h"

#define TEXT_CURSOR_BLINK_INTERVAL 500

// AUTHOR:  Kris Morness
// Intended for inclusion with SGP.

// NEW CHANGES:  January 16, 1998
// I have added the ability to stack the text input modes.  So, if you have a
// particular screen that has fields, then somehow, hit a key to go into another
// mode with text input, it will automatically disable the current fields, as you
// go on to define new ones.  Previously, you would have to make sure the mode
// was removed before initializing a new one.  There were potential side effects
// of crashes, and unpredictable results, as the new fields would cook the
// existing ones.
// NOTE:  You may have to modify you code now, so that you don't accidentally
// kill a text input mode when you don't one to begin with.  (like removing an
// already deleted button).  Also, remember that this works like a stack system
// and you can't flip through existing defined text input modes at will.

// NOTES ON LIMITATIONS:
//	-max number of fields 255 (per level)
//  -max num of chars in field 255

// The dosfilename inputtype is a perfect example of what is a exclusive
// handler. In this method, the input accepts only alphas and an underscore as
// the first character, then alphanumerics afterwards.  For further support,
// chances are you'll want to treat it as an exclusive handler, and you'll have
// to process it in the filter input function.
enum InputType {
  INPUTTYPE_NUMERICSTRICT,
  INPUTTYPE_FULL_TEXT,
  INPUTTYPE_DOSFILENAME,
  INPUTTYPE_COORDINATE,
  INPUTTYPE_24HOURCLOCK
};

// Simply initiates that you wish to begin inputting text.  This should only
// apply to screen initializations that contain fields that edit text.  It also
// verifies and clears any existing fields.  Your input loop must contain the
// function HandleTextInput and processed if the gfTextInputMode flag is set else
// process your regular input handler.  Note that this doesn't mean you are
// necessarily typing, just that there are text fields in your screen and may be
// inactive.  The TAB key cycles through your text fields, and special fields can
// be defined which will call a void functionName( uint16_t usFieldNum )
void InitTextInputMode();

// A hybrid version of InitTextInput() which uses a specific scheme.  JA2's
// editor uses scheme 1, so feel free to add new color/font schemes.
enum { DEFAULT_SCHEME };
void InitTextInputModeWithScheme(uint8_t ubSchemeID);

// Clears any existing fields, and ends text input mode.
void KillTextInputMode();
// Kills all levels of text input modes.  When you init a second consecutive
// text input mode, without first removing them, the existing mode will be
// preserved.  This function removes all of them in one call, though doing so
// "may" reflect poor coding style, though I haven't thought about any really
// just uses for it :(
void KillAllTextInputModes();

// Saves the current text input mode, then removes it and activates the previous
// text input mode, if applicable.  The second function restores the settings.
// Doesn't currently support nested calls.
void SaveAndRemoveCurrentTextInputMode();
void RestoreSavedTextInputMode();

void SetTextInputCursor(uint16_t usNewCursor);

// After calling InitTextInputMode, you want to define one or more text input
// fields.  The order of calls to this function dictate the TAB order from
// traversing from one field to the next.  This function adds mouse regions and
// processes them for you, as well as deleting them when you are done.
void AddTextInputField(int16_t sLeft, int16_t sTop, int16_t sWidth, int16_t sHeight,
                       int8_t bPriority, const wchar_t *szInitText, uint8_t ubMaxChars, InputType);

// This allows you to insert special processing functions and modes that can't
// be determined here.  An example would be a file dialog where there would be a
// file list.  This file list would be accessed using the Win95 convention by
// pressing TAB.  In there, your key presses would be handled differently and by
// adding a userinput field, you can make this hook into your function to
// accomplish this.  In a filedialog, alpha characters would be used to jump to
// the file starting with that letter, and setting the field in the text input
// field.  Pressing TAB again would place you back in the text input field.  All
// of that stuff would be handled externally, except for the TAB keys.
typedef void (*INPUT_CALLBACK)(uint8_t, BOOLEAN);
void AddUserInputField(INPUT_CALLBACK userFunction);
// INPUT_CALLBACK explanation:
// The function must use this signature:  void FunctionName( uint8_t ubFieldID,
// BOOLEAN fEntering ); ubFieldID contains the fieldID of that field fEntering is
// true if you are entering the user field, false if exiting.

// This is a useful call made from an external user input field.  Using the
// previous file dialog example, this call would be made when the user selected a
// different filename in the list via clicking or scrolling with the arrows, or
// even using alpha chars to jump to the appropriate filename.
void SetInputFieldStringWith16BitString(uint8_t ubField, const wchar_t *szNewText);
void SetInputFieldStringWith8BitString(uint8_t ubField, const char *szNewText);

// Allows external functions to access the strings within the fields at anytime.
wchar_t const *GetStringFromField(uint8_t ubField);

// Utility functions for the INPUTTYPE_24HOURCLOCK input type.
uint16_t GetExclusive24HourTimeValueFromField(uint8_t ubField);
void SetExclusive24HourTimeValue(uint8_t ubField, uint16_t usTime);

/* Return the field's string as a number. Return -1 if blank or invalid.  Only
 * works for positive numbers. */
int32_t GetNumericStrictValueFromField(uint8_t id);

// Converts a number to a numeric strict value.  If the number is negative, the
// field will be blank.
void SetInputFieldStringWithNumericStrictValue(uint8_t ubField, int32_t iNumber);

// Sets the active field to the specified ID number.
void SetActiveField(uint8_t ubField);
void SelectNextField();

// Returns the active field ID number.  It'll return -1 if no field is active.
int16_t GetActiveFieldID();

// These allow you to customize the general color scheme of your text input
// boxes.  I am assuming that under no circumstances would a user want a
// different color for each field.  It follows the Win95 convention that all text
// input boxes are exactly the same color scheme.  However, these colors can be
// set at anytime, but will effect all of the colors.
void SetTextInputFont(Font);
void Set16BPPTextFieldColor(uint16_t usTextFieldColor);
void SetTextInputRegularColors(uint8_t ubForeColor, uint8_t ubShadowColor);
void SetTextInputHilitedColors(uint8_t ubForeColor, uint8_t ubShadowColor, uint8_t ubBackColor);
// optional color setups
void SetBevelColors(uint16_t usBrighterColor, uint16_t usDarkerColor);
void SetCursorColor(uint16_t usCursorColor);

// All CTRL and ALT keys combinations, F1-F12 keys, ENTER and ESC are ignored
// allowing processing to be done with your own input handler.  Otherwise, the
// keyboard event is absorbed by this input handler, if used in the appropriate
// manner. This call must be added at the beginning of your input handler in this
// format: while( DequeueEvent(&Event) )
//{
//	if(	!HandleTextInput( &Event ) && (your conditions...ex:
// Event.usEvent == KEY_DOWN ) )
//  {
//		switch( Event.usParam )
//		{
//			//Normal key cases here.
//		}
//	}
//}
// It is only necessary for event loops that contain text input fields.
BOOLEAN HandleTextInput(InputAtom const *);

void RenderInactiveTextField(uint8_t ubID);
void RenderAllTextFields();

void DisableTextField(uint8_t ubID);
void EnableTextFields(uint8_t ubFirstID, uint8_t ubLastID);
void DisableTextFields(uint8_t ubFirstID, uint8_t ubLastID);
void EnableAllTextFields();
void DisableAllTextFields();

//
BOOLEAN EditingText();
BOOLEAN TextInputMode();

void InitClipboard();
void KillClipboard();

extern BOOLEAN gfNoScroll;

#endif
