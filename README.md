# sdmenu

shell dmenu

## Controls

* `Return` Output the selected item if any, else, the current input.
* `Tab` Select the next item.
* `Backspace` Erase input and select the first item.
* `Esc` Exit.

## Options

* `$SDMENU_LINES` How many lines of entries to display, default 2.
* `$SDMENU_WIDTH` The maximum number of characters of each entry to display,
  default 86.
* `$SDMENU_SELECTED` The escape code for the selected item, default bold.

## Installation

```sh
make && sudo make install
```
