function att=get_att()
% USAGE: ATT=GET_ATT;
%
%   GET_ATT returns the current programmable attenuator settings in dB.
%
%	ATT is 1x2, and its units are dB.
%
%	NOTE: Actually, GET_ATT does not query the attenuators to get their
%         current settings.  What it returns are the settings specified
%         by the last SET_ATT call (see SET_ATT help).  Thus, if you do 
%
%                   SET_ATT(ATT); 
%                   ATT2 = GET_ATT; 
%                   IS_PLAY;
%                   ATT3 = GET_ATT; 
%
%         calling IS_PLAY, you will have ATT2=ATT, but not ATT2=(actual
%         attenuator settings).  ATT3 _will_ be the actual attenuator
%         settings.
%
% SEE ALSO: IS_PLAY, SET_ATT, SYNTHESIZE

att=iserver('att');

