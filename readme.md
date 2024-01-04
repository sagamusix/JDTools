# JDTools - Patch conversion utility for Roland JD-800 / JD-990 and compatibles

While it is easy to convert JD-800 patches to use with the JD-990 (the JD-990 itself can do it through RAM card exchange, and there are PC tools for it as well),
there don't appear to be any tools to go in the opposite direction. In the general case this makes sense, because the JD-990 has more features.

However, I purchased a sound bank for the JD-800 where the original SysEx dump for the JD-800 was lost and all I got was a SysEx dump for the JD-990, which my JD-800 obviously couldn't load.

Knowing that the patches in this dump must be fully compatible with the JD-800, I started studying the SysEx dump structures of the two synths and threw together some code in half a day to convert between the two.

The tool can load both SYX files (raw SysEx dumps) and MID / SMF files (Standard MIDI Files) and convert from JD-800 patch format to JD-990 and vice versa. It can also convert special setups but other multi mode settings cannot be converted (there isn't much to convert that the two synths have in common...).

After the basic functionality was done, I wondered what would be required to support the JD-800 VST plugin or the JD-08 as well - turns out, quite a lot! Nevertheless, I managed to add support for them as well, so you can convert original JD-800 patches to the plugin's format (as long as they don't use ROM card waveforms), and also convert plugin banks to use with the original JD-800 (as long as they don't use extended features such as unison or tempo-synced LFOs).
As the plugin appears to be based on Roland's ZenCore engine, don't expect conversions that sound 100% identical (there are well-known differences). Some parameters seem to have much lower internal precision than on a real JD-800. On the upside, the converted files also work with the Zenology plugin. But as the conversion process is quite complex, it's always possible there is a bug, so please report those if you find any.

Figuring out the data structures shared by the plugin, SVZ and SVD patch formats was a lot of work, so if you find this tool useful, please consider [donating](https://paypal.me/JohannesSchultz) a few bucks. This would be especially appreciated if you are going to sell the converted files. Alternatively you can pay me by sending your converted sound sets. ;)

You can also support me by purchasing my "[Explorations](https://lfo.sellfy.store/p/roland-jd-800-990-explorations-soundset-64-presets/)" sound bank. It is compatible with all JD models supported by this software! 

# Usage

JDTools is a command line utility and has no Graphical User Interface. It can be invoked in the following ways. Values in  `<angled brackets>` indicate user-supplied values. The angled brackets must not be entered into the command line.

## Conversion

It is possible to convert between practically all format combinations:

- JD-800 SysEx dumps (SYX, MID) can be converted to...
  - JD-990 SysEx dump (SYX)
  - JD-800 VST / Zenology patch bank (BIN)
  - JD-08 patch bank (SVD)
  - ZC1 patch bank (SVZ)
- JD-990 SysEx dumps (SYX, MID) can be converted to...
  - JD-800 SysEx dump (SYX)
  - JD-800 VST / Zenology patch bank (BIN)
  - JD-08 patch bank (SVD)
  - ZC1 patch bank (SVZ)
- JD-800 VST or Zenology patch banks (BIN) can be converted to... 
  - JD-800 SysEx dump (SYX)
  - JD-08 patch bank (SVD)
  - ZC1 patch bank (SVZ)
- JD-08 patch banks (SVD) can be converted to...
  - JD-800 SysEx dump (SYX)
  - JD-800 VST or Zenology patch bank (BIN)
  - ZC1 patch bank (SVZ)
- ZC1 patch banks (SVZ) can be converted to...
  - JD-800 SysEx dump (SYX)
  - JD-800 VST or Zenology patch bank (BIN)
  - JD-08 patch bank (SVD)

The input format of the conversion is determined automatically, but due to the different output options, the desired target format has to be specified explicitly.

By invoking `JDTools convert syx <input.file> <output.syx>`, the input file is converted to a SysEx dump. If the source is a JD-800 SysEx dump, the output file is a JD-990 SysEx dump, in all other cases the output is a JD-800 SysEx dump.

By invoking `JDTools convert bin <input.file> <output.bin>`, the input file is converted to the JD-800 VST patch bank format (BIN).

By invoking `JDTools convert svd <input.file> <JD08Backup.svd> <position>`, the input file is converted to the JD-08 patch bank format (SVD). The provided output file must be an **already existing** JD08Backup.svd file obtained from your JD-08. The file is then overwritten, but its contents are replaced with the new patch data. The output file should be named JD08Backup.svd so that the JD-08 can find it. The last parameter is optional and specifies the starting patch position to overwrite. This can be just a bank (A/B/C/D) or a patch number (e.g. B42).

By invoking `JDTools convert svz <input.file> <output.svz>`, the input file is converted to the ZC1 hardware patch bank format (SVZ), for use with the Jupiter-X with the JD-800 Model Expansion and potentially other hardware synthesizers based on ZenCore.

To convert e.g. a JD-800 VST patch bank to a JD-990 SysEx dump, an intermediate conversion to a JD-800 SysEx dump is required.

As an example, the following batch script can be used to convert all SYX and MID files in the current directory and its subdirectories to BIN files to use with the plugin. It assumes that JDTools.exe is also placed in the current directory.
The script also creates a conversion log file called convert.txt, which you can review to check if any of the conversions were lossy (e.g. due to missing ROM card waveforms).

```
@echo off
for /R %%F in (*.syx, *.mid) do (
echo %%F >> convert.txt
JDTools convert bin "%%F" "%%F.bin" >> convert.txt 2>&1
)
```

## Merging

Merge any number of SysEx dumps (SYX, MID) containing temporary patches by invoking `JDTools merge <input1.syx> <input2.syx> <input3.syx> ... <output.syx>`. If an input file contains multiple dumps for the temporary patch area, they are all considered.

The following batch script can be used to pass a directory name instead of a list of individual files:

```
@echo off
REM Usage: merge.cmd Path\To\Directory output.syx 
setlocal enabledelayedexpansion enableextensions
set baseDir=%~1
set LIST=
for %%x in ("%baseDir%\*.syx") do set LIST=!LIST! "%%x"
set LIST=%LIST:~1%

JDTools merge %LIST% %2
```

## Listing

List all the contents of a SysEx dump (or any of the other supported input formats) by invoking `JDTools list <input.syx>`. This also lists objects that JDTools cannot convert (such as the JD-800 display area), but the actual contents are not shown for most of them. Useful for easily creating a patch listing of your banks.

# Version History

## v0.17 (2024-01-xx)

- Fewer warnings are shown when converting from JD-990 patch format, e.g. when an incompatible structure is used but both tones of the structure are muted anyway.
- When converting to ZenCore format, negative pitch envelope levels are scaled more appropriately and a warning is shown when negative pitch levels exceeding one octave are used (ZenCore only supports a +/-1 octave range, while the JD-800 supports a -3 to +1 octave range)
- When converting from a JD-990 SysEx dump, the source format was not displayed correctly.

## v0.16 (2022-11-28)

- All extended JD-990 waveforms are now replaced with JD-800 waveforms when converting to JD-800 format. Obviously a lot of those approximations will not be even close, but are hopefully still more helpful than just silent tones.
- A few more simple fixups when doing a lossy JD-990 to JD-800 conversion.
- Fixed conversion of JD-990 patches that don't use the expected tone control source / destination pairs.
- When inserting patches into SVD file, the following patches are no longer overwritten with empty slots.

## v0.15 (2022-09-05)

- Converting from SVD to BIN or SVZ did not split the patch banks correctly.
- Conversion to SVD could cause an incomplete SVD file to be written, which the JD-08 would then be unable to load.

## v0.14 (2022-09-05)

- The `list` command now lists all patches in BIN / SVD / SVZ files if there's more than 64 of them.
- The text output now refers to patches by their source index, not the destination index. So when converting e.g. from SVD format to SYX, the text output will list patches A11...D88 instead of going through I11...I88 four times in a row.

## v0.13 (2022-09-03)

- Converting to SVD format now allows to specify an optional parameter to specify the destination position of the patches, so that e.g. bank A can be preserved and converted patches are written to bank B instead.
- JD-800 and JD-990 setups can now be converted to BIN / SVD / SVZ as well. They are written to a separate files (so if the patches are written to "Patches.bin", the setup ends up in "Patches.setup.bin"). Every key is converted to a separate patch.

## v0.12 (2022-08-04)

- Converting a patch with tempo-synced delay taps or LFO rates to JD-800 SysEx now approximates the delay / rate. A tempo of 120 BPM is assumed.
- LFO offset + and - were swapped when converting from JD-800 SysEx to BIN / SVD / SVZ or vice versa.
- Support BIN files created with newer plugin versions.

## v0.11 (2022-08-03)

- Conversion to JD-08 format is more difficult than anticipated. As a result, the conversion process was changed so that the provided output file must be an **already existing** JD08Backup.svd file obtained from your JD-08. The file is then overwritten, but its contents are replaced with the new patch data.
- Recognize JD-08 effect group A pan feature and warn about it during conversion.
- When converting a bank to BIN / SVZ / SVD, but the target format required smaller banks than the input file provided, only the first bank was written.

## v0.10 (2022-08-02)

- Conversion from an .svd or .bin file with more than 64 patches now splits the output into multiple files.
- Avoid broken JD-800 SysEx when converting a tone from .bin / .svd / .svz with no waveform assigned, and don't warn about effect group A level if the group is not enabled.

## v0.9 (2022-08-01)

- Convert to / from JD-08 .svd format. Note that these files typically contain 256 patches (all 4 banks of 64 patches). The JD-800, JD-990 and the JD-800 VST can only access the first 64 patches, Zenology can access all 256 of them. In a future version, splitting these files into four individual files for each bank will be possible.

## v0.8 (2022-07-31)

- Convert to / from JD-800 VST and Zenology .bin format
- Convert to / from ZC1 hardware .svz format
- JD-990 to JD-800 conversion: Aftertouch bend depth is now initialized to +/-0 instead of -36 for special setups if no key uses aftertouch bend

## v0.7 (2022-07-25)

Various small improvements:

- JD-990 to JD-800 conversion: Aftertouch bend depth is now initialized to +/-0 instead of -36 if no tone uses aftertouch bend
- Temporary patches and setups are now also converted and listed
- On Windows 10 Version 1903 and newer, any UTF-8 file paths are now supported. On earlier Windows versions, only paths conforming to the current locale (ANSI codepage) are supported, as before.


## v0.6 (2022-07-22)

- Adds support for reading Standard MIDI Files (MID / SMF).

## v0.5 (2022-07-21)

- Initial release with conversion, listing and merging functionality.

# License

JDTools is provided under the BSD 3-clause license. JDTools makes use of miniz, which is released under the MIT license. See [license.md](license.md) for details.

# Building

This project is written in C++ 20. To build it, use the cross-platform CMake project, or alternatively the Visual Studio 2019 / 2022 solution for Windows.

