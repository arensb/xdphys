function time=is_play(side)
% USAGE: TIMESTAMP=IS_PLAY;
%        TIMESTAMP=IS_PLAY(SIDE);
%
%	IS_PLAY performs AD/DA conversion, causing whatever is in the DA buffers
%	to be played, and recording whatever is on the AD inputs into the AD 
%	buffers.
%
%   SIDE, if supplied, should be either 'left' or 'right'.  If SIDE is 
%	given, IS_PLAY substitutes a buffer of all zeroes for the the opposite 
%	channel;  it does not change the attenuator settings, nor does it
%	modify the opposite channel.
%
%   TIMESTAMP is the time of playback, and is a derived from the system
%	clock; it is in seconds, and its granularity is 1 microsecond.
%
%   NOTE: In regards to the programmable attenuators and SET_ATT: SET_ATT
%       only _prepares_ to set the attenuators.  They are _physically_
%		set when IS_PLAY is called, just before playback.
%
%	A typical way to use IS_PLAY is as follows:
%
%       1.  IS_INIT(FC, EPOCH);
%           This initializes the TDT driver and hardware and readies it
%           conversion.  It also zeroes out the AD/DA buffers. You only
%           have to do this once.  FC and EPOCH determine the length of
%           the playback and record buffers: they will be (FC*(EPOCH/1000))
%           samples long.
%		2.  Synthesize waveforms, DATA, of length <= (FC*(EPOCH/1000)) samples
%           For example:
%            
%         [DATA,ATT]=SYNTHESIZE(SPEC,EPOCH,DELAY,BC,ITD,RISE,FALL,LDB,RDB,SIDE)
%
%       3.  IS_LOAD(DATA);
%           This loads the waveforms synthsized in (2) into the DA buffers.
%       4.  SET_ATT(ATT);
%           Sets the programmable attenuators.  
%       5.  TIMESTAMP=IS_PLAY;
%			This performs the AD/DA conversion, storing the playing the
%           waveforms loaded to the DA buffers in step (3) (additionally
%           recording into the AD buffers).
%
%       Repeat 2-5 as necessary.
%
%       6. IS_SHUTDOWN;
%           Do this when you're done.
%
% SEE ALSO: IS_INIT, IS_LOAD, IS_RECORD, IS_SHUTDOWN, SET_ATT, SYNTHESIZE

time=iserver('play');

