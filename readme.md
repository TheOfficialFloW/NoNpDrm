# NoNpDrm Plugin by TheFloW

## Features
- Exports PS Vita content license keys as fake licences.
- Bypasses expiration of PlayStation Plus and other timed licenses.
- Allows you to run trial versions as full versions.
- Allows sharing PFS encrypted content (unmodified non decrypted games) across multiple PS Vita accounts and devices using generated fake license files.
- Imported games behave as purchased games and allow the use of game updates seemlessly downloaded from the Sony Interactive Entertainment Network (PlayStation Network) as long as these updates run on firmware 3.68 and lower.
- Games can also be stripped of their PFS encryption using tools such as Vitamin just as any other purchased games would.
- Using purchased applications on deactivated devices.

In a nutshell, this plugin allows you to bypass DRM protection on any PS Vita content.

### This software WILL NOT
- Allow modifications to your games/applications.
- Work with PFS decrypted content (such as games dumped using applications such as Vitamin or MaiDumpTool).
- Enable you to run applications/use content without a valid license or a fake license file.
- Work with PlayStation Portable or PlayStation 1 titles (should you wish to play such titles, you may want to look into the [Adrenaline software](https://github.com/TheOfficialFloW/Adrenaline/releases)).

## Legal Disclaimer
- The removal and distribution of DRM content and/or circumventing copy protection mechanisms for any other purpose than archiving/preserving games you own licenses for is illegal.  
- This software is meant to be strictly reserved for your own **PERSONAL USE**.
- The author does not take any responsibility for your actions using this software.

## Software Requirements
This software will only work on PlayStation Vita, PlayStation Vita TV, PlayStation TV devices running on firmware 3.60-3.68 the taiHEN framework and HENkaku need to be running on your device, for more information please connect to https://henkaku.xyz/  
For all the possibilities described below, you should use [VitaShell v1.6](https://github.com/TheOfficialFloW/VitaShell/releases) or higher for faster transfers.  
VitaShell lets you mount your PS Vita's Memory Card or Game Card to your PC over USB.  
On a PS TV device, you can mount a USB flash drive and copy files to `uma0:`.

## Installation
Download the latest [nonpdrm.skprx](https://github.com/TheOfficialFloW/NoNpDrm/releases), copy it to `ux0:tai` and modify the `ux0:tai/config.txt` file to add the path to the module under `*KERNEL` as follows

```
*KERNEL
ux0:tai/nonpdrm.skprx
```

Don't forget to reboot your device, otherwise the plugin will have no effect yet.  
If you know what you are doing, you may change this path to an arbitrary location as long as it matches the exact location of the module. 
You may also edit the `ur0:tai/config.txt` instead assuming you do not have a config.txt file inside the `ux0:tai/` folder

## Creating the fake license
In order to generate a fake license file containing the application's rif key, you must first launch the application with the NoNpDrm plugin enabled.  
The fake licenses for the applications will then be stored at
- `ux0:nonpdrm/license/app/TITLE_ID/6488b73b912a753a492e2714e9b38bc7.rif`
- `ux0:nonpdrm/license/addcont/TITLE_ID/DLC_FOLDER/6488b73b912a753a492e2714e9b38bc7.rif` (for additional content)

## Sharing Digital Applications
- If you wish to use the application on the same device but on a different account, simply copy the fake license `6488b73b912a753a492e2714e9b38bc7.rif` to
  `ux0:license/app/TITLE_ID/6488b73b912a753a492e2714e9b38bc7.rif`.
- If you wish to use the application on a different device, transfer the content of `ux0:app/TITLE_ID` to your PC and copy the fake license `ux0:nonpdrm/license/app/TITLE_ID/6488b73b912a753a492e2714e9b38bc7.rif` file as `TITLE_ID/sce_sys/package/work.bin` **You need to overwrite the original work.bin**

**Note**: on games obtained through the PlayStation Store, `work.bin` is tied to your Sony Interactive Entertainment (also known as PlayStation Network) account and contains your account ID. The fake license does however **NOT** contain any personal information.

## Sharing Game Cards
Transfer the `gro0:app/TITLE_ID` folder and its content to `ux0:app/TITLE_ID` or to your computer and save the fake license
`ux0:nonpdrm/license/app/TITLE_ID/6488b73b912a753a492e2714e9b38bc7.rif` as `TITLE_ID/sce_sys/package/work.bin`.

For faster transfers you can mount the Game Card over USB. To do so, open VitaShell (See the Software Requirements section of this documentation), press the START button of your PS Vita, in the `Main settings` menu, select `Game Card` next to the `USB device` option and press START once again to close the settings tab.  
Now connect your PS Vita to your computer over USB and press the SELECT button.

**Note**: Mounting Game Cards over USB does not work on PlayStation TV or PlayStation Vita TV devices.

## Sharing Additional Content
You may share any additonal content across devices from `ux0:addcont/TITLE_ID/DLC_FOLDER` or, on selected card games, from `grw0:addcont/TITLE_ID/DLC_FOLDER`  
To do so, copy the fake license `ux0:nonpdrm/license/addcont/TITLE_ID/DLC_FOLDER/6488b73b912a753a492e2714e9b38bc7.rif` to `ux0:license/addcont/TITLE_ID/DLC_FOLDER/6488b73b912a753a492e2714e9b38bc7.rif`.

## Sharing Game Updates
While you may simply copy the content of `ux0:patch/TITLE_ID` or `grw0:patch/TITLE_ID` (in the case of selected card titles), game updates can be downloaded and installed directly from the PlayStation Network (unless the newest update is not compatible on 3.60-3.68).

## Installing shared applications
- Digital Application and Game Cards must be stored at the following location: `ux0:app/TITLE_ID`
- Additional contents must be stored at the following location: `ux0:addcont/TITLE_ID/DLC_FOLDER` and their associated licenses must be copied to
  `ux0:license/addcont/TITLE_ID/DLC_FOLDER/6488b73b912a753a492e2714e9b38bc7.rif`.
- Game Updates must be stored at the following location: `ux0:patch/TITLE_ID`.

Open VitaShell (version 1.6 or later) and press △ in the `home` section of VitaShell and choose `Refresh livearea`.  
This will trigger the installation if the files have been placed correctly and the licenses within `work.bin` files are valid.

## Overview
Should you decide to store your game contents on your computer, it is recommended to use the same structure as `ux0:` as shown below:

```
├───addcont
│   └───TITLE_ID
│   │   └───DLC_FOLDER
├───app
│   └───TITLE_ID
│   │   └───sce_sys
│   │       └───package
│   │           └───work.bin (copied or overwritten from ux0:nonpdrm/license/app/TITLE_ID/6488b73b912a753a492e2714e9b38bc7.rif)
├───license
│   └───addcont
│   │   └───TITLE_ID
│   │       └───DLC_FOLDER
│   │           └───6488b73b912a753a492e2714e9b38bc7.rif (copied from ux0:nonpdrm/license/addcont/TITLE_ID/DLC_FOLDER/6488b73b912a753a492e2714e9b38bc7.rif)
├───patch
│   └───TITLE_ID
```

## Source code
The source code is located within the `src` directory and is licensed under `GPLv3`.

## Troubleshooting
- "I am getting a `C1-2758-2` error when trying to run a game/application" - Your game has not been copied properly and at least one of the file is corrupt, please copy it again and retry.
- "I am getting a `C1-6703-6` error when trying to run a game/application" - You are running NoNpDrm from a Devkit/Testkit (PDEL/PTEL) these devices are not currently supported.
- "I am getting a `C0-9250-6` error when trying to run a game/application" - The `nonpdrm.skprx` module is not loaded, make sure the path to the module is written in `ur0:tai/config.txt` or `ux0:tai/config.txt` if the later exists on your device.
- "I am getting a `NP-6182-7` error when trying to run a game/application" - This error occured only once during our test while attempting to run an expired PlayStation Plus timed application, attempting to run the game once more fixed the issue, we never managed to reproduce this error, should you manage to consistently reproduce this issue, please open an issue on github.
- "My game/application displays as a trial version in the livearea" - This happens because you copied a game/application featuring a trial mode, without or with an invalid/corrupt `work.bin`.
- "I somehow messed up the installation, how can I reinstall a game?" - You can delete the (fake) license at `ux0:license/app/TITLE_ID` and use the refresh option in VitaShell.

## Changelog

### v1.2
- Added support for 3.65/3.67/3.68 firmware.

### v1.1
- Fixed bug where fake license files of addcont on grw0: were not created.

### v1.0
- Initial release

## Special thanks
- Thanks to Team molecule for HENkaku and thanks to [yifanlu](https://twitter.com/yifanlu) for taiHEN
- Thanks to [Mathieulh](https://twitter.com/Mathieulh) for beta testing and helping me writing this readme
