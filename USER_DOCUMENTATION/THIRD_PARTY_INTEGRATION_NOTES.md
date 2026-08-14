# THIRD_PARTY_RESEARCH_NOTES

### YuVi_Rays-DVS.cacherefresh.io 
pure javascript webaudio (slower but runs in browser, with enough compute can work as a backup dvs same way we bring backup n
needles and a backup mic.

the ISSUE is we need to decouple PURE ALSA form this app appp and have the abililty to choose ALSA for native application use such as Serato or Mixxx, vs WASM downstream in the pipeline to be able to integreate the midi with webaudio api.

actually PLAYING the audio is a different beast because we arnt sending signals, but sending audio.
needs to be well thought out and tabled for now. 

to be honest a COMPLETE REWRITE of this app in webaudio and javascript /nodejs etc would be almost be more feasableas the midi input for botha  mixer and midi controller such as code-49 and mpd226 and let the audio of three different things play at the same time.  --> requires moreweb processing though and buffering. as of now, let's have all YuVi-Rays-DVS.cacherefresh.io and webaudio api in a completly seperate BRANCH- integrate with WebBrowser based DVS

### MIXXX

Confirmed — mixxx directly links liblilv-0.so.0, real LV2 plugin hosting. This changes everything. Let me check if JUCE supports building an LV2 target.

Major finding — our exact pinned JUCE version (8.0.15) has real, first-party LV2 plugin support in its CMake tooling, and Mixxx 2.4.0 (installed here) definitively hosts LV2 plugins (liblilv is directly linked, confirmed by ldd). Let me finish the two approved concrete tasks first, then bring this back to you since it likely changes the calculus on the other two questions.

### DJM-S9 

suppports midi over the usb just as wella s serato over it. 
note: to get the colors - you must send midi out. also channel 1 and channel 2 and channel 3 are live, not just buttons all on one channel.

see HARDWARE_NOTES for more. 