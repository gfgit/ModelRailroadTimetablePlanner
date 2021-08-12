# HOW TO CONTRIBUTE

I'm New to Git and Github so I don't know yet how to manage repositories and contributions.

The model should be creating pull requests with topic branches and merging on master or `master` or `develop`.


# Usage of branches

- `master`: stable code ready to be released

- `develop`: unstable code or code that needs further testing

To develop new features please create new branches and finally merge to develop

# Translations

UI is localized with Qt Linguist.
Translation files live in [`src/translations`](src/translations) folder.
For more information see [Qt Documentation](https://doc.qt.io/qt-5/linguist-overview.html).

## Adding a new language

1. Create a file named `traintimetable_*.ts` in translation folder.
Replace placeholder with language code (i.e. `it`, `de`, `fr`, etc).

2. Make file known to Cmake by adding it to
[`src/translations/CMakeLists.txt`](src/translations/CMakeLists.txt).
Add the file name with path in `TRAINTIMETABLE_TS_FILES` variabile.

Then follow next paragraph.

## Update translations to match new UI elements

3. Run `lupdate` to fill with text to be traslated.
This is done by enabling `UPDATE_TS` option in CMake and
running `RELEASE_TRANSLATIONS` target.

4. Use Qt Linguist or other software to translate text.


# Do you have suggestions?
If you think this workflow model is not efficient please let me know!
