function set_att(db, side)
% USAGE: SET_ATT(ATT);
%
%   SET_ATT sets the programmable attenuators to the values given in the
%   matrix ATT.
%
%   ATT must be 1x2.  ATT(1) and ATT(2) are the desired settings for the 
%	LEFT attenuators, respectively, in dB.
%     
%	SYNTHESIZE returns an attenuation matrix appropriate to use with SET_ATT.
%     
%   NOTE: Actually SET_ATT only _prepares_ to set the attenuators.  
%         They aren't _physically_ set until  IS_PLAY is called.
%         Thus, you will not see the displays on the  attenuators 
%         change until you do IS_PLAY.
%
% SEE ALSO: IS_PLAY, GET_ATT, SYNTHESIZE

if size(db,2) > 2
	error('DB must be 1x2!');
else
	iserver('att',db);
end


