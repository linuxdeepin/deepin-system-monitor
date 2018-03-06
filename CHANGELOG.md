# Change Log
Deepin system monitor: a more user-friendly system monitor.

## [Unreleased]

## [1.4.1] - 2018-03-06
- Init object double/float value, avoid use un-init value to draw wrong result. Create function 'filterInvalidNumber' to filter Inf/NaN numnber.

## [1.4] - 2018-03-01
- Update translations.
- Follow DTK's API
- Move end process dialog at center of window.
- Add compact mode, provide multi-cpu graphics.
- Add disk monitor graphics.
- Fixed getcmdline crash error cause by cmdline.length() == 0
- Fix searchbar focus problem.
- Use default icon instead, if not found "Icon=" line in desktop file.

## [1.3.7] - 2017-11-28
- Split flatpak cmdline's fullpath to get real filename to find location of flatpak application.

## [1.3.6.1] - 2017-11-22
- Update translation

## [1.3.6] - 2017-11-06
- Make interactive kill support FlatPak application window
- Make first tab display FlatPak application process
- Click 'location' menu item to find FlatPak application location
- Fix the bug that which command that can't found location of dde-file-manager and nautils
- Fix cursor not hide after interactive kill
- Find desktop file from process name if found environ variable 'GIO_LAUNCHED_DESKTOP_FILE' failed from tray process
- Find icon from FlatPak application's local directory
- Rasie window to top of stack window list after call deepin-system-monitor command again
- Change code with DTK's newest setFrame API
- Synchronous translation


