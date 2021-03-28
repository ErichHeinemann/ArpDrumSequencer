# ArpDrumSequencer
16 Step Sequencer based on Arduino, 16 instruments, 4 or 5 instruments are used to trigger Arpeggios based on the played notes on MIDI-In.



This is a new project not not working well yet.
It uses the same hardware and shares a lot of code with this project: https://github.com/ErichHeinemann/MPX16Sequencer

What I wanted to build:
based on my Drumsequencer for 16 Steps and 16 Instruments, 
I had reroutet the first 4 instruments to Midi-Channel 1 and their Pitch is controlled by the incomming MIDI-Notes.

Play some MIDI-Notes on a connected MIDI-Keyboard and start the sequencer.
Then select the first instrument and activate some steps. You will hear the lowest of Your played notes
Then select the second instrument and activate some steps. You will hear the next higher note which You played via MIDI
Then select the third instrument and activate some steps. You will hear the next higher note which You played via MIDI
Then select the fourth instrument and activate some steps. You will hear the lowest note which You played via MIDI but now an octave lower.

I did it, to use a played chord as the input and use it for an arpeggiator. The drum-Sequence on channel 10 would be played in synce.

These things are not working:
- MIDI-Sync-IN
- MIDI-Sync-Out



 

