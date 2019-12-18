# Disclaimer

**This is only intended to be used as such:**

- **PAL console on firmware 5.5.X**
- **European Brain Age bought and installed from the eShop**
- **Access to the WiiU Internet Browser**


# haxchi (5.5.x ONLY and EUROPE BRAIN AGE ONLY)

Self-made haxchi! 

# Support

- [x] 5.5.x ~~JAP~~/~~USA~~/EUR
- [x] Backdoor (press HOME -> emergency exit)
- [x] 100% stability
- [x] Crash-less sd_loader
- [x] Boots up the homebrew channel
- [x] HBL .elf support
- [ ] HBL .rpx support (currently returns to HBL)
- [ ] haxchi selection menu
- [ ] sd_loader selection usage



# Credits

smealum and all of the people credited here:
https://github.com/smealum/haxchi

+ WiiU people for making such a terrible thing with the 0x1300 bytes limit of the sd_loader :(

# Build

- devkitPRO, PPC and ARM 
- put [**xxd**](https://lmgtfy.com/?q=how+to+get+xxd+on+windows "xxd") and [**armips**](https://github.com/Kingcom/armips/releases "armips") in one of the folder of the PATH env-var
- ``make``
- Put ``rom.zip`` -> ``/storage_mlc/usr/title/00050000/10179c00/content/0010`` using [``FTPiiU Everywhere``](https://wiiubru.com/appstore/#/app/ftpiiu_everywhere "FTPiiU") or [``wupclient``](https://github.com/dimok789/mocha/tree/master/ios_mcp "wupclient")
- Exit the Homebrew Launcher and go to your system settings, exit, and you're done!
