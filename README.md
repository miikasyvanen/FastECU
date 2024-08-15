# FastECU

Open source software to work on and modify Subaru ECUs and TCUs!

Software contains some modified source code and kernels from nisprog by fenugrec:
https://github.com/fenugrec

And from forked nisprog/npkernel project by rimwall (modified for Subaru):
https://github.com/rimwall

Huge thanks to following:
- rimwall
- SergArb
- alesv
- jimihimisimi

Also big thanks to:
- fenugrec, whos kernel development with rimwall for Renesas SH-processors made this project to start at the first place.

**Without all mentioned efforts this project would not be succeeded so far!**

The code, unless otherwise specified, is licensed under GPLv3 which has certain restrictions. Here is a short summary:

Source: https://tldrlegal.com/license/gnu-general-public-license-v3-(gpl-3)

You may copy, distribute and modify the software as long as you track changes/dates in source files. Any modifications to or **software including (via compiler) GPL-licensed code** must also be made available under the GPL along with build & install instructions.

Commercial uses is possible, but **all code linked with GPL 3.0 source code must be disclosed under a GPL 3.0 compatible license.**

If you find **FastECU** useful, please consider supporting us as this software is developed on our free time and will always be open source and free of charge. Donations will be used to aquire new hardware and software to help reverse engineer ROMs and software development. Also lots of coffee plays important role!

https://www.paypal.com/paypalme/miikasyvanen

I can also be reached via email: info@fastecu.fi
Support forum found here: https://www.fastecu.fi/forum/

### Flashing with OBD
- **Supported ECU models**
  - Subaru Forester, Impreza, Legacy Turbo 1999-2000 K-Line (UJ WA12212920/128KB)
  - Subaru Forester JDM Cross Sports (SH7055)
  - Subaru Legacy JDM GT (SH7055)
  - Subaru Impreza WRX/STi MY05-07 (SH7058)
  - Subaru Impreza WRX/STi MY08+ (SH7058S)
  - Subaru Impreza, Legacy Diesel MY08+ EURO4 (SH7058S)
  - Subaru Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212930/256KB)
  - Subaru Forester, Impreza, Legacy 2002-2005 K-Line (UJ/Hitachi WA12212940/384KB)
  - Subaru Forester, Impreza, Legacy 2002-2005 K-Line (UJ/Hitachi WA12212970/512KB)

- **Supported TCU models**
  - CAN (Denso SH7055/512KB)
  - CAN (Denso SH7058/1024KB)
  - CVT CAN (Hitachi M32176F4/512KB)
  - K-Line (Hitachi M32176F4/512KB)
  - CAN (Hitachi M32176F4/512KB)
  - TBA - TCU CVT CAN (Mitsubishi MH8104/512KB)
  - TBA - TCU CVT CAN (Mitsubishi MH8111/1536KB)

- **ECU models currently under development**
  - Forester/Impreza/Legacy 16bit Denso 2001-2005 K-Line (HC16/160KB)

- **ECU models planned**
  - Subaru Forester 2006, Impreza 2006-2007 K-Line (Hitachi WA12212970WWW/512KB)
  - Subaru Impreza, Legacy Diesel EURO5 CAN (SH7059)
  - Subaru Forester 2007-2008, Impreza 2008+, Legacy 2006+ CAN (Hitachi WA12212970WWW/512KB)
  - Subaru Forester 2009-2011/Legacy 2010-2011 CAN (Hitachi SH7058/1MB)
  - Subaru Forester 2013+ CAN (Hitachi SH7059/1.5MB)
  - Subaru Forester 2013+ CAN (Hitachi SH7254/2MB)

### Unbricking with FastECU-m32r-flasher (bench flash)
- **Supported models**
  - Subaru Forester, Impreza, Legacy Turbo MY99-00 (UJ WA12212920/128KB)
  - Subaru Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212920/128KB) 
  - Subaru Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212930/256KB)

- **ECU models currently under development**
  - Subaru Forester, Impreza, Legacy 2002-2005 K-Line (UJ/Hitachi WA12212940/384KB) 
  - Subaru Forester, Impreza, Legacy 2002-2005 K-Line (UJ/Hitachi WA12212970/512KB)
