function data=is_record()
% USAGE: DATA=IS_RECORD;
%
%   IS_RECORD returns DATA, a 2xN matrix of samples recorded during the 
%	last IS_PLAY.  Recording and playback happen at the same time (IS_PLAY
%   should really be called IS_CONVERT, to signify that what is really
%   happening there is AD/DA conversion, not just DA conversion, but
%	IS_CONVERT is kind of ambiguous), so IS_RECORD simply retrieves from the
%	the AD buffers the waveforms recorded during the conversion.
%
%   IS_RECORD might be used as follows, for example:
%
%       1. IS_INIT(FC, EPOCH);    
%           This initializes the TDT driver and hardware and readies it
%           conversion.  It also zeroes out the AD/DA buffers. You only
%           have to do this once.  FC and EPOCH determine the length of
%           the playback and record buffers: they will be (FC*(EPOCH/1000))
%           samples long.
%       2. TIMESTAMP=IS_PLAY;  
%			This performs the AD/DA conversion, storing the recorded 
%           waveforms in internal AD buffers.
%       3. DATA=IS_RECORD;
%           This retrieves the waveforms recorded in (2).
%       4. MV=IS_ADTOMV(DATA);
%           This converts DATA from AD ticks to mV.
%
%       Repeat 2-4 as necessary.
%
%       4. IS_SHUTDOWN;
%           Do this when you're done.
%
% SEE ALSO: IS_INIT, IS_PLAY, IS_ADTOMV, IS_SHUTDOWN, IS_LOAD
%

data=iserver('record');

