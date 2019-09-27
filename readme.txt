 READ THE GNU LICENSE BEFORE CONTINUING!!!!!!!!!!!
 copyright.txt

           *********************************
           ******  The Apollo Project ******
           ******    v0.01d Source    ******
           ******      01-29-2001     ******
           *********************************



DISCLAIMER and LEGAL STUFF: (Please read before running the software!!!)
--------------------------- (Last Updated 12-24-2000)
 Apollo is freeware - NEVER SELL Apollo or any portion of the software!
   We will take any legal action necessary to protect our rights.

 Anything you do with Apollo, or any portion of the software, is your
   responsibility.  We can not be held accountable for any damages Apollo
   does to you or anything else by your use of this software.  If you do
   not agree, delete this software NOW. (IOW don't blame us for killing
   dad's computer for christ sakes)

 NEVER, EVER.... (like Chris Jericho says it) EVER distribute Apollo with
   commercial ROMs!!!!  It's illegal!  I didn't write this software for some
   one to distribute commercial ROMs with it.  In fact, don't distribute
   Apollo in a modified archive.  Apollo should contain ALL files as they
   were when distributed with the release of the EXE.

 The rights of all trademarks mentioned in this document and in the emulator
   binary executable, Apollo, are retained by their respective owners.

 We have NO affiliated with rightful owner of trademarks, copyrights, patents,
   or otherwise of emulated hardware, nor of software that functions in the
   emulation.

 Please no rom begging.  You emails will be sent immediately to our legal team
   for possible charges of harassment.  You have been warned!

INTRODUCTION:
------------- (Last Updated 05-19-2000)
   This is just going to be a brief introduction to give you a little background
 on this project.  The Apollo Project really started way back in September when
 4 friends got together to make an N64 emulator.  We were doing very well until
 differences got between us, and we are now down to 2 members.  After wondering
 what to do with the project, Phrodide decided to create something new without
 using any of the former coder's code.  What you see here is the result of that
 effort and a month of time stress.  We are still alive and the next release
 will be a lot better.
    -Azimer (5/19/00)

SYSTEM REQUIREMENTS:
-------------------- (Last updated 12-24-2000)
   As fast as you can get for a CPU...  On my 465Mhz (O/Cd Celeron 300a) and a TNT
 video board I get about 31fps in N64Stars and 15/15.6fps in FireDemo.  Once again
 this is using the interpretive core so without a 2+Ghz computer, you will not hit
 the framerate you should be getting.  32MB or 64MB of memory is preferred though
 while running firedemo with the current core, I use < 8MB of memory.  FireDemo is
 a very small ROM image so plan to use 3MB + size of the ROM image.  You should use
 a Direct Draw accelerated graphics card.  I use to own a Matrox Mystique 220 which
 did hardware blt stretching on the driver.  You don't want that.  I only got 4fps 
 max :( - Got a ~6fps increase increase in speed from v0.01c.

RELEASE NOTES:
-------------- (Last updated 01-29-2001)
  Just source... and not 100%... Have fun...

-Azimer

USAGE:
------ (Last updated 12-24-2000)
 apollo.exe -<language>:
	<language> : Choose from english, spanish, portuguese, swedish, danish, dutch,
                     or french
 ex: apollo.exe -spanish

 Load up a rom image from the File/Load menu.  The image should play automatically.

WHAT'S NEXT:
------------ (Last updated 12-24-2000)
 Dynarec
 RDP + Graphics Plugin
 Built-in software-only RDP
 Complete the pif emulation
 Complete Controll implementation...
 Re-Add Recent Files
 Remove the language support
 Remove everything I don't like that Phrodide did
 Add more debugging features (not like the enduser cares)

WHAT'S NEW/HISTORY:
------------------- (Last updated 12-24-2000)
 - January  29th, 2001 - -=Source Only Release=-
                       - Just the source... Don't feel like writing down what's new...

 - December 24th, 2000 - -=First Emu64 Only WIP Release=-
                       - Plugin Support
		       - Fixed the nasty RSP Bug.  3D Demos work!!!
		       - Got Zelda64 to partially work...  Broke other roms though
		       - Put in controllers again... Very basic input.dll... Z=A X=B and
			 arrows do all the moving around.
		       - Several Speed increases
		       - Merry Christmas everyone!

 - July 09th, 2000 - -=Third  Release=-
                   - Multi-lingual support
                   - Plugin support (very preliminary) (just the video plugin)
                   - Exception code rewritten (finally)
                   - RSP, RDP, and Audio dummy support added, next release will have more.
                   - Speed hack --some roms run too fast while others no change.
                   - Seriously cleaned up some graphics code
                   - adaptoid support
                   - Removed the VI Hack (if you want it, get an older version)

 - June 19th, 2000 - -=Second Release=-
		   - Complete Rewrite.
                   - Began RSP Emulation (Biggest consumer of my time) :(
                   - Removed RSP code for this version
                   - Sped up the emulation 1.5fps
                   - Added a debugging message window
                   - Removed Controller support and most of the pif
                   - Added VI Hack to make SP_CRAP and Kid Star's Intro to work properly
                   - Added silent Audio Emulation... (goldcrap works)
                   - More dynarec added (soon to be complete)

 - May  19th, 2000 - Initial Version... everything is new :)


THANK YOUs and COMMENTS:
------------------------ (Last updated 07-09-2000)
  If you find bugs in this emulator then feel free to contact us at our website:
    http://apollo.emulation64.com/
  If there isn't a feature in here that you would like please use the forum.
  If you want a better readme, give me a suggestion... I can't read minds.
  Now... I would like to thank the follow people for info and help or whatnot:
    Zilmar, LaC, CricketNE, Mike Tedder, Niki Waibel, Marius Dumitrean, Daniel
    Lehman, Atreides, hWnd, _Demo_, schibo, icepir8, gerrit, Martin64,
    #emulation64, and everyone else I neglected to mention :(.
  Greets go out to:
    All the people in #n64dev, #nesdev, and #snesdev.
  Multilanguage support is made possible by:
    LP-S and boxy (Spanish), LP-S (Portuguese), Martin64 (Swedish), Zersion (Danish),
    NGN (Dutch), RamboDrop (French).  Thank you guys for your contribution!!!
  Now for the most important person to the project:
    -=HUGE=- UBERL33T THX to Jabo.  Truly if it wasn't for him APOLLO 
    would have never existed.

-Eclipse Can Not Die
 ReadMe By: Azimer
 Coding By: Azimer and Phrodide (Quit)
 Apollo Icon By: Jabo (it looks cool man!)
