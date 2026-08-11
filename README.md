# AbActSrv - Simple activation server for Angry Birds games on Windows

## Background

All **Angry Birds** PC games include the complete content, but out of the box only a demo mode is enabled. A one-time process where an activation code (purchased retail or online) is transmitted to an activation server unlocks the full game on the current system.

After discontinuing the games, the developer, Rovio, had made the commendable decision to make them completely free, by configuring the activation servers to accept any code.  Eventually, however, Rovio has started shutting down the servers, which breaks activation. Fortunately, the activation algorithm was trivial to reverse-engineer due to its simplicity. You can see [here](https://cloakedthargoid.wordpress.com/angry-birds-activation/) the details of the request and response packets, as well as server URLs. Running a simple server that implements the activation backend on **localhost** and redirecting the activation traffic to it (e.g., by editing the HOSTS file) is sufficient.

[The project by Niko (nikolan123)](https://github.com/nikolan123/rovio-drm-servers) implements such an activation endpoint and supports an extra feature of the original servers - codes to unlock certain in-game "goodies". It is written in Python, and a precompiled executable is provided. The server works great, but uses a fairly modern Python framework, so getting it to compile/run on old versions of Windows would be challenging.

## Project goal

The purpose of this project is to provide a simple activation server, equivalent to the above, which uses only native Win32 APIs and compiles/runs on any version of Windows starting from XP, since that is the minimum OS required to run the **Angry Birds** games themselves.

The feature that edits the HOSTS file on behalf of the user is deliberately excluded. Read below for instructions on how to do it yourself.

## Supported Games

The activation server supports all the games below. Later versions of the games were not officially released for the PC.

 - Angry Birds (Classic) up to 4.0.0
 - Angry Birds Rio up to 2.2.0
 - Angry Birds Seasons up to 4.1.0
 - Angry Birds Space up to 2.0.0
 - Angry Birds Star Wars up to 1.5.0
 - Angry Birds Star Wars II up to 1.5.1
 - Bad Piggies up to 1.5.1

## Instructions

The executable provided in the "releases" section should run on any version of Windows starting from Windows XP. I tested it on Windows 10, 7, XP, and even Windows Me, even though the games themselves cannot run on that OS.

The solution was created in Visual Studio 2008, but the code can be compiled in other versions (tested in Visual Studio 2022), and probably using other compilers as well. Expect some security/compatibility warnings when using modern toolchains.

## Editing the HOSTS file

On Windows, the special file `%WINDIR%\system32\drivers\etc\HOSTS` can be used as a local DNS resolver to disable or re-route DNS queries. It requires administrative privileges to edit. The format is explained in comments inside the file. The following two lines should be added, anywhere in the file, to redirect outgoing traffic to the Rovio servers to the localhost IP (127.0.0.1):

    127.0.0.1   cloud.rovio.com
    127.0.0.1   drm-pc.angrybirdsgame.com

## FAQ

**Q:** Can I run the server on a different PC on my network?
**A:** The server binds only to the **localhost** address, for security. You would need to change `server_addr.sin_addr.s_addr` to allow all IPs (or specific ranges), and rebuild. Then instead of 127.0.0.1, specify the local IP of the server PC in the HOSTS file of the client system. To work beyond **localhost**, you would probably also need to approve the server in the Windows firewall.

**Q:** Can I run it on a public server?
**A:** **I strongly advise against it.** The code was deliberately written in a very simplistic way, and absolutely no effort was made to adhere to any security or availability standards. The only guarantee is that it correctly handles the expected traffic coming from the official Angry Birds PC games, which is limited to a very small set of specific requests. It is not intended to withstand heavy or malicious traffic. Of course, you may use the code as reference for writing a more robust solution, or integrating into an existing one.

**Q:** Is this piracy?
**A:** This is reverse-engineering, conducted using only free and publicly available tools for network packet inspection. Rovio had chosen not to obfuscate the traffic or the algorithm, so there was no cracking or decryption involved. Furthermore, for years, Rovio had silently allowed any copy of their games to be activated for free at any time. This server merely reproduces that behavior. The purpose is to enable the users who purchased the games to reactivate them, should they lose the original installation or need to move them to a different machine. As a side effect, it will also allow users who did not pay for the games to activate them, but that is the choice of the individual.